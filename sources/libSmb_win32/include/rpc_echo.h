/* 
   Unix SMB/CIFS implementation.

   Samba rpcecho definitions.

   Copyright (C) Tim Potter 2003

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

#ifndef _RPC_ECHO_H
#define _RPC_ECHO_H

#define ECHO_ADD_ONE          0x00
#define ECHO_DATA             0x01
#define ECHO_SINK_DATA        0x02
#define ECHO_SOURCE_DATA      0x03

typedef struct echo_q_add_one
{
	uint32 request;
} ECHO_Q_ADD_ONE;

typedef struct echo_r_add_one
{
	uint32 response;
} ECHO_R_ADD_ONE;

typedef struct echo_q_echo_data
{
	uint32 size;
	char *data;
} ECHO_Q_ECHO_DATA;

typedef struct echo_r_echo_data
{
	uint32 size;
	char *data;
} ECHO_R_ECHO_DATA;

typedef struct echo_q_source_data
{
	uint32 size;
} ECHO_Q_SOURCE_DATA;

typedef struct echo_r_source_data
{
	uint32 size;
	char *data;
} ECHO_R_SOURCE_DATA;

typedef struct echo_q_sink_data
{
	uint32 size;
	char *data;
} ECHO_Q_SINK_DATA;

typedef struct echo_r_sink_data
{
	int dummy;		/* unused */
} ECHO_R_SINK_DATA;

#endif  
