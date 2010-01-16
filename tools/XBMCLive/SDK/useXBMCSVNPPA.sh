#!/bin/sh

# Add XBMC SVN PPA to sources

echo "deb http://ppa.launchpad.net/team-xbmc-svn/ppa/ubuntu/ karmic main" >> buildLive/Files/chroot_sources/xbmc-svn.list.chroot
echo "deb-src http://ppa.launchpad.net/team-xbmc-svn/ppa/ubuntu/ karmic main" >> buildLive/Files/chroot_sources/xbmc-svn.list.chroot

echo "deb http://ppa.launchpad.net/team-xbmc-svn/ppa/ubuntu/ karmic main" >> buildLive/Files/chroot_sources/xbmc-svn.list.binary
echo "deb-src http://ppa.launchpad.net/team-xbmc-svn/ppa/ubuntu/ karmic main" >> buildLive/Files/chroot_sources/xbmc-svn.list.binary
