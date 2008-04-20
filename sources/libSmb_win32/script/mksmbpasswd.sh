#!/bin/sh
awk 'BEGIN {FS=":"
	printf("#\n# SMB password file.\n#\n")
	}
{ printf( "%s:%s:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:[UD         ]:LCT-00000000:%s\n", $1, $3, $5) }
'
