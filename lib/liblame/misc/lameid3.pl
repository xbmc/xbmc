#
#  From: Per Bolmstedt <tomten@kol14.com>
# 
#   AC> If someone has scripts that read input ID3 tags and convert
#   AC> them to args for lame (which then encodes the tags into the
#   AC> output files), let me know, too!
#
#  This is easy peasy using Perl.  Especially using Chris Nandor's excellent
#  MP3::Info package (available on CPAN).  Here's a program I just wrote that
#  I think does what you want.  Invoke it with "<program> <file> [options]"
#  (where the options can include an output filename), like for example:
#
#          lameid3.pl HQ.mp3 LQ.mp3 -fv
#
#  (Note how the syntax differs from that of Lame's.)  The program will
#  extract ID3 tags from the input file and invoke Lame with arguments for
#  including them.  (This program has not undergone any real testing..)

use MP3::Info;
use strict;

my %flds = ( 
	TITLE => 'tt',
	ARTIST => 'ta',
	ALBUM => 'tl',
	YEAR => 'ty',
	COMMENT => 'tc',
	GENRE => 'tg',
	TRACKNUM => 'tn'
	);

my $f = shift @ARGV;
my $s = "lame ${f} " . &makeid3args( $f ) . join ' ', @ARGV;
print STDERR "[${s}]\n";
system( $s );

sub makeid3args( $ )
{
	my $s;
	if ( my $tag = get_mp3tag( @_->[ 0 ] ) )
	{
		for ( keys %flds )
		{
			if ( $tag->{ $_ } )
			{
				$s .= sprintf(
					"--%s \"%s\" ",
					%flds->{ $_ },
					$tag->{ $_ } );
			}
		}
	}
	return $s || "";
}

