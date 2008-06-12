function reset_vals() {
	upperstr = "";
	lowerstr = "";
	flagstr = "0";
}

function print_val() {
	upperstr = $13;
	lowerstr = $14;
	if ( upperstr == "" )
		upperstr = strval;
	if ( lowerstr == "" )
		lowerstr = strval;

	if ( $3 == "Lu" )
		flagstr = sprintf("%s|%s", flagstr, "UNI_UPPER");
	if ( $3 == "Ll" )
		flagstr = sprintf("%s|%s", flagstr, "UNI_LOWER");
	if ( val >= 48 && val <= 57)
		flagstr = sprintf("%s|%s", flagstr, "UNI_DIGIT");
	if ((val >= 48 && val <= 57) || (val >= 65 && val <= 70) || (val >=97 && val <= 102))
		flagstr = sprintf("%s|%s", flagstr, "UNI_XDIGIT");
	if ( val == 32 || (val >=9 && val <= 13))
		flagstr = sprintf("%s|%s", flagstr, "UNI_SPACE");
	if( index(flagstr, "0|") == 1)
		flagstr = substr(flagstr, 3, length(flagstr) - 2);
	printf("{ 0x%s, 0x%s, %s }, \t\t\t/* %s %s */\n", lowerstr, upperstr, flagstr, strval, $2);
	val++;
	strval=sprintf("%04X", val);
	reset_vals();
}

BEGIN {
	val=0
	FS=";"
	strval=sprintf("%04X", val);
	reset_vals();
}

{
	if ( $1 == strval ) {
		print_val();
	} else {
		while ( $1 != strval) {
			printf("{ 0x%04X, 0x%04X, 0 }, \t\t\t/* %s NOMAP */\n", val, val, strval);
			val++;
			strval=sprintf("%04X", val);
		}
		print_val();
	}
}

END {
	while ( val < 65536 ) {
		printf("{ 0x%04X, 0x%04X, 0 }, \t\t\t/* %s NOMAP */\n", val, val, strval);
		val++;
		strval=sprintf("%04X", val);
	}
}
