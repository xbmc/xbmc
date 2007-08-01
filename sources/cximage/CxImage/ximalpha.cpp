// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
// xImalpha.cpp : Alpha channel functions
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximage.h"

#if CXIMAGE_SUPPORT_ALPHA

////////////////////////////////////////////////////////////////////////////////
void CxImage::AlphaClear()
{
	if (pAlpha)	memset(pAlpha,0,head.biWidth * head.biHeight);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::AlphaSet(BYTE level)
{
	if (pAlpha)	memset(pAlpha,level,head.biWidth * head.biHeight);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::AlphaCreate()
{
	AlphaDelete();
	pAlpha = (BYTE*)calloc(head.biWidth * head.biHeight, 1);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::AlphaDelete()
{
	if (pAlpha) { free(pAlpha); pAlpha=0; }
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::AlphaInvert()
{
	if (pAlpha) {
		BYTE *iSrc=pAlpha;
		long n=head.biHeight*head.biWidth;
		for(long i=0; i < n; i++){
			*iSrc=(BYTE)~(*(iSrc));
			iSrc++;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::AlphaCopy(CxImage &from)
{
	if (from.pAlpha == NULL || head.biWidth != from.head.biWidth || head.biWidth != from.head.biWidth) return false;
	if (pAlpha==NULL) pAlpha = (BYTE*)malloc(head.biWidth * head.biHeight);
	memcpy(pAlpha,from.pAlpha,head.biWidth * head.biHeight);
	info.nAlphaMax=from.info.nAlphaMax;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::AlphaSet(CxImage &from)
{
	if (!from.IsGrayScale() || head.biWidth != from.head.biWidth || head.biWidth != from.head.biWidth) return false;
	if (pAlpha==NULL) pAlpha = (BYTE*)malloc(head.biWidth * head.biHeight);
	BYTE* src = from.info.pImage;
	BYTE* dst = pAlpha;
	for (long y=0; y<head.biHeight; y++){
		memcpy(dst,src,head.biWidth);
		dst += head.biWidth;
		src += from.info.dwEffWidth;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::AlphaSet(long x,long y,BYTE level)
{
	if (pAlpha && IsInside(x,y)) pAlpha[x+y*head.biWidth]=level;
}
////////////////////////////////////////////////////////////////////////////////
BYTE CxImage::AlphaGet(long x,long y)
{
	if (pAlpha && IsInside(x,y)) return pAlpha[x+y*head.biWidth];
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::AlphaPaletteClear()
{
	RGBQUAD c;
	for(WORD ip=0; ip<head.biClrUsed;ip++){
		c=GetPaletteColor((BYTE)ip);
		c.rgbReserved=0;
		SetPaletteColor((BYTE)ip,c);
	}
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::AlphaPaletteIsValid()
{
	RGBQUAD c;
	for(WORD ip=0; ip<head.biClrUsed;ip++){
		c=GetPaletteColor((BYTE)ip);
		if (c.rgbReserved != 0) return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::AlphaStrip()
{
	bool bAlphaPaletteIsValid = AlphaPaletteIsValid();
	bool bAlphaIsValid = AlphaIsValid();
	if (!(bAlphaIsValid || bAlphaPaletteIsValid)) return;
	RGBQUAD c;
	long a, a1;
	if (head.biBitCount==24){
		for(long y=0; y<head.biHeight; y++){
			for(long x=0; x<head.biWidth; x++){
				c=GetPixelColor(x,y);
				if (bAlphaIsValid) a=(AlphaGet(x,y)*info.nAlphaMax)/255; else a=info.nAlphaMax;
				a1 = 255-a;
				c.rgbBlue = (BYTE)((c.rgbBlue * a + a1 * info.nBkgndColor.rgbBlue)/255);
				c.rgbGreen = (BYTE)((c.rgbGreen * a + a1 * info.nBkgndColor.rgbGreen)/255);
				c.rgbRed = (BYTE)((c.rgbRed * a + a1 * info.nBkgndColor.rgbRed)/255);
				SetPixelColor(x,y,c);
			}
		}
		AlphaDelete();
	} else {
		CxImage tmp(head.biWidth,head.biHeight,24);
		for(long y=0; y<head.biHeight; y++){
			for(long x=0; x<head.biWidth; x++){
				c=GetPixelColor(x,y);
				if (bAlphaIsValid) a=(AlphaGet(x,y)*info.nAlphaMax)/255; else a=info.nAlphaMax;
				if (bAlphaPaletteIsValid) a=(c.rgbReserved*a)/255;
				a1 = 255-a;
				c.rgbBlue = (BYTE)((c.rgbBlue * a + a1 * info.nBkgndColor.rgbBlue)/255);
				c.rgbGreen = (BYTE)((c.rgbGreen * a + a1 * info.nBkgndColor.rgbGreen)/255);
				c.rgbRed = (BYTE)((c.rgbRed * a + a1 * info.nBkgndColor.rgbRed)/255);
				tmp.SetPixelColor(x,y,c);
			}
		}
		Transfer(tmp);
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::AlphaFlip()
{
	if (!pAlpha) return false;
	BYTE* pAlpha2 = (BYTE*)malloc(head.biWidth * head.biHeight);
	if (!pAlpha2) return false;
	BYTE *iSrc,*iDst;
	iSrc=pAlpha + (head.biHeight-1)*head.biWidth;
	iDst=pAlpha2;
    for(long y=0; y < head.biHeight; y++){
		memcpy(iDst,iSrc,head.biWidth);
		iSrc-=head.biWidth;
		iDst+=head.biWidth;
	}
	free(pAlpha);
	pAlpha=pAlpha2;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::AlphaMirror()
{
	if (!pAlpha) return false;
	BYTE* pAlpha2 = (BYTE*)malloc(head.biWidth * head.biHeight);
	if (!pAlpha2) return false;
	BYTE *iSrc,*iDst;
	long wdt=head.biWidth-1;
	iSrc=pAlpha + wdt;
	iDst=pAlpha2;
	for(long y=0; y < head.biHeight; y++){
		for(long x=0; x <= wdt; x++)
			*(iDst+x)=*(iSrc-x);
		iSrc+=head.biWidth;
		iDst+=head.biWidth;
	}
	free(pAlpha);
	pAlpha=pAlpha2;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::AlphaSplit(CxImage *dest)
{
	if (!pAlpha || !dest) return false;

	CxImage tmp(head.biWidth,head.biHeight,8);

	for(long y=0; y<head.biHeight; y++){
		for(long x=0; x<head.biWidth; x++){
			tmp.SetPixelIndex(x,y,pAlpha[x+y*head.biWidth]);
		}
	}

	tmp.SetGrayPalette();
	dest->Transfer(tmp);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::AlphaPaletteSplit(CxImage *dest)
{
	if (!AlphaPaletteIsValid() || !dest) return false;

	CxImage tmp(head.biWidth,head.biHeight,8);

	for(long y=0; y<head.biHeight; y++){
		for(long x=0; x<head.biWidth; x++){
			tmp.SetPixelIndex(x,y,GetPixelColor(x,y).rgbReserved);
		}
	}

	tmp.SetGrayPalette();
	dest->Transfer(tmp);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_ALPHA
