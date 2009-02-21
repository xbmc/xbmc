#!/bin/bash
#
# XBMC disk installer
# V2.0 - 20090218
# Luigi Capriotti @2009
#

declare -a availableDrives
declare -a isRemovableDrive

nDisks=0

min_sizeGB=2
fixedDisk_min_sizeGB=4

CFG_FILE=/etc/initramfs-tools/moblin-initramfs.cfg
if [ -f ${CFG_FILE} ]; then
	. ${CFG_FILE}
else
	echo "Did not find config file: ${CFG_FILE}"
	echo "Error locating config file: ${CFG_FILE}"
	echo "Press a key to shutdown..."
	read
	shutdown -h now
	sleep 10
fi


BOOT_DISK=aaa
SRCMEDIA=bbb

# Msg, goOnKeys, stopKeys
function userChoice()
{
	bGoOn=0
	while true
	do
		echo -n $1
		read -n 1 choice
		echo ""
		case "$choice" in
			$2 )
				bGoOn=1
				break
				;;
			$3 ) break ;;
			* ) ;;
		esac
	done
	return $bGoOn
}

#
# Finds boot device
#
function findBootDrive
{
	CMDLINE=$(cat /proc/cmdline)

	for i in ${CMDLINE}; do
		case "${i}" in
			boot\=*)
				SRCMEDIA=$(echo "$i" | cut -d "=" -f 2)
				;;
		esac
	done

	case "$SRCMEDIA" in
	"disk" )
		driveList="sda sdb sdc sdd sde sdf sdg" ;;
	"usb" )
		driveList="sda sdb sdc sdd sde sdf sdg" ;;
	"cd" )
		driveList="sr0 sr1 sr2 sr3" ;;
	esac

	# Find the drive we booted from
	found="no"
	for device in $driveList; do
		echo "Checking device /dev/${device} for installation source..."
		if [ -b /dev/${device} ]; then
			if [ -e /sys/block/$device/removable ]; then
				case "$SRCMEDIA" in
					"disk" )
						if [ "$(cat /sys/block/$device/removable)" = "0" ]; then
							mount /dev/${device}1  /mnt  &> /dev/null
						fi
						;;
					"usb" )
						if [ "$(cat /sys/block/$device/removable)" = "1" ]; then
							mount /dev/${device}1  /mnt  &> /dev/null
						fi
						;;
					"cd" )
						mount -o ro -t iso9660 /dev/${device} /mnt &> /dev/null
						;;
				esac
				if [ -f /mnt/rootfs.img ] ; then
					echo "Found Boot drive at /dev/${device}"
					found="yes"
				fi
			fi
		fi
		umount /dev/${device}1  &> /dev/null
		if [ "$found" = "yes" ]; then
			break;
		fi
		echo "/dev/${device} does not contain a rootfs"
	done
	if [ "$found" = "no" ]; then
		echo "Error locating boot media, press a key to shutdown..."
		read
		shutdown -h now
		sleep 10
	fi

	BOOT_DISK=${device}
}

function findDisks()
{
	nDisks=0
	while true
	do
		found="no"
		for device in 'sda' 'sdb' 'sdc' 'sdd' 'sde' 'sdf' 'sdg'; do
			if [ "$device" = "$1" ]; then
				echo "Skipping boot device (${device}) ..."
			else
				echo "Checking device /dev/${device} ..."
	
				if [ -b /dev/${device} ]; then
					if [ "$(cat /sys/block/$device/size)" != "0" ]; then
						availableDrives[$nDisks]=${device}
						isRemovableDrive[$nDisks]=0
						echo "Found disk: $device."
						if [ -e /sys/block/$device/removable ]; then
							if [ "$(cat /sys/block/$device/removable)" = "1" ]; then
								echo "$device is a removable disk."
								isRemovableDrive[$nDisks]=1
							fi
						fi
						let "nDisks = nDisks + 1"
						found="yes"
					fi
				fi
			fi
		done
		if [ "$found" = "no" ]; then
			echo "No drives detected, connect a disk and press return..."
			read
			echo ""
			echo "Scanning again disk drives..."
			echo ""
		fi
		if [ "$found" = "yes" ]; then
			break
		fi
	done
}

function chooseDisk()
{
	echo ""
	echo ""
	echo "Choose disk to use"
	echo ""

	index=0
	while [ "$index" -lt "$nDisks" ]
	do
		let "nChoice = $index + 1"

		DSIZE=`fdisk -l /dev/${availableDrives[$index]} | grep "Disk /dev/${availableDrives[$index]}" | cut -f 5 -d " "`
		DSIZEMB=$[$DSIZE/1000000];

		if [ "${isRemovableDrive[$index]}" = 1 ]; then
			echo "   $nChoice: ${availableDrives[$index]} ($DSIZEMB MB) - Removable"
		else
			echo "   $nChoice: ${availableDrives[$index]} ($DSIZEMB MB)"
		fi
		let "index = $index + 1"
	done

	echo ""
	userChoice "Type the digit, or 0 to rescan disks: " "[1-9]" "0"
	return $?
}

# device, bootPartSize, swapPartSize
function partitionDisk
{
	dd if=/dev/zero of=/dev/$1 bs=512 count=2 &> /dev/null
	sync

	if [ $2 -le 0 ]; then
		partEnd="+size"
	else
		partEnd=+$2
		partEnd+=M
	fi

	fdisk /dev/$1 &> /dev/null <<EOF
o
n
p
1

$partEnd
t
c
a
1
w
EOF
	if [ $3 -ne 0 ] ; then
		partEnd=+$3
		partEnd+=M

		fdisk /dev/$1 &> /dev/null <<EOF
n
p
2

$partEnd
t
2
82
w
EOF

		fdisk /dev/$1 &> /dev/null <<EOF
n
p
3


w
EOF
	else
		if [ $2 -gt 0 ]; then
			fdisk /dev/$1 &> /dev/null <<EOF
n
p
2


w
EOF
		fi
	fi
	sync
}

function formatDisk
{
	mkfs.vfat -I -F 32 -n XBMCLive /dev/${1}1  &> /dev/null
	sync

	if [ $2 -gt 0 ]; then
		if [ $3 -ne 0 ]; then
			mkswap -c /dev/${1}2 &> /dev/null
			mkfs.ext3 /dev/${1}3 &> /dev/null
		else 
			mkfs.ext3 /dev/${1}2 &> /dev/null
		fi
	fi
	sync
}



function copySystemFiles
{
	if [ ! -d /tmp/bootDisk ]; then
		mkdir /tmp/bootDisk
	fi

	echo "Copying files from boot media /dev/$BOOT_DISK ($SRCMEDIA), please be patient..."
	case "$SRCMEDIA" in
	"disk" )
		mount /dev/${BOOT_DISK}1 /tmp/bootDisk &> /dev/null ;;
	"usb" )
		mount /dev/${BOOT_DISK}1 /tmp/bootDisk &> /dev/null ;;
	"cd" )
        	mount -o ro -t iso9660 /dev/$BOOT_DISK /tmp/bootDisk &> /dev/null ;;
	esac

	# Do not copy storage file
	if [ -f /tmp/bootDisk/ext3fs.img ]; then
		mv /tmp/bootDisk/ext3fs.img /tmp/bootDisk/ext3fs.img.nocopy
	fi
	cp /tmp/bootDisk/vmlinuz /tmp/bootDisk/*.img /tmp/bootPart
	if [ -f /tmp/bootDisk/ext3fs.img.nocopy ]; then
		mv /tmp/bootDisk/ext3fs.img.nocopy /tmp/bootDisk/ext3fs.img
	fi

	cp -R /tmp/bootDisk/boot /tmp/bootPart

	umount /tmp/bootDisk
	rm -rf /tmp/bootDisk

	if [ ! -d /tmp/bootPart/Config ]; then
		mkdir /tmp/bootPart/Config
	fi
	if [ -d /mnt/Config ]; then
		cp -R /mnt/Config* /tmp/bootPart/Config
	fi
}

function createPermanentStorageFile
{
	storageSize=0

	function choosePermanentStorageSize
	{
		echo ""
		echo "Select the permanent system storage size: "
		echo ""
		echo "  0 - None"
		let s="${1}/3"
		let s="${s}*2"
		echo "  1 - $s MB"

		echo ""
		while true
		do
			echo -n "Type the digit: "
			read -n 1 choice
			echo ""
			case "$choice" in
				[0-1] ) break ;;
				* ) ;;
			esac
		done

		case "$choice" in
			0 ) storageSize=0 ;;
			1 ) let storageSize=$s ;;
		esac
	}

	diskFree=`df -h -BM | grep $1 | awk -F ' ' '{print $4}'`
	diskFree=${diskFree/M/}

	# FAT32 max file size = 4GB
	if [ "$diskFree" -gt 4000 ] ; then
		diskFree=4000
	fi

	if [ "$2" = 1 ]; then
		choosePermanentStorageSize $diskFree

	else
		let storageSize="${diskFree/M/}-64"
	fi
	if [ "$storageSize" -gt 0 ] ; then
		echo "Creating permanent system storage file, please wait..."
		dd if=/dev/zero of=/tmp/bootPart/ext3fs.img  bs=1M count=$storageSize &> /dev/null
		mkfs.ext3 /tmp/bootPart/ext3fs.img -F &> /dev/null
	fi
}

function modifyGrubMenu
{
	if [ "$1" = 1 ]; then
		sed -i 's/boot=[a-z]*/boot=usb/g' /tmp/bootPart/boot/grub/menu.lst
	else
		# Defaults to current GPU
		hasAMD="$(/bin/lspci -nn | grep 0300 | grep 1002)"
		hasNVIDIA="$(/bin/lspci -nn | grep 0300 | grep 10de)"

		gpu=2
		if [ "$hasNVIDIA" != "" ] ; then
			gpu=0
		fi
		if [ "$hasAMD" != "" ] ; then
			gpu=1
		fi

		sed -i 's/boot=[a-z]*/boot=disk/g' /tmp/bootPart/boot/grub/menu.lst

		cat /tmp/bootPart/boot/grub/menu.lst | grep -v default | grep -v timeout > /tmp/menu.lst
		echo "default $gpu" > /tmp/bootPart/boot/grub/menu.lst
		echo "timeout 1"  >> /tmp/bootPart/boot/grub/menu.lst
		echo "hiddenmenu" >> /tmp/bootPart/boot/grub/menu.lst
		
		ROOT_UUID=$(blkid | grep $1 | cut -d " " -f 2 | cut -d "=" -f 2 | sed 's/"//g')
		
		echo "groot=$ROOT_UUID" >> /tmp/bootPart/boot/grub/menu.lst
		cat /tmp/menu.lst >> /tmp/bootPart/boot/grub/menu.lst
	fi
}

function changePasswords
{
	#
	# Lock root account
	#
	passwd -l root &> /dev/null

	echo "Please set a new password for user 'xbmc':"
	while true
	do
		passwd xbmc
		if [ "$?" = 0 ]; then
			break
		fi
	done

	cp /etc/shadow /tmp/bootPart/Config
}

function addFstabEntries
{
	#
	# Prepare /etc/fstab
	#
	echo "unionfs     /               unionfs defaults    0 0" > /tmp/bootPart/Config/fstab
	echo "proc                    /proc                   proc    defaults        0 0" >> /tmp/bootPart/Config/fstab
	if [ $2 -ne 0 ]; then
        	echo "/dev/${1}2  none    swap    sw,auto 0 0" >> /tmp/bootPart/Config/fstab
        	echo "/dev/${1}3  /home   ext3    defaults,auto 0 0" >> /tmp/bootPart/Config/fstab
	else
        	echo "/dev/${1}2  /home   ext3    defaults,auto 0 0" >> /tmp/bootPart/Config/fstab
	fi
}

function prepareHomeDirectory
{
	if [ ! -d /tmp/homePart ]; then
		mkdir /tmp/homePart
	fi
	
	if [ $2 -ne 0 ] ; then
		mount /dev/${1}3 /tmp/homePart
	else
		mount /dev/${1}2 /tmp/homePart
	fi

	if [ ! -d /tmp/homePart/xbmc ]; then
		mkdir /tmp/homePart/xbmc
	fi
	if [ ! -d /tmp/homePart/xbmc/Videos ]; then
		mkdir /tmp/homePart/xbmc/Videos
	fi
	if [ ! -d /tmp/homePart/xbmc/Pictures ]; then
		mkdir /tmp/homePart/xbmc/Pictures
	fi
	if [ ! -d /tmp/homePart/xbmc/Music ]; then
		mkdir /tmp/homePart/xbmc/Music
	fi
	
	# Create a sources.xml referencing the above created directories
	mkdir /tmp/homePart/xbmc/.xbmc
	mkdir /tmp/homePart/xbmc/.xbmc/userdata


cat > /tmp/homePart/xbmc/.xbmc/userdata/sources.xml <<EOF
<sources>
    <video>
       <default></default>
        <source>
            <name>Videos</name>
            <path>/home/xbmc/Videos/</path>
        </source>
    </video>
    <music>
        <default></default>
        <source>
            <name>Music</name>
            <path>/home/xbmc/Music/</path>
        </source>
    </music>
    <pictures>
        <default></default>
        <source>
            <name>Pictures</name>
            <path>/home/xbmc/Pictures/</path>
        </source>
    </pictures>
</sources>
EOF

	chown -R xbmc:xbmc /tmp/homePart/xbmc

	umount /tmp/homePart
	rm -rf /tmp/homePart
}

#
# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

while true
do
	echo ""
	echo "XBMC Live bootable disk creator"
	echo "---------------------"
	echo ""
	echo "The procedure will create a XBMC Live bootable disk"
	echo ""
	echo "Requirement for USB flash disks: the disk must have at least $min_sizeGB GB of capacity!"
	echo "Requirement for fixed disks: the disk must have at least $fixedDisk_min_sizeGB GB of capacity!"
	echo ""
	echo "Select the disk you want to use."
	echo "CAUTION: the process will erase all data on the specified disk drive!"
	echo "CAUTION: this is an experimental tool, use at your own risk!"
	echo ""
	echo -n "Plug any removable disk and press any key to proceed... "
	read -n 1 choice

	echo ""
	echo "Identifying boot disk..."
	echo ""
	findBootDrive

        echo ""
        echo "Enumerating available disks..."
        echo ""
	findDisks $BOOT_DISK

	chooseDisk
        if [ "$?" = 0 ]; then
                continue
        fi

	let "index = $choice -1"

	DSIZE=`fdisk -l /dev/${availableDrives[$index]} | grep "Disk /dev/${availableDrives[$index]}" | cut -f 5 -d " "`
	DSIZEMB=$[$DSIZE/1000000]

	minSizeMB=$[$min_sizeGB*1000]
	if [ "${isRemovableDrive[$index]}" = 0 ]; then
		minSizeMB=$[$fixedDisk_min_sizeGB*1000]
	fi

	if [ "$DSIZEMB" -lt "$minSizeMB" ] ; then
		echo ""
		echo ""
		echo "The selected disk is too small."
		echo "The miminum requirements for USB flash disks are: $min_sizeGB GB of disk size"
		echo "The miminum requirements for fixed disks are: $fixedDisk_min_sizeGB GB of disk size"

		echo "The selected disk has only $DSIZEMB MB of disk space."
		echo ""
		echo "Please ignore the selected disk and try again."
		echo ""
		echo -n "Press a button to restart the procedure. "
		read -n 1 choice
		continue
	fi

	userChoice "Erasing disk ${availableDrives[$index]}, proceed (Y/N)? " "[Y,y]" "[N,n]"
	if [ "$?" = 0 ]; then
		continue
	fi

	echo "Partitioning disk..."

	if [ "${isRemovableDrive[$index]}" = 1 ]; then
		boot_partition_size=-1
		swap_partition_size=0
	fi
	partitionDisk ${availableDrives[$index]} $boot_partition_size $swap_partition_size

	echo "Formatting disk..."
	formatDisk ${availableDrives[$index]} $boot_partition_size $swap_partition_size

	echo 'Preparing disk...'

	if [ ! -d /tmp/bootPart ]; then
		mkdir /tmp/bootPart
	fi
	mount -t vfat /dev/${availableDrives[$index]}1 /tmp/bootPart  &> /dev/null

	copySystemFiles

	createPermanentStorageFile ${availableDrives[$index]}1 ${isRemovableDrive[$index]}

	#
	# Mangle grub menu
	modifyGrubMenu ${isRemovableDrive[$index]} ${availableDrives[$index]}1

	if [ "${isRemovableDrive[$index]}" = 0 ]; then
		echo "Applying system changes..."

		addFstabEntries ${availableDrives[$index]} $swap_partition_size
		changePasswords
		prepareHomeDirectory ${availableDrives[$index]} $swap_partition_size
	fi

	umount /tmp/bootPart
	rm -rf /tmp/bootPart

	echo "All done!"
	echo ""

	userChoice "Do you want to create another bootable disk (Y/N)? " "[Y,y]" "[N,n]"
        if [ "$?" = 0 ]; then
		echo "Shutting down the system in 5 seconds..."
		sleep 5
                shutdown -h now
        fi
done
