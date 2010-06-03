#!/bin/sh 

############################################################################
#   
#  Run the LAME encoder on multiple files, with option to delete .wav files
#  after encoding.  "mlame -?" will give instructions.
#
#  Robert Hegemann
#  modified on request: Frank Klemm <pfk@uni-jena.de>
#
############################################################################

# encoder path to use
mp3coder="lame"
mp3analyzer="mlame_corr"

# default options to use
options_low="-h -d -mj -b 128"
options_high="-h -d -mj -V 1 -b 112 -B 320"
options=$options_high

# remove source?
removesource=false

# force overwrite of destination
testoverwrite=true

# waiting after error report n seconds
errordelay=1 

helptext="\n\
This script runs the LAME mp3 encoder on multiple files: \n\n\
    $0 [options] <file 1> ... <file n>\n\
\n\
  options:\n\
    -?                  this help text\n\
    -r                  remove files after encoding\n\
    -f                  force overwrite of destination if exists\n\
    -l                  low quality settings\n\
    -h                  high quality settings\n\
    -o \"<lame options>\" overrides script default options
\n\
  example:\n\
    $0  -r  -f  -o \"-v -V 0 -b 112\" a*.wav z*.aif g*.mp?\n\
\n\
"

#   process command-line options
#   this could be extended to fake the 
#   commandline interface of the mp3encoder

while getopts ":o:r:h:l:f" optn; do
    case $optn in
    o ) options=$OPTARG 	# replace default options
	echo New lame options are \'$options\'
        ;; 
    r ) removesource=true
        echo Removing source files after successfully converting
	;;
    f ) testoverwrite=false
        echo Force overwriting existing destination files
	;;
    h ) options=$options_high
        ;;
    l ) options=$options_low
        ;;
    \? ) printf "$helptext"
	sleep $errordelay
        exit 1  
        ;;
    esac
done
shift $(($OPTIND - 1))

# no files remaining?

if [ "$1" = "" ]; then
    printf "$helptext"
    sleep $errordelay
    exit 1  
fi

#   process input-files

for src in "$@"; do

    case $src in
    *[.][wW][aA][vV]  )
        dst=${src%[.][wW][aA][vV]}.mp3
        if [ -f "$src" ]; then
            if [ $testoverwrite = true -a -f "$dst" ]; then
                echo \'$dst\' already exists, skipping
		sleep $errordelay
            elif $mp3coder $options `$mp3analyzer "$src"` "$src" "$dst"; then
                if [ $removesource = true ]; then
                    rm -f "$src"
                fi
            else
                echo converting of \'$src\' to \'$dst\' failed
		sleep $errordelay
            fi
        else
            echo No source file \'$src\' found.
	    sleep $errordelay
        fi
        ;;

    *[.][aA][iI][fF]  )
        dst=${src%[.][aA][iI][fF]}.mp3
        if [ -f "$src" ]; then
            if [ $testoverwrite = true -a -f "$dst" ]; then
                echo \'$dst\' already exists, skipping
		sleep $errordelay
            elif $mp3coder $options "$src" "$dst"; then
                if [ $removesource = true ]; then
                    rm -f "$src"
                fi
            else
                echo converting of \'$src\' to \'$dst\' failed
		sleep $errordelay
            fi
        else
            echo No source file \'$src\' found.
	    sleep $errordelay
        fi
        ;;

    *[.][aA][iI][fF][fF] )
        dst=${src%[.][aA][iI][fF][fF]}.mp3
        if [ -f "$src" ]; then
            if [ $testoverwrite = true -a -f "$dst" ]; then
                echo \'$dst\' already exists, skipping
		sleep $errordelay
            elif $mp3coder $options "$src" "$dst"; then
                if [ $removesource = true ]; then
                    rm -f "$src"
                fi
            else
                echo converting of \'$src\' to \'$dst\' failed
		sleep $errordelay
            fi
        else
            echo No source file \'$src\' found.
	    sleep $errordelay
        fi
        ;;

    *[.][mM][pP][gG12]  )
        dst=${src%[.][mM][pP][gG12]}.mp3
        if [ -f "$src" ]; then
            if [ $testoverwrite = true -a -f "$dst" ]; then
                echo \'$dst\' already exists, skipping
		sleep $errordelay
            elif $mp3coder $options "$src" "$dst"; then
                if [ $removesource = true ]; then
                    rm -f "$src"
                fi
            else
                echo converting of \'$src\' to \'$dst\' failed
		sleep $errordelay
            fi
        else
            echo No source file \'$src\' found.
	    sleep $errordelay
        fi
        ;;

    *[.][mM][pP]3 )
        dst=${src%[.][mM][pP]3}-new-converted-file.${src##*.}
        if [ -f "$src" ]; then
            if [ $testoverwrite = true -a -f "$dst" ]; then
                echo \'$dst\' already exists, skipping
		sleep $errordelay
            elif $mp3coder $options "$src" "$dst"; then
                if [ $removesource = true ]; then
                    mv -f "$dst" "$src"
                fi
            else
                echo converting of \'$src\' to \'$dst\' failed
		sleep $errordelay
            fi
        else
            echo No source file \'$src\' found.
	    sleep $errordelay
        fi
        ;;

    * ) # the rest
        echo warning: File extention \'.${src##*.}\' not supported
        sleep $errordelay
        ;;

    esac

done
