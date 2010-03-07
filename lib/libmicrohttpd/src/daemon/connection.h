/*
     This file is part of libmicrohttpd
     (C) 2007 Daniel Pittman and Christian Grothoff

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @file connection.h
 * @brief  Methods for managing connections
 * @author Daniel Pittman
 * @author Christian Grothoff
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include "internal.h"

/**
 * Obtain the select sets for this connection.
 *
 * @return MHD_YES on success
 */
int
MHD_connection_get_fdset (struct MHD_Connection *connection,
                          fd_set * read_fd_set,
                          fd_set * write_fd_set,
                          fd_set * except_fd_set, int *max_fd);

/**
 * Obtain the pollfd for this connection. The poll interface allows large
 * file descriptors. Select goes stupid when the fd overflows fdset (which
 * is fixed).
 */
int MHD_connection_get_pollfd(struct MHD_Connection *connection,
                              struct MHD_Pollfd *p);

void MHD_set_http_calbacks (struct MHD_Connection *connection);

int MHD_connection_handle_read (struct MHD_Connection *connection);

int MHD_connection_handle_write (struct MHD_Connection *connection);

int MHD_connection_handle_idle (struct MHD_Connection *connection);

/**
 * Close the given connection and give the
 * specified termination code to the user.
 */
void MHD_connection_close (struct MHD_Connection *connection,
                           enum MHD_RequestTerminationCode termination_code);

#endif
