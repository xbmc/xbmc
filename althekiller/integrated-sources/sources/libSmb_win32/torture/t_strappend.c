/*
 * Copyright (C) 2005 by Volker Lendecke
 *
 * Test harness for sprintf_append
 */

#include "includes.h"
#include <assert.h>

int main(int argc, char *argv[])
{
	TALLOC_CTX *mem_ctx;
	char *string = NULL;
	int len = 0;
	int bufsize = 4;
	int i;

	mem_ctx = talloc_init("t_strappend");
	if (mem_ctx == NULL) {
		fprintf(stderr, "talloc_init failed\n");
		return 1;
	}

	sprintf_append(mem_ctx, &string, &len, &bufsize, "");
	assert(strlen(string) == len);
	sprintf_append(mem_ctx, &string, &len, &bufsize, "");
	assert(strlen(string) == len);
	sprintf_append(mem_ctx, &string, &len, &bufsize,
		       "01234567890123456789012345678901234567890123456789\n");
	assert(strlen(string) == len);


	for (i=0; i<(100000); i++) {
		if (i%1000 == 0) {
			printf("%d %d\r", i, bufsize);
			fflush(stdout);
		}
		sprintf_append(mem_ctx, &string, &len, &bufsize, "%d\n", i);
		assert(strlen(string) == len);
	}

	talloc_destroy(mem_ctx);

	return 0;
}
