EXTRA_DIST = wmvolman-master.xpm wmvolman-buttons.xpm \
	     icon_none.xpm

bin_PROGRAMS = wmvolman

wmvolman_SOURCES = main.c ui.h ui.c udisks.h udisks.c
wmvolman_CFLAGS = -DWMVM_ICONS_DIR=\"$(pkgdatadir)\" @X_CFLAGS@ @GLIB2_CFLAGS@ @GIO_CFLAGS@ @UDISKS_CFLAGS@
wmvolman_LDADD = $(LIBOBJS) @X_LIBS@ @GLIB2_LIBS@ @GIO_LIBS@ @UDISKS_LIBS@
