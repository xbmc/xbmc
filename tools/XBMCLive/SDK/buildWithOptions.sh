#!/bin/sh

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

# Using apt-cacher(-ng) to speed up apt-get downloads

SDK_BUILDHOOKS=""

# getopt-parse.bash

TEMP=$(getopt -o snp:ulkgiv:h:xP --long xbmc-svn,nvidia-only,proxy:,usb-image,live-only,keep-workarea,grub2,intel-only,variant:,hook:,x-swat,proposed -- "$@")
eval set -- "$TEMP"

while true
do
	case $1 in
	-s|--xbmc-svn)
		echo "Enable option: Use XBMC SVN PPA"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./buildHook-xbmcSvn.sh"
		shift
		;;
	-n|--nvidia-only)
		echo "Enable option: NVIDIA support only"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./buildHook-nvidiaOnly.sh"
		shift
		;;
	-u|--usb-image)
		echo "Enable option: Generate USBHDD disk image"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./buildHook-usbhddImage.sh"
		shift
		;;
	-l|--live-only)
		echo "Enable option: Do not include Debian Installer"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./buildHook-liveOnly.sh"
		shift
		;;
	-k|--keep-workarea)
		echo "Enable option: Do not delete temporary workarea"
		export KEEP_WORKAREA=1
		shift
		;;
	-g|--grub2)
		echo "Enable option: Use grub2"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./buildHook-grub2.sh"
		shift
		;;
	-i|--intel-only)
		echo "Enable option: Intel support only"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./buildHook-intelOnly.sh"
		shift
		;;
	-v|--variant)
		case "$2" in
			"") echo "No variant name provided, exiting"; exit ;;
			*)  VARIANTNAME=$2;;
		esac
		echo "Enable option: Build variant $VARIANTNAME"
		export VARIANTNAME
		shift 2
		;;
	-h|--hook)
		case "$2" in
			"") echo "No hook name provided, exiting"; exit ;;
			*)  HOOKNAME=$2;;
		esac
		echo "Enable option: Custom hook $HOOKNAME"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS $HOOKNAME"
		shift 2
		;;
	-x|--x-swat)
		echo "Enable option: Use x-swat repository (Ubuntu-X team Updates)"
		export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./buildHook-xswat.sh"
		shift
		;;
	-P|--proposed)
                echo "Enable option: Use proposed repository"
                export SDK_BUILDHOOKS="$SDK_BUILDHOOKS ./buildHook-proposed.sh"
                shift
		;;
	-p|--proxy)
		echo "Enable option: Use APT proxy"
		case "$2" in
			"") echo "No proxy URL provided, exiting"; exit ;;
			*)  PROXY_URL=$2;;
		esac

		export APT_HTTP_PROXY=$PROXY_URL
		export APT_FTP_PROXY=$PROXY_URL

		# We use apt-cacher when retrieving d-i udebs, too
		export http_proxy=$PROXY_URL
		export ftp_proxy=$PROXY_URL
		shift 2
		;;
	--) shift ; break ;;
	esac
done

./build.sh
