#!/usr/bin/perl

if($#ARGV<0){
    &usage;
}

$|=1;

$bands=0;

foreach $arg (@ARGV){
    ($entries[$bands],$file[$bands])=split(/,/,$arg);
    $bands++;
}

# no time to deal with globs right now
if($bands>0){
    die unless open(ONE,"<".$file[0]);
}
if($bands>1){
    die unless open(TWO,"<".$file[1]);
}
if($bands>2){
    die unless open(THREE,"<".$file[2]);
}


while (<ONE>) {    
    my@nums = ();
    @nums = split(/,/);
    my$cols=$#nums;
    for($i=0;$i<$cols;$i++){
	print 0+$nums[$i].", ";
    }
    if($bands>1){
	$_=<TWO>;
	@nums = ();
	@nums = split(/,/);
	$cols=$#nums;
	for($i=0;$i<$cols;$i++){
	    print $nums[$i]+$entries[0].", ";
	}
	if($bands>2){
	    $_=<THREE>;
	    @nums = ();
	    @nums = split(/,/);
	    $cols=$#nums;
	    for($i=0;$i<$cols;$i++){
		print $nums[$i]+$entries[0]+$entries[1].", ";
	    }
	}
    }
    print "\n";

}

if($bands>0){
    close ONE;
}
if($bands>1){
    close TWO;
}
if($bands>2){
    close THREE;
}
    
sub usage{
    print "\nOggVorbis auxbook spectral partitioner\n\n";
    print "auxpartition.pl <part_entries>,file [<part_entries>,file...]\n\n";
    exit(1);
}
