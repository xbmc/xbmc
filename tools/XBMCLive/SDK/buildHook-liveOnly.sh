#!/bin/sh

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

cat $WORKPATH/buildLive/mkConfig.sh | grep -v debian-installer | grep -v win32-loader > $WORKPATH/buildLive/mkConfig.live.sh
rm $WORKPATH/buildLive/mkConfig.sh
mv $WORKPATH/buildLive/mkConfig.live.sh $WORKPATH/buildLive/mkConfig.sh

rm -rf $WORKPATH/buildLive/Files/binary_local-includes/install

rm $WORKPATH/copyFiles-installer.sh

rm $WORKPATH/buildDEBs/build-installer.sh

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
