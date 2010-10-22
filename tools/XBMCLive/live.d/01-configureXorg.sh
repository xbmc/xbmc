#!/bin/bash

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
 
kernelParams=$(cat /proc/cmdline)
subString=${kernelParams##*xbmc=}
xbmcParams=${subString%% *}

activationToken="nogenxconf"

# if strings are NOT the same the token is part of the parameters list
# here we want to stop script if the token is there
if [ "$xbmcParams" != "${xbmcParams%$activationToken*}" ] ; then
	exit 0
fi

# Generates valid xorg.conf for proprietary drivers if missing
if [ -e /etc/X11/xorg.conf ] ; then
	exit 0
fi

# Identify Ubuntu release
LSBRELEASE="$(lsb_release -r | cut -f2 | sed 's/\.//')"

# Identify GPU, Intel by default
GPUTYPE="INTEL"

GPU=$(lspci -nn | grep 0300)
# 10de == NVIDIA
if [ "$(echo $GPU | grep 10de)" ]; then
	GPUTYPE="NVIDIA"
else
	# 1002 == AMD
	if [ "$(echo $GPU | grep 1002)" ]; then
		GPUTYPE="AMD"
	fi
fi

if [ "$GPUTYPE" = "NVIDIA" ]; then
	if [ $LSBRELEASE -gt 910 ]; then
		# only on lucid!
		update-alternatives --set gl_conf /usr/lib/nvidia-current/ld.so.conf
		ldconfig
	fi

	# run nvidia-xconfig
	/usr/bin/nvidia-xconfig -s --no-logo --no-composite --no-dynamic-twinview --force-generate

	# Disable scaling to make sure the gpu does not loose performance
	sed -i -e 's%Section \"Screen\"%&\n    Option      \"FlatPanelProperties\" \"Scaling = Native\"\n    Option      \"HWCursor\" \"Off\"%' /etc/X11/xorg.conf
fi

if [ "$GPUTYPE" = "AMD" ]; then
	# Try fglrx first
	if [ $LSBRELEASE -gt 910 ]; then
		# only on lucid!
		update-alternatives --set gl_conf /usr/lib/fglrx/ld.so.conf
		ldconfig
	fi

	# run aticonfig
	/usr/lib/fglrx/bin/aticonfig --initial --sync-vsync=on -f
	ATICONFIG_RETURN_CODE=$? 

	if [ $ATICONFIG_RETURN_CODE -eq 255 ]; then
		# aticonfig returns 255 on old unsuported ATI cards 
		# Let the X default ati driver handle the card 
		if [ $LSBRELEASE -gt 910 ]; then
			# only on lucid!
			# revert to mesa
			update-alternatives --set gl_conf /usr/lib/mesa/ld.so.conf
			ldconfig
		fi
		modprobe radeon # Required to permit KMS switching and support hardware GL  
	fi
fi

exit 0
