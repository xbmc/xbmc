sudo mount -o loop $1 Image
cp Image/rootfs.img Image/vmlinuz Image/initrd0.img cdimage
sudo umount Image
rm XBMCLive.iso
./make-iso.sh
