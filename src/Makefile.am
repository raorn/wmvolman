EXTRA_DIST = wmvolman-master.xpm wmvolman-buttons.xpm \
	     icon_none.xpm \
	     marshal.list

bin_PROGRAMS = wmvolman

BUILT_SOURCES = \
		udisks-daemon-glue.h \
		udisks-device-glue.h \
		udisks-marshal.h udisks-marshal.c

wmvolman_SOURCES = main.c ui.h ui.c udisks.h udisks.c
nodist_wmvolman_SOURCES = $(BUILT_SOURCES)
wmvolman_CFLAGS = -DWMVM_ICONS_DIR=\"$(pkgdatadir)\" @GLIB2_CFLAGS@ @DBUS_GLIB_CFLAGS@
wmvolman_LDADD = $(LIBOBJS) @X_LIBS@ -ldockapp @GLIB2_LIBS@ @DBUS_GLIB_LIBS@

udisks-daemon-glue.h: /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.xml Makefile.am
	dbus-binding-tool --mode=glib-client --output=udisks-daemon-glue.h /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.xml

udisks-device-glue.h: /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Device.xml Makefile.am
	dbus-binding-tool --mode=glib-client --output=udisks-device-glue.h /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Device.xml

udisks-marshal.h: marshal.list
	glib-genmarshal $< --prefix=udisks_marshal --header > $@

udisks-marshal.c: marshal.list
	echo "#include \"udisks-marshal.h\"" > $@ && glib-genmarshal $< --prefix=udisks_marshal --body >> $@
