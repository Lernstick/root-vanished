# vim:ts=4:sw=4:et

# Compiling with -Os does not result in fewer pages being mlock()ed.
AM_CFLAGS = -Wall -Wextra

bin_PROGRAMS = root-vanished

root_vanished_SOURCES = main.c \
                        get_colorpixel.c \
                        mlock.c \
                        mountpoint_to_blockdev.c \
                        open_font.c \
                        open_fullscreen_window.c \
                        reboot.c \
                        wait_for_blockdev_removal.c \
                        utf8_to_ucs2.c \
                        prepare_message_window.c

root_vanished_CPPFLAGS = $(XCB_CFLAGS) \
                         $(XCB_AUX_CFLAGS) \
                         $(DBUS_CFLAGS) \
                         -DLOCALEDIR=\"$(localedir)\"

root_vanished_LDFLAGS = $(XCB_LIBS) \
                        $(XCB_AUX_LIBS) \
                        $(DBUS_LIBS)

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = config.rpath m4/ChangeLog

SUBDIRS = po
