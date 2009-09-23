/*   -*- c -*-
 * 
 *  ----------------------------------------------------------------------
 *  Translate raw strings so that they can be inserted into xml.
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

#ifndef CC_XMLTRANS_H_INCLUDED
#define CC_XMLTRANS_H_INCLUDED 1

char *cc_xstream_xml_encode(const char *raw);
char *cc_xstream_xml_decode(const char *xml);

#endif /* CC_XMLTRANS_H_INCLUDED */
/* eof (ccxmltrans.h) */
