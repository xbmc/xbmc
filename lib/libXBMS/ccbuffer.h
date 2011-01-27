/*   -*- c -*-
 * 
 *  ----------------------------------------------------------------------
 *  Buffer stuff.
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


#ifndef CCBUFFER_H_INCLUDED
#define CCBUFFER_H_INCLUDED 1

struct CcBufferRec {
  unsigned char *data;
  size_t len;
};

typedef struct CcBufferRec *CcBuffer;
typedef struct CcBufferRec CcBufferRec;

CcBuffer cc_buffer_allocate(void);
void cc_buffer_free(CcBuffer buffer);
void cc_buffer_init(CcBuffer buffer);
void cc_buffer_uninit(CcBuffer buffer);
size_t cc_buffer_len(CcBuffer buffer);
unsigned char *cc_buffer_ptr(CcBuffer buffer);
void cc_buffer_append(CcBuffer buffer, const unsigned char *data, size_t len);
void cc_buffer_append_space(CcBuffer buffer, size_t len);
void cc_buffer_append_string(CcBuffer buffer, const char *string);
void cc_buffer_consume(CcBuffer buffer, size_t len);
void cc_buffer_consume_end(CcBuffer buffer, size_t len);
void cc_buffer_clear(CcBuffer buffer);
void cc_buffer_prepend(CcBuffer buffer, const unsigned char *data, size_t len);
void cc_buffer_prepend_string(CcBuffer buffer, const char *string);
void cc_buffer_steal(CcBuffer buffer, unsigned char **data, size_t *len);

#endif /* CCBUFFER_H_INCLUDED */
/* eof (ccbuffer.h) */
