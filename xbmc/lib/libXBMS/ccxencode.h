/*   -*- c -*-
 * 
 *  ----------------------------------------------------------------------
 *  Protocol packet encode/decode for CcXstream Server for XBMC Media Center
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

#ifndef CC_XENCODE_H_INCLUDED
#define CC_XENCODE_H_INCLUDED 1

#include "ccbuffer.h"

void cc_xstream_encode_int(unsigned char *buf, unsigned long x);
void cc_xstream_buffer_encode_int(CcBuffer buf, unsigned long x);
void cc_xstream_buffer_encode_int64(CcBuffer buf, CC_UINT_64_TYPE_NAME x);
void cc_xstream_buffer_encode_byte(CcBuffer buf, unsigned char b);
void cc_xstream_buffer_encode_string(CcBuffer buf, const char *str);
void cc_xstream_buffer_encode_data_string(CcBuffer buf, const unsigned char *str, size_t str_len);
void cc_xstream_buffer_encode_packet_length(CcBuffer buf);

unsigned long cc_xstream_decode_int(const unsigned char *buf);
int cc_xstream_buffer_decode_int(CcBuffer buf, unsigned long *x);
int cc_xstream_buffer_decode_int64(CcBuffer buf, CC_UINT_64_TYPE_NAME *x);
int cc_xstream_buffer_decode_byte(CcBuffer buf, unsigned char *b);
int cc_xstream_buffer_decode_string(CcBuffer buf, unsigned char **str, size_t *str_len);

#endif /* CC_XENCODE_H_INCLUDED */
/* eof (ccxencode.h) */
