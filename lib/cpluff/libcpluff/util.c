/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

/** @file
 * Internal utility functions
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "../kazlib/list.h"
#include "cpluff.h"
#include "defines.h"
#include "util.h"


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

CP_HIDDEN int cpi_comp_ptr(const void *ptr1, const void *ptr2) {
	return !(ptr1 == ptr2);
}

CP_HIDDEN hash_val_t cpi_hashfunc_ptr(const void *ptr) {
	return (hash_val_t) ptr;
}

CP_HIDDEN int cpi_ptrset_add(list_t *set, void *ptr) {
	

	// Only add the pointer if it is not already included 
	if (cpi_ptrset_contains(set, ptr)) {
		return 1;
	} else {
		lnode_t *node;

		/* Add the pointer to the list */		
		node = lnode_create(ptr);
		if (node == NULL) {
			return 0;
		}
		list_append(set, node);
		return 1;
	}
	
}

CP_HIDDEN int cpi_ptrset_remove(list_t *set, const void *ptr) {
	lnode_t *node;
	
	// Find the pointer if it is in the set 
	node = list_find(set, ptr, cpi_comp_ptr);
	if (node != NULL) {
		list_delete(set, node);
		lnode_destroy(node);
		return 1;
	} else {
		return 0;
	}
}

CP_HIDDEN int cpi_ptrset_contains(list_t *set, const void *ptr) {
	return list_find(set, ptr, cpi_comp_ptr) != NULL;
}

CP_HIDDEN void cpi_process_free_ptr(list_t *list, lnode_t *node, void *dummy) {
	void *ptr = lnode_get(node);
	list_delete(list, node);
	lnode_destroy(node);
	free(ptr);
}

static const char *vercmp_nondigit_end(const char *v) {
	while (*v != '\0' && (*v < '0' || *v > '9')) {
		v++;
	}
	return v;
}

static const char *vercmp_digit_end(const char *v) {
	while (*v >= '0' && *v <= '9') {
		v++;
	}
	return v;
}

static int vercmp_char_value(char c) {
	if (c == '\0') {
		return 0;
	} else if (c >= 'A' && c <= 'Z') {
		return 1 + (c - 'A');
	} else if (c >= 'a' && c <= 'z') {
		return 1 + ('Z' - 'A' + 1) + (c - 'a');
	} else {
		int i = 1 + ('Z' - 'A' + 1) + ('z' - 'a' + 1) + ((int) c - CHAR_MIN);
		if (c > 'z') {
			i -= 'z' - 'a' + 1;
		}
		if (c > 'Z') {
			i -= 'Z' - 'A' + 1;
		}
		if (c > '\0') {
			i--;
		}
		return i;
	}
}

static int vercmp_num_value(const char *v, const char *vn) {
	
	// Skip leading zeros
	while (v < vn && *v == '0') {
		v++;
	}
	
	// Empty string equals to zero
	if (v == vn) {
		return 0;
	}
	
	// Otherwise return the integer value
	else {
		char str[16];
		strncpy(str, v, vn - v < 16 ? vn - v : 16);
		str[vn - v < 16 ? vn - v : 15] = '\0';
		return atoi(str);
	}
}

CP_HIDDEN int cpi_vercmp(const char *v1, const char *v2) {
	const char *v1n;
	const char *v2n;
	
	// Check for NULL versions
	if (v1 == NULL && v2 != NULL) {
		return -1;
	} else if (v1 == NULL && v2 == NULL) {
		return 0;
	} else if (v1 != NULL && v2 == NULL) {
		return 1;
	}
	assert(v1 != NULL && v2 != NULL);
	
	// Component comparison loop
	while (*v1 != '\0' || *v2 != '\0') {
		
		// Determine longest non-digit prefix
		v1n = vercmp_nondigit_end(v1);
		v2n = vercmp_nondigit_end(v2);
		
		// Compare the non-digit strings
		while (v1 < v1n || v2 < v2n) {
			char c1 = '\0';
			char c2 = '\0';
			
			if (v1 < v1n) {
				c1 = *v1++;
			}
			if (v2 < v2n) {
				c2 = *v2++;
			}
			int diff = vercmp_char_value(c1) - vercmp_char_value(c2);
			if (diff != 0) {
				return diff;
			}
			assert(v1 <= v1n && v2 <= v2n);
		}
		assert(v1 == v1n && v2 == v2n);
		
		// Determine the longest digit prefix
		v1n = vercmp_digit_end(v1);
		v2n = vercmp_digit_end(v2);
		
		// Compare the digit strings
		{
			int i1 = vercmp_num_value(v1, v1n);
			int i2 = vercmp_num_value(v2, v2n);
			int diff = i1 - i2;
			if (diff != 0) {
				return diff;
			}
		}
		v1 = v1n;
		v2 = v2n;
		
	}
	return 0;
}
