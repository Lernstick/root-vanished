#pragma once

void prepare_message_window(xcb_connection_t *conn,
                            const xcb_screen_t *root_screen,
                            xcb_window_t window, xcb_pixmap_t pixmap,
                            xcb_gcontext_t pixmap_gc, int *message_width,
                            int *message_height,
                            const bool reboot_when_removed);
