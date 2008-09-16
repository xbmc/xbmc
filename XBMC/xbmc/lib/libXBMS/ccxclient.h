/*   -*- c -*-
 * 
 *  ----------------------------------------------------------------------
 *  CcXstream Client Library for XBMC Media Center
 *  ----------------------------------------------------------------------
 *
 *  Copyright (c) 2002-2003 by PuhPuh
 *  
 *  This code is copyrighted property of the author.  It can still
 *  be used for any non-commercial purpose following conditions:
 *  
 *      1) This copyright notice is not removed.
 *      2) Source code follows any distribution of the software
 *         if possible.
 *      3) Copyright notice above is found in the documentation
 *         of the distributed software.
 *  
 *  Any express or implied warranties are disclaimed.  Author is
 *  not liable for any direct or indirect damages caused by the use
 *  of this software.
 *
 *  ----------------------------------------------------------------------
 *
 *  This code has been integrated into XBMC Media Center.  
 *  As such it can me copied, redistributed and modified under
 *  the same conditions as the XBMC itself.
 *
 */


#ifndef CC_XCLIENT_H_INCLUDED
#define CC_XCLIENT_H_INCLUDED 1

#include "ccxversion.h"
#include "ccxpacket.h"

/* The definition of CcXstreamServerConnection is system dependent.
   In unix like systems this is simply int that is a file descriptor
   of the connection socket. */
#ifdef _XBOX
typedef SOCKET CcXstreamServerConnection;
#else /* _XBOX */
typedef int CcXstreamServerConnection;
#endif /* _XBOX */

typedef struct CcXstreamReplyPacketRec *CcXstreamReplyPacket;

struct CcXstreamReplyPacketRec {
  CcXstreamPacket type;
  unsigned long id;
  unsigned long handle;
  unsigned long error;
  unsigned char *string1;
  size_t string1_len;
  unsigned char *string2;
  size_t string2_len;
};

typedef enum {
  /* Command was succesful and session can continue. */
  CC_XSTREAM_CLIENT_OK = 0,
  /* Command was failed but session is ok. */
  CC_XSTREAM_CLIENT_COMMAND_FAILED = 1,
  /* Command was probably failed and session is broken. */
  CC_XSTREAM_CLIENT_FATAL_ERROR = 2,
  /* Server host not found. */
  CC_XSTREAM_CLIENT_SERVER_NOT_FOUND = 3,
  /* Server host found but connection attempt failed. */
  CC_XSTREAM_CLIENT_SERVER_CONNECTION_FAILED = 4
} CcXstreamClientError;

CcXstreamClientError cc_xstream_client_connect(const char *host,
					       int port,
					       CcXstreamServerConnection *s);

CcXstreamClientError cc_xstream_client_disconnect(CcXstreamServerConnection s);
					       
unsigned char *cc_xstream_client_read_data(CcXstreamServerConnection s, 
					   size_t len, 
					   unsigned long timeout_ms);
int cc_xstream_client_write_data(CcXstreamServerConnection s, 
				 unsigned char *buf,
				 size_t len, 
				 unsigned long timeout_ms);

/* Make a packet that can be sent to the server directly.  Return
   value is an operation identifier that is selecte by the client
   library and embedded into the packet.  Server always returns a
   packet with same id number.  This is a low level interface that may
   be used, if fully asynchronous client is needed. */

CcXstreamClientError cc_xstream_client_version_handshake(CcXstreamServerConnection s);

unsigned long cc_xstream_client_mkpacket_setcwd(const char *path, 
						unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_upcwd(unsigned long levels,
					       unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_filelist_open(unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_filelost_read(unsigned long handle,
						       unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_file_info(const char *path,
						   unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_file_open(const char *path,
						   unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_file_read(unsigned long handle, size_t len,
						   unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_seek(unsigned long handle, 
					      int seek_type, CC_UINT_64_TYPE_NAME bytes,
					      unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_close(unsigned long handle,
					       unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_close_all(unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_setconfoption(const char *option, 
						       const char *value,
						       unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_authentication_init(const char *method,
	       						     unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_authenticate_password(unsigned long handle,
							       const char *user_id,
							       const char *password,
							       unsigned char **p, size_t *p_len);

unsigned long cc_xstream_client_mkpacket_server_discovery(unsigned char **p, size_t *p_len);


/* Packet reading and parsing fuctionality is also quite low level.
   It is mainly needed for applications that implement fully
   asynchronous protocol session. */

/* Packet is passed to this function without length field. */
CcXstreamReplyPacket cc_xstream_client_reply_packet_parse(const unsigned char *packet,
							  size_t packet_len);

/* Read a packet from the socket and parse it. */
CcXstreamClientError cc_xstream_client_reply_packet_read(CcXstreamServerConnection s,
							 CcXstreamReplyPacket *packet);

/* Free the packet. */
void cc_xstream_client_reply_packet_free(CcXstreamReplyPacket packet);

/* Following interfaces provide a synchronous interface to the server.
   If this interface is used, the user can't send his own commands
   directly bypassing this interface. */

CcXstreamClientError cc_xstream_client_setcwd(CcXstreamServerConnection s,
					      const char *path);
CcXstreamClientError cc_xstream_client_upcwd(CcXstreamServerConnection s,
					     unsigned long levels);
CcXstreamClientError cc_xstream_client_close_all(CcXstreamServerConnection s);
CcXstreamClientError cc_xstream_client_file_open(CcXstreamServerConnection s,
						 const char *path,
						 unsigned long *handle);
CcXstreamClientError cc_xstream_client_file_read(CcXstreamServerConnection s,
						 unsigned long handle,
						 size_t len,
						 unsigned char **data,
						 size_t *data_len);
CcXstreamClientError cc_xstream_client_dir_open(CcXstreamServerConnection s,
						unsigned long *handle);
CcXstreamClientError cc_xstream_client_dir_read(CcXstreamServerConnection s,
						unsigned long handle,
						char **name,
						char **info);
CcXstreamClientError cc_xstream_client_close(CcXstreamServerConnection s,
					     unsigned long handle);
CcXstreamClientError cc_xstream_client_file_forward(CcXstreamServerConnection s,
					            unsigned long handle,
					            CC_UINT_64_TYPE_NAME bytes,
					            int seek_eof_if_fails);
CcXstreamClientError cc_xstream_client_file_backwards(CcXstreamServerConnection s,
					       	      unsigned long handle,
					              CC_UINT_64_TYPE_NAME bytes,
					              int rewind_if_fails);

CcXstreamClientError cc_xstream_client_file_rewind(CcXstreamServerConnection s,
					       	   unsigned long handle);
CcXstreamClientError cc_xstream_client_file_end(CcXstreamServerConnection s,
						unsigned long handle);
CcXstreamClientError cc_xstream_client_file_info(CcXstreamServerConnection s,
						 const char *path,
       						 char **info);
CcXstreamClientError cc_xstream_client_set_configuration_option(CcXstreamServerConnection s,
								const char *option,
								const char *value);
CcXstreamClientError cc_xstream_client_password_authenticate(CcXstreamServerConnection s,
							     const char *user_id,
							     const char *password);

/* Server discovery function.  May not be supported in all systems. */

typedef void (*CcXstreamServerDiscoveryCB)(const char *addr,
					   const char *port,
					   const char *version,
					   const char *comment,
					   void *context);

CcXstreamClientError ccx_client_discover_servers(CcXstreamServerDiscoveryCB callback, void *context);


#define CC_XSTREAM_CLIENT_VERSION_STR "XBMSP-1.0 CcXstream Client Library " CC_XSTREAM_SW_VERSION
#define CC_XSTREAM_CLIENT_VERSION     "1.0"

/* If the server end is inresponsive for 10 seconds, connection will halt. */
#define CCXSTREAM_CLIENT_TIMEOUT_SECONDS 10

#endif /* CC_XCLIENT_H_INCLUDED */
/* eof (ccxclient.h) */
