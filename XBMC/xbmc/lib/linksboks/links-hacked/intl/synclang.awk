#!/bin/awk -f
BEGIN {
	FS=","
}

/^T_/ {
	s2=$2;
	for(i = 3; i <= NF; i++) s2 = s2","$i;
	aa[$1] = s2;
}

END {
	while (getline < "english.lng") {
		if (aa[$1] != "") printf("%s,%s\n", $1, aa[$1]);
		else printf("%s, NULL,\n", $1);
	}
}
