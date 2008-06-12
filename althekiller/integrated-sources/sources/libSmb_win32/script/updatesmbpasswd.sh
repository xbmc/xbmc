#!/bin/sh
nawk 'BEGIN {FS=":"} 
{
	if( $0 ~ "^#" ) {
		print $0
	} else if( (length($4) == 32) && (($4 ~ "^[0-9A-F]*$") || ($4 ~ "^[X]*$") || ( $4 ~ "^[*]*$"))) {
		print $0
	} else {
		printf( "%s:%s:%s:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX:", $1, $2, $3);
		for(i = 4; i <= NF; i++)
			printf("%s:", $i)
		printf("\n")
	}
}'
