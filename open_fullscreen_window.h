#pragma once

void open_fullscreen_window(xcb_connection_t *conn, const xcb_window_t window,
                            const xcb_screen_t *root_screen,
                            const uint16_t screen_width,
                            const uint16_t screen_height,
                            const uint32_t colorpixel);
