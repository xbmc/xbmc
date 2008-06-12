/*
 * File:	ximaj2k.h
 * Purpose:	J2K Image Class Loader and Writer
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * CxImageJ2K (c) 04/Aug/2002 <ing.davide.pizzolato@libero.it>
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CxImage version 5.80 29/Sep/2003
 * See the file history.htm for the complete bugfix and news report.
 *
 * based on LIBJ2K Copyright (c) 2001-2002, David Janssens - All rights reserved.
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
#if !defined(__ximaJ2K_h)
#define __ximaJ2K_h

#include "ximage.h"

#if CXIMAGE_SUPPORT_J2K

//#define LIBJ2K_EXPORTS
extern "C" {
#include "../j2k/j2k.h"
};

class CxImageJ2K: public CxImage
{
public:
	CxImageJ2K(): CxImage(CXIMAGE_FORMAT_J2K) {}

//	bool Load(const char * imageFileName){ return CxImage::Load(imageFileName,CXIMAGE_FORMAT_J2K);}
//	bool Save(const char * imageFileName){ return CxImage::Save(imageFileName,CXIMAGE_FORMAT_J2K);}
	bool Decode(CxFile * hFile);
	bool Encode(CxFile * hFile);

	bool Decode(FILE *hFile) { CxIOFile file(hFile); return Decode(&file); }
	bool Encode(FILE *hFile) { CxIOFile file(hFile); return Encode(&file); }
protected:
	void j2k_calc_explicit_stepsizes(j2k_tccp_t *tccp, int prec);
	void j2k_encode_stepsize(int stepsize, int numbps, int *expn, int *mant);
	int j2k_floorlog2(int a);
};

#endif

#endif
