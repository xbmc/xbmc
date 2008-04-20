/*
 * Copyright (C) 2004 by Volker Lendecke
 *
 * Test harness for asn1_write_*, inspired by Love Hornquist Astrand
 */

#include "includes.h"

static DATA_BLOB tests[] = {
        {"\x02\x01\x00", 3, NULL},
        {"\x02\x01\x7f", 3, NULL},
        {"\x02\x02\x00\x80", 4, NULL},
        {"\x02\x02\x01\x00", 4, NULL},
        {"\x02\x01\x80", 3, NULL},
        {"\x02\x02\xff\x7f", 4, NULL},
        {"\x02\x01\xff", 3, NULL},
        {"\x02\x02\xff\x01", 4, NULL},
        {"\x02\x02\x00\xff", 4, NULL},
        {"\x02\x04\x80\x00\x00\x00", 6, NULL},
        {"\x02\x04\x7f\xff\xff\xff", 6, NULL},
	{NULL, 0, NULL}
};

static int values[] = {0, 127, 128, 256, -128, -129, -1, -255, 255,
		       0x80000000, 0x7fffffff};

int main(void)
{
	int i = 0;
	int val;
	BOOL ok = True;

	for (i=0; tests[i].data != NULL; i++) {
		ASN1_DATA data;
		DATA_BLOB blob;

		ZERO_STRUCT(data);
		asn1_write_Integer(&data, values[i]);

		if ((data.length != tests[i].length) ||
		    (memcmp(data.data, tests[i].data, data.length) != 0)) {
			printf("Test for %d failed\n", values[i]);
			ok = False;
		}

		blob.data = data.data;
		blob.length = data.length;
		asn1_load(&data, blob);
		if (!asn1_read_Integer(&data, &val)) {
			printf("Could not read our own Integer for %d\n",
			       values[i]);
			ok = False;
		}
		if (val != values[i]) {
			printf("%d -> ASN -> Int %d\n", values[i], val);
			ok = False;
		}
	}

	return ok ? 0 : 1;
}
