dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([WM Volume Manager],[0.7],[raorn@altlinux.ru],[wmvolman])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE
AC_PROG_CC

AC_ARG_WITH([mount], AS_HELP_STRING(--with-mount=PROGRAM,mount program (default is auto)),
	[ac_cv_mount=$withval], [ac_cv_mount=auto])

AC_ARG_WITH([umount], AS_HELP_STRING(--with-umount=PROGRAM,umount program (default is auto)),
	[ac_cv_umount=$withval], [ac_cv_umount=auto])

case "$ac_cv_mount" in
    auto|AUTO)
	AC_PATH_PROG([PMOUNT_HAL], [pmount-hal], [no])
	AC_PATH_PROG([PMOUNT],     [pmount], [no])
	AC_PATH_PROG([MOUNT],      [mount], [no])

	if test "x$PMOUNT_HAL" != "xno"; then
	    mount_program="$PMOUNT_HAL"
	    mount_arg="udi"
	elif test "x$PMOUNT" != "xno"; then
	    mount_program="$PMOUNT"
	    mount_arg="device"
	elif test "x$MOUNT" != "xno"; then
	    mount_program="$MOUNT"
	    mount_arg="device"
	else
	    AC_MSG_ERROR([unable to find suitable mount program])
	fi
	;;
    /*)
	mount_program="$ac_cv_mount"
	AC_ARG_WITH([mount-arg], AS_HELP_STRING(--with-mount-arg,argument for mount (udi, device)),
		[mount_arg="$withval"], [mount_arg=no])
	case "$mount_arg" in
	    udi|device)
		;;
	    *)
		AC_MSG_ERROR([bad mount argument, use "udi" or "device"])
		;;
	esac
	;;
    *)
	AC_MSG_ERROR([please specify full path for mount program or "auto"])
	;;
esac
AC_MSG_CHECKING([for mount command])
AC_MSG_RESULT([$mount_program %$mount_arg])

case "$ac_cv_umount" in
    auto|AUTO)
	AC_PATH_PROG([PUMOUNT],    [pumount], [no])
	AC_PATH_PROG([UMOUNT],     [umount], [no])

	if test "x$PUMOUNT" != "xno"; then
	    umount_program="$PUMOUNT"
	    umount_arg="device"
	elif test "x$UMOUNT" != "xno"; then
	    umount_program="$UMOUNT"
	    umount_arg="device"
	else
	    AC_MSG_ERROR([unable to find suitable umount program])
	fi
	;;
    /*)
	umount_program="$ac_cv_umount"
	AC_ARG_WITH([umount-arg], AS_HELP_STRING(--with-umount-arg,argument for umount (device, mountpoint)),
		[umount_arg="$withval"], [umount_arg=no])
	case "$umount_arg" in
	    device|mountpoint)
		;;
	    *)
		AC_MSG_ERROR([bad umount argument, use "device" or "mountpoint"])
		;;
	esac
	;;
    *)
	AC_MSG_ERROR([please specify full path for mount program or "auto"])
	;;
esac
AC_MSG_CHECKING([for umount program])
AC_MSG_RESULT([$umount_program %$umount_arg])

AC_DEFINE_UNQUOTED([BIN_MOUNT], ["${mount_program}"], [mount program])
AC_DEFINE_UNQUOTED([BIN_UMOUNT], ["${umount_program}"], [unmount program])
AC_DEFINE_UNQUOTED([MOUNT_ARG], [${mount_arg}], [mount argument])
AC_DEFINE_UNQUOTED([UMOUNT_ARG], [${umount_arg}], [umount argument])

AC_PATH_XTRA
AC_HEADER_STDC

AC_CHECK_LIB([dockapp], [DAMakeShapedPixmapFromFile], , AC_MSG_ERROR([libdockapp >= 0.6.0 is required.]))

PKG_CHECK_MODULES(GLIB2, [glib-2.0 >= 2.6.0])
AC_SUBST(GLIB2_CFLAGS)
AC_SUBST(GLIB2_LIBS)

PKG_CHECK_MODULES(DBUS_GLIB, [dbus-glib-1 >= 0.33])
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

PKG_CHECK_MODULES(HAL, [hal >= 0.5.0])
AC_SUBST(HAL_CFLAGS)
AC_SUBST(HAL_LIBS)

AC_ARG_ENABLE([Werror], AS_HELP_STRING(--disable-Werror,do no add -Wall -Werror to CFLAGS),
	, [enable_Werror=yes])

AC_MSG_CHECKING([wether to enable -Werror])
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
fdi/Makefile
])
AC_OUTPUT
