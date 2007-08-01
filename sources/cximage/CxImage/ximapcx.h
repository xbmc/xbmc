/*
 * File:	ximapcx.h
 * Purpose:	PCX Image Class Loader and Writer
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * CxImagePCX (c) 05/Jan/2002 <ing.davide.pizzolato@libero.it>
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CxImage version 5.80 29/Sep/2003
 * See the file history.htm for the complete bugfix and news report.
 *
 * Parts of the code come from Paintlib
 * Copyright (c) 1996-1998 Ulrich von Zadow
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
#if !defined(__ximaPCX_h)
#define __ximaPCX_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_PCX

class CxImagePCX: public CxImage
{
// PCX Image File
#pragma pack(1)
typedef struct tagPCXHEADER
{
  char Manufacturer;	// always 0X0A
  char Version;			// version number
  char Encoding;		// always 1
  char BitsPerPixel;	// color bits
  WORD Xmin, Ymin;		// image origin
  WORD Xmax, Ymax;		// image dimensions
  WORD Hres, Vres;		// resolution values
  BYTE ColorMap[16][3];	// color palette
  char Reserved;
  char ColorPlanes;		// color planes
  WORD BytesPerLine;	// line buffer size
  WORD PaletteType;		// grey or color palette
  char Filter[58];
} PCXHEADER;
#pragma pack()

public:
	CxImagePCX(): CxImage(CXIMAGE_FORMAT_PCX) {}

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_PCX);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_PCX);}
	bool Decode(CxFile * hFile);
	bool Encode(CxFile * hFile);

	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
protected:
	void PCX_PlanesToPixels(BYTE * pixels, BYTE * bitplanes, short bytesperline, short planes, short bitsperpixel);
	void PCX_UnpackPixels(BYTE * pixels, BYTE * bitplanes, short bytesperline, short planes, short bitsperpixel);
	void PCX_PackPixels(const long p,BYTE &c, BYTE &n, CxFile &f);
	void PCX_PackPlanes(BYTE* buff, const long size, CxFile &f);
	void PCX_PixelsToPlanes(BYTE* raw, long width, BYTE* buf, long plane);
};

#endif

#endif
