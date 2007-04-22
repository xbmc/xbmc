#!/usr/bin/perl -w
# mailcap to links.cfg converter (aka quick hack)
# version 1.00 by <grin@tolna.net>
# Released under GPLv2 or later
#
# Usage: mailcap-convert.pl /etc/mailcap >> ~/.links/links.cfg
#

print "association \"-=BEGIN DEBIAN CONVERT=-\" \"\" \"\" 23 1\n";
while( <> ) {
    chomp;
    next if /^\s*(#|$)/;
    @fields = split /;\s*/;
    # change %s to % in the command
    $fields[1] =~ s/%s/%/g;
    
    my @out = ( "External association", $fields[0], $fields[1] );
    
    for( my $i=2; $i<=$#fields; $i++ ) {
        if( $fields[$i] =~ m/description="?([^"]+)"?/ ) {
            # description
            $out[0] = $1;
        } elsif( $fields[$i] =~ m/nametemplate=(.+)/ ) {
            # extension for the mime type
            my $ext = $1;
            $ext =~ s/%s\.(.+)$/$1/;
            &new_ext($ext,$fields[0]);
        }
    }
    &new_assoc( \@out );
}
print "association \"-=END DEBIAN CONVERT=-\" \"\" \"\" 23 1\n";

sub new_assoc {
    my $aref = shift;
    print "association ";
    for my $i (0..2) {
        print "\"$aref->[$i]\" ";
    }
    print "23 1\n";
}

sub new_ext {
    print "extension";
    for my $i (0..1) {
        print " \"$_[$i]\"";
    }
    print "\n";
}


