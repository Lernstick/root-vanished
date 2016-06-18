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
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <string.h>

void open_fullscreen_window(xcb_connection_t *conn, const xcb_window_t window,
                            const xcb_screen_t *root_screen,
                            const uint16_t screen_width,
                            const uint16_t screen_height,
                            const uint32_t colorpixel) {
  uint32_t mask = 0;
  uint32_t values[3];

  mask |= XCB_CW_BACK_PIXEL;
  values[0] = colorpixel;

  mask |= XCB_CW_OVERRIDE_REDIRECT;
  values[1] = 1;

  mask |= XCB_CW_EVENT_MASK;
  values[2] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS |
              XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_VISIBILITY_CHANGE |
              XCB_EVENT_MASK_STRUCTURE_NOTIFY;

  xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, root_screen->root, 0, 0,
                    screen_width, screen_height, 0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    XCB_WINDOW_CLASS_COPY_FROM_PARENT, mask, values);

  const char *name = "root-vanished";
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING, 8, strlen(name), name);

  /* Ensure that the window is created and set up before returning */
  xcb_aux_sync(conn);
}
