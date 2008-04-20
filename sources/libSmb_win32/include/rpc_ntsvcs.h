/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Gerald (Jerry) Carter        2005
   
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

#ifndef _RPC_NTSVCS_H /* _RPC_NTSVCS_H */
#define _RPC_NTSVCS_H

/* ntsvcs pipe */

#define NTSVCS_GET_VERSION		0x02
#define NTSVCS_VALIDATE_DEVICE_INSTANCE	0x06
#define NTSVCS_GET_ROOT_DEVICE_INSTANCE	0x07
#define NTSVCS_GET_DEVICE_LIST		0x0a
#define NTSVCS_GET_DEVICE_LIST_SIZE	0x0b
#define NTSVCS_GET_DEVICE_REG_PROPERTY	0x0d
#define NTSVCS_HW_PROFILE_FLAGS		0x28
#define NTSVCS_GET_HW_PROFILE_INFO	0x29
#define NTSVCS_GET_VERSION_INTERNAL	0x3e


/**************************/

typedef struct {
	/* nothing in the request */
	uint32 dummy;
} NTSVCS_Q_GET_VERSION;

typedef struct {
	uint32 version;
	WERROR status;
} NTSVCS_R_GET_VERSION;


/**************************/

typedef struct {
	UNISTR2 *devicename;
	uint32 flags;
} NTSVCS_Q_GET_DEVICE_LIST_SIZE;

typedef struct {
	uint32 size;
	WERROR status;
} NTSVCS_R_GET_DEVICE_LIST_SIZE;


/**************************/

typedef struct {
	UNISTR2 *devicename;
	uint32 buffer_size;
	uint32 flags;
} NTSVCS_Q_GET_DEVICE_LIST;

typedef struct {
	UNISTR2 devicepath;
	uint32 needed;
	WERROR status;
} NTSVCS_R_GET_DEVICE_LIST;

/**************************/

typedef struct {
	UNISTR2 devicepath;
	uint32 flags;
} NTSVCS_Q_VALIDATE_DEVICE_INSTANCE;

typedef struct {
	WERROR status;
} NTSVCS_R_VALIDATE_DEVICE_INSTANCE;

/**************************/

#define DEV_REGPROP_DESC	1

typedef struct {
	UNISTR2 devicepath;
	uint32 property;
	uint32 unknown2;
	uint32 buffer_size1;
	uint32 buffer_size2;
	uint32 unknown5;
} NTSVCS_Q_GET_DEVICE_REG_PROPERTY;

typedef struct {
	uint32 unknown1;
	REGVAL_BUFFER value;
	uint32 size;
	uint32 needed;
	WERROR status;
} NTSVCS_R_GET_DEVICE_REG_PROPERTY;


/**************************/

typedef struct {
	uint32 index;
	uint8 *buffer;
	uint32 buffer_size;
	uint32 unknown1;
} NTSVCS_Q_GET_HW_PROFILE_INFO;

typedef struct {
	uint32 buffer_size;	/* the size (not included in the reply) 
				   if just matched from the request */
	uint8 *buffer;
	WERROR status;
} NTSVCS_R_GET_HW_PROFILE_INFO;


/**************************/

typedef struct {
	uint32 unknown1;
	UNISTR2 devicepath;
	uint32 unknown2;
	uint32 unknown3;
	uint32 unknown4;
	uint32 unknown5;
	uint32 unknown6;
	uint32 unknown7;
} NTSVCS_Q_HW_PROFILE_FLAGS;

typedef struct {
	uint32 unknown1;
	uint32 unknown2;
	uint32 unknown3;
	WERROR status;
} NTSVCS_R_HW_PROFILE_FLAGS;

#endif /* _RPC_NTSVCS_H */
