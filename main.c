/*
 * Copyright 2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <locale.h>
#include <errno.h>
#include <sys/time.h>

#include "gettext.h"

#include "open_fullscreen_window.h"
#include "mountpoint_to_blockdev.h"
#include "wait_for_blockdev_removal.h"
#include "reboot.h"
#include "mlock.h"
#include "get_colorpixel.h"
#include "open_font.h"
#include "utf8_to_ucs2.h"
#include "prepare_message_window.h"

void usage(void) {
  printf("root-vanished [options]\n");
  printf("\n");
  printf("Displays a blue screen once the root file system vanishes, e.g. "
         "because the user (accidentally) unplugged the USB drive on which the "
         "root file system resides.\n");
  printf("\n");
  printf("Options:\n");
  printf("\t--mountpoint\tMountpoint to resolve into a block device to watch. "
         "(default: \"/\")\n");
  printf("\t--reboot\tInitiate a reboot once the user pressed a key. (default: "
         "false)\n");
  printf("\t--reboot_fallback_seconds\tIn case the keyboard cannot be grabbed, "
         "automatically reboot after this many seconds. (default: -1)\n");
}

int main(int argc, char *argv[]) {
  bool reboot_when_removed = false;
  char *mountpoint = "/";
  int option_index = 0;
  int opt;
  int reboot_fallback_seconds = -1;
  const struct option options[] = {
      {"mountpoint", required_argument, NULL, 'm'},
      {"reboot", no_argument, NULL, 'r'},
      {"reboot_fallback_seconds", required_argument, NULL, 'f'},
      {"version", no_argument, NULL, 'v'},
      {"help", no_argument, NULL, 'h'},
      {0, 0, 0, 0},
  };

  setlocale(LC_ALL, "");
  bindtextdomain("root-vanished", LOCALEDIR);
  if (bind_textdomain_codeset("root-vanished", "UTF-8") == NULL) {
    err(EXIT_FAILURE, "bind_textdomain_codeset(root-vanished, UTF-8)");
  }
  textdomain("root-vanished");

  while ((opt = getopt_long(argc, argv, "vh", options, &option_index)) != -1) {
    switch (opt) {
    case 'm':
      if ((mountpoint = strdup(optarg)) == NULL)
        err(EXIT_FAILURE, "strdup");
      break;

    case 'r':
      reboot_when_removed = true;
      break;

    case 'f':
      errno = 0;
      char *end = NULL;
      const unsigned long int val = strtoul(optarg, &end, 0);
      if (errno != 0) {
        err(EXIT_FAILURE, "strtol(\"%s\")", optarg);
      }
      if (*end != '\0') {
        errx(EXIT_FAILURE,
             "Could not convert --reboot_fallback_seconds (\"%s\") to integer",
             optarg);
      }
      reboot_fallback_seconds = (int)val;
      break;

    case 'v':
      printf("root-vanished version " VERSION "\n");
      return 0;

    case 'h':
      usage();
      return 0;

    default:
      usage();
      return 1;
    }
  }

  char *blockdev = mountpoint_to_blockdev(mountpoint);
  printf("Resolved mountpoint \"%s\" to block device \"%s\"\n", mountpoint,
         blockdev);

  int conn_screen;
  xcb_connection_t *conn = xcb_connect(NULL, &conn_screen);
  if (xcb_connection_has_error(conn))
    errx(EXIT_FAILURE, "Cannot open display\n");

  const xcb_screen_t *root_screen = xcb_aux_get_screen(conn, conn_screen);
  const int screen_width = root_screen->width_in_pixels;
  const int screen_height = root_screen->height_in_pixels;
  const xcb_window_t window = xcb_generate_id(conn);
  const xcb_pixmap_t pixmap = xcb_generate_id(conn);
  const xcb_gcontext_t pixmap_gc = xcb_generate_id(conn);
  int message_height;
  int message_width;
  prepare_message_window(conn, root_screen, window, pixmap, pixmap_gc,
                         &message_width, &message_height, reboot_when_removed);

  if (reboot_when_removed) {
    reboot_prepare();
  }

  mlock_files();

  wait_for_blockdev_removal(blockdev);

  /* Map the window (= make it visible) */
  xcb_map_window(conn, window);

  /* Raise window (put it on top) */
  xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_STACK_MODE,
                       (uint32_t[]){XCB_STACK_MODE_ABOVE});

  /* Copy the contents of the pixmap to the real window */
  for (int x = 0; x < screen_width; x += message_width) {
    for (int y = 0; y < screen_height; y += message_height) {
      xcb_copy_area(conn, pixmap, window, pixmap_gc, 0, 0, x, y, message_width,
                    message_height);
    }
  }
  xcb_flush(conn);

  struct timeval start_tv;
  if (gettimeofday(&start_tv, NULL) == -1) {
    start_tv.tv_sec = 0;
    start_tv.tv_usec = 0;
  }

  if (reboot_when_removed) {
    /* When rebooting is enabled, grab the keyboard to listen for input and
     * reboot once a key was pressed. */
    int tries = 10000;
    while (tries-- > 0) {
      const xcb_grab_keyboard_cookie_t kcookie =
          xcb_grab_keyboard(conn, true, root_screen->root, XCB_CURRENT_TIME,
                            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

      xcb_grab_keyboard_reply_t *kreply;
      if ((kreply = xcb_grab_keyboard_reply(conn, kcookie, NULL)) &&
          kreply->status == XCB_GRAB_STATUS_SUCCESS) {
        free(kreply);
        break;
      }

      usleep(50);
    }

    if (tries <= 0) {
      warnx("Could not grab keyboard. Will reboot in %d seconds.",
            reboot_fallback_seconds);
      if (reboot_fallback_seconds > -1) {
        sleep(reboot_fallback_seconds);
        warnx("Rebooting, %d seconds passed", reboot_fallback_seconds);
        reboot();
      }
    }
  }

  xcb_generic_event_t *event;
  while ((event = xcb_wait_for_event(conn)) != NULL) {
    if (event->response_type == 0) {
      warn("X11 error received for sequence %x", event->sequence);
      continue;
    }

    /* Strip off the highest bit (set if the event is generated) */
    int type = (event->response_type & 0x7F);

    switch (type) {
    case XCB_KEY_PRESS:
      /* Verify at least 0.5s passed to prevent accidental inputs */
      if (reboot_when_removed) {
        bool too_quickly = false;
        struct timeval keypress_tv;
        if (gettimeofday(&keypress_tv, NULL) == 0) {
          struct timeval diff;
          timersub(&keypress_tv, &start_tv, &diff);
          too_quickly = (diff.tv_sec == 0 && diff.tv_usec < 500000);
        }
        if (!too_quickly) {
          reboot();
        }
        return 0;
      }
      break;

    case XCB_EXPOSE:
      /* Copy the contents of the pixmap to the real window */
      for (int x = 0; x < screen_width; x += message_width) {
        for (int y = 0; y < screen_height; y += message_height) {
          xcb_copy_area(conn, pixmap, window, pixmap_gc, 0, 0, x, y,
                        message_width, message_height);
        }
      }
      xcb_flush(conn);
      break;

    case XCB_VISIBILITY_NOTIFY:
      if (((xcb_visibility_notify_event_t *)event)->state !=
          XCB_VISIBILITY_UNOBSCURED) {
        xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_STACK_MODE,
                             (uint32_t[]){XCB_STACK_MODE_ABOVE});
        xcb_flush(conn);
      }
      break;
    }

    free(event);
  }
}
