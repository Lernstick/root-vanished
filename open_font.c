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
#include <stdlib.h>
#include <err.h>
#include <string.h>

xcb_font_t open_font(xcb_connection_t *conn, const char *pattern,
                     int *font_height) {
  const xcb_font_t result = xcb_generate_id(conn);
  const xcb_void_cookie_t font_cookie =
      xcb_open_font_checked(conn, result, strlen(pattern), pattern);
  const xcb_list_fonts_with_info_cookie_t info_cookie =
      xcb_list_fonts_with_info(conn, 1, strlen(pattern), pattern);

  xcb_generic_error_t *error = xcb_request_check(conn, font_cookie);
  if (error != NULL) {
    errx(EXIT_FAILURE,
         "Could not open X11 font by pattern \"%s\", X11 error code %d",
         pattern, error->error_code);
  }

  error = NULL;
  xcb_list_fonts_with_info_reply_t *reply =
      xcb_list_fonts_with_info_reply(conn, info_cookie, &error);
  if (reply == NULL || error != NULL) {
    errx(EXIT_FAILURE,
         "Could not open X11 font by pattern \"%s\", X11 error code %d",
         pattern, error->error_code);
  }

  *font_height = reply->font_ascent + reply->font_descent;
  free(reply);

  return result;
}
