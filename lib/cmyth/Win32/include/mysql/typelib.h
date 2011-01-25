/* Copyright (C) 2000 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */


#ifndef _typelib_h
#define _typelib_h

#include "my_alloc.h"

typedef struct st_typelib {	/* Different types saved here */
  unsigned int count;		/* How many types */
  const char *name;		/* Name of typelib */
  const char **type_names;
  unsigned int *type_lengths;
} TYPELIB;

extern my_ulonglong find_typeset(char *x, TYPELIB *typelib,int *error_position);
extern int find_type_or_exit(const char *x, TYPELIB *typelib,
                             const char *option);
extern int find_type(char *x, const TYPELIB *typelib, unsigned int full_name);
extern void make_type(char *to,unsigned int nr,TYPELIB *typelib);
extern const char *get_type(TYPELIB *typelib,unsigned int nr);
extern TYPELIB *copy_typelib(MEM_ROOT *root, TYPELIB *from);

extern TYPELIB sql_protocol_typelib;

#endif /* _typelib_h */
