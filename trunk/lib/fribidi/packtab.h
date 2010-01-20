/* PackTab - Pack a static table
 * Copyright (C) 2001 Behdad Esfahbod. 
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
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library, in a file named COPYING; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA  
 * 
 * For licensing issues, contact <fwpg@sharif.edu>. 
 */

#ifndef PACKTAB_H
#define PACKTAB_H

#ifdef __cplusplus
extern "C"
{
#endif

#define packtab_version 2

  int pack_table (int *base,
		  int key_num,
		  int key_size,
		  int max_depth,
		  int tab_width,
		  const char * const *name,
		  const char *key_type_name,
		  const char *table_name,
		  const char *macro_name,
		  FILE * out);

#ifdef	__cplusplus
}
#endif

#endif				/* PACKTAB_H */
