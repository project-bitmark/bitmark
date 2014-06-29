
Debian
====================
This directory contains files used to package bitmarkd/bitmark-qt
for Debian-based Linux systems. If you compile bitmarkd/bitmark-qt yourself, there are some useful files here.

## bitmark: URI support ##


bitmark-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install bitmark-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your bitmark-qt binary to `/usr/bin`
and the `../../share/pixmaps/bitmark128.png` to `/usr/share/pixmaps`

bitmark-qt.protocol (KDE)

