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
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/netlink.h>

void wait_for_blockdev_removal(const char *blockdev) {
  printf("Waiting for blockdev \"%s\" to be removed\n", blockdev);

  struct sockaddr_nl nls;
  struct pollfd pfd;
  char buf[512];

  memset(&nls, 0, sizeof(struct sockaddr_nl));
  nls.nl_family = AF_NETLINK;
  nls.nl_pid = getpid();
  nls.nl_groups = -1;

  pfd.events = POLLIN;
  // As per netlink(7), Linux 3.0 allows unprivileged users to use
  // NETLINK_KOBJECT_UEVENT.
  pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
  if (pfd.fd == -1) {
    err(EXIT_FAILURE, "socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT)");
  }

  if (bind(pfd.fd, (void *)&nls, sizeof(struct sockaddr_nl))) {
    err(EXIT_FAILURE, "bind");
  }

  while (poll(&pfd, 1, -1) != -1) {
    const int n = recv(pfd.fd, buf, sizeof(buf), MSG_DONTWAIT);
    if (n == -1)
      err(EXIT_FAILURE, "recv");

    printf("Read hotplug event, first line is \"%s\"\n", buf);
    const size_t len = strlen(buf);
    // If the line starts with remove@ and ends in the block device, the block
    // device was removed.
    if (strncmp(buf, "remove@", strlen("remove@")) == 0 &&
        len > strlen(blockdev) &&
        strcmp(buf + len - strlen(blockdev), blockdev) == 0) {
      close(pfd.fd);
      return;
    }
  }
}
