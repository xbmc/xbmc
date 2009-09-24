/*
 * File:	ximajbg.h
 * Purpose:	JBG Image Class Loader and Writer
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * CxImageJBG (c) 18/Aug/2002 <ing.davide.pizzolato@libero.it>
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CxImage version 5.80 29/Sep/2003
 * See the file history.htm for the complete bugfix and news report.
 *
 * based on LIBJBG Copyright (c) 2002, Markus Kuhn - All rights reserved.
 *
 * COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION, WARRANTIES
 * THAT THE COVERED CODE IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED
 * CODE IS WITH YOU. SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT
 * THE INITIAL DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
 * SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL
 * PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.
 *
 * Use at your own risk!
 * ==========================================================
 */
#if !defined(__ximaJBG_h)
#define __ximaJBG_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_JBG

extern "C" {
#include "../jbig/jbig.h"
};

class CxImageJBG: public CxImage
{
public:
	CxImageJBG(): CxImage(CXIMAGE_FORMAT_JBG) {}

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_JBG);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_JBG);}
	bool Decode(CxFile * hFile);
	bool Encode(CxFile * hFile);

	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
protected:
	static void jbig_data_out(BYTE *buffer, unsigned int len, void *file)
							{((CxFile*)file)->Write(buffer,len,1);}
};

#endif

#endif
