/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Gerald Carter                   2002-2005.
   
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

#ifndef _REG_OBJECTS_H /* _REG_OBJECTS_H */
#define _REG_OBJECTS_H

/* structure to contain registry values */

typedef struct {
	fstring		valuename;
	uint16		type;
	/* this should be encapsulated in an RPC_DATA_BLOB */
	uint32		size;	/* in bytes */
	uint8           *data_p;
} REGISTRY_VALUE;

/* container for registry values */

typedef struct {
	uint32          num_values;
	REGISTRY_VALUE	**values;
} REGVAL_CTR;

/* container for registry subkey names */

typedef struct {
	uint32          num_subkeys;
	char            **subkeys;
} REGSUBKEY_CTR;

#endif /* _REG_OBJECTS_H */

