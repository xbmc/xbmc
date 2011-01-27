#!/usr/bin/perl -w 
use strict;
use Tk;
use Tk::Xrm;
use Tk qw(exit); 

my $version="Analyzer 20020429";

my %bases;
my $first_file=undef;
my $last_file=undef;
my $fileno=0;

my @panel_labels;
my @panel_ones;
my @panel_twos;
my @panel_onevars;
my @panel_twovars;
my @panel_keys;
my $panel_count;

# pop the toplevels

my $toplevel=new MainWindow(-class=>'AnalyzerGraph');
my $Xname=$toplevel->Class;
$toplevel->optionAdd("$Xname.geometry",  "800x600",20);

my $geometry=$toplevel->optionGet('geometry','');
$geometry=~/^(\d+)x(\d+)/;

$toplevel->configure(-width=>$1);
$toplevel->configure(-height=>$2);





$toplevel->optionAdd("$Xname.background",  "#4fc627",20);
$toplevel->optionAdd("$Xname*highlightBackground",  "#80c0d3",20);
$toplevel->optionAdd("$Xname.Panel.background",  "#4fc627",20);
$toplevel->optionAdd("$Xname.Panel.foreground",  "#d0d0d0",20);
$toplevel->optionAdd("$Xname.Panel.font",
                     '-*-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Statuslabel.font",
                     '-*-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Statuslabel.foreground", "#606060");
$toplevel->optionAdd("$Xname*Status.font",
                     '-*-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*',20);

$toplevel->optionAdd("$Xname*AlertDetail.font",
                     '-*-helvetica-medium-r-*-*-10-*-*-*-*-*-*-*',20);


$toplevel->optionAdd("$Xname*background",  "#d0d0d0",20);
$toplevel->optionAdd("$Xname*foreground",  '#000000',20);

$toplevel->optionAdd("$Xname*Button*background",  "#f0d0b0",20);
$toplevel->optionAdd("$Xname*Button*foreground",  '#000000',20);
$toplevel->optionAdd("$Xname*Button*borderWidth",  '2',20);
$toplevel->optionAdd("$Xname*Button*relief",  'groove',20);
$toplevel->optionAdd("$Xname*Button*padY",  1,20);

#$toplevel->optionAdd("$Xname*Scale*background",  "#f0d0b0",20);
$toplevel->optionAdd("$Xname*Scale*foreground",  '#000000',20);
$toplevel->optionAdd("$Xname*Scale*borderWidth",  '1',20);
#$toplevel->optionAdd("$Xname*Scale*relief",  'groove',20);
$toplevel->optionAdd("$Xname*Scale*padY",  1,20);

$toplevel->optionAdd("$Xname*Checkbutton*background",  "#f0d0b0",20);
$toplevel->optionAdd("$Xname*Checkbutton*foreground",  '#000000',20);
$toplevel->optionAdd("$Xname*Checkbutton*borderWidth",  '2',20);
$toplevel->optionAdd("$Xname*Checkbutton*relief",  'groove',20);

$toplevel->optionAdd("$Xname*activeBackground",  "#ffffff",20);
$toplevel->optionAdd("$Xname*activeForeground",  '#0000a0',20);
$toplevel->optionAdd("$Xname*borderWidth",         0,20);
$toplevel->optionAdd("$Xname*relief",         'flat',20);
$toplevel->optionAdd("$Xname*activeBorderWidth",         1,20);
$toplevel->optionAdd("$Xname*highlightThickness",         0,20);
$toplevel->optionAdd("$Xname*padX",         2,20);
$toplevel->optionAdd("$Xname*padY",         2,20);
$toplevel->optionAdd("$Xname*font",    
                     '-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Entry.font",    
                     '-*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Exit.font",    
                     '-*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*',20);
$toplevel->optionAdd("$Xname*Exit.relief",          'groove',20);
$toplevel->optionAdd("$Xname*Exit.padX",          1,20);
$toplevel->optionAdd("$Xname*Exit.padY",          1,20);
$toplevel->optionAdd("$Xname*Exit.borderWidth",          2,20);
$toplevel->optionAdd("$Xname*Exit*background",  "#a0a0a0",20);
$toplevel->optionAdd("$Xname*Exit*disabledForeground",  "#ffffff",20);

#$toplevel->optionAdd("$Xname*Canvas.background",  "#c0c0c0",20);

$toplevel->optionAdd("$Xname*Entry.background",  "#ffffff",20);
$toplevel->optionAdd("$Xname*Entry.disabledForeground",  "#c0c0c0",20);
$toplevel->optionAdd("$Xname*Entry.relief",  "sunken",20);
$toplevel->optionAdd("$Xname*Entry.borderWidth",  1,20);

$toplevel->optionAdd("$Xname*Field.background",  "#ffffff",20);
$toplevel->optionAdd("$Xname*Field.disabledForeground",  "#c0c0c0",20);
$toplevel->optionAdd("$Xname*Field.relief",  "flat",20);
$toplevel->optionAdd("$Xname*Field.borderWidth",  1,20);

$toplevel->optionAdd("$Xname*Label.disabledForeground",  "#c0c0c0",20);
$toplevel->optionAdd("$Xname*Label.borderWidth",  1,20);

$toplevel->configure(-background=>$toplevel->optionGet("background",""));

#$toplevel->resizable(FALSE,FALSE);

my $panel=new MainWindow(-class=>'AnalyzerPanel');
my $X2name=$panel->Class;

$panel->optionAdd("$X2name.background",  "#353535",20);
$panel->optionAdd("$X2name*highlightBackground",  "#80c0d3",20);
$panel->optionAdd("$X2name.Panel.background",  "#353535",20);
$panel->optionAdd("$X2name.Panel.foreground",  "#4fc627",20);
$panel->optionAdd("$X2name.Panel.font",
                     '-*-helvetica-bold-o-*-*-18-*-*-*-*-*-*-*',20);
$panel->optionAdd("$X2name*Statuslabel.font",
                     '-*-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*',20);
$panel->optionAdd("$X2name*Statuslabel.foreground", "#4fc627",20);
$panel->optionAdd("$X2name*Status.font",
                     '-*-helvetica-bold-r-*-*-18-*-*-*-*-*-*-*',20);

$panel->optionAdd("$X2name*AlertDetail.font",
                     '-*-helvetica-medium-r-*-*-10-*-*-*-*-*-*-*',20);


$panel->optionAdd("$X2name*background",  "#d0d0d0",20);
$panel->optionAdd("$X2name*foreground",  '#000000',20);

$panel->optionAdd("$X2name*Button*background",  "#f0d0b0",20);
$panel->optionAdd("$X2name*Button*foreground",  '#000000',20);
$panel->optionAdd("$X2name*Button*borderWidth",  '2',20);
$panel->optionAdd("$X2name*Button*relief",  'groove',20);
$panel->optionAdd("$X2name*Button*padY",  1,20);

$panel->optionAdd("$X2name*Checkbutton*background",  "#f0d0b0",20);
$panel->optionAdd("$X2name*Checkbutton*foreground",  '#000000',20);
$panel->optionAdd("$X2name*Checkbutton*borderWidth",  '2',20);
#$panel->optionAdd("$X2name*Checkbutton*padX",  '0',20);
#$panel->optionAdd("$X2name*Checkbutton*padY",  '0',20);
#$panel->optionAdd("$X2name*Checkbutton*relief",  'groove',20);

$panel->optionAdd("$X2name*activeBackground",  "#ffffff",20);
$panel->optionAdd("$X2name*activeForeground",  '#0000a0',20);
$panel->optionAdd("$X2name*borderWidth",         0,20);
$panel->optionAdd("$X2name*relief",         'flat',20);
$panel->optionAdd("$X2name*activeBorderWidth",         1,20);
$panel->optionAdd("$X2name*highlightThickness",         0,20);
$panel->optionAdd("$X2name*padX",         2,20);
$panel->optionAdd("$X2name*padY",         2,20);
$panel->optionAdd("$X2name*font",    
                     '-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*',20);
$panel->optionAdd("$X2name*Entry.font",    
                     '-*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*',20);

$panel->optionAdd("$X2name*Exit.font",    
                     '-*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*',20);
$panel->optionAdd("$X2name*Exit.relief",          'groove',20);
$panel->optionAdd("$X2name*Exit.padX",          1,20);
$panel->optionAdd("$X2name*Exit.padY",          1,20);
$panel->optionAdd("$X2name*Exit.borderWidth",          2,20);
$panel->optionAdd("$X2name*Exit*background",  "#a0a0a0",20);
$panel->optionAdd("$X2name*Exit*disabledForeground",  "#ffffff",20);

$panel->optionAdd("$X2name*Entry.background",  "#ffffff",20);
$panel->optionAdd("$X2name*Entry.disabledForeground",  "#c0c0c0",20);
$panel->optionAdd("$X2name*Entry.relief",  "sunken",20);
$panel->optionAdd("$X2name*Entry.borderWidth",  1,20);

$panel->optionAdd("$X2name*Field.background",  "#ffffff",20);
$panel->optionAdd("$X2name*Field.disabledForeground",  "#c0c0c0",20);
$panel->optionAdd("$X2name*Field.relief",  "flat",20);
$panel->optionAdd("$X2name*Field.borderWidth",  1,20);

$panel->optionAdd("$X2name*Label.disabledForeground",  "#c0c0c0",20);
$panel->optionAdd("$X2name*Label.borderWidth",  1,20);

$panel->configure(-background=>$panel->optionGet("background",""));

#$panel->resizable("FALSE","FALSE");

my $panel_shell=$panel->Label(Name=>"shell",borderwidth=>1,relief=>'raised')->
    place(-x=>10,-y=>36,-relwidth=>1.0,-relheight=>1.0,
          -width=>-20,-height=>-46,-anchor=>'nw');

my $panel_quit=$panel_shell->Button(-class=>"Exit",text=>"quit",-command=>[sub{Shutdown()}])->
    place(-x=>-1,-y=>-1,-relx=>1.0,-rely=>1.0,-anchor=>'se');

$panel->Label(Name=>"logo text",-class=>"Panel",text=>$version)->
    place(-x=>5,-y=>5,-anchor=>'nw');


my $graph_shell=$toplevel->Label(Name=>"shell",borderwidth=>1,relief=>'raised')->
    place(-x=>10,-y=>36,-relwidth=>1.0,-relheight=>1.0,
          -width=>-20,-height=>-46,-anchor=>'nw');

my $graph_status=$toplevel->Label(Name=>"logo text",-class=>"Panel",text=>"Starting up")->
    place(-x=>5,-y=>5,-anchor=>'nw');


my $panely=5;
my $panel_rescan=$panel_shell->Button(text=>"rescan",command=>[sub{scan_directory()}])->
    place(-x=>-5,-relx=>1.,-y=>$panely,-anchor=>'ne');
$panely+=$panel_rescan->reqheight()+6;


my$temp=$graph_shell->Button(-text=>"<<",
		     -command=>[sub{$fileno-=10;$fileno=$first_file if($fileno<$first_file);
				load_graph();}])->
    place(-x=>5,-y=>-5,-rely=>1.,-relwidth=>.2,-width=>-5,-anchor=>'sw');
$graph_shell->Button(-text=>">>",
		     -command=>[sub{$fileno+=10;$fileno=$last_file if($fileno>$last_file);
				load_graph();}])->
    place(-x=>-5,-y=>-5,-relwidth=>.2,-rely=>1.,-width=>-5,-relx=>1.,-anchor=>'se');
$graph_shell->Button(-text=>"<",
		     -command=>[sub{$fileno-=1;$fileno=$first_file if($fileno<$first_file);
				load_graph();}])->
    place(-x=>5,-y=>-5,-relwidth=>.3,-width=>-7,-rely=>1.,-relx=>.2,-anchor=>'sw');
$graph_shell->Button(-text=>">",
		     -command=>[sub{$fileno+=1;$fileno=$last_file if($fileno>$last_file);
				load_graph();}])->
    place(-x=>-5,-y=>-5,-relwidth=>.3,-width=>-7,-rely=>1.,-relx=>.8,-anchor=>'se');
my$graphy=-10-$temp->reqheight();
my$graph_slider=$temp=$graph_shell->Scale(-bigincrement=>1,
				    -resolution=>1,
				    -showvalue=>'TRUE',-variable=>\$fileno,-orient=>'horizontal')->
    place(-x=>5,-y=>$graphy,-relwidth=>1.,-rely=>1.,-width=>-10,-anchor=>'sw');
$graphy-=$temp->reqheight()+5;

my$onecrop;
my$twocrop;

my$oneresize=$temp=$graph_shell->Checkbutton(text=>"rescale",-variable=>\$onecrop,
				-command=>[sub{draw_graph();}])->
    place(-x=>5,-y=>5,-anchor=>'nw');

my$one=$graph_shell->Canvas()->
    place(-relwidth=>1.,-width=>-10,-relheight=>.5,-height=>($graphy/2)-5-$temp->reqheight(),
				     -x=>5,-y=>5+$temp->reqheight,-anchor=>'nw');


my$tworesize=$temp=$graph_shell->Checkbutton(text=>"rescale",-variable=>\$twocrop,
				-command=>[sub{draw_graph();}])->
    place(-rely=>1.,-y=>5,-anchor=>'nw',-in=>$one);
my$two=$graph_shell->Canvas()->
    place(-relwidth=>1.,-relheight=>1.,-rely=>1.,-y=>5+$temp->reqheight(),-anchor=>'nw',-in=>$one);

scan_directory();

my%onestate;
my%twostate;
my @data;

$onestate{"canvas"}=$one;
$onestate{"vars"}=\@panel_onevars;
$twostate{"canvas"}=$two;
$twostate{"vars"}=\@panel_twovars;

$graph_slider->configure(-command=>[sub{load_graph()}]);
load_graph();
$toplevel->bind('MainWindow','<Configure>',[sub{$toplevel->update();
						draw_graph()}]);

Tk::MainLoop();

sub load_graph{

    scan_directory()if(!defined($panel_count));

    @data=undef;
    
    for(my$i=0;$i<$panel_count;$i++){
	my$filename=$panel_keys[$i]."_$fileno.m";
	if(open F, "$filename"){
	    $data[$i]=[(<F>)];
	    close F;
	}
    }
    draw_graph();
}

sub graphhelper{
    my($graph)=@_;
    my$count=0;
    my@colors=("#ff0000","#00df00","#0000ff","#ffff00","#ff00ff","#00ffff","#ffffff",
	       "#9f0000","#007f00","#00009f","#8f8f00","#8f008f","#008f8f","#000000");
    
    my$w=$graph->{"canvas"};
    my$rescale=0;

    Status("Plotting $fileno");
    $w->delete('foo');
    $w->delete('legend');
    $w->delete('lines');

    # count range 
    for(my$i=0;$i<$panel_count;$i++){
	if($graph->{"vars"}->[$i]){
	    if(defined($data[$i])){
		if(!defined($graph->{"minx"})){
		    $data[$i]->[0]=~m/^\s*(-?[0-9\.]*)[ ,]+(-?[0-9\.]*)/;
		    $graph->{"maxx"}=$1;
		    $graph->{"minx"}=$1;
		    $graph->{"maxy"}=$2;
		    $graph->{"miny"}=$2;
		    $rescale=1;
		}
		
		for(my$j=0;$j<=$#{$data[$i]};$j++){
		    $data[$i]->[$j]=~m/^\s*(-?[0-9\.]*)[ ,]+(-?[0-9\.]*)/;
		    $rescale=1 if($1>$graph->{"maxx"});
		    $rescale=1 if($1<$graph->{"minx"});
		    $rescale=1 if($2>$graph->{"maxy"});
		    $rescale=1 if($2<$graph->{"miny"});
		    $graph->{"maxx"}=$1 if($1>$graph->{"maxx"});
		    $graph->{"minx"}=$1 if($1<$graph->{"minx"});
		    $graph->{"maxy"}=$2 if($2>$graph->{"maxy"});
		    $graph->{"miny"}=$2 if($2<$graph->{"miny"});
		}
	    }
	    $count++;
	}
    }

    my$width=$w->width();
    my$height=$w->height();

    $rescale=1 if(!defined($graph->{"width"}) || 
		  $width!=$graph->{"width"} || 
		  $height!=$graph->{"height"});
    
    $graph->{"width"}=$width; 
    $graph->{"height"}=$height; 

    if(defined($graph->{"maxx"})){
	# draw axes, labels
	# look for appropriate axis scales

	if($rescale){

	    $w->delete('ylabel');
	    $w->delete('xlabel');
	    $w->delete('axes');
	    
	    my$yscale=1.;
	    my$xscale=1.;
	    my$iyscale=1.;
	    my$ixscale=1.;
	    while(($graph->{"maxx"}-$graph->{"minx"})*$xscale>15){$xscale*=.1;$ixscale*=10.;}
	    while(($graph->{"maxy"}-$graph->{"miny"})*$yscale>15){$yscale*=.1;$iyscale*=10.;}
	    
	    while(($graph->{"maxx"}-$graph->{"minx"})*$xscale<3){$xscale*=10.;$ixscale*=.1;}
	    while(($graph->{"maxy"}-$graph->{"miny"})*$yscale<3){$yscale*=10.;$iyscale*=.1;}
	    
	    # how tall are the x axis labels?
	    $w->createText(-1,-1,-anchor=>'se',-tags=>['foo'],-text=>"0123456789.");
	    my($x1,$y1,$x2,$y2)=$w->bbox('foo');
	    $w->delete('foo');
	    my$maxlabelheight=$y2-$y1;
	    my$useabley=$height-$maxlabelheight-3;
	    my$pixelpery=$useabley/($graph->{"maxy"}-$graph->{"miny"});
	    
	    # place y axis labels at proper spacing/height
	    my$lasty=-$maxlabelheight/2;
	    my$topyval=int($graph->{"maxy"}*$yscale+1.)*$iyscale;
	    
	    for(my$i=0;;$i++){
		my$yval= $topyval-$i*$iyscale;
		my$y= ($graph->{"maxy"}-$yval)*$pixelpery;
		last if($y>$useabley);
		if($y-$maxlabelheight>=$lasty){
		    $w->createText(0,$y,-anchor=>'e',-tags=>['ylabel'],-text=>"$yval");
		    $lasty=$y;
		}
	    }
	    
	    # get the max ylabel width and place them at proper x
	    ($x1,$y1,$x2,$y2)=$w->bbox('ylabel');
	    my$maxylabelwidth=$x2-$x1;
	    $w->move('ylabel',$maxylabelwidth,0);
	    
	    my$beginx=$maxylabelwidth+3;
	    my$useablex=$width-$beginx;
	    
	    # draw basic axes
	    $w->createLine($beginx,0,$beginx,$useabley,$width,$useabley,
			   -tags=>['axes'],-width=>2);
	    # draw y tix
	    $lasty=-$maxlabelheight/2;
	    for(my$i=0;;$i++){
		my$yval= $topyval-$i*$iyscale;
		my$y= ($graph->{"maxy"}-$yval)*$pixelpery;
		last if($y>$useabley);
		if($yval==0){
		    $w->createLine($beginx,$y,$width,$y,
				   -tags=>['axes'],-width=>1);
		}else{
		    if($y-$maxlabelheight>=$lasty){
			$w->createLine($beginx,$y,$width,$y,
				       -tags=>['axes'],-width=>1,
				       -stipple=>'gray50');
			
			$lasty=$y;
		    }
		}
	    }
	    
	    # place x axis labels at proper spacing
	    my$topxval=int($graph->{"maxx"}*$xscale+1.)*$ixscale;
	    my$pixelperx=$useablex/($graph->{"maxx"}-$graph->{"minx"});
	    
	    for(my$i=0;;$i++){
		my$xval= $topxval-$i*$ixscale;
		my$x= $width-($graph->{"maxx"}-$xval)*$pixelperx;
		
		last if($x<$beginx);
		# bounding boxen are hard.  place temp labels.
		$w->createText(-1,-1,-anchor=>'e',-tags=>['foo'],-text=>"$xval");
	    }
	    
	    ($x1,$y1,$x2,$y2)=$w->bbox('foo');
	    my$maxxlabelwidth=$x2-$x1;
	    $w->delete('foo');
	    my$lastx=$width;
	    
	    for(my$i=0;;$i++){
		my$xval= $topxval-$i*$ixscale;
		my$x= $width-($graph->{"maxx"}-$xval)*$pixelperx;
		
		last if($x-$maxxlabelwidth/2<0 || $x<$beginx);
		if($xval==0 && $x<$width){
		    $w->createLine($x,0,$x,$useabley,-tags=>['axes'],-width=>1);
		}
	    
		if($x+$maxxlabelwidth<=$lastx){
		    $w->createText($x,$height-1,-anchor=>'s',-tags=>['xlabel'],-text=>"$xval");
		    $w->createLine($x,0,$x,$useabley,-tags=>['axes'],-width=>1,-stipple=>"gray50");
		    $lastx=$x;
		}
	    }
	    $graph->{"labelheight"}=$maxlabelheight;
	    $graph->{"xo"}=$beginx;
	    $graph->{"ppx"}=$pixelperx;
	    $graph->{"ppy"}=$pixelpery;
	}

	# plot the files
	$count=0;
	my$legendy=$graph->{"labelheight"}/2;
	for(my$i=0;$i<$panel_count;$i++){
	    if($graph->{"vars"}->[$i]){
		$count++; # count here for legend color selection stability
		if(defined($data[$i])){
		    # place a legend placard;
		    my$color=$colors[($count-1)%($#colors+1)];
		    $w->createText($width,$legendy,-anchor=>'e',-tags=>['legend'],
				   -fill=>$color,-text=>$panel_keys[$i]);
		    $legendy+=$graph->{"labelheight"};

		    # plot the lines
		    my@pairs=map{if(/^\s*(-?[0-9\.]*)[ ,]+(-?[0-9\.]*)/){
			(($1-$graph->{"minx"})*$graph->{"ppx"}+$graph->{"xo"},
			 (-$2+$graph->{"maxy"})*$graph->{"ppy"})}} (@{$data[$i]});
		    
		    $w->createLine((@pairs),-fill=>$color,-tags=>['lines']);
		}
	    }
	}
    }
}    

sub draw_graph{

    if($onecrop){
	$onestate{"minx"}=undef;
	$onestate{"miny"}=undef;
	$onestate{"maxx"}=undef;
	$onestate{"maxy"}=undef;
    }
    if($twocrop){
	$twostate{"minx"}=undef;
	$twostate{"miny"}=undef;
	$twostate{"maxx"}=undef;
	$twostate{"maxy"}=undef;
    }

    for(my$i=0;$i<$panel_count;$i++){
	if($twostate{"vars"}->[$i]){
	    
	    #re-place the canvases
	    
	    $oneresize->place(-x=>5,-y=>5,-anchor=>'nw');

	    $one->place(-relwidth=>1.,-width=>-10,-relheight=>.5,
			 -height=>($graphy/2)-5-$oneresize->reqheight(),
			 -x=>5,-y=>5+$oneresize->reqheight,-anchor=>'nw');
	    
	    $tworesize->place(-rely=>1.,-y=>5,-anchor=>'nw',-in=>$one);
	    $two->place(-relwidth=>1.,-relheight=>1.,-rely=>1.,
			-y=>5+$tworesize->reqheight(),-anchor=>'nw',-in=>$one);

	    graphhelper(\%onestate);
	    graphhelper(\%twostate);
	    return;
	}
    }

    $oneresize->place(-x=>5,-y=>5,-anchor=>'nw');
    
    $one->place(-relwidth=>1.,-width=>-10,-relheight=>1.,
		 -height=>$graphy-5-$oneresize->reqheight(),
		 -x=>5,-y=>5+$oneresize->reqheight,-anchor=>'nw');
    
    $tworesize->placeForget();
    $two->placeForget();

    graphhelper(\%onestate);
}

sub depopulate_panel{
    my $win;
    foreach $win (@panel_labels){
	$win->destroy();
    }
    @panel_labels=();
    foreach $win (@panel_ones){
	$win->destroy();
    }
    @panel_ones=();
    foreach $win (@panel_twos){
	$win->destroy();
    }
    @panel_twos=();
    @panel_keys=();
}

sub populate_panel{
    my $localy=$panely;
    my $key;
    my $i=0;
    foreach $key (sort (keys %bases)){
	$panel_keys[$i]=$key;
	if(!defined($panel_onevars[$i])){
	    $panel_onevars[$i]=0;
	    $panel_twovars[$i]=0;
	}

	my $temp=$panel_twos[$i]=$panel_shell->
	    Checkbutton(-variable=>\$panel_twovars[$i],-command=>['main::draw_graph'],text=>'2')->
	    place(-y=>$localy,-x=>-5,-anchor=>"ne",-relx=>1.);
	my $oney=$temp->reqheight();
	my $onex=$temp->reqwidth()+15;

	$temp=$panel_ones[$i]=$panel_shell->
	    Checkbutton(-variable=>\$panel_onevars[$i],-command=>['main::draw_graph'],text=>'1')->
	    place(-y=>0,-x=>0,-anchor=>"ne",-in=>$temp,-bordermode=>'outside');
	$oney=$temp->reqheight() if ($oney<$temp->reqheight());
	$onex+=$temp->reqwidth();

	$temp=$panel_labels[$i]=$panel_shell->Label(-text=>$key,-class=>'Field',justify=>'left')->
	    place(-y=>$localy,-x=>5,-anchor=>"nw",-relwidth=>1.,-width=>-$onex,
		  -bordermode=>'outside');
	$oney=$temp->reqheight() if ($oney<$temp->reqheight());
	
	$localy+=$oney+2;
	$i++;
    }
    $panel_count=$i;    

    $localy+=$panel_quit->reqheight()+50;
    my $geometry=$panel->geometry();
    $geometry=~/^(\d+)/;

    $panel->configure(-height=>$localy);
    $panel->configure(-width=>$1);
}

sub Shutdown{
  Tk::exit();
}

sub Status{
    my$text=shift @_;
    $graph_status->configure(text=>"$text");
    $toplevel->update();
}

sub scan_directory{
    
    %bases=();
    my$count=0;

    $first_file=undef;
    $last_file=undef;

    if(opendir(D,".")){
        my$file;
        while(defined($file=readdir(D))){
	    if($file=~m/^(\S*)_(\d+).m/){
		$bases{"$1"}="0";
		$first_file=$2 if(!defined($first_file) || $2<$first_file);
		$last_file=$2 if(!defined($last_file) || $2>$last_file);
		$count++;
		
		Status("Reading... $count")if($count%117==0);
	    }
        }	
        closedir(D);
    }
    Status("Done Reading: $count files");
    depopulate_panel();
    populate_panel();
    
    $fileno=$first_file if($fileno<$first_file);
    $fileno=$last_file if($fileno>$last_file);

    $graph_slider->configure(-from=>$first_file,-to=>$last_file);

}





