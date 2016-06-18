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
#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

static DBusConnection *conn;
static DBusMessage *msg;

void reboot_prepare(void) {
  DBusError err;
  dbus_error_init(&err);
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
  if (dbus_error_is_set(&err)) {
    errx(EXIT_FAILURE, "Could not connect to system dbus (for --reboot): %s",
         err.message);
  }

  if ((msg = dbus_message_new_method_call(
           "org.freedesktop.login1", "/org/freedesktop/login1",
           "org.freedesktop.login1.Manager", "Reboot")) == NULL) {
    errx(EXIT_FAILURE, "dbus_message_new_method_call failed");
  }

  DBusMessageIter args;
  dbus_message_iter_init_append(msg, &args);
  const dbus_bool_t param = TRUE;
  if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &param)) {
    errx(EXIT_FAILURE, "dbus_message_iter_append_basic failed");
  }
}

void reboot(void) {
  if (!dbus_connection_send(conn, msg, NULL)) {
    errx(EXIT_FAILURE, "dbus_connection_send failed");
  }
  dbus_connection_flush(conn);
}
