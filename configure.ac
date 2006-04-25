dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

AC_INIT([WM Volume Manager], [0.5], [raorn@altlinux.ru], [wmvolman])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
AM_CONFIG_HEADER([config.h])

AC_GNU_SOURCE
AC_PROG_CC

AC_ARG_ENABLE([pmount], AC_HELP_STRING([--disable-pmount], [don't use pmount (default is yes)]),
	[ac_cv_use_pmount=$withval], [ac_cv_use_pmount=yes])

if test "x$ac_cv_use_pmount" = "xyes"; then
    AC_PATH_PROG([mount_path], [pmount], [no])
    AC_PATH_PROG([umount_path], [pumount], [no])
else
    AC_PATH_PROG([mount_path], [mount], [no])
    AC_PATH_PROG([umount_path], [umount], [no])
fi

if test "x$mount_path" = "xno" -o "x$umount_path" = "xno"; then
    AC_MSG_FAILURE([unable to find suitable mount/umount program])
fi

AC_DEFINE_UNQUOTED([BIN_MOUNT], ["${mount_path}"], [mount program])
AC_DEFINE_UNQUOTED([BIN_UMOUNT], ["${umount_path}"], [unmount program])

AC_PATH_XTRA
AC_HEADER_STDC

AC_CHECK_LIB([dockapp], [DAMakeShapedPixmapFromFile], , AC_MSG_ERROR([libdockapp >= 0.6.0 is required.]))

PKG_CHECK_MODULES(GLIB2, [glib-2.0 >= 2.6.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(DBUS_GLIB, [dbus-glib-1 >= 0.33])
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

PKG_CHECK_MODULES(HAL, [hal >= 0.5.0])
AC_SUBST(HAL_CFLAGS)
AC_SUBST(HAL_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
icons/Makefile
fdi/Makefile
])
