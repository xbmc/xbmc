#/bin/bash

restoreAutomount=0
CHECKGCONFDIR="$(gconftool-2 --dir-exists=/apps/nautilus/preferences)"
if [ "${CHECKGCONFDIR}" = "0" ]; then
	CHECKAUTOMOUNT="$(gconftool-2 --get /apps/nautilus/preferences/media_automount)"
	if [ "${CHECKAUTOMOUNT}" = "true" ]; then
		restoreAutomount=1
		echo "Please disconnect any USB disks and press a key when done..."
		read choice
		echo Disabling USB automount...
		gconftool-2 --set /apps/nautilus/preferences/media_automount --type=bool false
		echo "Please connect the USB disk and press a key when done..."
		read choice
	fi
fi

sudo python ./installXBMC.py -d ./XBMCLive.log -i ./XBMCLive.iso

if [ "$restoreAutomount" = "1" ]; then
	echo Restoring USB automount...
	gconftool-2 --set /apps/nautilus/preferences/media_automount --type=bool true
fi
