/*
 * Copyright (C) 2003 by Martin Pool
 *
 * Test harness for strstr_m
 */

#include "includes.h"

int main(int argc, char *argv[])
{
	int i;
	int iters = 1;
	
	const char *ret = NULL;

	/* Needed to initialize character set */
	lp_load("/dev/null", True, False, False, True);

	if (argc < 3) {
		fprintf(stderr, "usage: %s STRING1 STRING2 [ITERS]\n"
			"Compares two strings, prints the results of strstr_m\n",
			argv[0]);
		return 2;
	}
	if (argc >= 4)
		iters = atoi(argv[3]);

	for (i = 0; i < iters; i++) {
		ret = strstr_m(argv[1], argv[2]);
	}

	if (ret == NULL)
		ret = "(null)";

	printf("%s\n", ret);
	
	return 0;
}
