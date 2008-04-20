/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell 1992-1999
   Copyright (C) Luke Kenneth Casson Leighton 1996-1999
   
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

#ifndef _RPC_BRS_H /* _RPC_BRS_H */
#define _RPC_BRS_H 


/* brssvc pipe */
#define BRS_QUERY_INFO    0x02


/* BRS_Q_QUERY_INFO - probably a capabilities request */
typedef struct q_brs_query_info_info
{
	uint32 ptr_srv_name;         /* pointer (to server name?) */
	UNISTR2 uni_srv_name;        /* unicode server name starting with '\\' */

	uint16 switch_value1;            /* info level 100 (0x64) */
	/* align */
	uint16 switch_value2;            /* info level 100 (0x64) */

	uint32 ptr;
	uint32 pad1;
	uint32 pad2;

} BRS_Q_QUERY_INFO;


/* BRS_INFO_100 - level 100 info */
typedef struct brs_info_100_info
{
	uint32 pad1;
	uint32 ptr2;
	uint32 pad2;
	uint32 pad3;

} BRS_INFO_100;


/* BRS_R_QUERY_INFO - probably a capabilities request */
typedef struct r_brs_query_info_info
{
	uint16 switch_value1;          /* 100 (0x64) - switch value */
	/* align */
	uint16 switch_value2;            /* info level 100 (0x64) */

	/* for now, only level 100 is supported.  this should be an enum container */
	uint32 ptr_1;              /* pointer 1 */

	union
	{
		BRS_INFO_100 *brs100;      /* browser info level 100 */
		void *id;

	} info;

	NTSTATUS status;             /* return status */

} BRS_R_QUERY_INFO;

#endif /* _RPC_BRS_H */

