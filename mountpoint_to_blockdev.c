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
#include <string.h>
#include <limits.h>

char *mountpoint_to_blockdev(const char *mountpoint) {
  printf("Finding blockdevice mounted at mountpoint %s\n", mountpoint);
  FILE *f = fopen("/proc/mounts", "r");
  if (f == NULL) {
    err(EXIT_FAILURE, "fopen(/proc/mounts)");
  }
  char buffer[4096];
  while ((fgets(buffer, sizeof(buffer), f) != NULL)) {
    char *first = strtok(buffer, " ");
    if (first == NULL) {
      continue;
    }
    char *fs_spec = strdup(first);
    if (fs_spec == NULL) {
      err(EXIT_FAILURE, "strdup");
    }
    char *second = strtok(NULL, " ");
    if (strcmp(second, mountpoint) != 0) {
      free(fs_spec);
      continue;
    }
    printf("Canonicalizing path %s\n", fs_spec);
    char *fs_spec_real = realpath(fs_spec, NULL);
    free(fs_spec);
    fclose(f);
    if (strncmp(fs_spec_real, "/dev/", strlen("/dev/")) != 0) {
      errx(EXIT_FAILURE, "block device %s does not start with /dev/, cannot "
                         "match with hotplug events later",
           fs_spec_real);
    }
    // Strip /dev/ prefix.
    return fs_spec_real + strlen("/dev/");
  }
  errx(EXIT_FAILURE, "Could not find --mountpoint=%s in /proc/mounts",
       mountpoint);
}
