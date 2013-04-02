dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([Window Maker Volume Manager],[2.0.1],[raorn@raorn.name],[wmvolman])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE
AC_PROG_CC
AM_PROG_CC_C_O

AC_HEADER_STDC

PKG_CHECK_MODULES([X],[x11])
AC_SUBST([X_CFLAGS])
AC_SUBST([X_LIBS])

AC_CHECK_LIB([dockapp],[DAMakeShapedPixmapFromFile],,AC_MSG_ERROR([libdockapp >= 0.6.0 is required.]))

PKG_CHECK_MODULES([GLIB2],[glib-2.0 >= 2.31.13])
AC_SUBST([GLIB2_CFLAGS])
AC_SUBST([GLIB2_LIBS])

PKG_CHECK_MODULES([GIO],[gio-2.0 >= 2.31.13])
AC_SUBST([GIO_CFLAGS])
AC_SUBST([GIO_LIBS])

PKG_CHECK_MODULES([UDISKS],[udisks2 >= 2.0.0])
AC_SUBST([UDISKS_CFLAGS])
AC_SUBST([UDISKS_LIBS])

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
