#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iniparser.h"

void create_example_ini_file(void);
int  parse_ini_file(char * ini_name);

int main(int argc, char * argv[])
{
	int		status ;

	if (argc<2) {
		create_example_ini_file();
		status = parse_ini_file("example.ini");
	} else {
		status = parse_ini_file(argv[1]);
	}
	return status ;
}

void create_example_ini_file(void)
{
	FILE	*	ini ;

	ini = fopen("example.ini", "w");
	fprintf(ini, "\n\
#\n\
# This is an example of ini file\n\
#\n\
\n\
[Pizza]\n\
\n\
Ham       = yes ;\n\
Mushrooms = TRUE ;\n\
Capres    = 0 ;\n\
Cheese    = NO ;\n\
\n\
\n\
[Wine]\n\
\n\
Grape     = Cabernet Sauvignon ;\n\
Year      = 1989 ;\n\
Country   = Spain ;\n\
Alcohol   = 12.5  ;\n\
\n\
#\n\
# end of file\n\
#\n");

	fclose(ini);
}


int parse_ini_file(char * ini_name)
{
	dictionary	*	ini ;

	/* Some temporary variables to hold query results */
	int				b ;
	int				i ;
	double			d ;
	char		*	s ;

	ini = iniparser_load(ini_name);
	if (ini==NULL) {
		fprintf(stderr, "cannot parse file [%s]", ini_name);
		return -1 ;
	}
	iniparser_dump(ini, stderr);

	/* Get pizza attributes */
	printf("Pizza:\n");

	b = iniparser_getboolean(ini, "pizza:ham", -1);
	printf("Ham:       [%d]\n", b);
	b = iniparser_getboolean(ini, "pizza:mushrooms", -1);
	printf("Mushrooms: [%d]\n", b);
	b = iniparser_getboolean(ini, "pizza:capres", -1);
	printf("Capres:    [%d]\n", b);
	b = iniparser_getboolean(ini, "pizza:cheese", -1);
	printf("Cheese:    [%d]\n", b);

	/* Get wine attributes */
	printf("Wine:\n");
	s = iniparser_getstr(ini, "wine:grape");
	if (s) {
		printf("grape:     [%s]\n", s);
	} else {
		printf("grape:     not found\n");
	}
	i = iniparser_getint(ini, "wine:year", -1);
	if (i>0) {
		printf("year:      [%d]\n", i);
	} else {
		printf("year:      not found\n");
	}
	s = iniparser_getstr(ini, "wine:country");
	if (s) {
		printf("country:   [%s]\n", s);
	} else {
		printf("country:   not found\n");
	}
	d = iniparser_getdouble(ini, "wine:alcohol", -1.0);
	if (d>0.0) {
		printf("alcohol:   [%g]\n", d);
	} else {
		printf("alcohol:   not found\n");
	}

	iniparser_freedict(ini);
	return 0 ;
}


