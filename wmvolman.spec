# vim: set ft=spec: -*- rpm-spec -*-

Name: wmvolman
Version: 0.7
Release: alt1

Summary: Window Maker Volume Manager
Group: Graphical desktop/Window Maker
License: GPL
Url: http://people.altlinux.ru/~raorn/wmvolman.html

Packager: Sir Raorn <raorn@altlinux.ru>

PreReq: hal >= 0.5.0
Requires: pmount

Source: http://people.altlinux.ru/~raorn/%name-%version.tar.bz2

# Automatically added by buildreq on Fri May 05 2006
BuildRequires: libdbus-glib-devel libdockapp-devel libhal-devel libXpm-devel libXt-devel pkg-config

%description
wmVolMan is a small volume manager for Window Maker. For now
it only displays and allows mounting and unmounting removable
media and hotpluggable devices that are added to or removed
from the system. It uses D-BUS and HAL to listen for new
devices.

%prep
%setup -q

%build
%__autoreconf
%configure \
	--with-mount=%_bindir/pmount-hal \
	--with-mount-arg=udi \
	--with-umount=%_bindir/pumount \
	--with-umount-arg=device
%make_build

%install
%makeinstall

%files
%doc AUTHORS NEWS README
%config %_sysconfdir/hal/fdi//policy/*.fdi
%_bindir/%name
%dir %_datadir/%name/
%dir %_datadir/%name/default/
%_datadir/%name/default/*.xpm

%changelog
* Tue Aug 29 2006 Sir Raorn <raorn@altlinux.ru> 0.7-alt1
- [0.7]
 + Added README, described HAL properties and theming
 + Added CD-audio and "unknown" CD icons
 + Added support for memory cards (CF/MS/SD/MMC/SM)
 + Fixed and improved icon fallback

* Sat Jul 15 2006 Sir Raorn <raorn@altlinux.ru> 0.6.1-alt2
- GIT snapshot v0.6.1-g8dbe9c2
 + Fixed dvd_plus_r/rw drive type detection
 + Added CF/MS/SD/MMC/SM cards icons

* Wed Apr 26 2006 Sir Raorn <raorn@altlinux.ru> 0.6.1-alt1
- Fixed fdi location

* Wed Apr 26 2006 Sir Raorn <raorn@altlinux.ru> 0.6-alt1
- Built for Sisyphus

