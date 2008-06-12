BEGIN {
	for (i=0; i<256; i++) {
		tbl[sprintf("%02x",i)] = "0x0000";
	}
}

/^<U([[:xdigit:]][[:xdigit:]][[:xdigit:]][[:xdigit:]])>[[:space:]]*.x([[:xdigit:]][[:xdigit:]])[:space:]*.*$/ {
	tbl[substr($2,3,2)]=sprintf("0x%s",substr($1,3,4));
}

END {
	for(i=0; i<32; i++) {
		for(j=0; j<8; j++) {
			printf(" %s,", tbl[sprintf("%02x",i*8+j)]);
		}
		printf "\n"
	}
}