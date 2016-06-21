AC_INIT([root-vanished], [1], [stapelberg@debian.org], [lernstick-root-vanished])
AM_INIT_AUTOMAKE([foreign])

PKG_CHECK_MODULES([XCB], [xcb])
PKG_CHECK_MODULES([XCB_AUX], [xcb-aux])
PKG_CHECK_MODULES([DBUS], [dbus-1])

AC_PROG_CC_C99

AC_GNU_SOURCE
AM_GNU_GETTEXT([external])

AC_OUTPUT([Makefile po/Makefile.in])
