/*
 * File:	ximatif.h
 * Purpose:	TIFF Image Class Loader and Writer
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * CxImageTIF (c) 07/Aug/2001 <ing.davide.pizzolato@libero.it>
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CxImage version 5.80 29/Sep/2003
 * See the file history.htm for the complete bugfix and news report.
 *
 * Special thanks to Troels Knakkergaard for new features, enhancements and bugfixes
 *
 * Special thanks to Abe <God(dot)bless(at)marihuana(dot)com> for MultiPageTIFF code.
 *
 * Parts of the code come from FreeImage 2
 * Design and implementation by 
 * - Floris van den Berg (flvdberg@wxs.nl)
 * - Hervé Drolon (drolon@iut.univ-lehavre.fr)
 * - Markus Loibl (markus.loibl@epost.de)
 * - Luca Piergentili (l.pierge@terra.es)
 *
 * LibTIFF is:
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
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



#if !defined(__ximatif_h)
#define __ximatif_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_TIF

#include "../tiff/tiffio.h"

class DLL_EXP CxImageTIF: public CxImage
{
public:
	CxImageTIF(): CxImage(CXIMAGE_FORMAT_TIF) {m_tif2=NULL; m_multipage=false; m_pages=0;}
	~CxImageTIF();

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_TIF);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_TIF);}
	bool Decode(CxFile * hFile);
	bool Encode(CxFile * hFile, bool bAppend=false);
	bool Encode(CxFile * hFile, CxImage ** pImages, int pagecount);

	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }
	bool Encode(FILE *hFile, bool bAppend=false) { CxIOFile file(hFile); return Encode(&file,bAppend); }
	bool Encode(FILE *hFile, CxImage ** pImages, int pagecount)
				{ CxIOFile file(hFile); return Encode(&file, pImages, pagecount); }

protected:
	void TileToStrip(uint8* out, uint8* in,	uint32 rows, uint32 cols, int outskew, int inskew);
	bool EncodeBody(TIFF *m_tif, bool multipage=false, int page=0, int pagecount=0);
	TIFF *m_tif2;
	bool m_multipage;
	int  m_pages;
};

#endif

#endif
