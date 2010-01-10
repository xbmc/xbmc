#!/bin/bash
#
# Author: Prasad Bolisetty
#
# Script to load broadcom 70012 module and create device node.
# 
# 

bcm_dev_bin="crystalhd"
bcm_dev_bin_ko="crystalhd.ko"
bcm_dev_name="crystalhd"
bcm_node="/dev/crystalhd"

if  ! whoami | grep root > /dev/null ; then
	echo " Login as root and try.."	
	exit 1;
fi


if  /sbin/lsmod | grep $bcm_dev_bin > /dev/null ; then
	echo "Stopping Broadcom Crystal HD (BCM70012) Module"
	/sbin/rmmod $bcm_dev_bin >& /dev/null
	if [ $? -ne 0 ]; then
		echo "Failed to stop: Close applications and try again. "
		exit 1;
	fi
fi

bcm_major=`cat /proc/devices | grep "$bcm_dev_name" | cut -c1-3`

if [ -z "$bcm_major" ]; then
	/sbin/insmod $bcm_dev_bin_ko >& /dev/null
	bcm_major=`cat /proc/devices | grep "$bcm_dev_name" | cut -c1-3`
	if [ $? -ne 0 -o -z "$bcm_major" ]; then
		echo "Error($bcm_major): Loading Broadcom Crystal HD (BCM70012) Module"
		rmmod $bcm_dev_bin >& /dev/null
		exit 1;
	fi
fi
if [ -c $bcm_node ]; then
	rm -f $bcm_node >& /dev/null
fi

mknod -m 666 $bcm_node c $bcm_major 0

echo "Broadcom Crystal HD (BCM70012) Module loaded"

