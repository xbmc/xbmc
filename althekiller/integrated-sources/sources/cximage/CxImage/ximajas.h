/*
 * File:	ximajas.h
 * Purpose:	Jasper Image Class Loader and Writer
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * CxImageJAS (c) 12/Apr/2003 <ing.davide.pizzolato@libero.it>
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CxImage version 5.80 29/Sep/2003
 * See the file history.htm for the complete bugfix and news report.
 *
 * based on JasPer Copyright (c) 2001-2003 Michael David Adams - All rights reserved.
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
#if !defined(__ximaJAS_h)
#define __ximaJAS_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_JASPER

#ifdef _LINUX
#include <jasper.h>
#else
#include "../jasper/include/jasper/jasper.h"
#endif

class CxImageJAS: public CxImage
{
public:
	CxImageJAS(): CxImage(0) {}

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,0);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,0);}
	bool Decode(CxFile * hFile, DWORD imagetype = 0);
	bool Encode(CxFile * hFile, DWORD imagetype = 0);

	bool Decode(FILE *hFile, DWORD imagetype = 0) { CxIOFile file(hFile); return Decode(&file,imagetype); }
	bool Encode(FILE *hFile, DWORD imagetype = 0) { CxIOFile file(hFile); return Encode(&file,imagetype); }
protected:

	class CxFileJas
	{
	public:
		CxFileJas(CxFile* pFile,jas_stream_t *stream)
		{
			if (stream->obj_) jas_free(stream->obj_);
			stream->obj_ = pFile;
			stream->ops_->close_ = JasClose;
			stream->ops_->read_  = JasRead;
			stream->ops_->seek_  = JasSeek;
			stream->ops_->write_ = JasWrite;
		}
		static int JasRead(jas_stream_obj_t *obj, char *buf, int cnt)
		{		return ((CxFile*)obj)->Read(buf,1,cnt); }
		static int JasWrite(jas_stream_obj_t *obj, char *buf, int cnt)
		{		return ((CxFile*)obj)->Write(buf,1,cnt); }
		static long JasSeek(jas_stream_obj_t *obj, long offset, int origin)
		{		return ((CxFile*)obj)->Seek(offset,origin); }
		static int JasClose(jas_stream_obj_t *obj)
		{		return 1; }
	};

};

#endif

#endif
