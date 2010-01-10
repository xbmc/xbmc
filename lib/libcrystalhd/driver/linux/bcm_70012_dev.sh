#!/bin/bash
#
# Author: Prasad Bolisetty
#
# Script to load broadcom 70012 module and create device node.
# 
# 

bcm_dev_bin="bcm70012"
bcm_dev_name="crystalhd"
bcm_node="/dev/crystalhd"

if  ! whoami | grep root > /dev/null ; then
	echo " Login as root and try.."	
	exit 1;
fi

bcm_pci_id=`lspci -d 14e4:1612`
if [ $? -ne 0 -o -z "$bcm_pci_id" ]; then
	echo "BCM 70012 not installed.."
	exit 1;
fi

if  lsmod | grep $bcm_dev_bin > /dev/null ; then
	echo "Stopping Broadcom MediaPC 70012 Module"
	rmmod $bcm_dev_bin >& /dev/null
	if [ $? -ne 0 ]; then
		echo "Failed to stop: Close applications and try again. "
		exit 1;
	fi
fi

bcm_major=`cat /proc/devices | grep "$bcm_dev_name" | cut -c1-3`

if [ -z "$bcm_major" ]; then
	modinfo $bcm_dev_bin >& /dev/null
	if [ $? -ne 0 ]; then
		echo "Broadcom MediaPC 70012 Kernel Module not installed"
		exit 1;
	fi
	modprobe $bcm_dev_bin >& /dev/null
	bcm_major=`cat /proc/devices | grep "$bcm_dev_name" | cut -c1-3`
	if [ $? -ne 0 -o -z "$bcm_major" ]; then
		echo "Error($bcm_major): Loading Broadcom MediaPC 70012 Module"
		rmmod $bcm_dev_bin >& /dev/null
		exit 1;
	fi
fi
if [ -c $bcm_node ]; then
	rm -f $bcm_node >& /dev/null
fi

mknod -m 666 $bcm_node c $bcm_major 0

echo "Broadcom MediaPC 70012 Module loaded"

