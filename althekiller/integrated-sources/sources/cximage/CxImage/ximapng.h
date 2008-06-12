/*
 * File:	ximapng.h
 * Purpose:	PNG Image Class Loader and Writer
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * CxImagePNG (c) 07/Aug/2001 <ing.davide.pizzolato@libero.it>
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CxImage version 5.80 29/Sep/2003
 * See the file history.htm for the complete bugfix and news report.
 *
 * Special thanks to Troels Knakkergaard for new features, enhancements and bugfixes
 *
 * original CImagePNG  and CImageIterator implementation are:
 * Copyright:	(c) 1995, Alejandro Aguilar Sierra <asierra(at)servidor(dot)unam(dot)mx>
 *
 * libpng version 1.2.1 - December 12, 2001
 * Copyright (c) 1998-2001 Glenn Randers-Pehrson
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
#if !defined(__ximaPNG_h)
#define __ximaPNG_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_PNG

extern "C" {
#ifdef _LINUX
#define PNGAPI
#include <png.h>
#else
#include "../png/png.h"
#endif
}

class CxImagePNG: public CxImage
{
public:
	CxImagePNG(): CxImage(CXIMAGE_FORMAT_PNG) {}

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_PNG);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_PNG);}
	bool Decode(CxFile * hFile);
	bool Encode(CxFile * hFile);

	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }

protected:
	void ima_png_error(png_struct *png_ptr, char *message);
	void expand2to4bpp(BYTE* prow);

	static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		CxFile* hFile = (CxFile*)png_ptr->io_ptr;
		if (hFile->Read(data,1,length) != length) png_error(png_ptr, "Read Error");
	}

	static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		CxFile* hFile = (CxFile*)png_ptr->io_ptr;
		if (hFile->Write(data,1,length) != length) png_error(png_ptr, "Write Error");
	}

	static void user_flush_data(png_structp png_ptr)
	{
		CxFile* hFile = (CxFile*)png_ptr->io_ptr;
		if (!hFile->Flush()) png_error(png_ptr, "Flush Error");
	}
    static void user_error_fn(png_structp png_ptr,png_const_charp error_msg)
	{
		strncpy((char*)png_ptr->error_ptr,error_msg,255);
		longjmp(png_ptr->jmpbuf, 1);
	}
};

#endif

#endif
