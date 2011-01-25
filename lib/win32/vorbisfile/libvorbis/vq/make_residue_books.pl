#!/usr/bin/perl

# quick, very dirty little script so that we can put all the
# information for building a residue book set (except the original
# partitioning) in one spec file.

#eg:

# >res0_128_128 interleaved
# haux 44c0_s/resaux_0.vqd res0_96_128aux 0,4,2 9
# :1 res0_128_128_1.vqd, 4, nonseq cull, 0 +- 1
# :2 res0_128_128_2.vqd, 4, nonseq, 0 +- 1(.7) 2
# :3 res0_128_128_3.vqd, 4, nonseq, 0 +- 1(.7) 3 5
# :4 res0_128_128_4.vqd, 2, nonseq, 0 +- 1(.7) 3 5 8 11
# :5 res0_128_128_5.vqd, 1, nonseq, 0 +- 1 3 5 8 11 14 17 20 24 28 31 35 39 


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

    # >res0_128_128
    if($line=~m/^>(\S+)\s+(\S*)/){
	# set the output name
	$globalname=$1;
	$interleave=$2;
	next;
    }

    # haux 44c0_s/resaux_0.vqd res0_96_128aux 0,4,2 9
    if($line=~m/^h(.*)/){
	# build a huffman book (no mapping) 
	my($name,$datafile,$bookname,$interval,$range)=split(' ',$1);
 
	# check the desired subdir to see if the data file exists
	if(-e $datafile){
	    my $command="cp $datafile $bookname.tmp";
	    print ">>> $command\n";
	    die "Couldn't access partition data file.\n\tcommand:$command\n" 
		if syst($command);

	    my $command="huffbuild $bookname.tmp $interval";
	    print ">>> $command\n";
	    die "Couldn't build huffbook.\n\tcommand:$command\n" 
		if syst($command);

	    my $command="rm $bookname.tmp";
	    print ">>> $command\n";
	    die "Couldn't remove temporary file.\n\tcommand:$command\n" 
		if syst($command);
	}else{
	    my $command="huffbuild $bookname.tmp 0-$range";
	    print ">>> $command\n";
	    die "Couldn't build huffbook.\n\tcommand:$command\n" 
		if syst($command);

	}
	next;
    }

    # :1 res0_128_128_1.vqd, 4, nonseq, 0 +- 1
    if($line=~m/^:(.*)/){
	my($namedata,$dim,$seqp,$vals)=split(',',$1);
	my($name,$datafile)=split(' ',$namedata);
	# build value list
	my$plusminus="+";
	my$list;
	my$thlist;
	my$count=0;
	foreach my$val (split(' ',$vals)){
	    if($val=~/\-?\+?\d+/){
		my$th;

		# got an explicit threshhint?
		if($val=~/([0-9\.]+)\(([^\)]+)/){
		    $val=$1;
		    $th=$2;
		}

		if($plusminus=~/-/){
		    $list.="-$val ";
		    if(defined($th)){
			$thlist.="," if(defined($thlist));
			$thlist.="-$th";
		    }
		    $count++;
		}
		if($plusminus=~/\+/){
		    $list.="$val ";
		    if(defined($th)){
			$thlist.="," if(defined($thlist));
			$thlist.="$th";
		    }
		    $count++;
		}
	    }else{
		$plusminus=$val;
	    }
	}
	die "Couldn't open temp file temp$$.vql: $!" unless
	    open(G,">temp$$.vql");
	print G "$count $dim 0 ";
	if($seqp=~/non/){
	    print G "0\n$list\n";
	}else{	
	    print G "1\n$list\n";
	}
	close(G);

	my $command="latticebuild temp$$.vql > $globalname$name.vqh";
	print ">>> $command\n";
	die "Couldn't build latticebook.\n\tcommand:$command\n" 
	    if syst($command);

	my $command="latticehint $globalname$name.vqh $thlist > temp$$.vqh";
	print ">>> $command\n";
	die "Couldn't pre-hint latticebook.\n\tcommand:$command\n" 
	    if syst($command);

	if(-e $datafile){
	
	    if($interleave=~/non/){
		$restune="res1tune";
	    }else{
		$restune="res0tune";
	    }
	    
	    if($seqp=~/cull/){
		my $command="$restune temp$$.vqh $datafile 1 > $globalname$name.vqh";
		print ">>> $command\n";
		die "Couldn't tune latticebook.\n\tcommand:$command\n" 
		    if syst($command);
	    }else{
		my $command="$restune temp$$.vqh $datafile > $globalname$name.vqh";
		print ">>> $command\n";
		die "Couldn't tune latticebook.\n\tcommand:$command\n" 
		    if syst($command);
	    }

	    my $command="latticehint $globalname$name.vqh $thlist > temp$$.vqh";
	    print ">>> $command\n";
	    die "Couldn't post-hint latticebook.\n\tcommand:$command\n" 
		if syst($command);

	}else{
	    print "No matching training file; leaving this codebook untrained.\n";
	}

	my $command="mv temp$$.vqh $globalname$name.vqh";
	print ">>> $command\n";
	die "Couldn't rename latticebook.\n\tcommand:$command\n" 
	    if syst($command);

	my $command="rm temp$$.vql";
	print ">>> $command\n";
	die "Couldn't remove temp files.\n\tcommand:$command\n" 
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
