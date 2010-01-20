/*   -*- c -*-
 * 
 *  ----------------------------------------------------------------------
 *  Protocol packet definitions for CcXstream Server for XBMC Media Center
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

#ifndef CC_XPACKET_H_INCLUDED
#define CC_XPACKET_H_INCLUDED 1

typedef enum {
  /* Server -> Client */
  CC_XSTREAM_XBMSP_PACKET_OK = 1,
  CC_XSTREAM_XBMSP_PACKET_ERROR = 2,
  CC_XSTREAM_XBMSP_PACKET_HANDLE = 3,
  CC_XSTREAM_XBMSP_PACKET_FILE_DATA = 4,
  CC_XSTREAM_XBMSP_PACKET_FILE_CONTENTS = 5,
  CC_XSTREAM_XBMSP_PACKET_AUTHENTICATION_CONTINUE = 6,
  /* Client -> Server */
  CC_XSTREAM_XBMSP_PACKET_NULL = 10,
  CC_XSTREAM_XBMSP_PACKET_SETCWD = 11,
  CC_XSTREAM_XBMSP_PACKET_FILELIST_OPEN = 12,
  CC_XSTREAM_XBMSP_PACKET_FILELIST_READ = 13,
  CC_XSTREAM_XBMSP_PACKET_FILE_INFO = 14,
  CC_XSTREAM_XBMSP_PACKET_FILE_OPEN = 15,
  CC_XSTREAM_XBMSP_PACKET_FILE_READ = 16,
  CC_XSTREAM_XBMSP_PACKET_FILE_SEEK = 17,
  CC_XSTREAM_XBMSP_PACKET_CLOSE = 18,
  CC_XSTREAM_XBMSP_PACKET_CLOSE_ALL = 19,
  CC_XSTREAM_XBMSP_PACKET_SET_CONFIGURATION_OPTION = 20,
  CC_XSTREAM_XBMSP_PACKET_AUTHENTICATION_INIT = 21,
  CC_XSTREAM_XBMSP_PACKET_AUTHENTICATE = 22,
  CC_XSTREAM_XBMSP_PACKET_UPCWD = 23,
  /* Server discovery packets */
  CC_XSTREAM_XBMSP_PACKET_SERVER_DISCOVERY_QUERY = 90,
  CC_XSTREAM_XBMSP_PACKET_SERVER_DISCOVERY_REPLY = 91
} CcXstreamPacket;

typedef enum {
  CC_XSTREAM_XBMSP_ERROR_OK = 0,
  CC_XSTREAM_XBMSP_ERROR_FAILURE = 1,
  CC_XSTREAM_XBMSP_ERROR_UNSUPPORTED = 2,
  CC_XSTREAM_XBMSP_ERROR_NO_SUCH_FILE = 3,
  CC_XSTREAM_XBMSP_ERROR_INVALID_FILE = 4,
  CC_XSTREAM_XBMSP_ERROR_INVALID_HANDLE = 5,
  CC_XSTREAM_XBMSP_ERROR_OPEN_FAILED = 6,
  CC_XSTREAM_XBMSP_ERROR_TOO_MANY_OPEN_FILES = 7,
  CC_XSTREAM_XBMSP_ERROR_TOO_LONG_READ = 8,
  CC_XSTREAM_XBMSP_ERROR_ILLEGAL_SEEK = 9,
  CC_XSTREAM_XBMSP_ERROR_OPTION_IS_READ_ONLY = 10,
  CC_XSTREAM_XBMSP_ERROR_INVALID_OPTION_VALUE = 11,
  CC_XSTREAM_XBMSP_ERROR_AUTHENTICATION_NEEDED = 12,
  CC_XSTREAM_XBMSP_ERROR_AUTHENTICATION_FAILED = 13
} CcXstreamError;

#define CC_XSTREAM_DEFAULT_PORT 1400

#endif /* CC_XPACKET_H_INCLUDED */
/* eof (ccxpacket.h) */
