
Debian
====================
This directory contains files used to package ybd/ybq
for Debian-based Linux systems. If you compile ybd/ybq yourself, there are some useful files here.

## ybtc: URI support ##


ybq.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install ybq.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your ybq binary to `/usr/bin`
and the `../../share/pixmaps/ybtc128.png` to `/usr/share/pixmaps`

ybq.protocol (KDE)

