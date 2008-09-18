#!/usr/bin/perl

use strict;
use warnings;

sub make_dmg {
    my $mpkg = shift;
    my $volname = shift;
    my $pkgname = shift;
    my $dev_handle;
    my $size;

    die("Could not find \"$mpkg\"\n")
	if ! -d $mpkg;

    # calculate size
    $size = `du -sm "$mpkg"`;
    chomp $size;
    $size = $1 if $size =~ /^(\d+).*/;
    die("Could not calculate size of \"$mpkg\"\n")
	if (!$size || $size!~/\d+/);
    print "Calculated size \"$size\" MB\n";

    # add 2 megs just in case
    $size = $size + 2;

    # thanks to http://dev.simon-cozens.org/songbee/browser/release-manager-tools/build-dmg.sh
    `rm -fr dist`;
    `mkdir dist`;
    `hdiutil create "dist/$volname.dmg" -size ${size}m -fs HFS+ -volname "$volname"`;
    $dev_handle = `hdid dist/$volname.dmg | grep Apple_HFS`;
    chomp $dev_handle;
    $dev_handle = $1 if $dev_handle =~ /^\/dev\/(disk.)/;
    die("Could not obtain device handle\n") if !$dev_handle;
    print "Got device handle \"$dev_handle\"\n";
    `ditto "$mpkg" "/Volumes/$volname/$pkgname.mpkg"`;

    # set a volume icon if we have one
    if ( -f "VolumeIcon.icns" ) {
	`ditto VolumeIcon.icns "/Volumes/$volname/.VolumeIcon.icns"`;
    }
    if ( -d "background" ) {
	`mkdir "/Volumes/$volname/background"`;
	`ditto background "/Volumes/$volname/background/"`;
    }
    if ( -f "VolumeDSStore" ) {
	`ditto VolumeDSStore "/Volumes/$volname/.DS_Store"`
    }
    if ( -d "background" ) {
	`/Developer/Tools/SetFile -a V "/Volumes/$volname/background"`;
    }
    `/Developer/Tools/SetFile -a C "/Volumes/$volname/"`;
    `hdiutil detach $dev_handle`;
    `hdiutil convert dist/$volname.dmg -format UDZO -o dist/$volname.udzo.dmg`;
    `rm -f dist/$volname.dmg`;
    `mv dist/$volname.udzo.dmg dist/$volname.dmg`;
}

if (! defined $ARGV[0]) {
    die("Please specify the mpkg to make a DMG of as the first argument\n".
	"or -c to create a package and use it.\n");
}

if ( $ARGV[0] eq "-c" ) {
    die("TODO: -c\n");
    #make_dmg(make_mpkg(), "XBMC", "XBMC Media Center");
    exit;
}

make_dmg($ARGV[0], "XBMC", "XBMC Media Center");
