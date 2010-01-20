#!/usr/bin/perl

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
    `rm -fr dist`;
    `mkdir dist`;
    `hdiutil create -fs HFS+ -volname "$volname" -format UDRW -srcfolder "$mpkg" "dist/$volname.dmg"`;
    $dev_handle = `hdiutil attach -readwrite -noverify -noautoopen "dist/$volname.dmg" | grep Apple_HFS`;
    chomp $dev_handle;
    $dev_handle = $1 if $dev_handle =~ /^\/dev\/(disk.)/;
    die("Could not obtain device handle\n") if !$dev_handle;
    print "Got device handle \"$dev_handle\"\n";
    print "Ignore \"No space left on device\" warnings from ditto, they are an autosize artifact\n";
    `ditto "$mpkg" "/Volumes/$volname/$pkgname.$ext"`;

    # set a volume icon if we have one
    if ( -f "VolumeIcon.icns" ) {
	`ditto VolumeIcon.icns "/Volumes/$volname/.VolumeIcon.icns"`;
    }
    # make symlink to /Applications
    `ln -s /Applications "/Volumes/$volname/Applications"`;

    if ( -d "background" ) {
	`mkdir "/Volumes/$volname/background"`;
	`ditto background "/Volumes/$volname/background/"`;
    }
    if ( -f "VolumeDSStore" ) {
	`ditto VolumeDSStore "/Volumes/$volname/.DS_Store"` if $ext ne "app";
	`ditto VolumeDSStoreApp "/Volumes/$volname/.DS_Store"` if $ext eq "app";
    }
    if ( -d "background" ) {
	`/Developer/Tools/SetFile -a V "/Volumes/$volname/background"`;
    }
    `/Developer/Tools/SetFile -a C "/Volumes/$volname/"`;
    `hdiutil detach $dev_handle`;
    `hdiutil convert "dist/$volname.dmg" -format UDZO -imagekey zlib-level=9 -o "dist/$volname.udzo.dmg"`;
    `rm -f "dist/$volname.dmg"`;
    `mv "dist/$volname.udzo.dmg" "dist/$volname.dmg"`;
    `hdiutil internet-enable -yes "dist/$volname.dmg"`;
}

if (! defined $ARGV[0]) {
    die("Please specify the mpkg to make a DMG of as the first argument\n".
	"or -c to create a package and use it.\n");
}

if ( $ARGV[0] eq "-c" ) {
    die("TODO: -c\n");
    #make_dmg(make_mpkg(), "XBMC Atlantis - 8.10", "XBMC Media Center");
    exit;
}

make_dmg($ARGV[0], "XBMC", "XBMC");
