EXTRA_DIST = wmvolman-master.xpm wmvolman-buttons.xpm \
	     icon_none.xpm

bin_PROGRAMS = wmvolman

wmvolman_SOURCES = main.c ui.h ui.c hal.h hal.c
wmvolman_CFLAGS = -DWMVM_ICONS_DIR=\"$(pkgdatadir)\" @GLIB2_CFLAGS@ @DBUS_GLIB_CFLAGS@ @HAL_CFLAGS@
wmvolman_LDADD = $(LIBOBJS) @X_LIBS@ -ldockapp @GLIB2_LIBS@ @DBUS_GLIB_LIBS@ @HAL_LIBS@
