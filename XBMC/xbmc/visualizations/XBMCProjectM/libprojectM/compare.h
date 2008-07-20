/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id$
 *
 * $Log$
 */

#ifndef _COMPARE_H
#define _COMPARE_H
#include "wipemalloc.h"
#include "Common.hpp"

/// @bug this will be ripped away when splaytree is more standardly written or its removed in favor of stl-esque data structure
class SplayKeyFunctions {
public:
static int compare_int(const int * num1, const int * num2);
static int compare_string(const char * string1, const char * string2);

static void free_int(int * num);
static void free_string(char * string);
static void * copy_int(int * num);
static void * copy_string(char * string);
static int compare_string_version(const char * str1, const char * str2);
};


/** tree_types.cpp */
/* Compares integer value numbers in 32 bit range */
inline int SplayKeyFunctions::compare_int(const int * num1, const int * num2) {

	if ((*num1) < (*num2))
		return -1;
	if ((*num1) > (*num2))
		return 1;
	
	return 0;
}

/* Compares strings in lexographical order */
inline int SplayKeyFunctions::compare_string(const char * str1, const char * str2) {

  //  printf("comparing \"%s\" to \"%s\"\n", str1, str2);
  //return strcmp(str1, str2);
  return strncmp(str1, str2, MAX_TOKEN_SIZE-1);
	
}	

/* Compares a string in version order. That is, file1 < file2 < file10 */
inline int SplayKeyFunctions::compare_string_version(const char * str1, const char * str2) {

  return strcmp( str1, str2 );
#ifdef PANTS
  return strverscmp(str1, str2);
#endif
}


inline void SplayKeyFunctions::free_int(int * num) {
	free(num);
}



 inline void SplayKeyFunctions::free_string(char * string) {	
	free(string);	
}
 


 inline void * SplayKeyFunctions::copy_int(int * num) {
	
	int * new_num;
	
	if ((new_num = (int*)wipemalloc(sizeof(int))) == NULL)
		return NULL;

	*new_num = *num;
	
	return (void*)new_num;
}	


inline void * SplayKeyFunctions::copy_string(char * string) {
	
	char * new_string;
	
	if ((new_string = (char*)wipemalloc(MAX_TOKEN_SIZE)) == NULL)
		return NULL;
	
	strncpy(new_string, string, MAX_TOKEN_SIZE-1);
	
	return (void*)new_string;
}

#endif /** !_COMPARE_H */
