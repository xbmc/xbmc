/*
 * Copyright (C) 2003 by Martin Pool
 *
 * Test harness for StrCaseCmp
 */

#include "includes.h"

int main(int argc, char *argv[])
{
	int i, ret;
	int iters = 1;
	
	/* Needed to initialize character set */
	lp_load("/dev/null", True, False, False, True);

	if (argc < 3) {
		fprintf(stderr, "usage: %s STRING1 STRING2 [ITERS]\n"
			"Compares two strings, prints the results of StrCaseCmp\n",
			argv[0]);
		return 2;
	}
	if (argc >= 4)
		iters = atoi(argv[3]);

	for (i = 0; i < iters; i++)
		ret = StrCaseCmp(argv[1], argv[2]);

	printf("%d\n", ret);
	
	return 0;
}
