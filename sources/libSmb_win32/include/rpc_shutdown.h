/* 
   Unix SMB/CIFS implementation.
   
   Copyright (C) Jim McDonough (jmcd@us.ibm.com)      2003.
   Copyright (C) Gerald (Jerry) Carter                2005.
   
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

#ifndef _RPC_SHUTDOWN_H /* _RPC_SHUTDOWN_H */
#define _RPC_SHUTDOWN_H 


/* opnums */

#define SHUTDOWN_INIT		0x00
#define SHUTDOWN_ABORT		0x01
#define SHUTDOWN_INIT_EX	0x02


/***********************************************/
 
typedef struct {
	uint16 *server;
	UNISTR4 *message; 	
	uint32 timeout;		/* in seconds */
	uint8 force;		/* boolean: force shutdown */
	uint8 reboot;		/* boolean: reboot on shutdown */		
} SHUTDOWN_Q_INIT;

typedef struct {
	WERROR status;		/* return status */
} SHUTDOWN_R_INIT;

/***********************************************/
 
typedef struct {
	uint16 *server;
	UNISTR4 *message; 	
	uint32 timeout;		/* in seconds */
	uint8 force;		/* boolean: force shutdown */
	uint8 reboot;		/* boolean: reboot on shutdown */
	uint32 reason;		/* reason - must be defined code */
} SHUTDOWN_Q_INIT_EX;

typedef struct {
	WERROR status;
} SHUTDOWN_R_INIT_EX;

/***********************************************/

typedef struct {
	uint16 *server;
} SHUTDOWN_Q_ABORT;

typedef struct { 
	WERROR status; 
} SHUTDOWN_R_ABORT;



#endif /* _RPC_SHUTDOWN_H */

