#!/bin/sh

# Add NVIDIA PPA to sources

echo "deb http://ppa.launchpad.net/nvidia-vdpau/ppa/ubuntu karmic main" > buildLive/Files/chroot_sources/nvidia-vdpau.list.chroot
echo "deb http://ppa.launchpad.net/nvidia-vdpau/ppa/ubuntu karmic main" > buildLive/Files/chroot_sources/nvidia-vdpau.list.binary

# Add PPA key
if [ ! -f nvidia-vdpau.key ]; then
	# get key from launchpad

fi
cp nvidia-vdpau.key buildLive/Files/chroot_sources/nvidia-vdpau.chroot.gpg 
cp nvidia-vdpau.key buildLive/Files/chroot_sources/nvidia-vdpau.binary.gpg 

# Add packages

echo "nvidia-glx-190" > buildLive/Files/chroot_local-packageslists/nvidia-vdpau.list
echo "linux-headers-generic dkms" >> buildLive/Files/chroot_local-packageslists/nvidia-vdpau.list
echo 'fakeroot' >> buildLive/Files/chroot_local-packageslists/nvidia-vdpau.list

# Remove copy restricted from installer helper
rm buildDEBs/xbmclive-installhelpers/finish-install.d/07copyRestricted

# Modify menu.lst
# TODO mangle the existing one instead of creating a new one
#
cat >  buildLive/Files/binary_grub/menu.lst << 'EOF'
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

# Remove unneeded files
rm buildLive/Files/binary_local-includes/live/*.module
