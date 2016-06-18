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
#include <stdint.h>
#include <stdlib.h>
#include <err.h>
#include <xcb/xcb.h>

/*
 * Returns the colorpixel to use for the given hex color (think of HTML).
 *
 * The hex_color has to start with #, for example #FF00FF.
 *
 * NOTE that get_colorpixel() does _NOT_ check the given color code for
 *validity.
 * This has to be done by the caller.
 *
 */
uint32_t get_colorpixel(xcb_connection_t *conn, const xcb_screen_t *root_screen,
                        const char *hex) {
  char strgroups[3][3] = {
      {hex[1], hex[2], '\0'}, {hex[3], hex[4], '\0'}, {hex[5], hex[6], '\0'}};
  uint8_t r = strtol(strgroups[0], NULL, 16);
  uint8_t g = strtol(strgroups[1], NULL, 16);
  uint8_t b = strtol(strgroups[2], NULL, 16);

  /* Shortcut: if our screen is true color, no need to do a roundtrip to X11 */
  if (root_screen == NULL || root_screen->root_depth == 24 ||
      root_screen->root_depth == 32) {
    return (0xFF << 24) | (r << 16 | g << 8 | b);
  }

#define RGB_8_TO_16(i) (65535 * ((i)&0xFF) / 255)
  int r16 = RGB_8_TO_16(r);
  int g16 = RGB_8_TO_16(g);
  int b16 = RGB_8_TO_16(b);

  xcb_alloc_color_reply_t *reply;

  reply = xcb_alloc_color_reply(
      conn, xcb_alloc_color(conn, root_screen->default_colormap, r16, g16, b16),
      NULL);

  if (reply == NULL)
    errx(EXIT_FAILURE, "Could not allocate X11 color");

  const uint32_t pixel = reply->pixel;
  free(reply);

  return pixel;
}
