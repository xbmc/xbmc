// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
// xImaDsp.cpp : DSP functions
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximage.h"

#if CXIMAGE_SUPPORT_DSP

////////////////////////////////////////////////////////////////////////////////
bool CxImage::Threshold(BYTE level)
{
	if (!pDib) return false;
	if (head.biBitCount == 1) return true;

	GrayScale();

	CxImage tmp(head.biWidth,head.biHeight,1);

	for (long y=0;y<head.biHeight;y++){
		info.nProgress = (long)(100*y/head.biHeight);
		if (info.nEscape) break;
		for (long x=0;x<head.biWidth;x++){
			if (GetPixelIndex(x,y)>level)
				tmp.SetPixelIndex(x,y,1);
			else
				tmp.SetPixelIndex(x,y,0);
		}
	}
	tmp.SetPaletteColor(0,0,0,0);
	tmp.SetPaletteColor(1,255,255,255);
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SplitRGB(CxImage* r,CxImage* g,CxImage* b)
{
	if (!pDib) return false;
	if (r==NULL && g==NULL && b==NULL) return false;

	CxImage tmpr(head.biWidth,head.biHeight,8);
	CxImage tmpg(head.biWidth,head.biHeight,8);
	CxImage tmpb(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(long y=0; y<head.biHeight; y++){
		for(long x=0; x<head.biWidth; x++){
			color = GetPixelColor(x,y);
			if (r) tmpr.SetPixelIndex(x,y,color.rgbRed);
			if (g) tmpg.SetPixelIndex(x,y,color.rgbGreen);
			if (b) tmpb.SetPixelIndex(x,y,color.rgbBlue);
		}
	}

	if (r) tmpr.SetGrayPalette();
	if (g) tmpg.SetGrayPalette();
	if (b) tmpb.SetGrayPalette();

	/*for(long j=0; j<256; j++){
		BYTE i=(BYTE)j;
		if (r) tmpr.SetPaletteColor(i,i,0,0);
		if (g) tmpg.SetPaletteColor(i,0,i,0);
		if (b) tmpb.SetPaletteColor(i,0,0,i);
	}*/

	if (r) r->Transfer(tmpr);
	if (g) g->Transfer(tmpg);
	if (b) b->Transfer(tmpb);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SplitCMYK(CxImage* c,CxImage* m,CxImage* y,CxImage* k)
{
	if (!pDib) return false;
	if (c==NULL && m==NULL && y==NULL && k==NULL) return false;

	CxImage tmpc(head.biWidth,head.biHeight,8);
	CxImage tmpm(head.biWidth,head.biHeight,8);
	CxImage tmpy(head.biWidth,head.biHeight,8);
	CxImage tmpk(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(long yy=0; yy<head.biHeight; yy++){
		for(long xx=0; xx<head.biWidth; xx++){
			color = GetPixelColor(xx,yy);
			if (c) tmpc.SetPixelIndex(xx,yy,(BYTE)(255-color.rgbRed));
			if (m) tmpm.SetPixelIndex(xx,yy,(BYTE)(255-color.rgbGreen));
			if (y) tmpy.SetPixelIndex(xx,yy,(BYTE)(255-color.rgbBlue));
			if (k) tmpk.SetPixelIndex(xx,yy,(BYTE)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue));
		}
	}

	if (c) tmpc.SetGrayPalette();
	if (m) tmpm.SetGrayPalette();
	if (y) tmpy.SetGrayPalette();
	if (k) tmpk.SetGrayPalette();

	if (c) c->Transfer(tmpc);
	if (m) m->Transfer(tmpm);
	if (y) y->Transfer(tmpy);
	if (k) k->Transfer(tmpk);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SplitYUV(CxImage* y,CxImage* u,CxImage* v)
{
	if (!pDib) return false;
	if (y==NULL && u==NULL && v==NULL) return false;

	CxImage tmpy(head.biWidth,head.biHeight,8);
	CxImage tmpu(head.biWidth,head.biHeight,8);
	CxImage tmpv(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(long yy=0; yy<head.biHeight; yy++){
		for(long x=0; x<head.biWidth; x++){
			color = RGBtoYUV(GetPixelColor(x,yy));
			if (y) tmpy.SetPixelIndex(x,yy,color.rgbRed);
			if (u) tmpu.SetPixelIndex(x,yy,color.rgbGreen);
			if (v) tmpv.SetPixelIndex(x,yy,color.rgbBlue);
		}
	}

	if (y) tmpy.SetGrayPalette();
	if (u) tmpu.SetGrayPalette();
	if (v) tmpv.SetGrayPalette();

	if (y) y->Transfer(tmpy);
	if (u) u->Transfer(tmpu);
	if (v) v->Transfer(tmpv);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SplitYIQ(CxImage* y,CxImage* i,CxImage* q)
{
	if (!pDib) return false;
	if (y==NULL && i==NULL && q==NULL) return false;

	CxImage tmpy(head.biWidth,head.biHeight,8);
	CxImage tmpi(head.biWidth,head.biHeight,8);
	CxImage tmpq(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(long yy=0; yy<head.biHeight; yy++){
		for(long x=0; x<head.biWidth; x++){
			color = RGBtoYIQ(GetPixelColor(x,yy));
			if (y) tmpy.SetPixelIndex(x,yy,color.rgbRed);
			if (i) tmpi.SetPixelIndex(x,yy,color.rgbGreen);
			if (q) tmpq.SetPixelIndex(x,yy,color.rgbBlue);
		}
	}

	if (y) tmpy.SetGrayPalette();
	if (i) tmpi.SetGrayPalette();
	if (q) tmpq.SetGrayPalette();

	if (y) y->Transfer(tmpy);
	if (i) i->Transfer(tmpi);
	if (q) q->Transfer(tmpq);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::SplitXYZ(CxImage* x,CxImage* y,CxImage* z)
{
	if (!pDib) return false;
	if (x==NULL && y==NULL && z==NULL) return false;

	CxImage tmpx(head.biWidth,head.biHeight,8);
	CxImage tmpy(head.biWidth,head.biHeight,8);
	CxImage tmpz(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(long yy=0; yy<head.biHeight; yy++){
		for(long xx=0; xx<head.biWidth; xx++){
			color = RGBtoXYZ(GetPixelColor(xx,yy));
			if (x) tmpx.SetPixelIndex(xx,yy,color.rgbRed);
			if (y) tmpy.SetPixelIndex(xx,yy,color.rgbGreen);
			if (z) tmpz.SetPixelIndex(xx,yy,color.rgbBlue);
		}
	}

	if (x) tmpx.SetGrayPalette();
	if (y) tmpy.SetGrayPalette();
	if (z) tmpz.SetGrayPalette();

	if (x) x->Transfer(tmpx);
	if (y) y->Transfer(tmpy);
	if (z) z->Transfer(tmpz);

	return true;
}////////////////////////////////////////////////////////////////////////////////
bool CxImage::SplitHSL(CxImage* h,CxImage* s,CxImage* l)
{
	if (!pDib) return false;
	if (h==NULL && s==NULL && l==NULL) return false;

	CxImage tmph(head.biWidth,head.biHeight,8);
	CxImage tmps(head.biWidth,head.biHeight,8);
	CxImage tmpl(head.biWidth,head.biHeight,8);

	RGBQUAD color;
	for(long y=0; y<head.biHeight; y++){
		for(long x=0; x<head.biWidth; x++){
			color = RGBtoHSL(GetPixelColor(x,y));
			if (h) tmph.SetPixelIndex(x,y,color.rgbRed);
			if (s) tmps.SetPixelIndex(x,y,color.rgbGreen);
			if (l) tmpl.SetPixelIndex(x,y,color.rgbBlue);
		}
	}

	if (h) tmph.SetGrayPalette();
	if (s) tmps.SetGrayPalette();
	if (l) tmpl.SetGrayPalette();

	/* pseudo-color generator for hue channel (visual debug)
	if (h) for(long j=0; j<256; j++){
		BYTE i=(BYTE)j;
		RGBQUAD hsl={120,240,i,0};
		tmph.SetPaletteColor(i,HSLtoRGB(hsl));
	}*/

	if (h) h->Transfer(tmph);
	if (s) s->Transfer(tmps);
	if (l) l->Transfer(tmpl);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
#define  HSLMAX   255	/* H,L, and S vary over 0-HSLMAX */
#define  RGBMAX   255   /* R,G, and B vary over 0-RGBMAX */
                        /* HSLMAX BEST IF DIVISIBLE BY 6 */
                        /* RGBMAX, HSLMAX must each fit in a BYTE. */
/* Hue is undefined if Saturation is 0 (grey-scale) */
/* This value determines where the Hue scrollbar is */
/* initially set for achromatic colors */
#define UNDEFINED (HSLMAX*2/3)
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoHSL(RGBQUAD lRGBColor)
{
	BYTE R,G,B;					/* input RGB values */
	BYTE H,L,S;					/* output HSL values */
	BYTE cMax,cMin;				/* max and min RGB values */
	WORD Rdelta,Gdelta,Bdelta;	/* intermediate value: % of spread from max*/

	R = lRGBColor.rgbRed;	/* get R, G, and B out of DWORD */
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;

	cMax = max( max(R,G), B);	/* calculate lightness */
	cMin = min( min(R,G), B);
	L = (BYTE)((((cMax+cMin)*HSLMAX)+RGBMAX)/(2*RGBMAX));

	if (cMax==cMin){			/* r=g=b --> achromatic case */
		S = 0;					/* saturation */
		H = UNDEFINED;			/* hue */
	} else {					/* chromatic case */
		if (L <= (HSLMAX/2))	/* saturation */
			S = (BYTE)((((cMax-cMin)*HSLMAX)+((cMax+cMin)/2))/(cMax+cMin));
		else
			S = (BYTE)((((cMax-cMin)*HSLMAX)+((2*RGBMAX-cMax-cMin)/2))/(2*RGBMAX-cMax-cMin));
		/* hue */
		Rdelta = (WORD)((((cMax-R)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));
		Gdelta = (WORD)((((cMax-G)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));
		Bdelta = (WORD)((((cMax-B)*(HSLMAX/6)) + ((cMax-cMin)/2) ) / (cMax-cMin));

		if (R == cMax)
			H = (BYTE)(Bdelta - Gdelta);
		else if (G == cMax)
			H = (BYTE)((HSLMAX/3) + Rdelta - Bdelta);
		else /* B == cMax */
			H = (BYTE)(((2*HSLMAX)/3) + Gdelta - Rdelta);

//		if (H < 0) H += HSLMAX;     //always false
		if (H > HSLMAX) H -= HSLMAX;
	}
	RGBQUAD hsl={L,S,H,0};
	return hsl;
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::HueToRGB(float n1,float n2, float hue)
{
	//<F. Livraghi> fixed implementation for HSL2RGB routine
	float rValue;

	if (hue > 360)
		hue = hue - 360;
	else if (hue < 0)
		hue = hue + 360;

	if (hue < 60)
		rValue = n1 + (n2-n1)*hue/60.0f;
	else if (hue < 180)
		rValue = n2;
	else if (hue < 240)
		rValue = n1+(n2-n1)*(240-hue)/60;
	else
		rValue = n1;

	return rValue;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::HSLtoRGB(COLORREF cHSLColor)
{
	return HSLtoRGB(RGBtoRGBQUAD(cHSLColor));
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::HSLtoRGB(RGBQUAD lHSLColor)
{ 
	//<F. Livraghi> fixed implementation for HSL2RGB routine
	float h,s,l;
	float m1,m2;
	BYTE r,g,b;

	h = (float)lHSLColor.rgbRed * 360.0f/255.0f;
	s = (float)lHSLColor.rgbGreen/255.0f;
	l = (float)lHSLColor.rgbBlue/255.0f;

	if (l <= 0.5)	m2 = l * (1+s);
	else			m2 = l + s - l*s;

	m1 = 2 * l - m2;

	if (s == 0) {
		r=g=b=(BYTE)(l*255.0f);
	} else {
		r = (BYTE)(HueToRGB(m1,m2,h+120) * 255.0f);
		g = (BYTE)(HueToRGB(m1,m2,h) * 255.0f);
		b = (BYTE)(HueToRGB(m1,m2,h-120) * 255.0f);
	}

	RGBQUAD rgb = {b,g,r,0};
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::YUVtoRGB(RGBQUAD lYUVColor)
{
	int U,V,R,G,B;
	float Y = lYUVColor.rgbRed;
	U = lYUVColor.rgbGreen - 128;
	V = lYUVColor.rgbBlue - 128;

//	R = (int)(1.164 * Y + 2.018 * U);
//	G = (int)(1.164 * Y - 0.813 * V - 0.391 * U);
//	B = (int)(1.164 * Y + 1.596 * V);
	R = (int)( Y + 1.403f * V);
	G = (int)( Y - 0.344f * U - 0.714f * V);
	B = (int)( Y + 1.770f * U);

	R= min(255,max(0,R));
	G= min(255,max(0,G));
	B= min(255,max(0,B));
	RGBQUAD rgb={(BYTE)B,(BYTE)G,(BYTE)R,0};
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoYUV(RGBQUAD lRGBColor)
{
	int Y,U,V,R,G,B;
	R = lRGBColor.rgbRed;
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;

//	Y = (int)( 0.257 * R + 0.504 * G + 0.098 * B);
//	U = (int)( 0.439 * R - 0.368 * G - 0.071 * B + 128);
//	V = (int)(-0.148 * R - 0.291 * G + 0.439 * B + 128);
	Y = (int)(0.299f * R + 0.587f * G + 0.114f * B);
	U = (int)((B-Y) * 0.565f + 128);
	V = (int)((R-Y) * 0.713f + 128);

	Y= min(255,max(0,Y));
	U= min(255,max(0,U));
	V= min(255,max(0,V));
	RGBQUAD yuv={(BYTE)V,(BYTE)U,(BYTE)Y,0};
	return yuv;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::YIQtoRGB(RGBQUAD lYIQColor)
{
	int I,Q,R,G,B;
	float Y = lYIQColor.rgbRed;
	I = lYIQColor.rgbGreen - 128;
	Q = lYIQColor.rgbBlue - 128;

	R = (int)( Y + 0.956f * I + 0.621f * Q);
	G = (int)( Y - 0.273f * I - 0.647f * Q);
	B = (int)( Y - 1.104f * I + 1.701f * Q);

	R= min(255,max(0,R));
	G= min(255,max(0,G));
	B= min(255,max(0,B));
	RGBQUAD rgb={(BYTE)B,(BYTE)G,(BYTE)R,0};
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoYIQ(RGBQUAD lRGBColor)
{
	int Y,I,Q,R,G,B;
	R = lRGBColor.rgbRed;
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;

	Y = (int)( 0.2992f * R + 0.5868f * G + 0.1140f * B);
	I = (int)( 0.5960f * R - 0.2742f * G - 0.3219f * B + 128);
	Q = (int)( 0.2109f * R - 0.5229f * G + 0.3120f * B + 128);

	Y= min(255,max(0,Y));
	I= min(255,max(0,I));
	Q= min(255,max(0,Q));
	RGBQUAD yiq={(BYTE)Q,(BYTE)I,(BYTE)Y,0};
	return yiq;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::XYZtoRGB(RGBQUAD lXYZColor)
{
	int X,Y,Z,R,G,B;
	X = lXYZColor.rgbRed;
	Y = lXYZColor.rgbGreen;
	Z = lXYZColor.rgbBlue;
	double k=1.088751;

	R = (int)(  3.240479f * X - 1.537150f * Y - 0.498535f * Z * k);
	G = (int)( -0.969256f * X + 1.875992f * Y + 0.041556f * Z * k);
	B = (int)(  0.055648f * X - 0.204043f * Y + 1.057311f * Z * k);

	R= min(255,max(0,R));
	G= min(255,max(0,G));
	B= min(255,max(0,B));
	RGBQUAD rgb={(BYTE)B,(BYTE)G,(BYTE)R,0};
	return rgb;
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD CxImage::RGBtoXYZ(RGBQUAD lRGBColor)
{
	int X,Y,Z,R,G,B;
	R = lRGBColor.rgbRed;
	G = lRGBColor.rgbGreen;
	B = lRGBColor.rgbBlue;

	X = (int)( 0.412453f * R + 0.357580f * G + 0.180423f * B);
	Y = (int)( 0.212671f * R + 0.715160f * G + 0.072169f * B);
	Z = (int)((0.019334f * R + 0.119193f * G + 0.950227f * B)*0.918483657f);

	//X= min(255,max(0,X));
	//Y= min(255,max(0,Y));
	//Z= min(255,max(0,Z));
	RGBQUAD xyz={(BYTE)Z,(BYTE)Y,(BYTE)X,0};
	return xyz;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::HuePalette(float correction)
{
	if (head.biClrUsed==0) return;

	for(DWORD j=0; j<head.biClrUsed; j++){
		BYTE i=(BYTE)(j*correction*(255/(head.biClrUsed-1)));
		RGBQUAD hsl={120,240,i,0};
		SetPaletteColor((BYTE)j,HSLtoRGB(hsl));
	}
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Colorize(BYTE hue, BYTE sat)
{
	if (!pDib) return false;

	RGBQUAD color;
	if (head.biClrUsed==0){

		long xmin,xmax,ymin,ymax;
		if (pSelection){
			xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
			ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
		} else {
			xmin = ymin = 0;
			xmax = head.biWidth; ymax=head.biHeight;
		}

		for(long y=ymin; y<ymax; y++){
			for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					color = RGBtoHSL(GetPixelColor(x,y));
					color.rgbRed=hue;
					color.rgbGreen=sat;
					SetPixelColor(x,y,HSLtoRGB(color));
				}
			}
		}
	} else {
		for(DWORD j=0; j<head.biClrUsed; j++){
			color = RGBtoHSL(GetPaletteColor((BYTE)j));
			color.rgbRed=hue;
			color.rgbGreen=sat;
			SetPaletteColor((BYTE)j,HSLtoRGB(color));
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Light(long brightness, long contrast)
{
	if (!pDib) return false;
	RGBQUAD color;
	float c=(100 + contrast)/100.0f;
	brightness+=128;

	if (head.biClrUsed==0){

		long xmin,xmax,ymin,ymax;
		if (pSelection){
			xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
			ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
		} else {
			//xmin = ymin = 0;
			//xmax = head.biWidth; ymax=head.biHeight;
			// faster loop for full image
			BYTE *iSrc=info.pImage;
			for(unsigned long i=0; i < head.biSizeImage ; i++){
				*iSrc=(BYTE)max(0,min(255,(int)((*(iSrc)-128)*c + brightness)));
				iSrc++;
			}
			return true;
		}

		for(long y=ymin; y<ymax; y++){
			info.nProgress = (long)(100*y/ymax); //<Anatoly Ivasyuk>
			for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					color = GetPixelColor(x,y);
					color.rgbRed = (BYTE)max(0,min(255,(int)((color.rgbRed-128)*c + brightness)));
					color.rgbGreen = (BYTE)max(0,min(255,(int)((color.rgbGreen-128)*c + brightness)));
					color.rgbBlue = (BYTE)max(0,min(255,(int)((color.rgbBlue-128)*c + brightness)));
					SetPixelColor(x,y,color);
				}
			}
		}
	} else {
		for(DWORD j=0; j<head.biClrUsed; j++){
			color = GetPaletteColor((BYTE)j);
			color.rgbRed = (BYTE)max(0,min(255,(int)((color.rgbRed-128)*c + brightness)));
			color.rgbGreen = (BYTE)max(0,min(255,(int)((color.rgbGreen-128)*c + brightness)));
			color.rgbBlue = (BYTE)max(0,min(255,(int)((color.rgbBlue-128)*c + brightness)));
			SetPaletteColor((BYTE)j,color);
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::Mean()
{
	if (!pDib) return 0;
	CxImage tmp(*this,true);
	tmp.GrayScale();
	float sum=0;

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}
	if (xmin==xmax || ymin==ymax) return (float)0.0;

	BYTE *iSrc=tmp.info.pImage;
	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*y/ymax); //<Anatoly Ivasyuk>
		for(long x=xmin; x<xmax; x++){
			sum+=iSrc[x];
		}
		iSrc+=tmp.info.dwEffWidth;
	}
	return sum/(xmax-xmin)/(ymax-ymin);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Filter(long* kernel, long Ksize, long Kfactor, long Koffset)
{
	if (!pDib) return false;

	long k2 = Ksize/2;
	long kmax= Ksize-k2;
	long r,g,b,i;
	RGBQUAD c;
	CxImage tmp(*this,pSelection!=0,true,true);

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*y/head.biHeight);
		if (info.nEscape) break;
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
				r=b=g=0;
				for(long j=-k2;j<kmax;j++){
					for(long k=-k2;k<kmax;k++){
						c=GetPixelColor(x+j,y+k);
						i=kernel[(j+k2)+Ksize*(k+k2)];
						r += c.rgbRed * i;
						g += c.rgbGreen * i;
						b += c.rgbBlue * i;
					}
				}
				if (Kfactor==0){
					c.rgbRed   = (BYTE)min(255, max(0,(int)(r + Koffset)));
					c.rgbGreen = (BYTE)min(255, max(0,(int)(g + Koffset)));
					c.rgbBlue  = (BYTE)min(255, max(0,(int)(b + Koffset)));
				} else {
					c.rgbRed   = (BYTE)min(255, max(0,(int)(r/Kfactor + Koffset)));
					c.rgbGreen = (BYTE)min(255, max(0,(int)(g/Kfactor + Koffset)));
					c.rgbBlue  = (BYTE)min(255, max(0,(int)(b/Kfactor + Koffset)));
				}
				tmp.SetPixelColor(x,y,c);
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Erode(long Ksize)
{
	if (!pDib) return false;

	long k2 = Ksize/2;
	long kmax= Ksize-k2;
	BYTE r,g,b;
	RGBQUAD c;
	CxImage tmp(*this,pSelection!=0,true,true);

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*y/head.biHeight);
		if (info.nEscape) break;
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				r=b=g=255;
				for(long j=-k2;j<kmax;j++){
					for(long k=-k2;k<kmax;k++){
						c=GetPixelColor(x+j,y+k);
						if (c.rgbRed < r) r=c.rgbRed;
						if (c.rgbGreen < g) g=c.rgbGreen;
						if (c.rgbBlue < b) b=c.rgbBlue;
					}
				}
				c.rgbRed   = r;
				c.rgbGreen = g;
				c.rgbBlue  = b;
				tmp.SetPixelColor(x,y,c);
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Dilate(long Ksize)
{
	if (!pDib) return false;

	long k2 = Ksize/2;
	long kmax= Ksize-k2;
	BYTE r,g,b;
	RGBQUAD c;
	CxImage tmp(*this,pSelection!=0,true,true);

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*y/head.biHeight);
		if (info.nEscape) break;
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				r=b=g=0;
				for(long j=-k2;j<kmax;j++){
					for(long k=-k2;k<kmax;k++){
						c=GetPixelColor(x+j,y+k);
						if (c.rgbRed > r) r=c.rgbRed;
						if (c.rgbGreen > g) g=c.rgbGreen;
						if (c.rgbBlue > b) b=c.rgbBlue;
					}
				}
				c.rgbRed   = r;
				c.rgbGreen = g;
				c.rgbBlue  = b;
				tmp.SetPixelColor(x,y,c);
			}
		}
	}
	Transfer(tmp);
	return true;

}
////////////////////////////////////////////////////////////////////////////////
// thanks to Mwolski <mpwolski(at)hotmail(dot)com>
void CxImage::Mix(CxImage & imgsrc2, ImageOpType op, long lXOffset, long lYOffset)
{
    long lWide = min(GetWidth(),imgsrc2.GetWidth()-lXOffset);
    long lHeight = min(GetHeight(),imgsrc2.GetHeight()-lYOffset);

    RGBQUAD rgbBackgrnd = GetTransColor();
    RGBQUAD rgb1, rgb2, rgbDest;

    for(long lY=0;lY<lHeight;lY++)
    {
		info.nProgress = (long)(100*lY/head.biHeight);
		if (info.nEscape) break;

        for(long lX=0;lX<lWide;lX++)
        {
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(lX,lY) && imgsrc2.SelectionIsInside(lX+lXOffset,lY+lYOffset))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				rgb1 = GetPixelColor(lX,lY);
				rgb2 = imgsrc2.GetPixelColor(lX+lXOffset,lY+lYOffset);
				switch(op)
				{
					case OpAdd:
						rgbDest.rgbBlue = (BYTE)max(0,min(255,rgb1.rgbBlue+rgb2.rgbBlue));
						rgbDest.rgbGreen = (BYTE)max(0,min(255,rgb1.rgbGreen+rgb2.rgbGreen));
						rgbDest.rgbRed = (BYTE)max(0,min(255,rgb1.rgbRed+rgb2.rgbRed));
					break;
					case OpSub:
						rgbDest.rgbBlue = (BYTE)max(0,min(255,rgb1.rgbBlue-rgb2.rgbBlue));
						rgbDest.rgbGreen = (BYTE)max(0,min(255,rgb1.rgbGreen-rgb2.rgbGreen));
						rgbDest.rgbRed = (BYTE)max(0,min(255,rgb1.rgbRed-rgb2.rgbRed));
					break;
					case OpAnd:
						rgbDest.rgbBlue = (BYTE)(rgb1.rgbBlue&rgb2.rgbBlue);
						rgbDest.rgbGreen = (BYTE)(rgb1.rgbGreen&rgb2.rgbGreen);
						rgbDest.rgbRed = (BYTE)(rgb1.rgbRed&rgb2.rgbRed);
					break;
					case OpXor:
						rgbDest.rgbBlue = (BYTE)(rgb1.rgbBlue^rgb2.rgbBlue);
						rgbDest.rgbGreen = (BYTE)(rgb1.rgbGreen^rgb2.rgbGreen);
						rgbDest.rgbRed = (BYTE)(rgb1.rgbRed^rgb2.rgbRed);
					break;
					case OpOr:
						rgbDest.rgbBlue = (BYTE)(rgb1.rgbBlue|rgb2.rgbBlue);
						rgbDest.rgbGreen = (BYTE)(rgb1.rgbGreen|rgb2.rgbGreen);
						rgbDest.rgbRed = (BYTE)(rgb1.rgbRed|rgb2.rgbRed);
					break;
					case OpMask:
						if(rgb2.rgbBlue==0 && rgb2.rgbGreen==0 && rgb2.rgbRed==0)
							rgbDest = rgbBackgrnd;
						else
							rgbDest = rgb1;
						break;
					case OpSrcCopy:
						if(memcmp(&rgb1,&rgbBackgrnd,sizeof(RGBQUAD))==0)
							rgbDest = rgb2;
						else // copy straight over
							rgbDest = rgb1;
						break;
					case OpDstCopy:
						if(memcmp(&rgb2,&rgbBackgrnd,sizeof(RGBQUAD))==0)
							rgbDest = rgb1;
						else // copy straight over
							rgbDest = rgb2;
						break;
					case OpSrcBlend:
						if(memcmp(&rgb1,&rgbBackgrnd,sizeof(RGBQUAD))==0)
							rgbDest = rgb2;
						else
						{
							long lBDiff = abs(rgb1.rgbBlue - rgbBackgrnd.rgbBlue);
							long lGDiff = abs(rgb1.rgbGreen - rgbBackgrnd.rgbGreen);
							long lRDiff = abs(rgb1.rgbRed - rgbBackgrnd.rgbRed);

							double lAverage = (lBDiff+lGDiff+lRDiff)/3;
							double lThresh = 16;
							double dLarge = lAverage/lThresh;
							double dSmall = (lThresh-lAverage)/lThresh;
							double dSmallAmt = dSmall*((double)rgb2.rgbBlue);

							if( lAverage < lThresh+1){
								rgbDest.rgbBlue = (BYTE)max(0,min(255,(int)(dLarge*((double)rgb1.rgbBlue) +
												dSmallAmt)));
								rgbDest.rgbGreen = (BYTE)max(0,min(255,(int)(dLarge*((double)rgb1.rgbGreen) +
												dSmallAmt)));
								rgbDest.rgbRed = (BYTE)max(0,min(255,(int)(dLarge*((double)rgb1.rgbRed) +
												dSmallAmt)));
							}
							else
								rgbDest = rgb1;
						}
						break;
						default:
						return;
				}
				SetPixelColor(lX,lY,rgbDest);
			}
		}
	}
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::ShiftRGB(long r, long g, long b)
{
	if (!pDib) return false;
	RGBQUAD color;
	if (head.biClrUsed==0){

		long xmin,xmax,ymin,ymax;
		if (pSelection){
			xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
			ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
		} else {
			xmin = ymin = 0;
			xmax = head.biWidth; ymax=head.biHeight;
		}

		for(long y=ymin; y<ymax; y++){
			for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					color = GetPixelColor(x,y);
					color.rgbRed = (BYTE)max(0,min(255,(int)(color.rgbRed + r)));
					color.rgbGreen = (BYTE)max(0,min(255,(int)(color.rgbGreen + g)));
					color.rgbBlue = (BYTE)max(0,min(255,(int)(color.rgbBlue + b)));
					SetPixelColor(x,y,color);
				}
			}
		}
	} else {
		for(DWORD j=0; j<head.biClrUsed; j++){
			color = GetPaletteColor((BYTE)j);
			color.rgbRed = (BYTE)max(0,min(255,(int)(color.rgbRed + r)));
			color.rgbGreen = (BYTE)max(0,min(255,(int)(color.rgbGreen + g)));
			color.rgbBlue = (BYTE)max(0,min(255,(int)(color.rgbBlue + b)));
			SetPaletteColor((BYTE)j,color);
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Gamma(float gamma)
{
	if (!pDib) return false;
	RGBQUAD color;

	double dinvgamma = 1/gamma;
	double dMax = pow(255.0, dinvgamma) / 255.0;
	
	if (head.biClrUsed==0){

		long xmin,xmax,ymin,ymax;
		if (pSelection){
			xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
			ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
		} else {
			xmin = ymin = 0;
			xmax = head.biWidth; ymax=head.biHeight;
		}

		for(long y=ymin; y<ymax; y++){
			info.nProgress = (long)(100*y/ymax); //<Anatoly Ivasyuk>
			for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
				if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
					color = GetPixelColor(x,y);
					color.rgbRed = (BYTE)max(0,min(255,(int)( pow((double)color.rgbRed, dinvgamma) / dMax)));
					color.rgbGreen = (BYTE)max(0,min(255,(int)( pow((double)color.rgbGreen, dinvgamma) / dMax)));
					color.rgbBlue = (BYTE)max(0,min(255,(int)( pow((double)color.rgbBlue, dinvgamma) / dMax)));
					SetPixelColor(x,y,color);
				}
			}
		}
	} else {
		for(DWORD j=0; j<head.biClrUsed; j++){
			color = GetPaletteColor((BYTE)j);
			color.rgbRed = (BYTE)max(0,min(255,(int)( pow((double)color.rgbRed, dinvgamma) / dMax)));
			color.rgbGreen = (BYTE)max(0,min(255,(int)( pow((double)color.rgbGreen, dinvgamma) / dMax)));
			color.rgbBlue = (BYTE)max(0,min(255,(int)( pow((double)color.rgbBlue, dinvgamma) / dMax)));
			SetPaletteColor((BYTE)j,color);
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_WINCE == 0
bool CxImage::Median(long Ksize)
{
	if (!pDib) return false;

	long k2 = Ksize/2;
	long kmax= Ksize-k2;
	long i,j,k;

	RGBQUAD* kernel = (RGBQUAD*)malloc(Ksize*Ksize*sizeof(RGBQUAD));

	CxImage tmp(*this,pSelection!=0,true,true);

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*y/head.biHeight);
		if (info.nEscape) break;
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
				for(j=-k2, i=0;j<kmax;j++)
					for(k=-k2;k<kmax;k++, i++)
						kernel[i]=GetPixelColor(x+j,y+k);

				qsort(kernel, i, sizeof(RGBQUAD), CompareColors);
				tmp.SetPixelColor(x,y,kernel[i/2]);
			}
		}
	}
	free(kernel);
	Transfer(tmp);
	return true;
}
#endif //CXIMAGE_SUPPORT_WINCE
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Noise(long level)
{
	if (!pDib) return false;
	RGBQUAD color;

	long xmin,xmax,ymin,ymax,n;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*y/ymax); //<Anatoly Ivasyuk>
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				color = GetPixelColor(x,y);
				n=(long)((rand()/(float)RAND_MAX - 0.5)*level);
				color.rgbRed = (BYTE)max(0,min(255,(int)(color.rgbRed + n)));
				n=(long)((rand()/(float)RAND_MAX - 0.5)*level);
				color.rgbGreen = (BYTE)max(0,min(255,(int)(color.rgbGreen + n)));
				n=(long)((rand()/(float)RAND_MAX - 0.5)*level);
				color.rgbBlue = (BYTE)max(0,min(255,(int)(color.rgbBlue + n)));
				SetPixelColor(x,y,color);
			}
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
long CxImage::Histogram(long* red, long* green, long* blue, long* gray, long colorspace)
{
	if (!pDib) return 0;
	RGBQUAD color;

	if (red) memset(red,0,256*sizeof(long));
	if (green) memset(green,0,256*sizeof(long));
	if (blue) memset(blue,0,256*sizeof(long));
	if (gray) memset(gray,0,256*sizeof(long));

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(long y=ymin; y<ymax; y++){
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				switch (colorspace){
				case 1:
					color = HSLtoRGB(GetPixelColor(x,y));
					break;
				case 2:
					color = YUVtoRGB(GetPixelColor(x,y));
					break;
				case 3:
					color = YIQtoRGB(GetPixelColor(x,y));
					break;
				case 4:
					color = XYZtoRGB(GetPixelColor(x,y));
					break;
				default:
					color = GetPixelColor(x,y);
				}

				if (red) red[color.rgbRed]++;
				if (green) green[color.rgbGreen]++;
				if (blue) blue[color.rgbBlue]++;
				if (gray) gray[(BYTE)RGB2GRAY(color.rgbRed,color.rgbGreen,color.rgbBlue)]++;
			}
		}
	}

	long n=0;
	for (int i=0; i<256; i++){
		if (red && red[i]>n) n=red[i];
		if (green && green[i]>n) n=green[i];
		if (blue && blue[i]>n) n=blue[i];
		if (gray && gray[i]>n) n=gray[i];
	}

	return n;
}
////////////////////////////////////////////////////////////////////////////////
#ifndef __BORLANDC__
////////////////////////////////////////////////////////////////////////////////
bool CxImage::FFT2(CxImage* srcReal, CxImage* srcImag, CxImage* dstReal, CxImage* dstImag,
				   long direction, bool bForceFFT, bool bMagnitude)
{
	//check if there is something to convert
	if (srcReal==NULL && srcImag==NULL) return false;

	long w,h;
	//get width and height
	if (srcReal) {
		w=srcReal->GetWidth();
		h=srcReal->GetHeight();
	} else {
		w=srcImag->GetWidth();
		h=srcImag->GetHeight();
	}

	bool bXpow2 = IsPowerof2(w);
	bool bYpow2 = IsPowerof2(h);
	//if bForceFFT, width AND height must be powers of 2
	if (bForceFFT && !(bXpow2 && bYpow2)) {
		long i;
		
		i=0;
		while((1<<i)<w) i++;
		w=1<<i;
		bXpow2=true;

		i=0;
		while((1<<i)<h) i++;
		h=1<<i;
		bYpow2=true;
	}

	// I/O images for FFT
	CxImage *tmpReal,*tmpImag;

	// select output
	tmpReal = (dstReal) ? dstReal : srcReal;
	tmpImag = (dstImag) ? dstImag : srcImag;

	// src!=dst -> copy the image
	if (srcReal && dstReal) tmpReal->Copy(*srcReal,true,false,false);
	if (srcImag && dstImag) tmpImag->Copy(*srcImag,true,false,false);

	// dst&&src are empty -> create new one, else turn to GrayScale
	if (srcReal==0 && dstReal==0){
		tmpReal = new CxImage(w,h,8);
		tmpReal->Clear(0);
		tmpReal->SetGrayPalette();
	} else {
		if (!tmpReal->IsGrayScale()) tmpReal->GrayScale();
	}
	if (srcImag==0 && dstImag==0){
		tmpImag = new CxImage(w,h,8);
		tmpImag->Clear(0);
		tmpImag->SetGrayPalette();
	} else {
		if (!tmpImag->IsGrayScale()) tmpImag->GrayScale();
	}

	if (!(tmpReal->IsValid() && tmpImag->IsValid())){
		if (srcReal==0 && dstReal==0) delete tmpReal;
		if (srcImag==0 && dstImag==0) delete tmpImag;
		return false;
	}

	//resample for FFT, if necessary 
	tmpReal->Resample(w,h,0);
	tmpImag->Resample(w,h,0);

	//ok, here we have 2 (w x h), grayscale images ready for a FFT

	double* real;
	double* imag;
	long j,k,m;

	_complex **grid;
	//double mean = tmpReal->Mean();
	/* Allocate memory for the grid */
	grid = (_complex **)malloc(w * sizeof(_complex));
	for (k=0;k<w;k++) {
		grid[k] = (_complex *)malloc(h * sizeof(_complex));
	}
	for (j=0;j<h;j++) {
		for (k=0;k<w;k++) {
			grid[k][j].x = tmpReal->GetPixelIndex(k,j)-128;
			grid[k][j].y = tmpImag->GetPixelIndex(k,j)-128;
		}
	}

	//DFT buffers
	double *real2,*imag2;
	real2 = (double*)malloc(max(w,h) * sizeof(double));
	imag2 = (double*)malloc(max(w,h) * sizeof(double));

	/* Transform the rows */
	real = (double *)malloc(w * sizeof(double));
	imag = (double *)malloc(w * sizeof(double));

	m=0;
	while((1<<m)<w) m++;

	for (j=0;j<h;j++) {
		for (k=0;k<w;k++) {
			real[k] = grid[k][j].x;
			imag[k] = grid[k][j].y;
		}

		if (bXpow2) FFT(direction,m,real,imag);
		else		DFT(direction,w,real,imag,real2,imag2);

		for (k=0;k<w;k++) {
			grid[k][j].x = real[k];
			grid[k][j].y = imag[k];
		}
	}
	free(real);
	free(imag);

	/* Transform the columns */
	real = (double *)malloc(h * sizeof(double));
	imag = (double *)malloc(h * sizeof(double));

	m=0;
	while((1<<m)<h) m++;

	for (k=0;k<w;k++) {
		for (j=0;j<h;j++) {
			real[j] = grid[k][j].x;
			imag[j] = grid[k][j].y;
		}

		if (bYpow2) FFT(direction,m,real,imag);
		else		DFT(direction,h,real,imag,real2,imag2);

		for (j=0;j<h;j++) {
			grid[k][j].x = real[j];
			grid[k][j].y = imag[j];
		}
	}
	free(real);
	free(imag);

	free(real2);
	free(imag2);

	/* converting from double to byte, there is a HUGE loss in the dynamics
	  "nn" tries to keep an acceptable SNR, but 8bit=48dB: don't ask more */
	double nn=pow((double)2,(double)log((double)max(w,h))/(double)log((double)2)-4);
	//reversed gain for reversed transform
	if (direction==-1) nn=1/nn;
	//bMagnitude : just to see it on the screen
	if (bMagnitude) nn*=4;

	for (j=0;j<h;j++) {
		for (k=0;k<w;k++) {
			if (bMagnitude){
				tmpReal->SetPixelIndex(k,j,(BYTE)max(0,min(255,(nn*(3+log(_cabs(grid[k][j])))))));
				if (grid[k][j].x==0){
					tmpImag->SetPixelIndex(k,j,(BYTE)max(0,min(255,(128+(atan(grid[k][j].y/0.0000000001)*nn)))));
				} else {
					tmpImag->SetPixelIndex(k,j,(BYTE)max(0,min(255,(128+(atan(grid[k][j].y/grid[k][j].x)*nn)))));
				}
			} else {
				tmpReal->SetPixelIndex(k,j,(BYTE)max(0,min(255,(128 + grid[k][j].x*nn))));
				tmpImag->SetPixelIndex(k,j,(BYTE)max(0,min(255,(128 + grid[k][j].y*nn))));
			}
		}
	}

	for (k=0;k<w;k++) free (grid[k]);
	free (grid);

	if (srcReal==0 && dstReal==0) delete tmpReal;
	if (srcImag==0 && dstImag==0) delete tmpImag;

	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //__BORLANDC__
////////////////////////////////////////////////////////////////////////////////
bool CxImage::IsPowerof2(long x)
{
	long i=0;
	while ((1<<i)<x) i++;
	if (x==(1<<i)) return true;
	return false;
}
////////////////////////////////////////////////////////////////////////////////
/*
   This computes an in-place complex-to-complex FFT 
   x and y are the real and imaginary arrays of n=2^m points.
   o(n)=n*log2(n)
   dir =  1 gives forward transform
   dir = -1 gives reverse transform 
   Written by Paul Bourke, July 1998
   FFT algorithm by Cooley and Tukey, 1965 
*/
bool CxImage::FFT(int dir,int m,double *x,double *y)
{
	long nn,i,i1,j,k,i2,l,l1,l2;
	double c1,c2,tx,ty,t1,t2,u1,u2,z;

	/* Calculate the number of points */
	nn = 1<<m;

	/* Do the bit reversal */
	i2 = nn >> 1;
	j = 0;
	for (i=0;i<nn-1;i++) {
		if (i < j) {
			tx = x[i];
			ty = y[i];
			x[i] = x[j];
			y[i] = y[j];
			x[j] = tx;
			y[j] = ty;
		}
		k = i2;
		while (k <= j) {
			j -= k;
			k >>= 1;
		}
		j += k;
	}

	/* Compute the FFT */
	c1 = -1.0;
	c2 = 0.0;
	l2 = 1;
	for (l=0;l<m;l++) {
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0;
		u2 = 0.0;
		for (j=0;j<l1;j++) {
			for (i=j;i<nn;i+=l2) {
				i1 = i + l1;
				t1 = u1 * x[i1] - u2 * y[i1];
				t2 = u1 * y[i1] + u2 * x[i1];
				x[i1] = x[i] - t1;
				y[i1] = y[i] - t2;
				x[i] += t1;
				y[i] += t2;
			}
			z =  u1 * c1 - u2 * c2;
			u2 = u1 * c2 + u2 * c1;
			u1 = z;
		}
		c2 = sqrt((1.0 - c1) / 2.0);
		if (dir == 1)
			c2 = -c2;
		c1 = sqrt((1.0 + c1) / 2.0);
	}

	/* Scaling for forward transform */
	if (dir == 1) {
		for (i=0;i<nn;i++) {
			x[i] /= (double)nn;
			y[i] /= (double)nn;
		}
	}

   return true;
}
////////////////////////////////////////////////////////////////////////////////
/*
   Direct fourier transform o(n)=n^2
   Written by Paul Bourke, July 1998 
*/
bool CxImage::DFT(int dir,long m,double *x1,double *y1,double *x2,double *y2)
{
   long i,k;
   double arg;
   double cosarg,sinarg;
   
   for (i=0;i<m;i++) {
      x2[i] = 0;
      y2[i] = 0;
      arg = - dir * 2.0 * 3.14159265358f * i / (double)m;
      for (k=0;k<m;k++) {
         cosarg = cos(k * arg);
         sinarg = sin(k * arg);
         x2[i] += (x1[k] * cosarg - y1[k] * sinarg);
         y2[i] += (x1[k] * sinarg + y1[k] * cosarg);
      }
   }
   
   /* Copy the data back */
   if (dir == 1) {
      for (i=0;i<m;i++) {
         x1[i] = x2[i] / m;
         y1[i] = y2[i] / m;
      }
   } else {
      for (i=0;i<m;i++) {
         x1[i] = x2[i];
         y1[i] = y2[i];
      }
   }
   
   return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Combine(CxImage* r,CxImage* g,CxImage* b,CxImage* a, long colorspace)
{
	if (r==0 || g==0 || b==0) return false;

	long w = r->GetWidth();
	long h = r->GetHeight();

	Create(w,h,24);

	g->Resample(w,h);
	b->Resample(w,h);

	if (a) {
		a->Resample(w,h);
#if CXIMAGE_SUPPORT_ALPHA
		AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA
	}

	RGBQUAD c;
	for (long y=0;y<h;y++){
		info.nProgress = (long)(100*y/h); //<Anatoly Ivasyuk>
		for (long x=0;x<w;x++){
			c.rgbRed=r->GetPixelIndex(x,y);
			c.rgbGreen=g->GetPixelIndex(x,y);
			c.rgbBlue=b->GetPixelIndex(x,y);
			switch (colorspace){
			case 1:
				SetPixelColor(x,y,HSLtoRGB(c));
				break;
			case 2:
				SetPixelColor(x,y,YUVtoRGB(c));
				break;
			case 3:
				SetPixelColor(x,y,YIQtoRGB(c));
				break;
			case 4:
				SetPixelColor(x,y,XYZtoRGB(c));
				break;
			default:
				SetPixelColor(x,y,c);
			}
#if CXIMAGE_SUPPORT_ALPHA
			if (a) AlphaSet(x,y,a->GetPixelIndex(x,y));
#endif //CXIMAGE_SUPPORT_ALPHA
		}
	}

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Repair(float radius, long niterations, long colorspace)
{
	if (!IsValid()) return false;

	long w = GetWidth();
	long h = GetHeight();

	CxImage r,g,b;

	r.Create(w,h,8);
	g.Create(w,h,8);
	b.Create(w,h,8);

	switch (colorspace){
	case 1:
		SplitHSL(&r,&g,&b);
		break;
	case 2:
		SplitYUV(&r,&g,&b);
		break;
	case 3:
		SplitYIQ(&r,&g,&b);
		break;
	case 4:
		SplitXYZ(&r,&g,&b);
		break;
	default:
		SplitRGB(&r,&g,&b);
	}
	
	for (int i=0; i<niterations; i++){
		RepairChannel(&r,radius);
		RepairChannel(&g,radius);
		RepairChannel(&b,radius);
	}

	CxImage* a=NULL;
#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){
		a = new CxImage();
		AlphaSplit(a);
	}
#endif

	Combine(&r,&g,&b,a,colorspace);

	delete a;

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::RepairChannel(CxImage *ch, float radius)
{
	if (ch==NULL) return false;
	CxImage tmp(*ch);
	long w = ch->GetWidth()-1;
	long h = ch->GetHeight()-1;

	double correction,ix,iy,ixx,ixy,iyy,den,num;
	int x,y,xy0,xp1,xm1,yp1,ym1;
	for(x=1; x<w; x++){
		for(y=1; y<h; y++){

			xy0 = ch->GetPixelIndex(x,y);
			xm1 = ch->GetPixelIndex(x-1,y);
			xp1 = ch->GetPixelIndex(x+1,y);
			ym1 = ch->GetPixelIndex(x,y-1);
			yp1 = ch->GetPixelIndex(x,y+1);

			ix= (xp1-xm1)/2.0;
			iy= (yp1-ym1)/2.0;
			ixx= xp1 - 2.0 * xy0 + xm1;
			iyy= yp1 - 2.0 * xy0 + ym1;
			ixy=(ch->GetPixelIndex(x+1,y+1)+ch->GetPixelIndex(x-1,y-1)-
				 ch->GetPixelIndex(x-1,y+1)-ch->GetPixelIndex(x+1,y-1))/4.0;

			num= (1.0+iy*iy)*ixx - ix*iy*ixy + (1.0+ix*ix)*iyy;
			den= 1.0+ix*ix+iy*iy;
			correction = num/den;

			tmp.SetPixelIndex(x,y,(BYTE)min(255,max(0,(xy0 + radius * correction))));
		}
	}

	for (x=0;x<=w;x++){
		tmp.SetPixelIndex(x,0,ch->GetPixelIndex(x,0));
		tmp.SetPixelIndex(x,h,ch->GetPixelIndex(x,h));
	}
	for (y=0;y<=h;y++){
		tmp.SetPixelIndex(0,y,ch->GetPixelIndex(0,y));
		tmp.SetPixelIndex(w,y,ch->GetPixelIndex(w,y));
	}
	ch->Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
// HistogramStretch function by <dave> : dave(at)posortho(dot)com
bool CxImage::HistogramStretch()
{
	if (!pDib) return false;
	// S = ( R - C ) ( B - A / D - C )
	double alimit = 0.0;
	double blimit = 255.0;
	double lowerc = 255.0;
	double upperd = 0.0;
	double tmpGray;

	RGBQUAD color;
	RGBQUAD	yuvClr;
	double  stretcheds;

	if ( head.biClrUsed == 0 ){
		long x, y, xmin, xmax, ymin, ymax;
		xmin = ymin = 0;
		xmax = head.biWidth; 
		ymax = head.biHeight;

		for( y = ymin; y < ymax; y++ ){
			info.nProgress = (long)(50*y/ymax);
			if (info.nEscape) break;
			for( x = xmin; x < xmax; x++ ){
				color = GetPixelColor( x, y );
				tmpGray = RGB2GRAY(color.rgbRed, color.rgbGreen, color.rgbBlue);
				if ( tmpGray < lowerc )	lowerc = tmpGray;
				if ( tmpGray > upperd )	upperd = tmpGray;
			}
		}
		if (upperd==lowerc) return false;
		
		for( y = ymin; y < ymax; y++ ){
			info.nProgress = (long)(50+50*y/ymax);
			if (info.nEscape) break;
			for( x = xmin; x < xmax; x++ ){

				color = GetPixelColor( x, y );
				yuvClr = RGBtoYUV(color);

				// Stretch Luminance
				tmpGray = (double)yuvClr.rgbRed;
				stretcheds = (double)(tmpGray - lowerc) * ( (blimit - alimit) / (upperd - lowerc) ); // + alimit;
				if ( stretcheds < 0.0 )	stretcheds = 0.0;
				else if ( stretcheds > 255.0 ) stretcheds = 255.0;
				yuvClr.rgbRed = (BYTE)stretcheds;

				color = YUVtoRGB(yuvClr);
				SetPixelColor( x, y, color );
			}
		}
	} else {
		DWORD  j;
		for( j = 0; j < head.biClrUsed; j++ ){
			color = GetPaletteColor( (BYTE)j );
			tmpGray = RGB2GRAY(color.rgbRed, color.rgbGreen, color.rgbBlue);
			if ( tmpGray < lowerc )	lowerc = tmpGray;
			if ( tmpGray > upperd )	upperd = tmpGray;
		}
		if (upperd==lowerc) return false;

		for( j = 0; j < head.biClrUsed; j++ ){

			color = GetPaletteColor( (BYTE)j );
			yuvClr = RGBtoYUV( color );

			// Stretch Luminance
			tmpGray = (double)yuvClr.rgbRed;
			stretcheds = (double)(tmpGray - lowerc) * ( (blimit - alimit) / (upperd - lowerc) ); // + alimit;
			if ( stretcheds < 0.0 )	stretcheds = 0.0;
			else if ( stretcheds > 255.0 ) stretcheds = 255.0;
			yuvClr.rgbRed = (BYTE)stretcheds;

			color = YUVtoRGB(yuvClr);
			SetPaletteColor( (BYTE)j, color );
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
// HistogramEqualize function by <dave> : dave(at)posortho(dot)com
bool CxImage::HistogramEqualize()
{
	if (!pDib) return false;

    int histogram[256];
	int map[256];
	int equalize_map[256];
    int x, y, i, j;
	RGBQUAD color;
	RGBQUAD	yuvClr;
	unsigned int YVal, high, low;

	memset( &histogram, 0, sizeof(int) * 256 );
	memset( &map, 0, sizeof(int) * 256 );
	memset( &equalize_map, 0, sizeof(int) * 256 );
 
     // form histogram
	for(y=0; y < head.biHeight; y++){
		info.nProgress = (long)(50*y/head.biHeight);
		if (info.nEscape) break;
		for(x=0; x < head.biWidth; x++){
			color = GetPixelColor( x, y );
			YVal = (unsigned int)RGB2GRAY(color.rgbRed, color.rgbGreen, color.rgbBlue);
			histogram[YVal]++;
		}
	}

	// integrate the histogram to get the equalization map.
	j = 0;
	for(i=0; i <= 255; i++){
		j += histogram[i];
		map[i] = j; 
	}

	// equalize
	low = map[0];
	high = map[255];
	if (low == high) return false;
	for( i = 0; i <= 255; i++ ){
		equalize_map[i] = (unsigned int)((((double)( map[i] - low ) ) * 255) / ( high - low ) );
	}

	// stretch the histogram
	if(head.biClrUsed == 0){ // No Palette
		for( y = 0; y < head.biHeight; y++ ){
			info.nProgress = (long)(50+50*y/head.biHeight);
			if (info.nEscape) break;
			for( x = 0; x < head.biWidth; x++ ){

				color = GetPixelColor( x, y );
				yuvClr = RGBtoYUV(color);

                yuvClr.rgbRed = (BYTE)equalize_map[yuvClr.rgbRed];

				color = YUVtoRGB(yuvClr);
				SetPixelColor( x, y, color );
			}
		}
	} else { // Palette
		for( i = 0; i < (int)head.biClrUsed; i++ ){

			color = GetPaletteColor((BYTE)i);
			yuvClr = RGBtoYUV(color);

            yuvClr.rgbRed = (BYTE)equalize_map[yuvClr.rgbRed];

			color = YUVtoRGB(yuvClr);
			SetPaletteColor( (BYTE)i, color );
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
// HistogramNormalize function by <dave> : dave(at)posortho(dot)com
bool CxImage::HistogramNormalize()
{
	if (!pDib) return false;

	int histogram[256];
	int threshold_intensity, intense;
	int x, y, i;
	unsigned int normalize_map[256];
	unsigned int high, low, YVal;

	RGBQUAD color;
	RGBQUAD	yuvClr;

	memset( &histogram, 0, sizeof( int ) * 256 );
	memset( &normalize_map, 0, sizeof( unsigned int ) * 256 );
 
     // form histogram
	for(y=0; y < head.biHeight; y++){
		info.nProgress = (long)(50*y/head.biHeight);
		if (info.nEscape) break;
		for(x=0; x < head.biWidth; x++){
			color = GetPixelColor( x, y );
			YVal = (unsigned int)RGB2GRAY(color.rgbRed, color.rgbGreen, color.rgbBlue);
			histogram[YVal]++;
		}
	}

	// find histogram boundaries by locating the 1 percent levels
	threshold_intensity = ( head.biWidth * head.biHeight) / 100;

	intense = 0;
	for( low = 0; low < 255; low++ ){
		intense += histogram[low];
		if( intense > threshold_intensity )	break;
	}

	intense = 0;
	for( high = 255; high != 0; high--){
		intense += histogram[ high ];
		if( intense > threshold_intensity ) break;
	}

	if ( low == high ){
		// Unreasonable contrast;  use zero threshold to determine boundaries.
		threshold_intensity = 0;
		intense = 0;
		for( low = 0; low < 255; low++){
			intense += histogram[low];
			if( intense > threshold_intensity )	break;
		}
		intense = 0;
		for( high = 255; high != 0; high-- ){
			intense += histogram [high ];
			if( intense > threshold_intensity )	break;
		}
	}
	if( low == high ) return false;  // zero span bound

	// Stretch the histogram to create the normalized image mapping.
	for(i = 0; i <= 255; i++){
		if ( i < (int) low ){
			normalize_map[i] = 0;
		} else {
			if(i > (int) high)
				normalize_map[i] = 255;
			else
				normalize_map[i] = ( 255 - 1) * ( i - low) / ( high - low );
		}
	}

	// Normalize
	if( head.biClrUsed == 0 ){
		for( y = 0; y < head.biHeight; y++ ){
			info.nProgress = (long)(50+50*y/head.biHeight);
			if (info.nEscape) break;
			for( x = 0; x < head.biWidth; x++ ){

				color = GetPixelColor( x, y );
				yuvClr = RGBtoYUV( color );

                yuvClr.rgbRed = (BYTE)normalize_map[yuvClr.rgbRed];

				color = YUVtoRGB( yuvClr );
				SetPixelColor( x, y, color );
			}
		}
	} else {
		for(i = 0; i < (int)head.biClrUsed; i++){

			color = GetPaletteColor( (BYTE)i );
			yuvClr = RGBtoYUV( color );

            yuvClr.rgbRed = (BYTE)normalize_map[yuvClr.rgbRed];

			color = YUVtoRGB( yuvClr );
 			SetPaletteColor( (BYTE)i, color );
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
// HistogramLog function by <dave> : dave(at)posortho(dot)com
bool CxImage::HistogramLog()
{
	if (!pDib) return false;

	//q(i,j) = 255/log(1 + |high|) * log(1 + |p(i,j)|);
    int x, y, i;
	RGBQUAD color;
	RGBQUAD	yuvClr;

	unsigned int YVal, high = 1;

    // Find Highest Luminance Value in the Image
	if( head.biClrUsed == 0 ){ // No Palette
		for(y=0; y < head.biHeight; y++){
			info.nProgress = (long)(50*y/head.biHeight);
			if (info.nEscape) break;
			for(x=0; x < head.biWidth; x++){
				color = GetPixelColor( x, y );
				YVal = (unsigned int)RGB2GRAY(color.rgbRed, color.rgbGreen, color.rgbBlue);
				if (YVal > high ) high = YVal;
			}
		}
	} else { // Palette
		for(i = 0; i < (int)head.biClrUsed; i++){
			color = GetPaletteColor((BYTE)i);
			YVal = (unsigned int)RGB2GRAY(color.rgbRed, color.rgbGreen, color.rgbBlue);
			if (YVal > high ) high = YVal;
		}
	}

	// Logarithm Operator
	double k = 255.0 / ::log( 1.0 + (double)high );
	if( head.biClrUsed == 0 ){
		for( y = 0; y < head.biHeight; y++ ){
			info.nProgress = (long)(50+50*y/head.biHeight);
			if (info.nEscape) break;
			for( x = 0; x < head.biWidth; x++ ){

				color = GetPixelColor( x, y );
				yuvClr = RGBtoYUV( color );
                
				yuvClr.rgbRed = (BYTE)(k * ::log( 1.0 + (double)yuvClr.rgbRed ) );

				color = YUVtoRGB( yuvClr );
				SetPixelColor( x, y, color );
			}
		}
	} else {
		for(i = 0; i < (int)head.biClrUsed; i++){

			color = GetPaletteColor( (BYTE)i );
			yuvClr = RGBtoYUV( color );

            yuvClr.rgbRed = (BYTE)(k * ::log( 1.0 + (double)yuvClr.rgbRed ) );
			
			color = YUVtoRGB( yuvClr );
 			SetPaletteColor( (BYTE)i, color );
		}
	}
 
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// HistogramRoot function by <dave> : dave(at)posortho(dot)com
bool CxImage::HistogramRoot()
{
	if (!pDib) return false;
	//q(i,j) = sqrt(|p(i,j)|);

    int x, y, i;
	RGBQUAD color;
	RGBQUAD	 yuvClr;
	double	dtmp;
	unsigned int YVal, high = 1;

     // Find Highest Luminance Value in the Image
	if( head.biClrUsed == 0 ){ // No Palette
		for(y=0; y < head.biHeight; y++){
			info.nProgress = (long)(50*y/head.biHeight);
			if (info.nEscape) break;
			for(x=0; x < head.biWidth; x++){
				color = GetPixelColor( x, y );
				YVal = (unsigned int)RGB2GRAY(color.rgbRed, color.rgbGreen, color.rgbBlue);
				if (YVal > high ) high = YVal;
			}
		}
	} else { // Palette
		for(i = 0; i < (int)head.biClrUsed; i++){
			color = GetPaletteColor((BYTE)i);
			YVal = (unsigned int)RGB2GRAY(color.rgbRed, color.rgbGreen, color.rgbBlue);
			if (YVal > high ) high = YVal;
		}
	}

	// Root Operator
	double k = 128.0 / ::log( 1.0 + (double)high );
	if( head.biClrUsed == 0 ){
		for( y = 0; y < head.biHeight; y++ ){
			info.nProgress = (long)(50+50*y/head.biHeight);
			if (info.nEscape) break;
			for( x = 0; x < head.biWidth; x++ ){

				color = GetPixelColor( x, y );
				yuvClr = RGBtoYUV( color );

				dtmp = k * ::sqrt( (double)yuvClr.rgbRed );
				if ( dtmp > 255.0 )	dtmp = 255.0;
				if ( dtmp < 0 )	dtmp = 0;
                yuvClr.rgbRed = (BYTE)dtmp;

				color = YUVtoRGB( yuvClr );
				SetPixelColor( x, y, color );
			}
		}
	} else {
		for(i = 0; i < (int)head.biClrUsed; i++){

			color = GetPaletteColor( (BYTE)i );
			yuvClr = RGBtoYUV( color );

			dtmp = k * ::sqrt( (double)yuvClr.rgbRed );
			if ( dtmp > 255.0 )	dtmp = 255.0;
			if ( dtmp < 0 ) dtmp = 0;
            yuvClr.rgbRed = (BYTE)dtmp;

			color = YUVtoRGB( yuvClr );
 			SetPaletteColor( (BYTE)i, color );
		}
	}
 
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Contour()
{
	if (!pDib) return false;

	long Ksize = 3;
	long k2 = Ksize/2;
	long kmax= Ksize-k2;
	long i,j,k;
	BYTE maxr,maxg,maxb;
	RGBQUAD pix1,pix2;

	CxImage tmp(*this,pSelection!=0,true,true);

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*y/head.biHeight);
		if (info.nEscape) break;
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
				{
				pix1 = GetPixelColor(x,y);
				maxr=maxg=maxb=0;
				for(j=-k2, i=0;j<kmax;j++){
					for(k=-k2;k<kmax;k++, i++){
						pix2=GetPixelColor(x+j,y+k);
						if ((pix2.rgbBlue-pix1.rgbBlue)>maxb) maxb = pix2.rgbBlue;
						if ((pix2.rgbGreen-pix1.rgbGreen)>maxg) maxg = pix2.rgbGreen;
						if ((pix2.rgbRed-pix1.rgbRed)>maxr) maxr = pix2.rgbRed;
					}
				}
				pix1.rgbBlue=(BYTE)(255-maxb);
				pix1.rgbGreen=(BYTE)(255-maxg);
				pix1.rgbRed=(BYTE)(255-maxr);
				tmp.SetPixelColor(x,y,pix1);
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Jitter(long radius)
{
	if (!pDib) return false;

	long nx,ny;

	CxImage tmp(*this,pSelection!=0,true,true);

	long xmin,xmax,ymin,ymax;
	if (pSelection){
		xmin = info.rSelectionBox.left; xmax = info.rSelectionBox.right;
		ymin = info.rSelectionBox.bottom; ymax = info.rSelectionBox.top;
	} else {
		xmin = ymin = 0;
		xmax = head.biWidth; ymax=head.biHeight;
	}

	for(long y=ymin; y<ymax; y++){
		info.nProgress = (long)(100*y/head.biHeight);
		if (info.nEscape) break;
		for(long x=xmin; x<xmax; x++){
#if CXIMAGE_SUPPORT_SELECTION
			if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
			{
				nx=x+(long)((rand()/(float)RAND_MAX - 0.5)*(radius*2));
				ny=y+(long)((rand()/(float)RAND_MAX - 0.5)*(radius*2));
				if (!IsInside(nx,ny)) {
					nx=x;
					ny=y;
				}
				if (head.biClrUsed==0){
					tmp.SetPixelColor(x,y,GetPixelColor(nx,ny));
				} else {
					tmp.SetPixelIndex(x,y,GetPixelIndex(nx,ny));
				}
#if CXIMAGE_SUPPORT_ALPHA
				tmp.AlphaSet(x,y,AlphaGet(nx,ny));
#endif //CXIMAGE_SUPPORT_ALPHA
			}
		}
	}
	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
/*bool CxImage::FloodFill(int x, int y, RGBQUAD FillColor)
{
	//<JDL>
	if (!pDib) return false;
    FloodFill2(x,y,GetPixelColor(x,y),FillColor);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::FloodFill2(int x, int y, RGBQUAD old_color, RGBQUAD new_color)
{
	// Fill in the actual pixels. 
	// Function steps recursively until it finds borders (color that is not old_color)
	if (!IsInside(x,y)) return;

	RGBQUAD r = GetPixelColor(x,y);
	COLORREF cr = RGB(r.rgbRed,r.rgbGreen,r.rgbBlue);

	if(cr == RGB(old_color.rgbRed,old_color.rgbGreen,old_color.rgbBlue)
		&& cr != RGB(new_color.rgbRed,new_color.rgbGreen,new_color.rgbBlue) ) {

		// the above if statement, after && is there to prevent
		// stack overflows.  The program will continue to find 
		// colors if you flood-fill an entire region (entire picture)

		SetPixelColor(x,y,new_color);

		FloodFill2((x+1),y,old_color,new_color);
		FloodFill2((x-1),y,old_color,new_color);
		FloodFill2(x,(y+1),old_color,new_color);
		FloodFill2(x,(y-1),old_color,new_color);
	}
}*/
///////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DSP
