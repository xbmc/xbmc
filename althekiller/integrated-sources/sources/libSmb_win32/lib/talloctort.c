/* 
   Unix SMB/CIFS implementation.

   local testing of talloc routines.

   Copyright (C) Andrew Tridgell 2004
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef _SAMBA_BUILD_
#include "includes.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include "talloc.h"
#endif

/* the test suite can be built standalone, or as part of Samba */
#ifndef _SAMBA_BUILD_
typedef enum {False=0,True=1} BOOL;
#endif

/* Samba3 does not define the timeval functions below */
#if !defined(_SAMBA_BUILD_) || ((SAMBA_VERSION_MAJOR==3)&&(SAMBA_VERSION_MINOR<9))

static double timeval_elapsed(struct timeval *tv)
{
	struct timeval tv2 = timeval_current();
	return (tv2.tv_sec - tv->tv_sec) + 
	       (tv2.tv_usec - tv->tv_usec)*1.0e-6;
}
#endif /* _SAMBA_BUILD_ */

#if SAMBA_VERSION_MAJOR<4
#ifdef malloc
#undef malloc
#endif
#ifdef strdup
#undef strdup
#endif
#endif

#define CHECK_SIZE(ptr, tsize) do { \
	if (talloc_total_size(ptr) != (tsize)) { \
		printf(__location__ " failed: wrong '%s' tree size: got %u  expected %u\n", \
		       #ptr, \
		       (unsigned)talloc_total_size(ptr), \
		       (unsigned)tsize); \
		talloc_report_full(ptr, stdout); \
		return False; \
	} \
} while (0)

#define CHECK_BLOCKS(ptr, tblocks) do { \
	if (talloc_total_blocks(ptr) != (tblocks)) { \
		printf(__location__ " failed: wrong '%s' tree blocks: got %u  expected %u\n", \
		       #ptr, \
		       (unsigned)talloc_total_blocks(ptr), \
		       (unsigned)tblocks); \
		talloc_report_full(ptr, stdout); \
		return False; \
	} \
} while (0)


/*
  test references 
*/
static BOOL test_ref1(void)
{
	void *root, *p1, *p2, *ref, *r1;

	printf("TESTING SINGLE REFERENCE FREE\n");

	root = talloc_named_const(NULL, 0, "root");
	p1 = talloc_named_const(root, 1, "p1");
	p2 = talloc_named_const(p1, 1, "p2");
	talloc_named_const(p1, 1, "x1");
	talloc_named_const(p1, 2, "x2");
	talloc_named_const(p1, 3, "x3");

	r1 = talloc_named_const(root, 1, "r1");	
	ref = talloc_reference(r1, p2);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 5);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(r1, 2);

	printf("Freeing p2\n");
	talloc_free(p2);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 5);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(r1, 1);

	printf("Freeing p1\n");
	talloc_free(p1);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(r1, 1);

	printf("Freeing r1\n");
	talloc_free(r1);
	talloc_report_full(NULL, stdout);

	printf("Testing NULL\n");
	if (talloc_reference(root, NULL)) {
		return False;
	}

	CHECK_BLOCKS(root, 1);

	CHECK_SIZE(root, 0);

	talloc_free(root);

	return True;
}

/*
  test references 
*/
static BOOL test_ref2(void)
{
	void *root, *p1, *p2, *ref, *r1;

	printf("TESTING DOUBLE REFERENCE FREE\n");

	root = talloc_named_const(NULL, 0, "root");
	p1 = talloc_named_const(root, 1, "p1");
	talloc_named_const(p1, 1, "x1");
	talloc_named_const(p1, 1, "x2");
	talloc_named_const(p1, 1, "x3");
	p2 = talloc_named_const(p1, 1, "p2");

	r1 = talloc_named_const(root, 1, "r1");	
	ref = talloc_reference(r1, p2);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 5);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(r1, 2);

	printf("Freeing ref\n");
	talloc_free(ref);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 5);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(r1, 1);

	printf("Freeing p2\n");
	talloc_free(p2);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 4);
	CHECK_BLOCKS(r1, 1);

	printf("Freeing p1\n");
	talloc_free(p1);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(r1, 1);

	printf("Freeing r1\n");
	talloc_free(r1);
	talloc_report_full(root, stdout);

	CHECK_SIZE(root, 0);

	talloc_free(root);

	return True;
}

/*
  test references 
*/
static BOOL test_ref3(void)
{
	void *root, *p1, *p2, *ref, *r1;

	printf("TESTING PARENT REFERENCE FREE\n");

	root = talloc_named_const(NULL, 0, "root");
	p1 = talloc_named_const(root, 1, "p1");
	p2 = talloc_named_const(root, 1, "p2");
	r1 = talloc_named_const(p1, 1, "r1");
	ref = talloc_reference(p2, r1);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 2);
	CHECK_BLOCKS(p2, 2);
	CHECK_BLOCKS(r1, 1);

	printf("Freeing p1\n");
	talloc_free(p1);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p2, 2);
	CHECK_BLOCKS(r1, 1);

	printf("Freeing p2\n");
	talloc_free(p2);
	talloc_report_full(root, stdout);

	CHECK_SIZE(root, 0);

	talloc_free(root);

	return True;
}

/*
  test references 
*/
static BOOL test_ref4(void)
{
	void *root, *p1, *p2, *ref, *r1;

	printf("TESTING REFERRER REFERENCE FREE\n");

	root = talloc_named_const(NULL, 0, "root");
	p1 = talloc_named_const(root, 1, "p1");
	talloc_named_const(p1, 1, "x1");
	talloc_named_const(p1, 1, "x2");
	talloc_named_const(p1, 1, "x3");
	p2 = talloc_named_const(p1, 1, "p2");

	r1 = talloc_named_const(root, 1, "r1");	
	ref = talloc_reference(r1, p2);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 5);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(r1, 2);

	printf("Freeing r1\n");
	talloc_free(r1);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 5);
	CHECK_BLOCKS(p2, 1);

	printf("Freeing p2\n");
	talloc_free(p2);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 4);

	printf("Freeing p1\n");
	talloc_free(p1);
	talloc_report_full(root, stdout);

	CHECK_SIZE(root, 0);

	talloc_free(root);

	return True;
}


/*
  test references 
*/
static BOOL test_unlink1(void)
{
	void *root, *p1, *p2, *ref, *r1;

	printf("TESTING UNLINK\n");

	root = talloc_named_const(NULL, 0, "root");
	p1 = talloc_named_const(root, 1, "p1");
	talloc_named_const(p1, 1, "x1");
	talloc_named_const(p1, 1, "x2");
	talloc_named_const(p1, 1, "x3");
	p2 = talloc_named_const(p1, 1, "p2");

	r1 = talloc_named_const(p1, 1, "r1");	
	ref = talloc_reference(r1, p2);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 7);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(r1, 2);

	printf("Unreferencing r1\n");
	talloc_unlink(r1, p2);
	talloc_report_full(root, stdout);

	CHECK_BLOCKS(p1, 6);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(r1, 1);

	printf("Freeing p1\n");
	talloc_free(p1);
	talloc_report_full(root, stdout);

	CHECK_SIZE(root, 0);

	talloc_free(root);

	return True;
}

static int fail_destructor(void *ptr)
{
	return -1;
}

/*
  miscellaneous tests to try to get a higher test coverage percentage
*/
static BOOL test_misc(void)
{
	void *root, *p1;
	char *p2;
	double *d;

	printf("TESTING MISCELLANEOUS\n");

	root = talloc_new(NULL);

	p1 = talloc_size(root, 0x7fffffff);
	if (p1) {
		printf("failed: large talloc allowed\n");
		return False;
	}

	p1 = talloc_strdup(root, "foo");
	talloc_increase_ref_count(p1);
	talloc_increase_ref_count(p1);
	talloc_increase_ref_count(p1);
	CHECK_BLOCKS(p1, 1);
	CHECK_BLOCKS(root, 2);
	talloc_free(p1);
	CHECK_BLOCKS(p1, 1);
	CHECK_BLOCKS(root, 2);
	talloc_unlink(NULL, p1);
	CHECK_BLOCKS(p1, 1);
	CHECK_BLOCKS(root, 2);
	p2 = talloc_strdup(p1, "foo");
	if (talloc_unlink(root, p2) != -1) {
		printf("failed: talloc_unlink() of non-reference context should return -1\n");
		return False;
	}
	if (talloc_unlink(p1, p2) != 0) {
		printf("failed: talloc_unlink() of parent should succeed\n");
		return False;
	}
	talloc_free(p1);
	CHECK_BLOCKS(p1, 1);
	CHECK_BLOCKS(root, 2);

	talloc_set_name(p1, "my name is %s", "foo");
	if (strcmp(talloc_get_name(p1), "my name is foo") != 0) {
		printf("failed: wrong name after talloc_set_name\n");
		return False;
	}
	CHECK_BLOCKS(p1, 2);
	CHECK_BLOCKS(root, 3);

	talloc_set_name_const(p1, NULL);
	if (strcmp(talloc_get_name(p1), "UNNAMED") != 0) {
		printf("failed: wrong name after talloc_set_name(NULL)\n");
		return False;
	}
	CHECK_BLOCKS(p1, 2);
	CHECK_BLOCKS(root, 3);
	

	if (talloc_free(NULL) != -1) {
		printf("talloc_free(NULL) should give -1\n");
		return False;
	}

	talloc_set_destructor(p1, fail_destructor);
	if (talloc_free(p1) != -1) {
		printf("Failed destructor should cause talloc_free to fail\n");
		return False;
	}
	talloc_set_destructor(p1, NULL);

	talloc_report(root, stdout);


	p2 = talloc_zero_size(p1, 20);
	if (p2[19] != 0) {
		printf("Failed to give zero memory\n");
		return False;
	}
	talloc_free(p2);

	if (talloc_strdup(root, NULL) != NULL) {
		printf("failed: strdup on NULL should give NULL\n");
		return False;
	}

	p2 = talloc_strndup(p1, "foo", 2);
	if (strcmp("fo", p2) != 0) {
		printf("failed: strndup doesn't work\n");
		return False;
	}
	p2 = talloc_asprintf_append(p2, "o%c", 'd');
	if (strcmp("food", p2) != 0) {
		printf("failed: talloc_asprintf_append doesn't work\n");
		return False;
	}
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(p1, 3);

	p2 = talloc_asprintf_append(NULL, "hello %s", "world");
	if (strcmp("hello world", p2) != 0) {
		printf("failed: talloc_asprintf_append doesn't work\n");
		return False;
	}
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(p1, 3);
	talloc_free(p2);

	d = talloc_array(p1, double, 0x20000000);
	if (d) {
		printf("failed: integer overflow not detected\n");
		return False;
	}

	d = talloc_realloc(p1, d, double, 0x20000000);
	if (d) {
		printf("failed: integer overflow not detected\n");
		return False;
	}

	talloc_free(p1);
	CHECK_BLOCKS(root, 1);

	p1 = talloc_named(root, 100, "%d bytes", 100);
	CHECK_BLOCKS(p1, 2);
	CHECK_BLOCKS(root, 3);
	talloc_unlink(root, p1);

	p1 = talloc_init("%d bytes", 200);
	p2 = talloc_asprintf(p1, "my test '%s'", "string");
	CHECK_BLOCKS(p1, 3);
	CHECK_SIZE(p2, 17);
	CHECK_BLOCKS(root, 1);
	talloc_unlink(NULL, p1);

	p1 = talloc_named_const(root, 10, "p1");
	p2 = talloc_named_const(root, 20, "p2");
	talloc_reference(p1, p2);
	talloc_report_full(root, stdout);
	talloc_unlink(root, p2);
	talloc_report_full(root, stdout);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(p1, 2);
	CHECK_BLOCKS(root, 3);
	talloc_unlink(p1, p2);
	talloc_unlink(root, p1);

	p1 = talloc_named_const(root, 10, "p1");
	p2 = talloc_named_const(root, 20, "p2");
	talloc_reference(NULL, p2);
	talloc_report_full(root, stdout);
	talloc_unlink(root, p2);
	talloc_report_full(root, stdout);
	CHECK_BLOCKS(p2, 1);
	CHECK_BLOCKS(p1, 1);
	CHECK_BLOCKS(root, 2);
	talloc_unlink(NULL, p2);
	talloc_unlink(root, p1);

	/* Test that talloc_unlink is a no-op */

	if (talloc_unlink(root, NULL) != -1) {
		printf("failed: talloc_unlink(root, NULL) == -1\n");
		return False;
	}

	talloc_report(root, stdout);
	talloc_report(NULL, stdout);

	CHECK_SIZE(root, 0);

	talloc_free(root);

	CHECK_SIZE(NULL, 0);

	talloc_enable_leak_report();
	talloc_enable_leak_report_full();

	return True;
}


/*
  test realloc
*/
static BOOL test_realloc(void)
{
	void *root, *p1, *p2;

	printf("TESTING REALLOC\n");

	root = talloc_new(NULL);

	p1 = talloc_size(root, 10);
	CHECK_SIZE(p1, 10);

	p1 = talloc_realloc_size(NULL, p1, 20);
	CHECK_SIZE(p1, 20);

	talloc_new(p1);

	p2 = talloc_realloc_size(p1, NULL, 30);

	talloc_new(p1);

	p2 = talloc_realloc_size(p1, p2, 40);

	CHECK_SIZE(p2, 40);
	CHECK_SIZE(root, 60);
	CHECK_BLOCKS(p1, 4);

	p1 = talloc_realloc_size(NULL, p1, 20);
	CHECK_SIZE(p1, 60);

	talloc_increase_ref_count(p2);
	if (talloc_realloc_size(NULL, p2, 5) != NULL) {
		printf("failed: talloc_realloc() on a referenced pointer should fail\n");
		return False;
	}
	CHECK_BLOCKS(p1, 4);

	talloc_realloc_size(NULL, p2, 0);
	talloc_realloc_size(NULL, p2, 0);
	CHECK_BLOCKS(p1, 3);

	if (talloc_realloc_size(NULL, p1, 0x7fffffff) != NULL) {
		printf("failed: oversize talloc should fail\n");
		return False;
	}

	talloc_realloc_size(NULL, p1, 0);

	CHECK_BLOCKS(root, 1);
	CHECK_SIZE(root, 0);

	talloc_free(root);

	return True;
}


/*
  test realloc with a child
*/
static BOOL test_realloc_child(void)
{
	void *root;
	struct el1 {
		int count;
		struct el2 {
			const char *name;
		} **list, **list2, **list3;
	} *el1;
	struct el2 *el2;

	printf("TESTING REALLOC WITH CHILD\n");

	root = talloc_new(NULL);

	el1 = talloc(root, struct el1);
	el1->list = talloc(el1, struct el2 *);
	el1->list[0] = talloc(el1->list, struct el2);
	el1->list[0]->name = talloc_strdup(el1->list[0], "testing");

	el1->list2 = talloc(el1, struct el2 *);
	el1->list2[0] = talloc(el1->list2, struct el2);
	el1->list2[0]->name = talloc_strdup(el1->list2[0], "testing2");

	el1->list3 = talloc(el1, struct el2 *);
	el1->list3[0] = talloc(el1->list3, struct el2);
	el1->list3[0]->name = talloc_strdup(el1->list3[0], "testing2");
	
	el2 = talloc(el1->list, struct el2);
	el2 = talloc(el1->list2, struct el2);
	el2 = talloc(el1->list3, struct el2);

	el1->list = talloc_realloc(el1, el1->list, struct el2 *, 100);
	el1->list2 = talloc_realloc(el1, el1->list2, struct el2 *, 200);
	el1->list3 = talloc_realloc(el1, el1->list3, struct el2 *, 300);

	talloc_free(root);

	return True;
}


/*
  test type checking
*/
static BOOL test_type(void)
{
	void *root;
	struct el1 {
		int count;
	};
	struct el2 {
		int count;
	};
	struct el1 *el1;

	printf("TESTING talloc type checking\n");

	root = talloc_new(NULL);

	el1 = talloc(root, struct el1);

	el1->count = 1;

	if (talloc_get_type(el1, struct el1) != el1) {
		printf("type check failed on el1\n");
		return False;
	}
	if (talloc_get_type(el1, struct el2) != NULL) {
		printf("type check failed on el1 with el2\n");
		return False;
	}
	talloc_set_type(el1, struct el2);
	if (talloc_get_type(el1, struct el2) != (struct el2 *)el1) {
		printf("type set failed on el1 with el2\n");
		return False;
	}

	talloc_free(root);

	return True;
}

/*
  test steal
*/
static BOOL test_steal(void)
{
	void *root, *p1, *p2;

	printf("TESTING STEAL\n");

	root = talloc_new(NULL);

	p1 = talloc_array(root, char, 10);
	CHECK_SIZE(p1, 10);

	p2 = talloc_realloc(root, NULL, char, 20);
	CHECK_SIZE(p1, 10);
	CHECK_SIZE(root, 30);

	if (talloc_steal(p1, NULL) != NULL) {
		printf("failed: stealing NULL should give NULL\n");
		return False;
	}

	if (talloc_steal(p1, p1) != p1) {
		printf("failed: stealing to ourselves is a nop\n");
		return False;
	}
	CHECK_BLOCKS(root, 3);
	CHECK_SIZE(root, 30);

	talloc_steal(NULL, p1);
	talloc_steal(NULL, p2);
	CHECK_BLOCKS(root, 1);
	CHECK_SIZE(root, 0);

	talloc_free(p1);
	talloc_steal(root, p2);
	CHECK_BLOCKS(root, 2);
	CHECK_SIZE(root, 20);
	
	talloc_free(p2);

	CHECK_BLOCKS(root, 1);
	CHECK_SIZE(root, 0);

	talloc_free(root);

	p1 = talloc_size(NULL, 3);
	CHECK_SIZE(NULL, 3);
	talloc_free(p1);

	return True;
}

/*
  test talloc_realloc_fn
*/
static BOOL test_realloc_fn(void)
{
	void *root, *p1;

	printf("TESTING talloc_realloc_fn\n");

	root = talloc_new(NULL);

	p1 = talloc_realloc_fn(root, NULL, 10);
	CHECK_BLOCKS(root, 2);
	CHECK_SIZE(root, 10);
	p1 = talloc_realloc_fn(root, p1, 20);
	CHECK_BLOCKS(root, 2);
	CHECK_SIZE(root, 20);
	p1 = talloc_realloc_fn(root, p1, 0);
	CHECK_BLOCKS(root, 1);
	CHECK_SIZE(root, 0);

	talloc_free(root);


	return True;
}


static BOOL test_unref_reparent(void)
{
	void *root, *p1, *p2, *c1;

	printf("TESTING UNREFERENCE AFTER PARENT FREED\n");

	root = talloc_named_const(NULL, 0, "root");
	p1 = talloc_named_const(root, 1, "orig parent");
	p2 = talloc_named_const(root, 1, "parent by reference");

	c1 = talloc_named_const(p1, 1, "child");
	talloc_reference(p2, c1);

	talloc_free(p1);
	talloc_unlink(p2, c1);

	CHECK_SIZE(root, 1);

	talloc_free(p2);
	talloc_free(root);

	return True;
}

/*
  measure the speed of talloc versus malloc
*/
static BOOL test_speed(void)
{
	void *ctx = talloc_new(NULL);
	unsigned count;
	struct timeval tv;

	printf("MEASURING TALLOC VS MALLOC SPEED\n");

	tv = timeval_current();
	count = 0;
	do {
		void *p1, *p2, *p3;
		p1 = talloc_size(ctx, count);
		p2 = talloc_strdup(p1, "foo bar");
		p3 = talloc_size(p1, 300);
		talloc_free(p1);
		count += 3;
	} while (timeval_elapsed(&tv) < 5.0);

	printf("talloc: %.0f ops/sec\n", count/timeval_elapsed(&tv));

	talloc_free(ctx);

	tv = timeval_current();
	count = 0;
	do {
		void *p1, *p2, *p3;
		p1 = malloc(count);
		p2 = strdup("foo bar");
		p3 = malloc(300);
		free(p1);
		free(p2);
		free(p3);
		count += 3;
	} while (timeval_elapsed(&tv) < 5.0);

	printf("malloc: %.0f ops/sec\n", count/timeval_elapsed(&tv));

	return True;	
}


BOOL torture_local_talloc(void) 
{
	BOOL ret = True;

	ret &= test_ref1();
	ret &= test_ref2();
	ret &= test_ref3();
	ret &= test_ref4();
	ret &= test_unlink1();
	ret &= test_misc();
	ret &= test_realloc();
	ret &= test_realloc_child();
	ret &= test_steal();
	ret &= test_unref_reparent();
	ret &= test_realloc_fn();
	ret &= test_type();
	if (ret) {
		ret &= test_speed();
	}

	return ret;
}



#if !defined(_SAMBA_BUILD_) || ((SAMBA_VERSION_MAJOR==3)&&(SAMBA_VERSION_MINOR<9))
 int main(void)
{
	if (!torture_local_talloc()) {
		printf("ERROR: TESTSUIE FAILED\n");
		return -1;
	}
	return 0;
}
#endif
