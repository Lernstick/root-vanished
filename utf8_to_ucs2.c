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
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <iconv.h>

static iconv_t conversion_descriptor = (iconv_t)-1;

/*
 * Converts the given string to UCS-2 big endian for use with
 * xcb_image_text_16(). The amount of real glyphs is stored in real_strlen,
 * a buffer containing the UCS-2 encoded string (16 bit per glyph) is
 * returned. It has to be freed when done.
 *
 */
char *utf8_to_ucs2(char *input, int *real_strlen) {
  size_t input_size = strlen(input) + 1;
  /* UCS-2 consumes exactly two bytes for each glyph */
  int buffer_size = input_size * 2;

  char *buffer = malloc(buffer_size);
  if (buffer == NULL) {
    err(EXIT_FAILURE, "malloc(%d)", buffer_size);
  }
  size_t output_size = buffer_size;
  /* We need to use an additional pointer, because iconv() modifies it */
  char *output = buffer;

  /* We convert the input into UCS-2 big endian */
  if (conversion_descriptor == (iconv_t)-1) {
    if ((conversion_descriptor = iconv_open("UCS-2BE", "UTF-8")) ==
        (iconv_t)-1) {
      err(EXIT_FAILURE, "icon_open(UCS-2BE, UTF-8)");
    }
  }

  /* Get the conversion descriptor back to original state */
  iconv(conversion_descriptor, NULL, NULL, NULL, NULL);

  /* Convert our text */
  if (iconv(conversion_descriptor, (void *)&input, &input_size, &output,
            &output_size) == (size_t)-1) {
    err(EXIT_FAILURE, "iconv()");
  }

  if (real_strlen != NULL) {
    *real_strlen = ((buffer_size - output_size) / 2) - 1;
  }

  return buffer;
}
