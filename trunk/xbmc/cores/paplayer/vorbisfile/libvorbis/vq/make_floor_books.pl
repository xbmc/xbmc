#!/usr/bin/perl

# quick, very dirty little script so that we can put all the
# information for building a floor book set in one spec file.

#eg:

# >floor_44
# =44c0_s 44c1_s 44c2_s
# build line_128x4_class0 0-256
# build line_128x4_0sub0  0-4

die "Could not open $ARGV[0]: $!" unless open (F,$ARGV[0]);

$goflag=0;
while($line=<F>){

    print "#### $line";
    if($line=~m/^GO/){
	$goflag=1;
	next;
    }

    if($goflag==0){
	if($line=~m/\S+/ && !($line=~m/^\#/) ){
	    my $command=$line;
	    print ">>> $command";
	    die "Couldn't shell command.\n\tcommand:$command\n" 
		if syst($command);
	}
	next;
    }

    # >floor_44
    # this sets the output bookset file name
    if($line=~m/^>(\S+)\s+(\S*)/){
	# set the output name
	$globalname=$1;
	
	$command="rm -f $globalname.vqh";
	die "Couldn't remove file.\n\tcommand:$command\n" 
	    if syst($command);

	next;
    }

    #=path1 path2 path3 
    #set the search path for input files; each build line will look
    #for input files in all of the directories in the search path and
    #append them for huffbuild input
    if($line=~m/^=(.*)/){
	# set the output name
	@paths=split(' ',$1);
	next;
    }

    # build book.vqd 0-3 [noguard]
    if($line=~m/^build (.*)/){
	# build a huffman book (no mapping) 
	my($datafile,$range,$guard)=split(' ',$1);
 
	$command="rm -f $datafile.tmp";
	print "\n\n>>> $command\n";
	die "Couldn't remove temp file.\n\tcommand:$command\n" 
	    if syst($command);

	# first find all the inputs we want; they'll need to be collected into a single input file
	foreach $dir (@paths){
	    if (-e "$dir/$datafile.vqd"){
		$command="cat $dir/$datafile.vqd >> $datafile.tmp";
		print ">>> $command\n";
		die "Couldn't append training data.\n\tcommand:$command\n" 
		    if syst($command);
	    }
	}
	
	my $command="huffbuild $datafile.tmp $range $guard";
	print ">>> $command\n";
	die "Couldn't build huffbook.\n\tcommand:$command\n" 
	    if syst($command);

	$command="cat $datafile.vqh >> $globalname.vqh";
	print ">>> $command\n";
	die "Couldn't append to output book.\n\tcommand:$command\n" 
	    if syst($command);

	$command="rm $datafile.vqh";
	print ">>> $command\n";
	die "Couldn't remove temporary output file.\n\tcommand:$command\n" 
	    if syst($command);

	$command="rm -f $datafile.tmp";
	print ">>> $command\n";
	die "Couldn't remove temporary output file.\n\tcommand:$command\n" 
	    if syst($command);
	next;
    }

}

$command="rm -f temp$$.vqd";
print ">>> $command\n";
die "Couldn't remove temp files.\n\tcommand:$command\n" 
    if syst($command);

sub syst{
    system(@_)/256;
}
