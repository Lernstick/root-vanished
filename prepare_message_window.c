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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <xcb/xcb.h>

#include "gettext.h"
#define _(String) gettext(String)

#include "get_colorpixel.h"
#include "open_fullscreen_window.h"
#include "open_font.h"
#include "utf8_to_ucs2.h"

void prepare_message_window(xcb_connection_t *conn,
                            const xcb_screen_t *root_screen,
                            xcb_window_t window, xcb_pixmap_t pixmap,
                            xcb_gcontext_t pixmap_gc, int *message_width,
                            int *message_height,
                            const bool reboot_when_removed) {
  const uint32_t background = get_colorpixel(conn, root_screen, "#0000A8");
  const uint32_t foreground = get_colorpixel(conn, root_screen, "#FFFFFE");
  const uint16_t screen_width = root_screen->width_in_pixels;
  const uint16_t screen_height = root_screen->height_in_pixels;
  open_fullscreen_window(conn, window, root_screen, screen_width, screen_height,
                         background);
  int font_height;
  xcb_font_t font = open_font(
      conn, "-misc-fixed-bold-r-normal--18-*-iso10646-1", &font_height);
  *message_width = 1024;
  *message_height = 2 * (font_height + 8);
  xcb_create_pixmap(conn, root_screen->root_depth, pixmap, window,
                    *message_width, *message_height);
  xcb_create_gc(conn, pixmap_gc, pixmap, 0, 0);
  xcb_change_gc(conn, pixmap_gc, XCB_GC_FONT, (uint32_t[]){font});
  xcb_change_gc(conn, pixmap_gc, XCB_GC_FOREGROUND, (uint32_t[]){background});

  xcb_rectangle_t border = {0, 0, *message_width, *message_height};
  xcb_poly_fill_rectangle(conn, pixmap, pixmap_gc, 1, &border);

  xcb_change_gc(conn, pixmap_gc, XCB_GC_FOREGROUND, (uint32_t[]){foreground});
  xcb_change_gc(conn, pixmap_gc, XCB_GC_BACKGROUND, (uint32_t[]){background});

  int real_strlen;
  xcb_char2b_t *converted = (xcb_char2b_t *)utf8_to_ucs2(
      _("The root file system vanished. This live operating system cannot be "
        "used anymore."),
      &real_strlen);
  xcb_image_text_16(conn, real_strlen, pixmap, pixmap_gc, 20 /* X */,
                    font_height + 2 /* Y = baseline of font */, converted);
  free(converted);
  if (reboot_when_removed) {
    converted = (xcb_char2b_t *)utf8_to_ucs2(_("Press any key to reboot."),
                                             &real_strlen);
    xcb_image_text_16(conn, real_strlen, pixmap, pixmap_gc, 20 /* X */,
                      2 * (font_height + 2) /* Y = baseline of font */,
                      converted);
    free(converted);
  }
}
