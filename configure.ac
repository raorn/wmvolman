dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([Window Maker Volume Manager],[1.0],[raorn@altlinux.org],[wmvolman])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE
AC_PROG_CC

AC_PATH_XTRA
AC_HEADER_STDC

AC_CHECK_LIB([dockapp],[DAMakeShapedPixmapFromFile],,AC_MSG_ERROR([libdockapp >= 0.6.0 is required.]))

PKG_CHECK_MODULES([GLIB2],[glib-2.0 >= 2.6.0])
AC_SUBST([GLIB2_CFLAGS])
AC_SUBST([GLIB2_LIBS])

PKG_CHECK_MODULES([DBUS_GLIB],[dbus-glib-1 >= 0.33])
AC_SUBST([DBUS_GLIB_CFLAGS])
AC_SUBST([DBUS_GLIB_LIBS])

PKG_CHECK_MODULES([UDISKS],[udisks >= 1.0.0])

AC_ARG_ENABLE([Werror],
	 AS_HELP_STRING([--disable-Werror],[do no add -Wall -Werror to CFLAGS]),,[enable_Werror=yes])

AC_MSG_CHECKING([whether to enable -Werror])
if test "x$enable_Werror" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -Werror"
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	icons/Makefile
])
AC_OUTPUT
