conf.d/README

Each file in this directory is a fontconfig configuration file.  Fontconfig
scans this directory, loading all files of the form [0-9][0-9]*. These files
are normally installed in ../conf.avail and then symlinked here, allowing
them to be easily installed and then enabled/disabled by adjusting the
symlinks.

The files are loaded in numeric order, the structure of the configuration
has led to the following conventions in usage:

 Files begining with:	Contain:
 
 00 through 09		Font directories
 10 through 19		system rendering defaults (AA, etc)
	10-autohint.conf
	10-no-sub-pixel.conf
 	10-sub-pixel-bgr.conf
 	10-sub-pixel-rgb.conf
 	10-sub-pixel-vbgr.conf
 	10-sub-pixel-vrgb.conf
	10-unhinted.conf
 20 through 29		font rendering options
 	20-fix-globaladvance.conf
	20-lohit-gujarati.conf
	20-unhint-small-vera.conf
 30 through 39		family substitution
 	30-urw-aliases.conf
	30-amt-aliases.conf
 40 through 49		generic identification, map family->generic
 	40-generic-id.conf
	49-sansserif.conf
 50 through 59		alternate config file loading
 	50-user.conf	Load ~/.fonts.conf
	51-local.conf	Load local.conf
 60 through 69		generic aliases
 	60-latin.conf
	65-fonts-persian.conf
	65-nonlatin.conf
	69-unifont.conf
 70 through 79		select font (adjust which fonts are available)
 	70-no-bitmaps.conf
	70-yes-bitmaps.conf
 80 through 89		match target="scan" (modify scanned patterns)
 	80-delicious.conf
 90 through 98		font synthesis
 	90-synthetic.conf
 
