
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

retrieveLatestNVIDIAURL()
{
	# Something like:
	# http://us.download.nvidia.com/XFree86/Linux-x86/190.42/NVIDIA-Linux-x86-190.42-pkg1.run

	driverPageURL=$(curl -x "" -f -s "http://www.nvidia.com/Download/processDriver.aspx?psid=63&pfid=431&rpf=1&osid=11&lid=1&lang=en-us&dtid=1")
	if [ "$?" -ne "0" ]; then
		echo "NVIDIA driver page URL not found, exiting..."
		exit 1
	fi

	# echo "Driver page URL=<$driverPageURL>"

	tmpFile=$(mktemp -q)
	curl -x "" -f -s -o $tmpFile $driverPageURL
	if [ "$?" -ne "0" ]; then
		echo "NVIDIA driver page not found, exiting..."
		exit 1
	fi

	# <a href="/content/DriverDownload-March2009/confirmation.php?url=/XFree86/Linux-x86/190.42/NVIDIA-Linux-x86-190.42-pkg1.run&lang=us&type=GeForce">
	# <img src="/content/DriverDownload-March2009/includes/us/images/bttn_download.jpg" border="0" alt="Download"/></a>

	confirmationPageURL=$(cat $tmpFile | grep -o -e "<a href=\"\([^\"]\+\?\)\">.* alt=\"Download\"")
	confirmationPageURL=$(echo $confirmationPageURL | grep -o -e "<a href=\"\([^\"]\+\?\)\">" | sed 's/^<a href=\"\|\">$//g')
	if [ -z "$confirmationPageURL" ]; then
		echo "NVIDIA driver confirmation page URL not found, exiting..."
		exit 1
	fi

	# echo "ConfirmationPageURL=http://www.nvidia.com$confirmationPageURL"

	curl -x "" -f -s -o $tmpFile "http://www.nvidia.com$confirmationPageURL"
	if [ "$?" -ne "0" ]; then
		echo "NVIDIA confirmation page not found, exiting..."
		exit 1
	fi

	# <td align="center"><a href="http://us.download.nvidia.com/XFree86/Linux-x86/190.42/NVIDIA-Linux-x86-190.42-pkg1.run">
	# <img src="/content/DriverDownload-March2009/includes/us/images/bttn_iagree.jpg" alt="Agree & Download" border="0" /></a></td>

	driverDownloadURL=$(cat $tmpFile | grep -o -e "<a href=\"\([^\"]\+\?\)\">.* alt=\"Agree & Download\"")
	driverDownloadURL=$(echo $driverDownloadURL | grep -o -e "<a href=\"\([^\"]\+\?\)\">" | sed 's/^<a href=\"\|\">$//g')

	if [ -z "$driverDownloadURL" ]; then
		echo "NVIDIA driver URL not found, exiting..."
		exit 1
	fi
	rm $tmpFile

	echo $driverDownloadURL
}

retrieveLatestAMDURL()
{
	# Something like:
	# https://a248.e.akamai.net/f/674/9206/0/www2.ati.com/drivers/linux/ati-driver-installer-9-11-x86.x86_64.run

	tmpFile=$(mktemp -q)

	curl -x "" -f -s -o $tmpFile "http://support.amd.com/us/gpudownload/linux/Pages/radeon_linux.aspx?type=2.4.1&product=2.4.1.3.42&lang=English"
	if [ "$?" -ne "0" ]; then
		echo "AMD driver page not found, exiting..."
		exit 1
	fi

	#<a class="submitButton" href="https://a248.e.akamai.net/f/674/9206/0/www2.ati.com/drivers/linux/ati-driver-installer-9-11-x86.x86_64.run">Download</a>

	driverDownloadURL=$(cat $tmpFile | grep -o -e "class=\"submitButton\" href=\"\([^\"]\+\?\)\">Download</a>")
	driverDownloadURL=$(echo $driverDownloadURL | sed -e "s/.*href=\"\([^\"]*\)\".*/\1/")

	if [ -z "$driverDownloadURL" ]; then
		echo "AMD driver URL not found, exiting..."
		exit 1
	fi
	rm $tmpFile

	echo $driverDownloadURL
}

getBCDriversSources()
{
	# Get Broadcom CristalHD main tree

	# Remove previous Broadcom file
	[ ! -f Files/chroot_local-includes/root/crystalhd*.gz ] || rm Files/chroot_local-includes/root/crystalhd*.gz

	echo "Downloading Broadcom drivers snapshot from http://git.wilsonet.com/crystalhd.git ..."
	wget -nc --no-proxy -q "http://git.wilsonet.com/crystalhd.git?a=snapshot;h=HEAD;sf=tgz" -O crystalhd-HEAD.tar.gz
	if [ ! -f crystalhd-HEAD.tar.gz ]; then
		echo "Error retrieving Broadcom drivers, exiting..."
		exit 1
	fi

	cp crystalhd-HEAD.tar.gz Files/chroot_local-includes/root
}

getNVIDIAInstaller()
{
	# Remove previous NVIDIA installer
	[ ! -f Files/chroot_local-includes/root/NVIDIA*.run ] || rm Files/chroot_local-includes/root/NVIDIA*.run

	# NVIDIA
	driverDownloadURL=$(retrieveLatestNVIDIAURL)

	echo "Downloading NVIDIA Installer from $driverDownloadURL ..."
	wget -nc --no-proxy -q $driverDownloadURL
	if ! ls NVIDIA*.run  > /dev/null 2>&1 ; then
		echo "Error retrieving NVIDIA drivers, exiting..."
		exit 1
	fi

	mv NVIDIA*.run Files/chroot_local-includes/root
}

getAMDInstaller()
{
	# Remove previous AMD installer
	[ ! -f Files/chroot_local-includes/root/ati*.run ] || rm Files/chroot_local-includes/root/ati*.run

	# AMD
	driverDownloadURL=$(retrieveLatestAMDURL)

	echo "Downloading AMD Installer from $driverDownloadURL ..."
	wget -nc --no-proxy -q $driverDownloadURL
	if ! ls ati*.run  > /dev/null 2>&1 ; then
		echo "Error retrieving ATI drivers, exiting..."
		exit 1
	fi

	mv ati*.run Files/chroot_local-includes/root
}
