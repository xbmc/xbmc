// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
// xImaPal.cpp : Palette and Pixel functions
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximage.h"

////////////////////////////////////////////////////////////////////////////////
// returns the palette dimension in byte
DWORD CxImage::GetPaletteSize()
{
	return (head.biClrUsed * sizeof(RGBQUAD));
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPaletteColor(BYTE idx, BYTE r, BYTE g, BYTE b, BYTE alpha)
{
	if ((pDib)&&(head.biClrUsed)){
		BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
		if (idx<head.biClrUsed){
			long ldx=idx*sizeof(RGBQUAD);
			iDst[ldx++] = (BYTE) b;
			iDst[ldx++] = (BYTE) g;
			iDst[ldx++] = (BYTE) r;
			iDst[ldx] = (BYTE) alpha;
			info.last_c_isvalid = false;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPaletteColor(BYTE idx, RGBQUAD c)
{
	if ((pDib)&&(head.biClrUsed)){
		BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
		if (idx<head.biClrUsed){
			long ldx=idx*sizeof(RGBQUAD);
			iDst[ldx++] = (BYTE) c.rgbBlue;
			iDst[ldx++] = (BYTE) c.rgbGreen;
			iDst[ldx++] = (BYTE) c.rgbRed;
			iDst[ldx] = (BYTE) c.rgbReserved;
			info.last_c_isvalid = false;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPaletteColor(BYTE idx, COLORREF cr)
{
	if ((pDib)&&(head.biClrUsed)){
		BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
		if (idx<head.biClrUsed){
			long ldx=idx*sizeof(RGBQUAD);
			iDst[ldx++] = (BYTE) GetBValue(cr);
			iDst[ldx++] = (BYTE) GetGValue(cr);
			iDst[ldx++] = (BYTE) GetRValue(cr);
			iDst[ldx] = (BYTE) 0;
			info.last_c_isvalid = false;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
// returns the pointer to the first palette index 
RGBQUAD* CxImage::GetPalette() const
{
	if ((pDib)&&(head.biClrUsed))
		return (RGBQUAD*)((BYTE*)pDib + sizeof(BITMAPINFOHEADER));
	return NULL;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::GetPaletteColor(BYTE idx)
{
	RGBQUAD rgb = {0,0,0,0};
	if ((pDib)&&(head.biClrUsed)){
		BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
		if (idx<head.biClrUsed){
			long ldx=idx*sizeof(RGBQUAD);
			rgb.rgbBlue = iDst[ldx++];
			rgb.rgbGreen=iDst[ldx++];
			rgb.rgbRed =iDst[ldx++];
			rgb.rgbReserved = iDst[ldx];
		}
	}
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
BYTE CxImage::GetPixelIndex(long x,long y)
{
	if ((pDib==NULL)||(head.biClrUsed==0)) return 0;

	if ((x<0)||(y<0)||(x>=head.biWidth)||(y>=head.biHeight)) {
		if (info.nBkgndIndex != -1)	return (BYTE)info.nBkgndIndex;
		else return *info.pImage;
	}
	if (head.biBitCount==8){
		return info.pImage[y*info.dwEffWidth + x];
	} else {
		BYTE pos;
		BYTE iDst= info.pImage[y*info.dwEffWidth + (x*head.biBitCount >> 3)];
		if (head.biBitCount==4){
			pos = (BYTE)(4*(1-x%2));
			iDst &= (0x0F<<pos);
			return (BYTE)(iDst >> pos);
		} else if (head.biBitCount==1){
			pos = (BYTE)(7-x%8);
			iDst &= (0x01<<pos);
			return (BYTE)(iDst >> pos);
		}
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::GetPixelColor(long x,long y)
{
//	RGBQUAD rgb={0,0,0,0};
	RGBQUAD rgb=info.nBkgndColor; //<mpwolski>
	if ((pDib==NULL)||(x<0)||(y<0)||
		(x>=head.biWidth)||(y>=head.biHeight)){
		if (info.nBkgndIndex != -1){
			if (head.biBitCount<24) rgb = GetPaletteColor((BYTE)info.nBkgndIndex);
			else rgb = info.nBkgndColor;
		} else if (pDib) rgb = GetPixelColor(0,0);
		return rgb;
	}

	if (head.biClrUsed){
		rgb = GetPaletteColor(GetPixelIndex(x,y));
	} else {
		BYTE* iDst  = info.pImage + y*info.dwEffWidth + x*3;
		rgb.rgbBlue = *iDst++;
		rgb.rgbGreen= *iDst++;
		rgb.rgbRed  = *iDst;
	}
#if CXIMAGE_SUPPORT_ALPHA
	if (pAlpha) rgb.rgbReserved = AlphaGet(x,y);
#else
	rgb.rgbReserved = 0;
#endif //CXIMAGE_SUPPORT_ALPHA
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
BYTE CxImage::GetPixelGray(long x, long y)
{
	RGBQUAD color = GetPixelColor(x,y);
	return (BYTE)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPixelIndex(long x,long y,BYTE i)
{
	if ((pDib==NULL)||(head.biClrUsed==0)||
		(x<0)||(y<0)||(x>=head.biWidth)||(y>=head.biHeight)) return ;

	if (head.biBitCount==8){
		info.pImage[y*info.dwEffWidth + x]=i;
		return;
	} else {
		BYTE pos;
		BYTE* iDst= info.pImage + y*info.dwEffWidth + (x*head.biBitCount >> 3);
		if (head.biBitCount==4){
			pos = (BYTE)(4*(1-x%2));
			*iDst &= ~(0x0F<<pos);
			*iDst |= ((i & 0x0F)<<pos);
			return;
		} else if (head.biBitCount==1){
			pos = (BYTE)(7-x%8);
			*iDst &= ~(0x01<<pos);
			*iDst |= ((i & 0x01)<<pos);
			return;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPixelColor(long x,long y,COLORREF cr)
{
	SetPixelColor(x,y,RGBtoRGBQUAD(cr));
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPixelColor(long x,long y,RGBQUAD c, bool bEditAlpha)
{
	if ((pDib==NULL)||(x<0)||(y<0)||
		(x>=head.biWidth)||(y>=head.biHeight)) return;
	if (head.biClrUsed)
		SetPixelIndex(x,y,GetNearestIndex(c));
	else {
		BYTE* iDst = info.pImage + y*info.dwEffWidth + x*3;
		*iDst++ = c.rgbBlue;
		*iDst++ = c.rgbGreen;
		*iDst   = c.rgbRed;
#if CXIMAGE_SUPPORT_ALPHA
		if (bEditAlpha) AlphaSet(x,y,c.rgbReserved);
#endif //CXIMAGE_SUPPORT_ALPHA
	}
}
////////////////////////////////////////////////////////////////////////////////
BYTE CxImage::GetNearestIndex(RGBQUAD c)
{
	if ((pDib==NULL)||(head.biClrUsed==0)) return 0;

	// <RJ> check matching with the previous result
	if (info.last_c_isvalid && (*(long*)&info.last_c == *(long*)&c)) return info.last_c_index;
	info.last_c = c;
	info.last_c_isvalid = true;

	BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
	long distance=200000;
	int i,j = 0;
	long k,l;
	int m = (int)(head.biClrImportant==0 ? head.biClrUsed : head.biClrImportant);
	for(i=0,l=0;i<m;i++,l+=sizeof(RGBQUAD)){
		k = (iDst[l]-c.rgbBlue)*(iDst[l]-c.rgbBlue)+
			(iDst[l+1]-c.rgbGreen)*(iDst[l+1]-c.rgbGreen)+
			(iDst[l+2]-c.rgbRed)*(iDst[l+2]-c.rgbRed);
//		k = abs(iDst[l]-c.rgbBlue)+abs(iDst[l+1]-c.rgbGreen)+abs(iDst[l+2]-c.rgbRed);
		if (k==0){
			j=i;
			break;
		}
		if (k<distance){
			distance=k;
			j=i;
		}
	}
	info.last_c_index = j;
	return (BYTE)j;
}
////////////////////////////////////////////////////////////////////////////////
// swaps the blue and red components (for RGB images)
// > buffer: pointer to the pixels
// > lenght: number of bytes to swap. lenght may not exceed the scan line.
void CxImage::RGBtoBGR(BYTE *buffer, int length)
{
	if (buffer && (head.biClrUsed==0)){
		BYTE temp;
		length = min(length,(int)info.dwEffWidth);
		for (int i=0;i<length;i+=3){
			temp = buffer[i]; buffer[i] = buffer[i+2]; buffer[i+2] = temp;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoRGBQUAD(COLORREF cr)
{
	RGBQUAD c;
	c.rgbRed = GetRValue(cr);	/* get R, G, and B out of DWORD */
	c.rgbGreen = GetGValue(cr);
	c.rgbBlue = GetBValue(cr);
	c.rgbReserved=0;
	return c;
}
////////////////////////////////////////////////////////////////////////////////
COLORREF CxImage::RGBQUADtoRGB (RGBQUAD c)
{
	return RGB(c.rgbRed,c.rgbGreen,c.rgbBlue);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::GetPaletteColor(BYTE i, BYTE* r, BYTE* g, BYTE* b)
{
	RGBQUAD* ppal=GetPalette();
	if (ppal) {
		*r = ppal[i].rgbRed;
		*g = ppal[i].rgbGreen;
		*b = ppal[i].rgbBlue; 
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPalette(DWORD n, BYTE *r, BYTE *g, BYTE *b)
{
	if ((!r)||(pDib==NULL)||(head.biClrUsed==0)) return;
	if (!g) g = r;
	if (!b) b = g;
	RGBQUAD* ppal=GetPalette();
	DWORD m=min(n,head.biClrUsed);
	for (DWORD i=0; i<m;i++){
		ppal[i].rgbRed=r[i];
		ppal[i].rgbGreen=g[i];
		ppal[i].rgbBlue=b[i];
	}
	info.last_c_isvalid = false;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPalette(rgb_color *rgb,DWORD nColors)
{
	if ((!rgb)||(pDib==NULL)||(head.biClrUsed==0)) return;
	RGBQUAD* ppal=GetPalette();
	DWORD m=min(nColors,head.biClrUsed);
	for (DWORD i=0; i<m;i++){
		ppal[i].rgbRed=rgb[i].r;
		ppal[i].rgbGreen=rgb[i].g;
		ppal[i].rgbBlue=rgb[i].b;
	}
	info.last_c_isvalid = false;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetPalette(RGBQUAD* pPal,DWORD nColors)
{
	if ((pPal==NULL)||(pDib==NULL)||(head.biClrUsed==0)) return;
	memcpy(GetPalette(),pPal,min(GetPaletteSize(),nColors*sizeof(RGBQUAD)));
	info.last_c_isvalid = false;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetGrayPalette()
{
	if ((pDib==NULL)||(head.biClrUsed==0)) return;
	RGBQUAD* pal=GetPalette();
	for (DWORD ni=0;ni<head.biClrUsed;ni++)
		pal[ni].rgbBlue=pal[ni].rgbGreen = pal[ni].rgbRed = (BYTE)(ni*(255/(head.biClrUsed-1)));
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::BlendPalette(COLORREF cr,long perc)
{
	if ((pDib==NULL)||(head.biClrUsed==0)) return;
	BYTE* iDst = (BYTE*)(pDib) + sizeof(BITMAPINFOHEADER);
	DWORD i,r,g,b;
	RGBQUAD* pPal=(RGBQUAD*)iDst;
	r = GetRValue(cr);
	g = GetGValue(cr);
	b = GetBValue(cr);
	if (perc>100) perc=100;
	for(i=0;i<head.biClrUsed;i++){
		pPal[i].rgbBlue=(BYTE)((pPal[i].rgbBlue*(100-perc)+b*perc)/100);
		pPal[i].rgbGreen =(BYTE)((pPal[i].rgbGreen*(100-perc)+g*perc)/100);
		pPal[i].rgbRed =(BYTE)((pPal[i].rgbRed*(100-perc)+r*perc)/100);
	}
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::IsGrayScale()
{
	RGBQUAD* ppal=GetPalette();
	if(!(pDib && ppal && head.biClrUsed)) return false;
	for(DWORD i=0;i<head.biClrUsed;i++){
		if (ppal[i].rgbBlue!=i || ppal[i].rgbGreen!=i || ppal[i].rgbRed!=i) return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
// swap two indexes in the image and their colors in the palette
void CxImage::SwapIndex(BYTE idx1, BYTE idx2)
{
	RGBQUAD* ppal=GetPalette();
	if(!(pDib && ppal)) return;
	//swap the colors
	RGBQUAD tempRGB=GetPaletteColor(idx1);
	SetPaletteColor(idx1,GetPaletteColor(idx2));
	SetPaletteColor(idx2,tempRGB);
	//swap the pixels
	BYTE idx;
	for(long y=0; y < head.biHeight; y++){
		for(long x=0; x <= head.biWidth; x++){
			idx=GetPixelIndex(x,y);
			if (idx==idx1) SetPixelIndex(x,y,idx2);
			if (idx==idx2) SetPixelIndex(x,y,idx1);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::IsTransparent(long x, long y)
{
	if (!pDib) return false;

	if (info.nBkgndIndex>=0){
		if (head.biClrUsed){
			if (GetPixelIndex(x,y) == info.nBkgndIndex) return true;
		} else {
			RGBQUAD ct = info.nBkgndColor;
			RGBQUAD c = GetPixelColor(x,y);
			c.rgbReserved = 0;
			if (*(long*)&c==*(long*)&ct) return true;
		}
	}

#if CXIMAGE_SUPPORT_ALPHA
	if (!pAlpha) return AlphaGet(x,y)==0;
#endif

	return false;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetClrImportant(DWORD ncolors)
{
	if (ncolors==0 || ncolors>256) {
		head.biClrImportant = 0;
		return;
	}

	switch(head.biBitCount){
	case 1:
		head.biClrImportant = min(ncolors,2);
		break;
	case 4:
		head.biClrImportant = min(ncolors,16);
		break;
	case 8:
		head.biClrImportant = ncolors;
		break;
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::DrawLine(int StartX, int EndX, int StartY, int EndY, COLORREF cr)
{
	DrawLine(StartX, EndX, StartY, EndY, RGBtoRGBQUAD(cr));
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::DrawLine(int StartX, int EndX, int StartY, int EndY, RGBQUAD color, bool bEditAlpha)
{
	if (!pDib) return;
	//////////////////////////////////////////////////////
	// Draws a line using the Bresenham line algorithm
	// Thanks to Jordan DeLozier <JDL>
	//////////////////////////////////////////////////////
	int x1 = StartX;
	int y1 = StartY;
	int x = x1;                       // Start x off at the first pixel
	int y = y1;                       // Start y off at the first pixel
	int x2 = EndX;
	int y2 = EndY;

	int xinc1,xinc2,yinc1,yinc2;      // Increasing values
	int den, num, numadd,numpixels;   
	int deltax = abs(x2 - x1);        // The difference between the x's
	int deltay = abs(y2 - y1);        // The difference between the y's

	// Get Increasing Values
	if (x2 >= x1) {                // The x-values are increasing
		xinc1 = 1;
		xinc2 = 1;
	} else {                         // The x-values are decreasing
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {                // The y-values are increasing
		yinc1 = 1;
		yinc2 = 1;
	} else {                         // The y-values are decreasing
		yinc1 = -1;
		yinc2 = -1;
	}

	// Actually draw the line
	if (deltax >= deltay)         // There is at least one x-value for every y-value
	{
		xinc1 = 0;                  // Don't change the x when numerator >= denominator
		yinc2 = 0;                  // Don't change the y for every iteration
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;         // There are more x-values than y-values
	}
	else                          // There is at least one y-value for every x-value
	{
		xinc2 = 0;                  // Don't change the x for every iteration
		yinc1 = 0;                  // Don't change the y when numerator >= denominator
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;         // There are more y-values than x-values
	}
	
	for (int curpixel = 0; curpixel <= numpixels; curpixel++)
	{
		// Draw the current pixel
		SetPixelColor(x,y,color,bEditAlpha);
		
		num += numadd;              // Increase the numerator by the top of the fraction
		if (num >= den)             // Check if numerator >= denominator
		{
			num -= den;               // Calculate the new numerator value
			x += xinc1;               // Change the x as appropriate
			y += yinc1;               // Change the y as appropriate
		}
		x += xinc2;                 // Change the x as appropriate
		y += yinc2;                 // Change the y as appropriate
	}
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetStdPalette()
{
	if (!pDib) return;
	switch (head.biBitCount){
	case 8:
		{
			const BYTE pal256[1024] = {0,0,0,0,0,0,128,0,0,128,0,0,0,128,128,0,128,0,0,0,128,0,128,0,128,128,0,0,192,192,192,0,
			192,220,192,0,240,202,166,0,212,240,255,0,177,226,255,0,142,212,255,0,107,198,255,0,
			72,184,255,0,37,170,255,0,0,170,255,0,0,146,220,0,0,122,185,0,0,98,150,0,0,74,115,0,0,
			50,80,0,212,227,255,0,177,199,255,0,142,171,255,0,107,143,255,0,72,115,255,0,37,87,255,0,0,
			85,255,0,0,73,220,0,0,61,185,0,0,49,150,0,0,37,115,0,0,25,80,0,212,212,255,0,177,177,255,0,
			142,142,255,0,107,107,255,0,72,72,255,0,37,37,255,0,0,0,254,0,0,0,220,0,0,0,185,0,0,0,150,0,
			0,0,115,0,0,0,80,0,227,212,255,0,199,177,255,0,171,142,255,0,143,107,255,0,115,72,255,0,
			87,37,255,0,85,0,255,0,73,0,220,0,61,0,185,0,49,0,150,0,37,0,115,0,25,0,80,0,240,212,255,0,
			226,177,255,0,212,142,255,0,198,107,255,0,184,72,255,0,170,37,255,0,170,0,255,0,146,0,220,0,
			122,0,185,0,98,0,150,0,74,0,115,0,50,0,80,0,255,212,255,0,255,177,255,0,255,142,255,0,255,107,255,0,
			255,72,255,0,255,37,255,0,254,0,254,0,220,0,220,0,185,0,185,0,150,0,150,0,115,0,115,0,80,0,80,0,
			255,212,240,0,255,177,226,0,255,142,212,0,255,107,198,0,255,72,184,0,255,37,170,0,255,0,170,0,
			220,0,146,0,185,0,122,0,150,0,98,0,115,0,74,0,80,0,50,0,255,212,227,0,255,177,199,0,255,142,171,0,
			255,107,143,0,255,72,115,0,255,37,87,0,255,0,85,0,220,0,73,0,185,0,61,0,150,0,49,0,115,0,37,0,
			80,0,25,0,255,212,212,0,255,177,177,0,255,142,142,0,255,107,107,0,255,72,72,0,255,37,37,0,254,0,
			0,0,220,0,0,0,185,0,0,0,150,0,0,0,115,0,0,0,80,0,0,0,255,227,212,0,255,199,177,0,255,171,142,0,
			255,143,107,0,255,115,72,0,255,87,37,0,255,85,0,0,220,73,0,0,185,61,0,0,150,49,0,0,115,37,0,
			0,80,25,0,0,255,240,212,0,255,226,177,0,255,212,142,0,255,198,107,0,255,184,72,0,255,170,37,0,
			255,170,0,0,220,146,0,0,185,122,0,0,150,98,0,0,115,74,0,0,80,50,0,0,255,255,212,0,255,255,177,0,
			255,255,142,0,255,255,107,0,255,255,72,0,255,255,37,0,254,254,0,0,220,220,0,0,185,185,0,0,150,150,0,
			0,115,115,0,0,80,80,0,0,240,255,212,0,226,255,177,0,212,255,142,0,198,255,107,0,184,255,72,0,
			170,255,37,0,170,255,0,0,146,220,0,0,122,185,0,0,98,150,0,0,74,115,0,0,50,80,0,0,227,255,212,0,
			199,255,177,0,171,255,142,0,143,255,107,0,115,255,72,0,87,255,37,0,85,255,0,0,73,220,0,0,61,185,0,
			0,49,150,0,0,37,115,0,0,25,80,0,0,212,255,212,0,177,255,177,0,142,255,142,0,107,255,107,0,72,255,72,0,
			37,255,37,0,0,254,0,0,0,220,0,0,0,185,0,0,0,150,0,0,0,115,0,0,0,80,0,0,212,255,227,0,177,255,199,0,
			142,255,171,0,107,255,143,0,72,255,115,0,37,255,87,0,0,255,85,0,0,220,73,0,0,185,61,0,0,150,49,0,0,
			115,37,0,0,80,25,0,212,255,240,0,177,255,226,0,142,255,212,0,107,255,198,0,72,255,184,0,37,255,170,0,
			0,255,170,0,0,220,146,0,0,185,122,0,0,150,98,0,0,115,74,0,0,80,50,0,212,255,255,0,177,255,255,0,
			142,255,255,0,107,255,255,0,72,255,255,0,37,255,255,0,0,254,254,0,0,220,220,0,0,185,185,0,0,
			150,150,0,0,115,115,0,0,80,80,0,242,242,242,0,230,230,230,0,218,218,218,0,206,206,206,0,194,194,194,0,
			182,182,182,0,170,170,170,0,158,158,158,0,146,146,146,0,134,134,134,0,122,122,122,0,110,110,110,0,
			98,98,98,0,86,86,86,0,74,74,74,0,62,62,62,0,50,50,50,0,38,38,38,0,26,26,26,0,14,14,14,0,240,251,255,0,
			164,160,160,0,128,128,128,0,0,0,255,0,0,255,0,0,0,255,255,0,255,0,0,0,255,0,255,0,255,255,0,0,255,255,255,0};
			memcpy(GetPalette(),pal256,1024);
			break;
		}
	case 4:
		{
			const BYTE pal16[64]={0,0,0,0,0,0,128,0,0,128,0,0,0,128,128,0,128,0,0,0,128,0,128,0,128,128,0,0,192,192,192,0,
								128,128,128,0,0,0,255,0,0,255,0,0,0,255,255,0,255,0,0,0,255,0,255,0,255,255,0,0,255,255,255,0};
			memcpy(GetPalette(),pal16,64);
			break;
		}
	}
	return;
}
////////////////////////////////////////////////////////////////////////////////
