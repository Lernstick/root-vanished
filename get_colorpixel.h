#pragma once

uint32_t get_colorpixel(xcb_connection_t *conn, const xcb_screen_t *root_screen,
                        const char *hex);
