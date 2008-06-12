/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell 1992-1997
   Copyright (C) Luke Kenneth Casson Leighton 1996-1997
   Copyright (C) Paul Ashton 1997
   
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

#ifndef _RPC_WKS_H /* _RPC_WKS_H */
#define _RPC_WKS_H 


/* wkssvc pipe */
#define WKS_QUERY_INFO    0x00


/* WKS_Q_QUERY_INFO - probably a capabilities request */
typedef struct q_wks_query_info_info
{
	uint32 ptr_srv_name;         /* pointer (to server name?) */
	UNISTR2 uni_srv_name;        /* unicode server name starting with '\\' */

	uint16 switch_value;            /* info level 100 (0x64) */

} WKS_Q_QUERY_INFO;


/* WKS_INFO_100 - level 100 info */
typedef struct wks_info_100_info
{
	uint32 platform_id;          /* 0x0000 01f4 - unknown */
	uint32 ptr_compname;       /* pointer to server name */
	uint32 ptr_lan_grp ;       /* pointer to domain name */
	uint32 ver_major;          /* 4 - unknown */
	uint32 ver_minor;          /* 0 - unknown */

	UNISTR2 uni_compname;      /* unicode server name */
	UNISTR2 uni_lan_grp ;      /* unicode domain name */

} WKS_INFO_100;


/* WKS_R_QUERY_INFO - probably a capabilities request */
typedef struct r_wks_query_info_info
{
	uint16 switch_value;          /* 100 (0x64) - switch value */

	/* for now, only level 100 is supported.  this should be an enum container */
	uint32 ptr_1;              /* pointer 1 */
	WKS_INFO_100 *wks100;      /* workstation info level 100 */

	NTSTATUS status;             /* return status */

} WKS_R_QUERY_INFO;


#endif /* _RPC_WKS_H */

