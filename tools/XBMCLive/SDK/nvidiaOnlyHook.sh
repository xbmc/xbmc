#!/bin/sh

# Add NVIDIA PPA to sources

echo "deb http://ppa.launchpad.net/nvidia-vdpau/ppa/ubuntu karmic main" > $WORKPATH/buildLive/Files/chroot_sources/nvidia-vdpau.list.chroot
echo "deb http://ppa.launchpad.net/nvidia-vdpau/ppa/ubuntu karmic main" > $WORKPATH/buildLive/Files/chroot_sources/nvidia-vdpau.list.binary

# Add PPA key
# Download if not already available
if [ ! -f $WORKPATH/nvidia-vdpau.gpg ]; then
	wget "http://keyserver.ubuntu.com:11371/pks/lookup?op=get&search=0x1DABDBB4CEC06767" -O $WORKPATH/nvidia-vdpau.key
	cat $WORKPATH/nvidia-vdpau.key | grep -v "<" > $WORKPATH/nvidia-vdpau.gpg 
	rm $WORKPATH/nvidia-vdpau.key

	# gpg --keyserver keyserver.ubuntu.com -n --recv-keys 1DABDBB4CEC06767
	# gpg --export --armor 1DABDBB4CEC06767 > $WORKPATH/nvidia-vdpau.gpg
fi

cp $WORKPATH/nvidia-vdpau.gpg $WORKPATH/buildLive/Files/chroot_sources/nvidia-vdpau.binary.gpg
cp $WORKPATH/nvidia-vdpau.gpg $WORKPATH/buildLive/Files/chroot_sources/nvidia-vdpau.chroot.gpg  

# Add packages

echo "nvidia-glx-190" > $WORKPATH/buildLive/Files/chroot_local-packageslists/nvidia-vdpau.list
echo "linux-headers-generic dkms" >> $WORKPATH/buildLive/Files/chroot_local-packageslists/nvidia-vdpau.list
echo "fakeroot" >> $WORKPATH/buildLive/Files/chroot_local-packageslists/nvidia-vdpau.list

# Remove copy restricted from installer helper
rm $WORKPATH/buildDEBs/xbmclive-installhelpers/finish-install.d/07copyRestricted

# Remove holding on the kernel
rm $WORKPATH/buildLive/Files/chroot_local-hooks/01-holdKernel

# Remove unneeded files
rm $WORKPATH/buildLive/Files/binary_local-includes/live/*.module

export DONOTBUILDRESTRICTEDDRIVERS=1


# Modify menu.lst
# TODO mangle the existing one instead of creating a new one
#
cat >  $WORKPATH/buildLive/Files/binary_grub/menu.lst << 'EOF'
default 0
timeout 10

foreground eeeeee
background 333333

splashimage=/boot/grub/splash.xpm.gz


title  XBMCLive
kernel /live/vmlinuz boot=live vga=788 xbmc=autostart,tempfs,nodiskmount,setvolume splash quiet loglevel=0 persistent quickreboot quickusbmodules notimezone noaccessibility noapparmor noaptcdrom noautologin noxautologin noconsolekeyboard nofastboot nognomepanel nohosts nokpersonalizer nolanguageselector nolocales nonetworking nopowermanagement noprogramcrashes nojockey nosudo noupdatenotifier nouser nopolkitconf noxautoconfig noxscreensaver nopreseed union=aufs
initrd /live/initrd.img
quiet
boot

title  XBMCLive - SAFE MODE
kernel /live/vmlinuz boot=live vga=788 xbmc=tempfs,nodiskmount,setvolume quiet loglevel=0 persistent quickreboot quickusbmodules notimezone noaccessibility noapparmor noaptcdrom noautologin noxautologin noconsolekeyboard nofastboot nognomepanel nohosts nokpersonalizer nolanguageselector nolocales nonetworking nopowermanagement noprogramcrashes nojockey nosudo noupdatenotifier nouser nopolkitconf noxautoconfig noxscreensaver nopreseed union=aufs
initrd /live/initrd.img
quiet
boot

title  ---
root

title Boot Operating System on Hard Disk
root (hd0)
savedefault
makeactive
chainloader +1
quiet
boot

title Memory test (memtest86+)
kernel /live/memtest   
quiet
boot
EOF

