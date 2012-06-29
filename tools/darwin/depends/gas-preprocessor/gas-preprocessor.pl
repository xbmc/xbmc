#!/usr/bin/env perl
# by David Conrad
# This code is licensed under GPLv2 or later; go to gnu.org to read it
#  (not that it much matters for an asm preprocessor)
# usage: set your assembler to be something like "perl gas-preprocessor.pl gcc"
use strict;

# Apple's gas is ancient and doesn't support modern preprocessing features like
# .rept and has ugly macro syntax, among other things. Thus, this script
# implements the subset of the gas preprocessor used by x264 and ffmpeg
# that isn't supported by Apple's gas.

my @gcc_cmd = @ARGV;
my @preprocess_c_cmd;

my $fix_unreq = $^O eq "darwin";

if ($gcc_cmd[0] eq "-fix-unreq") {
    $fix_unreq = 1;
    shift @gcc_cmd;
} elsif ($gcc_cmd[0] eq "-no-fix-unreq") {
    $fix_unreq = 0;
    shift @gcc_cmd;
}

if (grep /\.c$/, @gcc_cmd) {
    # C file (inline asm?) - compile
    @preprocess_c_cmd = (@gcc_cmd, "-S");
} elsif (grep /\.[sS]$/, @gcc_cmd) {
    # asm file, just do C preprocessor
    @preprocess_c_cmd = (@gcc_cmd, "-E");
} else {
    die "Unrecognized input filetype";
}

# if compiling, avoid creating an output file named '-.o'
if ((grep /^-c$/, @gcc_cmd) && !(grep /^-o/, @gcc_cmd)) {
    foreach my $i (@gcc_cmd) {
        if ($i =~ /\.[csS]$/) {
            my $outputfile = $i;
            $outputfile =~ s/\.[csS]$/.o/;
            push(@gcc_cmd, "-o");
            push(@gcc_cmd, $outputfile);
            last;
        }
    }
}
@gcc_cmd = map { /\.[csS]$/ ? qw(-x assembler -) : $_ } @gcc_cmd;
@preprocess_c_cmd = map { /\.o$/ ? "-" : $_ } @preprocess_c_cmd;

my $comm;

# detect architecture from gcc binary name
if      ($gcc_cmd[0] =~ /arm/) {
    $comm = '@';
} elsif ($gcc_cmd[0] =~ /powerpc|ppc/) {
    $comm = '#';
}

# look for -arch flag
foreach my $i (1 .. $#gcc_cmd-1) {
    if ($gcc_cmd[$i] eq "-arch") {
        if ($gcc_cmd[$i+1] =~ /arm/) {
            $comm = '@';
        } elsif ($gcc_cmd[$i+1] =~ /powerpc|ppc/) {
            $comm = '#';
        }
    }
}

# assume we're not cross-compiling if no -arch or the binary doesn't have the arch name
if (!$comm) {
    my $native_arch = qx/arch/;
    if ($native_arch =~ /arm/) {
        $comm = '@';
    } elsif ($native_arch =~ /powerpc|ppc/) {
        $comm = '#';
    }
}

if (!$comm) {
    die "Unable to identify target architecture";
}

my %ppc_spr = (ctr    => 9,
               vrsave => 256);

open(ASMFILE, "-|", @preprocess_c_cmd) || die "Error running preprocessor";

my $current_macro = '';
my $macro_level = 0;
my %macro_lines;
my %macro_args;
my %macro_args_default;
my $macro_count = 0;
my $altmacro = 0;

my @pass1_lines;
my @ifstack;

my %symbols;

# pass 1: parse .macro
# note that the handling of arguments is probably overly permissive vs. gas
# but it should be the same for valid cases
while (<ASMFILE>) {
    # remove all comments (to avoid interfering with evaluating directives)
    s/(?<!\\)$comm.*//x;

    # comment out unsupported directives
    s/\.type/$comm.type/x;
    s/\.func/$comm.func/x;
    s/\.endfunc/$comm.endfunc/x;
    s/\.ltorg/$comm.ltorg/x;
    s/\.size/$comm.size/x;
    s/\.fpu/$comm.fpu/x;
    s/\.arch/$comm.arch/x;
    s/\.object_arch/$comm.object_arch/x;

    # the syntax for these is a little different
    s/\.global/.globl/x;
    # also catch .section .rodata since the equivalent to .const_data is .section __DATA,__const
    s/(.*)\.rodata/.const_data/x;
    s/\.int/.long/x;
    s/\.float/.single/x;

    # catch unknown section names that aren't mach-o style (with a comma)
    if (/.section ([^,]*)$/) {
        die ".section $1 unsupported; figure out the mach-o section name and add it";
    }

    parse_line($_);
}

sub eval_expr {
    my $expr = $_[0];
    $expr =~ s/([A-Za-z._][A-Za-z0-9._]*)/$symbols{$1}/g;
    eval $expr;
}

sub handle_if {
    my $line = $_[0];
    # handle .if directives; apple's assembler doesn't support important non-basic ones
    # evaluating them is also needed to handle recursive macros
    if ($line =~ /\.if(n?)([a-z]*)\s+(.*)/) {
        my $result = $1 eq "n";
        my $type   = $2;
        my $expr   = $3;

        if ($type eq "b") {
            $expr =~ s/\s//g;
            $result ^= $expr eq "";
        } elsif ($type eq "c") {
            if ($expr =~ /(.*)\s*,\s*(.*)/) {
                $result ^= $1 eq $2;
            } else {
                die "argument to .ifc not recognized";
            }
        } elsif ($type eq "") {
            $result ^= eval_expr($expr) != 0;
        } elsif ($type eq "eq") {
            $result = eval_expr($expr) == 0;
        } elsif ($type eq "lt") {
            $result = eval_expr($expr) < 0;
        } else {
	    chomp($line);
            die "unhandled .if varient. \"$line\"";
        }
        push (@ifstack, $result);
        return 1;
    } else {
        return 0;
    }
}

sub parse_line {
    my $line = @_[0];

    # evaluate .if blocks
    if (scalar(@ifstack)) {
        if (/\.endif/) {
            pop(@ifstack);
            return;
        } elsif ($line =~ /\.elseif\s+(.*)/) {
            if ($ifstack[-1] == 0) {
                $ifstack[-1] = !!eval_expr($1);
            } elsif ($ifstack[-1] > 0) {
                $ifstack[-1] = -$ifstack[-1];
            }
            return;
        } elsif (/\.else/) {
            $ifstack[-1] = !$ifstack[-1];
            return;
        } elsif (handle_if($line)) {
            return;
        }

        # discard lines in false .if blocks
        foreach my $i (0 .. $#ifstack) {
            if ($ifstack[$i] <= 0) {
                return;
            }
        }
    }

    if (/\.macro/) {
        $macro_level++;
        if ($macro_level > 1 && !$current_macro) {
            die "nested macros but we don't have master macro";
        }
    } elsif (/\.endm/) {
        $macro_level--;
        if ($macro_level < 0) {
            die "unmatched .endm";
        } elsif ($macro_level == 0) {
            $current_macro = '';
            return;
        }
    }

    if ($macro_level > 1) {
        push(@{$macro_lines{$current_macro}}, $line);
    } elsif ($macro_level == 0) {
        expand_macros($line);
    } else {
        if ($line =~ /\.macro\s+([\d\w\.]+)\s*(.*)/) {
            $current_macro = $1;

            # commas in the argument list are optional, so only use whitespace as the separator
            my $arglist = $2;
            $arglist =~ s/,/ /g;

            my @args = split(/\s+/, $arglist);
            foreach my $i (0 .. $#args) {
                my @argpair = split(/=/, $args[$i]);
                $macro_args{$current_macro}[$i] = $argpair[0];
                $argpair[0] =~ s/:vararg$//;
                $macro_args_default{$current_macro}{$argpair[0]} = $argpair[1];
            }
            # ensure %macro_lines has the macro name added as a key
            $macro_lines{$current_macro} = [];

        } elsif ($current_macro) {
            push(@{$macro_lines{$current_macro}}, $line);
        } else {
            die "macro level without a macro name";
        }
    }
}

sub expand_macros {
    my $line = @_[0];

    # handle .if directives; apple's assembler doesn't support important non-basic ones
    # evaluating them is also needed to handle recursive macros
    if (handle_if($line)) {
        return;
    }

    if (/\.purgem\s+([\d\w\.]+)/) {
        delete $macro_lines{$1};
        delete $macro_args{$1};
        delete $macro_args_default{$1};
        return;
    }

    if ($line =~ /\.altmacro/) {
        $altmacro = 1;
        return;
    }

    if ($line =~ /\.noaltmacro/) {
        $altmacro = 0;
        return;
    }

    $line =~ s/\%([^,]*)/eval_expr($1)/eg if $altmacro;

    if ($line =~ /\.set\s+(.*),\s*(.*)/) {
        $symbols{$1} = eval_expr($2);
    }

    if ($line =~ /(\S+:|)\s*([\w\d\.]+)\s*(.*)/ && exists $macro_lines{$2}) {
        push(@pass1_lines, $1);
        my $macro = $2;

        # commas are optional here too, but are syntactically important because
        # parameters can be blank
        my @arglist = split(/,/, $3);
        my @args;
        my @args_seperator;

        my $comma_sep_required = 0;
        foreach (@arglist) {
            # allow arithmetic/shift operators in macro arguments
            $_ =~ s/\s*(\+|-|\*|\/|<<|>>)\s*/$1/g;

            my @whitespace_split = split(/\s+/, $_);
            if (!@whitespace_split) {
                push(@args, '');
                push(@args_seperator, '');
            } else {
                foreach (@whitespace_split) {
                        #print ("arglist = \"$_\"\n");
                    if (length($_)) {
                        push(@args, $_);
                        my $sep = $comma_sep_required ? "," : " ";
                        push(@args_seperator, $sep);
                        #print ("sep = \"$sep\", arg = \"$_\"\n");
                        $comma_sep_required = 0;
                    }
                }
            }

            $comma_sep_required = 1;
        }

        my %replacements;
        if ($macro_args_default{$macro}){
            %replacements = %{$macro_args_default{$macro}};
        }

        # construct hashtable of text to replace
        foreach my $i (0 .. $#args) {
            my $argname = $macro_args{$macro}[$i];
            my @macro_args = @{ $macro_args{$macro} };
            if ($args[$i] =~ m/=/) {
                # arg=val references the argument name
                # XXX: I'm not sure what the expected behaviour if a lot of
                # these are mixed with unnamed args
                my @named_arg = split(/=/, $args[$i]);
                $replacements{$named_arg[0]} = $named_arg[1];
            } elsif ($i > $#{$macro_args{$macro}}) {
                # more args given than the macro has named args
                # XXX: is vararg allowed on arguments before the last?
                $argname = $macro_args{$macro}[-1];
                if ($argname =~ s/:vararg$//) {
                    #print "macro = $macro, args[$i] = $args[$i], args_seperator=@args_seperator, argname = $argname, arglist[$i] = $arglist[$i], arglist = @arglist, args=@args, macro_args=@macro_args\n";
                    #$replacements{$argname} .= ", $args[$i]";
                    $replacements{$argname} .= "$args_seperator[$i] $args[$i]";
                } else {
                    die "Too many arguments to macro $macro";
                }
            } else {
                $argname =~ s/:vararg$//;
                $replacements{$argname} = $args[$i];
            }
        }

        my $count = $macro_count++;

        # apply replacements as regex
        foreach (@{$macro_lines{$macro}}) {
            my $macro_line = $_;
            # do replacements by longest first, this avoids wrong replacement
            # when argument names are subsets of each other
            foreach (reverse sort {length $a <=> length $b} keys %replacements) {
                $macro_line =~ s/\\$_/$replacements{$_}/g;
            }
            $macro_line =~ s/\\\@/$count/g;
            $macro_line =~ s/\\\(\)//g;     # remove \()
            parse_line($macro_line);
        }
    } else {
        push(@pass1_lines, $line);
    }
}

close(ASMFILE) or exit 1;
open(ASMFILE, "|-", @gcc_cmd) or die "Error running assembler";
#open(ASMFILE, ">/tmp/a.S") or die "Error running assembler";

my @sections;
my $num_repts;
my $rept_lines;

my %literal_labels;     # for ldr <reg>, =<expr>
my $literal_num = 0;

my $in_irp = 0;
my @irp_args;
my $irp_param;

# pass 2: parse .rept and .if variants
# NOTE: since we don't implement a proper parser, using .rept with a
# variable assigned from .set is not supported
foreach my $line (@pass1_lines) {
    # handle .previous (only with regard to .section not .subsection)
    if ($line =~ /\.(section|text|const_data)/) {
        push(@sections, $line);
    } elsif ($line =~ /\.previous/) {
        if (!$sections[-2]) {
            die ".previous without a previous section";
        }
        $line = $sections[-2];
        push(@sections, $line);
    }

    # handle ldr <reg>, =<expr>
    if ($line =~ /(.*)\s*ldr([\w\s\d]+)\s*,\s*=(.*)/) {
        my $label = $literal_labels{$3};
        if (!$label) {
            $label = ".Literal_$literal_num";
            $literal_num++;
            $literal_labels{$3} = $label;
        }
        $line = "$1 ldr$2, $label\n";
    } elsif ($line =~ /\.ltorg/) {
        foreach my $literal (keys %literal_labels) {
            $line .= "$literal_labels{$literal}:\n .word $literal\n";
        }
        %literal_labels = ();
    }

    # @l -> lo16()  @ha -> ha16()
    $line =~ s/,\s+([^,]+)\@l\b/, lo16($1)/g;
    $line =~ s/,\s+([^,]+)\@ha\b/, ha16($1)/g;

    # move to/from SPR
    if ($line =~ /(\s+)(m[ft])([a-z]+)\s+(\w+)/ and exists $ppc_spr{$3}) {
        if ($2 eq 'mt') {
            $line = "$1${2}spr $ppc_spr{$3}, $4\n";
        } else {
            $line = "$1${2}spr $4, $ppc_spr{$3}\n";
        }
    }

    # old gas versions store upper and lower case names on .req,
    # but they remove only one on .unreq
    if ($fix_unreq) {
        if ($line =~ /\.unreq\s+(.*)/) {
            $line = ".unreq " . lc($1) . "\n";
            print ASMFILE ".unreq " . uc($1) . "\n";
        }
    }

    if ($line =~ /\.rept\s+(.*)/) {
        $num_repts = $1;
        $rept_lines = "\n";

        # handle the possibility of repeating another directive on the same line
        # .endr on the same line is not valid, I don't know if a non-directive is
        if ($num_repts =~ s/(\.\w+.*)//) {
            $rept_lines .= "$1\n";
        }
        $num_repts = eval($num_repts);
    } elsif ($line =~ /\.irp\s+([\d\w\.]+)\s*(.*)/) {
        $in_irp = 1;
        $num_repts = 1;
        $rept_lines = "\n";
        $irp_param = $1;

        # only use whitespace as the separator
        my $irp_arglist = $2;
        $irp_arglist =~ s/,/ /g;
        $irp_arglist =~ s/^\s+//;
        @irp_args = split(/\s+/, $irp_arglist);
    } elsif ($line =~ /\.irpc\s+([\d\w\.]+)\s*(.*)/) {
        $in_irp = 1;
        $num_repts = 1;
        $rept_lines = "\n";
        $irp_param = $1;

        my $irp_arglist = $2;
        $irp_arglist =~ s/,/ /g;
        $irp_arglist =~ s/^\s+//;
        @irp_args = split(//, $irp_arglist);
    } elsif ($line =~ /\.endr/) {
        if ($in_irp != 0) {
            foreach my $i (@irp_args) {
                my $line = $rept_lines;
                $line =~ s/\\$irp_param/$i/g;
                $line =~ s/\\\(\)//g;     # remove \()
                print ASMFILE $line;
            }
        } else {
            for (1 .. $num_repts) {
                print ASMFILE $rept_lines;
            }
        }
        $rept_lines = '';
        $in_irp = 0;
        @irp_args = '';
    } elsif ($rept_lines) {
        $rept_lines .= $line;
    } else {
        print ASMFILE $line;
    }
}

print ASMFILE ".text\n";
foreach my $literal (keys %literal_labels) {
    print ASMFILE "$literal_labels{$literal}:\n .word $literal\n";
}

close(ASMFILE) or exit 1;
#exit 1
