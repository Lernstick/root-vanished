#pragma once

xcb_font_t open_font(xcb_connection_t *conn, const char *pattern,
                     int *font_height);
