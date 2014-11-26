#!/usr/bin/perl

#   Copyright (C) 2008-2013 Team Kodi
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

use strict;
use warnings;

sub make_dmg {
    my $mpkg = shift;
    my $volname = shift;
    my $pkgname = shift;
    my $dev_handle;

    die("Could not find \"$mpkg\"\n")
	if ! -d $mpkg;

    my $ext = $1 if $mpkg =~ /.*\.(.*?)$/;
    $ext = "mpkg" if !$ext;

    # thanks to http://dev.simon-cozens.org/songbee/browser/release-manager-tools/build-dmg.sh
    `hdiutil create -fs HFS+ -volname "$pkgname" -format UDRW -srcfolder "$mpkg" "$volname.dmg"`;
    $dev_handle = `hdiutil attach -readwrite -noverify -noautoopen "$volname.dmg" | grep Apple_HFS`;
    chomp $dev_handle;
    $dev_handle = $1 if $dev_handle =~ /^\/dev\/(disk.)/;
    die("Could not obtain device handle\n") if !$dev_handle;
    print "Got device handle \"$dev_handle\"\n";
    #clear the volume - we will copy stuff on it with ditto later
    #this removes crap which might have come in via the srcfolder 
    #parameter of hdiutil above
    `rm -r /Volumes/$pkgname/*`;
    print "Ignore \"No space left on device\" warnings from ditto, they are an autosize artifact\n";
    `ditto "$mpkg" "/Volumes/$pkgname/$pkgname.$ext"`;

    # set a volume icon if we have one
    if ( -f "VolumeIcon.icns" ) {
	`ditto VolumeIcon.icns "/Volumes/$pkgname/.VolumeIcon.icns"`;
    }
    # make symlink to /Applications
    `ln -s /Applications "/Volumes/$pkgname/Applications"`;

    `mkdir "/Volumes/$pkgname/background"`;
    `ditto ../media/osx/background "/Volumes/$pkgname/background/"`;
    `xcrun SetFile -a V "/Volumes/$pkgname/background"`;
    `xcrun SetFile -a C "/Volumes/$pkgname/"`;
    `cp VolumeDSStoreApp "/Volumes/$pkgname/.DS_Store"`;
    `hdiutil detach $dev_handle`;
    `hdiutil convert "$volname.dmg" -format UDZO -imagekey zlib-level=9 -o "$volname.udzo.dmg"`;
    `rm -f "$volname.dmg"`;
    `mv "$volname.udzo.dmg" "$volname.dmg"`;
    `hdiutil internet-enable -yes "$volname.dmg"`;
}

if (! defined $ARGV[0]) {
    die("Please specify the mpkg to make a DMG of as the first argument\n".
	"or -c to create a package and use it.\n");
}

if ( $ARGV[0] eq "-c" ) {
    die("TODO: -c\n");
    #make_dmg(make_mpkg(), "Kodi Atlantis - 8.10", "Kodi Media Center");
    exit;
}

make_dmg($ARGV[0], $ARGV[1], "Kodi");
