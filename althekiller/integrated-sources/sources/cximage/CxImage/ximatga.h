/*
 * File:	ximatga.h
 * Purpose:	TARGA Image Class Loader and Writer
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * CxImageTGA (c) 05/Jan/2002 <ing.davide.pizzolato@libero.it>
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
#if !defined(__ximaTGA_h)
#define __ximaTGA_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_TGA

class CxImageTGA: public CxImage
{
#pragma pack(1)
typedef struct tagTgaHeader
{
    BYTE   IdLength;            // Image ID Field Length
    BYTE   CmapType;            // Color Map Type
    BYTE   ImageType;           // Image Type

    WORD   CmapIndex;           // First Entry Index
    WORD   CmapLength;          // Color Map Length
    BYTE   CmapEntrySize;       // Color Map Entry Size

    WORD   X_Origin;            // X-origin of Image
    WORD   Y_Origin;            // Y-origin of Image
    WORD   ImageWidth;          // Image Width
    WORD   ImageHeight;         // Image Height
    BYTE   PixelDepth;          // Pixel Depth
    BYTE   ImagDesc;            // Image Descriptor
} TGAHEADER;
#pragma pack()

public:
	CxImageTGA(): CxImage(CXIMAGE_FORMAT_TGA) {}

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_TGA);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_TGA);}
	bool Decode(CxFile * hFile);
	bool Encode(CxFile * hFile);

	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
protected:
	BYTE ExpandCompressedLine(BYTE* pDest,TGAHEADER* ptgaHead,CxFile *hFile,int width, int y, BYTE rleLeftover);
	void ExpandUncompressedLine(BYTE* pDest,TGAHEADER* ptgaHead,CxFile *hFile,int width, int y, int xoffset);
};

#endif

#endif
