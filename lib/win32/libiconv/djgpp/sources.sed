# Fixes for lib/iconv.c.
# All encodings files recide in encdings dir now.
/^#[ 	]*include/ s|"canonical\.h|"canonical/canonical.h|
/^#[ 	]*include/ s|"canonical_aix\.h|"canonical/aix.h|
/^#[ 	]*include/ s|"canonical_dos\.h|"canonical/dos.h|
/^#[ 	]*include/ s|"canonical_osf1\.h|"canonical/osf1.h|
/^#[ 	]*include/ s|"canonical_local\.h|"canonical/local.h|
/^#[ 	]*include/ s|"canonical_extra\.h|"canonical/extra.h|
/^#[ 	]*include/ s|"encodings\.def|"encodings/encodings.def|
/^#[ 	]*include/ s|"encodings_aix\.def|"encodings/aix.def|
/^#[ 	]*include/ s|"encodings_dos\.def|"encodings/dos.def|
/^#[ 	]*include/ s|"encodings_osf1\.def|"encodings/osf1.def|
/^#[ 	]*include/ s|"encodings_local\.def|"encodings/local.def|
/^#[ 	]*include/ s|"encodings_extra\.def|"encodings/extra.def|
/^#[ 	]*include/ s|"aliases\.h|"aliases/aliases.h|
/^#[ 	]*include/ s|"aliases2\.h|"aliases/aliases2.h|

# Fixes for lib/iconv.c, lib/aliases/aliases2.h and lib/big5hkscs/1999, 2001, 2004.h
# All encodings files recide in encdings dir now.
/^#[ 	]*include/ s|"aliases_aix\.h|"aliases/aix.h|
/^#[ 	]*include/ s|"aliases_dos\.h|"aliases/dos.h|
/^#[ 	]*include/ s|"aliases_osf1\.h|"aliases/osf1.h|
/^#[ 	]*include/ s|"aliases_local\.h|"aliases/local.h|
/^#[ 	]*include/ s|"aliases_extra\.h|"aliases/extra.h|
/^#[ 	]*include/ s|"hkscs1999\.h|"hkscs/1999.h|
/^#[ 	]*include/ s|"hkscs2001\.h|"hkscs/2001.h|
/^#[ 	]*include/ s|"hkscs2004\.h|"hkscs/2004.h|

# Fixes for lib/converters.h, cns11643??.h and iso?????.h files.
# All cns, iso, georgian and mac files recide in their respective dirs now.
/^#[ 	]*include/ s|"cns|&/|
/^#[ 	]*include/ s|"iso|&/|
/^#[ 	]*include/ s|"georgian_|"georgian/|
/^#[ 	]*include/ s|"mac_|"mac/|
/^#[ 	]*include/ s|"big5hkscs|&/|
/^#[ 	]*include/ s|"hkscs|&/|
