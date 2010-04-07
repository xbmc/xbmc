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
 * Declarations for internal utility functions
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "../kazlib/list.h"
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

// For operating on smallish pointer sets implemented as lists 

/**
 * Compares pointers.
 * 
 * @param ptr1 the first pointer
 * @param ptr2 the second pointer
 * @return zero if the pointers are equal, otherwise non-zero
 */
CP_HIDDEN int cpi_comp_ptr(const void *ptr1, const void *ptr2) CP_GCC_CONST;

/**
 * Returns a hash value for a pointer.
 * 
 * @param ptr the pointer being hashed
 * @return the corresponding hash value
 */
CP_HIDDEN hash_val_t cpi_hashfunc_ptr(const void *ptr) CP_GCC_CONST;

/**
 * Adds a new pointer to a list if the pointer is not yet included.
 * 
 * @param set the set being operated on
 * @param ptr the pointer being added
 * @return non-zero if the operation was successful, zero if allocation failed
 */
CP_HIDDEN int cpi_ptrset_add(list_t *set, void *ptr);

/**
 * Removes a pointer from a pointer set, if it is included.
 * 
 * @param set the set being operated on
 * @param ptr the pointer being removed
 * @return whether the pointer was contained in the set
 */
CP_HIDDEN int cpi_ptrset_remove(list_t *set, const void *ptr);

/**
 * Returns whether a pointer is included in a pointer set.
 * 
 * @param set the set being operated on
 * @param ptr the pointer
 * @return non-zero if the pointer is included, zero otherwise
 */
CP_HIDDEN int cpi_ptrset_contains(list_t *set, const void *ptr) CP_GCC_PURE;


// Other list processing utility functions 

/**
 * Processes a node of the list by freeing the associated pointer and
 * deleting and destroying the node.
 * 
 * @param list the list being processed
 * @param node the list node being processed
 * @param dummy a dummy argument to comply with prototype
 */
CP_HIDDEN void cpi_process_free_ptr(list_t *list, lnode_t *node, void *dummy);


// Version strings

/**
 * Compares two version strings. The comparison algorithm is derived from the
 * way Debian package management system compares package versions. First the
 * the longest prefix of each string composed entirely of non-digit characters
 * is determined. These are compared lexically so that all the letters sort
 * earlier than all the non-letters and otherwise the ordering is based on
 * ASCII values. If there is a difference it is returned. Otherwise the longest
 * prefix of remainder of each string composed entirely of digit characters
 * is determined. These are compared numerically with empty string interpreted
 * as zero. Again, if there is different it is returned. Otherwise the
 * comparison continues with a non-digit component and so on. A NULL version
 * is earlier than any non-NULL version. Two NULL versions are equal.
 * 
 * @param v1 the first version string to compare or NULL
 * @param v2 the second version string to compare or NULL
 * @return less than, equal to or greater than zero when @a v1 < @a v2, @a v1 == @a v2 or @a v1 > @a v2, correspondingly
 */
CP_HIDDEN int cpi_vercmp(const char *v1, const char *v2) CP_GCC_PURE;


#ifdef __cplusplus
}
#endif //__cplusplus 

#endif //UTIL_H_
