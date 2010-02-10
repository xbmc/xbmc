
#      Copyright (C) 2005-2008 Team XBMC
#      http://www.xbmc.org
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with XBMC; see the file COPYING.  If not, write to
#  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#  http://www.gnu.org/copyleft/gpl.html

makeConfig()
{
	CATEGORIES="main restricted universe multiverse"

	configString=""
	configString="$configString --mode ubuntu"
	configString="$configString --distribution karmic"
	configString="$configString --mirror-chroot-security http://security.ubuntu.com/ubuntu/"
	configString="$configString --mirror-binary-security http://security.ubuntu.com/ubuntu/"
	configString="$configString --binary-images iso"
	configString="$configString --binary-filesystem fat32"
	configString="$configString --hostname XBMCLive"
	configString="$configString --iso-application XBMC_Live"
	configString="$configString --iso-volume XBMC_Live"
	configString="$configString --iso-publisher http://xbmc.org"

	configString="$configString --bootloader grub"

	configString="$configString --debian-installer live"
        configString="$configString --debian-installer-preseedfile preseed.cfg"
	configString="$configString --debian-installer-gui disabled"
	configString="$configString --win32-loader disabled"

	configString="$configString --initramfs live-initramfs"

	if [ -n "$APT_HTTP_PROXY" ]; then
		configString="$configString --apt-http-proxy $APT_HTTP_PROXY"
	fi
	if [ -n "$APT_FTP_PROXY" ]; then
		configString="$configString --apt-ftp-proxy $APT_FTP_PROXY"
	fi

	configString="$configString --mirror-bootstrap http://archive.ubuntu.com/ubuntu/"
	configString="$configString --mirror-binary http://archive.ubuntu.com/ubuntu/"
	configString="$configString --mirror-chroot http://archive.ubuntu.com/ubuntu/"

	lh config --mode ubuntu --archive-areas "$CATEGORIES" $configString

	# Copy files for chroot
	cp -R "$THISDIR"/Files/chroot_* config

	# Copy files for ISO
	cp -R "$THISDIR"/Files/binary_* config
}
