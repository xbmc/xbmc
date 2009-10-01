/* relaylib.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __RELAYLIB_H__
#define __RELAYLIB_H__

#include "srtypes.h"
#include "compat.h"

/* Each relay connection gets this struct */
typedef struct RELAY_LIST_struct RELAY_LIST;
struct RELAY_LIST_struct
{
    SOCKET m_sock;
    int m_is_new;
    
    char* m_buffer;           // large enough for 1 block & 1 metadata
    u_long m_buffer_size;
    u_long m_cbuf_pos;        // must lie along chunck boundary for mp3
    u_long m_offset;
    u_long m_left_to_send;
    int m_icy_metadata;        // true if client requested metadata

    char* m_header_buf_ptr;    // for ogg header pages
    u_long m_header_buf_len;   // for ogg header pages
    u_long m_header_buf_off;   // for ogg header pages

    RELAY_LIST* m_next;
};


/*****************************************************************************
 * Global variables
 *****************************************************************************/
extern RELAY_LIST* g_relay_list;
extern unsigned long g_relay_list_len;
extern HSEM g_relay_list_sem;


/*****************************************************************************
 * Function prototypes
 *****************************************************************************/
error_code relaylib_set_response_header(char *http_header);
error_code relaylib_init (BOOL search_ports, u_short relay_port, u_short max_port, 
	       u_short *port_used, char *if_name, int max_connections, 
	       char *relay_ip, int have_metadata);
error_code relaylib_start();
error_code relaylib_send(char *data, int len, int accept_new, int is_meta);
void relaylib_shutdown();
BOOL relaylib_isrunning();
error_code relaylib_send_meta_data(char *track);
//int get_relay_sockname(struct sockaddr_in *relay);
void relaylib_disconnect (RELAY_LIST* prev, RELAY_LIST* ptr);

#endif //__RELAYLIB__
