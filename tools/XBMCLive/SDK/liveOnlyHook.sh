#!/bin/sh

cat $WORKPATH/buildLive/mkConfig.sh | grep -v debian-installer | grep -v win32-loader > $WORKPATH/buildLive/mkConfig.live.sh
rm $WORKPATH/buildLive/mkConfig.sh
mv $WORKPATH/buildLive/mkConfig.live.sh $WORKPATH/buildLive/mkConfig.sh

rm -rf $WORKPATH/buildLive/Files/binary_local-includes/install

export DONOTINCLUDEINSTALLER=1

# Minimise flash memory writes
#
cat > $WORKPATH/buildLive/Files/chroot_local-hooks/02-setFstab << EOF
#!/bin/sh

echo ""
echo "Set fstab entries..."
echo ""

echo "tmpfs /var/log tmpfs defaults 0 0" > /etc/fstab
echo "tmpfs /tmp tmpfs defaults 0 0" >> /etc/fstab
echo "tmpfs /var/tmp tmpfs defaults 0 0" >> /etc/fstab
EOF

chmod +x $WORKPATH/buildLive/Files/chroot_local-hooks/02-setFstab


# Modify menu.lst
sed -i '/## BEGIN INSTALLER ##/,/## END INSTALLER ##/d' $WORKPATH/buildLive/Files/binary_grub/menu.lst
