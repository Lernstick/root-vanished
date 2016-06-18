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
#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/mman.h>
#include <string.h>

static unsigned long long int must_hex_to_int(const char *hex) {
  char *end = NULL;
  errno = 0;
  const unsigned long long int val = strtoull(hex, &end, 16);
  if ((errno == ERANGE && (val == ULLONG_MAX)) || (errno != 0 && val == 0)) {
    err(EXIT_FAILURE, "strtoull");
  }

  if (*end != '\0') {
    errx(EXIT_FAILURE,
         "invalid entry in /proc/self/maps, could not interpret \"%s\" as hex",
         hex);
  }

  return val;
}

void mlock_files(void) {
  unsigned long long int mlocked = 0;
  FILE *f = fopen("/proc/self/maps", "r");
  if (f == NULL) {
    err(EXIT_FAILURE, "fopen(/proc/self/maps)");
  }
  char *starthex = NULL;
  char *endhex = NULL;
  char *perms = NULL;
  char *pathname = NULL;
  char buffer[4096];
  while ((fgets(buffer, sizeof(buffer), f) != NULL)) {
    /* free(NULL) is defined as a no-op, and freeing old variables allows us
     * to use “continue” below. */
    free(starthex);
    free(endhex);
    free(perms);
    free(pathname);
    /* Format (see proc(5)):
     * address           perms offset  dev   inode       pathname
     * 00400000-00452000 r-xp 00000000 08:02 173521      /usr/bin/dbus-daemon */
    const int ret = sscanf(buffer, "%m[^-]-%ms %ms %*s %*s %*s %ms", &starthex,
                           &endhex, &perms, &pathname);
    if (ret == EOF) {
      break;
    }
    if (ret < 4) {
      continue;
    }
    if (*pathname != '/') {
      /* Ignore all mappings which do not refer to files. The point of
       * mlock()ing is to make (failing) file system access unnecessary. */
      continue;
    }
    if (strcmp(perms, "---p") == 0) {
      /* Ignore the unmapped gap, for more details, see
       * http://unix.stackexchange.com/a/226317 */
      continue;
    }
    const unsigned long long int start = must_hex_to_int(starthex);
    const unsigned long long int end = must_hex_to_int(endhex);
    const unsigned long long int len = (end - start);
    if (mlock((const void *)start, len) == -1) {
      warn("mlock(%llu, %llu) (for \"%s\")", start, len, pathname);
      warnx("Not all files could be locked into memory. Verify RLIMIT_MEMLOCK "
            "is set to RLIMIT_INFINITY (check ulimit -l).");
      fclose(f);
      return;
    }
    mlocked += len;
  }
  printf("mlocked %llu bytes\n", mlocked);
  fclose(f);
}
