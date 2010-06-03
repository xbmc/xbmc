#!/usr/bin/perl

#   Copyright (C) 2008-2009 Team XBMC http://www.xbmc.org
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

##############################################################################
# Post install script
# -d4rk 09/13/08
##############################################################################

# use strict;    # Apple TV's Perl doesn't support this
# use warnings;  #

my $installer_dir = "/var/tmp/installer.teamxbmc.xbmc";

# install any plugins that were copied
sub copy_plugins {
    my $pluginsrc = "$installer_dir/plugins/";
    my $plugindst = get_home()."/Library/Application Support/XBMC/plugins/";
    
    `mkdir -p "$plugindst"`;
    `cp -r "$pluginsrc" "$plugindst"`;
}

sub setup_sources {
    my $xbmchome = get_xbmc_home();
    my $userdata = get_userdata_path();
    if (!$userdata) {
	print STDERR "Unable to obtain userdata path\n";
	exit;
    }

    my $sources = $userdata."/sources.xml";

    # create userdata directory if it doesn't exist
    `mkdir -p "$userdata"`;

    # check whether a sources.xml already exists, and if it does back it up
    if ( -f $sources ) {
	print STDERR "sources.xml found at \"$sources\", backing up\n";
	my $backup_sources = $sources.".".time().".xml";
	`cp -f \"$sources\" \"$backup_sources\"`;
    }

    # construct a sources.xml string
    my $sources_xml = get_sources_xml( get_default_sources() );
    if ( $sources_xml ) {
	open SOURCES, ">$sources" || exit;
	print SOURCES get_sources_xml( get_default_sources() );
	close SOURCES;
    }
}

sub get_userdata_path {
    my $xhome = get_xbmc_home();
    return if !defined $xhome;
    return "$xhome/userdata";
}

sub get_sources_xml {
    my $sources = shift;
    my ($sourcetype, $source);

    return if !defined $sources;

    my $xml = "<sources>\n";
    while ( ($sourcetype, $source) = each ( %$sources ) ) {
        $xml .= (" " x 4)."<$sourcetype>\n";
        $xml .= (" " x 8)."<default></default>\n";
        my ($name, $path);
        while ( ($name, $path) = each( %{ $source } ) ) {
            $xml .= (" " x 8)."<source>\n";
            $xml .= (" " x 12)."<name>$name</name>\n";
	    if ( $path =~ /(.*)\^\^(.*)/ ) {
		$xml .= (" " x 12)."<path>".$1."</path>\n";
		$xml .= (" " x 12)."<thumbnail>".$2."</thumbnail>\n";
	    }
	    else {
		$xml .= (" " x 12)."<path>".$path."</path>\n";
	    }
            $xml .= (" " x 8)."</source>\n";
        }
        $xml .= (" " x 4)."</$sourcetype>\n";
    }
    $xml .= "</sources>\n";
    return $xml;
}

sub get_home {
    return $ENV{'HOME'} if defined $ENV{'HOME'};
}

sub get_xbmc_home {
    my $os = get_os();
    my $home = get_home();
    return if !defined $home;
    if ( $os eq "osx" ) {
        return $home."/Library/Application Support/XBMC";
    }
    elsif ( $os eq "linux" ) {
        return $home."/.xbmc";
    }
    return;
}

sub get_os {
    if ( defined $ENV{'OSTYPE'} && $ENV{'OSTYPE'} =~ /linux/ ) {
        return "linux";
    }
    return "osx";
}

sub get_default_sources {
    my $sources = {};
    my $home = get_home();
    my $xbmchome = get_xbmc_home();
    return if !defined $xbmchome;

    $sources->{'programs'} = {};
    $sources->{'video'} = {};
    $sources->{'music'} = {};
    $sources->{'pictures'} = {};
    $sources->{'files'} = {};

    if ( get_os() eq "osx" ) {

        # Default sources for OS X
        for my $key ( keys %$sources ) {
            $sources->{$key}->{'Volumes'} = "/Volumes";
            $sources->{$key}->{'Home'} = "$home";
            $sources->{$key}->{'Desktop'} = "$home/Desktop";
        }
        
        $sources->{'music'}->{'Music'} = $home."/Music";
        $sources->{'music'}->{'Music Playlists'} = "special://musicplaylists";
        $sources->{'video'}->{'Movies'} = $home."/Movies";
        $sources->{'video'}->{'Video Playlists'} = "special://videoplaylists";
        $sources->{'pictures'}->{'Pictures'} = $home."/Pictures";

	my $plugindir = "$xbmchome/plugins/";

	# iPhoto
	if ( -f "$plugindir/pictures/iPhoto/default.py" ) {
	    $sources->{'pictures'}->{'iPhoto'} =
		"plugin://pictures/iPhoto/^^$plugindir/pictures/iPhoto/default.tbn";
	}
	
	# iTunes
	if ( -f "$plugindir/music/iTunes/default.py" ) {
	    $sources->{'music'}->{'iTunes'} =
		"plugin://music/iTunes/^^$plugindir/music/iTunes/default.tbn";
	}

	# AMT
	if ( -f "$plugindir/video/Apple Movie Trailers II/default.py" ) {
	    $sources->{'video'}->{'Apple Movie Trailers'} =
		"plugin://video/Apple Movie Trailers II/^^$plugindir/video/Apple Movie Trailers II/default.tbn";
	}
    }
    elsif ( get_os() eq "linux" ) {

        # Default source for Linux
        for my $key ( keys %$sources ) {
            $sources->{$key}->{'Media'} = "/media";
            $sources->{$key}->{'Home'} = "$home";
            $sources->{$key}->{'Desktop'} = "$home/Desktop";
            $sources->{$key}->{'Root'} = "/";
        }
    }
    return $sources;
}

sub cleanup() {
    `rm -rf "$installer_dir"`;
}

copy_plugins();
setup_sources();
cleanup();
