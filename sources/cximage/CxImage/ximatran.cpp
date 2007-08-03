// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
// xImaTran.cpp : Transformation functions
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximage.h"

#if CXIMAGE_SUPPORT_BASICTRANSFORMATIONS
////////////////////////////////////////////////////////////////////////////////
bool CxImage::GrayScale()
{
	if (!pDib) return false;
	if (head.biBitCount<=8){
		RGBQUAD* ppal=GetPalette();
		int gray;
		//converts the colors to gray, use the blue channel only
		for(DWORD i=0;i<head.biClrUsed;i++){
			gray=(int)RGB2GRAY(ppal[i].rgbRed,ppal[i].rgbGreen,ppal[i].rgbBlue);
			ppal[i].rgbBlue = (BYTE)gray;
		}
		// preserve transparency
		if (info.nBkgndIndex != -1) info.nBkgndIndex = ppal[info.nBkgndIndex].rgbBlue;
		//create a "real" 8 bit gray scale image
		if (head.biBitCount==8){
			BYTE *img=info.pImage;
			for(DWORD i=0;i<head.biSizeImage;i++) img[i]=ppal[img[i]].rgbBlue;
			SetGrayPalette();
		}
		//transform to 8 bit gray scale
		if (head.biBitCount==4 || head.biBitCount==1){
			CxImage ima;
			ima.CopyInfo(*this);
			if (!ima.Create(head.biWidth,head.biHeight,8,info.dwType)) return false;
			ima.SetGrayPalette();
#if CXIMAGE_SUPPORT_SELECTION
			ima.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION
#if CXIMAGE_SUPPORT_ALPHA
			ima.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA
			BYTE *img=ima.GetBits();
			long l=ima.GetEffWidth();
			for (long y=0;y<head.biHeight;y++){
				for (long x=0;x<head.biWidth; x++){
					img[x+y*l]=ppal[GetPixelIndex(x,y)].rgbBlue;
				}
			}
			Transfer(ima);
		}
	} else { //from RGB to 8 bit gray scale
		BYTE *iSrc=info.pImage;
		CxImage ima;
		ima.CopyInfo(*this);
		if (!ima.Create(head.biWidth,head.biHeight,8,info.dwType)) return false;
		ima.SetGrayPalette();
#if CXIMAGE_SUPPORT_SELECTION
		ima.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION
#if CXIMAGE_SUPPORT_ALPHA
		ima.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA
		BYTE *img=ima.GetBits();
		long l8=ima.GetEffWidth();
		long l=head.biWidth * 3;
		for(long y=0; y < head.biHeight; y++) {
			for(long x=0,x8=0; x < l; x+=3,x8++) {
				img[x8+y*l8]=(BYTE)RGB2GRAY(*(iSrc+x+2),*(iSrc+x+1),*(iSrc+x+0));
			}
			iSrc+=info.dwEffWidth;
		}
		Transfer(ima);
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Flip()
{
	if (!pDib) return false;
	CxImage* imatmp = new CxImage(*this,false,false,true);
	if (!imatmp) return false;
	BYTE *iSrc,*iDst;
	iSrc=info.pImage + (head.biHeight-1)*info.dwEffWidth;
	iDst=imatmp->info.pImage;
    for(long y=0; y < head.biHeight; y++){
		memcpy(iDst,iSrc,info.dwEffWidth);
		iSrc-=info.dwEffWidth;
		iDst+=info.dwEffWidth;
	}

#if CXIMAGE_SUPPORT_ALPHA
	imatmp->AlphaFlip();
#endif //CXIMAGE_SUPPORT_ALPHA

	Transfer(*imatmp);
	delete imatmp;
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Mirror()
{
	if (!pDib) return false;

	CxImage* imatmp = new CxImage(*this,false,false,true);
	if (!imatmp) return false;
	BYTE *iSrc,*iDst;
	long wdt=(head.biWidth-1) * (head.biBitCount==24 ? 3:1);
	iSrc=info.pImage + wdt;
	iDst=imatmp->info.pImage;
	long x,y;
	switch (head.biBitCount){
	case 24:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x+=3){
				*(iDst+x)=*(iSrc-x);
				*(iDst+x+1)=*(iSrc-x+1);
				*(iDst+x+2)=*(iSrc-x+2);
			}
			iSrc+=info.dwEffWidth;
			iDst+=info.dwEffWidth;
		}
		break;
	case 8:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x++)
				*(iDst+x)=*(iSrc-x);
			iSrc+=info.dwEffWidth;
			iDst+=info.dwEffWidth;
		}
		break;
	default:
		for(y=0; y < head.biHeight; y++){
			for(x=0; x <= wdt; x++)
				imatmp->SetPixelIndex(x,y,GetPixelIndex(wdt-x,y));
		}
	}

#if CXIMAGE_SUPPORT_ALPHA
	imatmp->AlphaMirror();
#endif //CXIMAGE_SUPPORT_ALPHA

	Transfer(*imatmp);
	delete imatmp;
	return true;
}

#ifdef XBMC
bool CxImage::RotateExif(int orientation /* = 0 */)
{
  bool ret = true;
  if (orientation <= 0)
    orientation = info.ExifInfo.Orientation;
  if (orientation == 3)
    ret = Rotate180();
  else if (orientation == 6)
    ret = RotateRight();
  else if (orientation == 8)
    ret = RotateLeft();
  info.ExifInfo.Orientation = 1;
  return ret;
}
#endif

////////////////////////////////////////////////////////////////////////////////
bool CxImage::RotateLeft(CxImage* iDst)
{
	if (!pDib) return false;

	long newWidth = GetHeight();
	long newHeight = GetWidth();

	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

	long x,x2,y,dlineup;
	
	// Speedy rotate for BW images <Robert Abram>
	if (head.biBitCount == 1) {
	
		BYTE *sbits, *dbits, *dbitsmax, bitpos, *nrow,*srcdisp;
		div_t div_r;

		BYTE *bsrc = GetBits(), *bdest = imgDest.GetBits();
		dbitsmax = bdest + imgDest.head.biSizeImage - 1;
		dlineup = 8 * imgDest.info.dwEffWidth - imgDest.head.biWidth;

		imgDest.Clear(0);
		for (y = 0; y < head.biHeight; y++) {
			// Figure out the Column we are going to be copying to
			div_r = div(y + dlineup, 8);
			// set bit pos of src column byte				
			bitpos = 1 << div_r.rem;
			srcdisp = bsrc + y * info.dwEffWidth;
			for (x = 0; x < (long)info.dwEffWidth; x++) {
				// Get Source Bits
				sbits = srcdisp + x;
				// Get destination column
				nrow = bdest + (x * 8) * imgDest.info.dwEffWidth + imgDest.info.dwEffWidth - 1 - div_r.quot;
				for (long z = 0; z < 8; z++) {
				   // Get Destination Byte
					dbits = nrow + z * imgDest.info.dwEffWidth;
					if ((dbits < bdest) || (dbits > dbitsmax)) break;
					if (*sbits & (128 >> z)) *dbits |= bitpos;
				}
			}
		}
	} else {
		for (x = 0; x < newWidth; x++){
			info.nProgress = (long)(100*x/newWidth); //<Anatoly Ivasyuk>
			x2=newWidth-x-1;
			for (y = 0; y < newHeight; y++){
				if(head.biClrUsed==0)	//RGB
					imgDest.SetPixelColor(x, y, GetPixelColor(y, x2));
				else					//PALETTE
					imgDest.SetPixelIndex(x, y, GetPixelIndex(y, x2));
			}
		}
	}

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){
		imgDest.AlphaCreate();
		for (x = 0; x < newWidth; x++){
			x2=newWidth-x-1;
			for (y = 0; y < newHeight; y++){
				imgDest.AlphaSet(x,y,AlphaGet(y, x2));
			}
		}
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CxImage::RotateRight(CxImage* iDst)
{
	if (!pDib) return false;

	long newWidth = GetHeight();
	long newHeight = GetWidth();

	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

	long x,y,y2;
	// Speedy rotate for BW images <Robert Abram>
	if (head.biBitCount == 1) {
	
		BYTE *sbits, *dbits, *dbitsmax, bitpos, *nrow,*srcdisp;
		div_t div_r;

		BYTE *bsrc = GetBits(), *bdest = imgDest.GetBits();
		dbitsmax = bdest + imgDest.head.biSizeImage - 1;

		imgDest.Clear(0);
		for (y = 0; y < head.biHeight; y++) {
			// Figure out the Column we are going to be copying to
			div_r = div(y, 8);
			// set bit pos of src column byte				
			bitpos = 128 >> div_r.rem;
			srcdisp = bsrc + y * info.dwEffWidth;
			for (x = 0; x < (long)info.dwEffWidth; x++) {
				// Get Source Bits
				sbits = srcdisp + x;
				// Get destination column
				nrow = bdest + (imgDest.head.biHeight-1-(x*8)) * imgDest.info.dwEffWidth + div_r.quot;
				for (long z = 0; z < 8; z++) {
				   // Get Destination Byte
					dbits = nrow - z * imgDest.info.dwEffWidth;
					if ((dbits < bdest) || (dbits > dbitsmax)) break;
					if (*sbits & (128 >> z)) *dbits |= bitpos;
				}
			}
		}
	} else {
		for (y = 0; y < newHeight; y++){
			info.nProgress = (long)(100*y/newHeight); //<Anatoly Ivasyuk>
			y2=newHeight-y-1;
			for (x = 0; x < newWidth; x++){
				if(head.biClrUsed==0)	//RGB
					imgDest.SetPixelColor(x, y, GetPixelColor(y2, x));
				else					//PALETTE
					imgDest.SetPixelIndex(x, y, GetPixelIndex(y2, x));
			}
		}
	}

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){
		imgDest.AlphaCreate();
		for (y = 0; y < newHeight; y++){
			y2=newHeight-y-1;
			for (x = 0; x < newWidth; x++){
				imgDest.AlphaSet(x,y,AlphaGet(y2, x));
			}
		}
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CxImage::Negative()
{
	if (!pDib) return false;

	if (head.biBitCount<=8){
		if (IsGrayScale()){ //GRAYSCALE, selection
			if (pSelection){
				for(long y=info.rSelectionBox.bottom; y<info.rSelectionBox.top; y++){
					for(long x=info.rSelectionBox.left; x<info.rSelectionBox.right; x++){
#if CXIMAGE_SUPPORT_SELECTION
						if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
						{
							SetPixelIndex(x,y,(BYTE)(255-GetPixelIndex(x,y)));
						}
					}
				}
			} else {
				for(long y=0; y<head.biHeight; y++){
					for(long x=0; x<head.biWidth; x++){
						SetPixelIndex(x,y,(BYTE)(255-GetPixelIndex(x,y)));
					}
				}
			}
		} else { //PALETTE, full image
			RGBQUAD* ppal=GetPalette();
			for(DWORD i=0;i<head.biClrUsed;i++){
				ppal[i].rgbBlue =(BYTE)(255-ppal[i].rgbBlue);
				ppal[i].rgbGreen =(BYTE)(255-ppal[i].rgbGreen);
				ppal[i].rgbRed =(BYTE)(255-ppal[i].rgbRed);
			}
		}
	} else {
		if (pSelection==NULL){ //RGB, full image
			BYTE *iSrc=info.pImage;
			for(unsigned long i=0; i < head.biSizeImage ; i++){
				*iSrc=(BYTE)~(*(iSrc));
				iSrc++;
			}
		} else { // RGB with selection
			RGBQUAD color;
			for(long y=info.rSelectionBox.bottom; y<info.rSelectionBox.top; y++){
				for(long x=info.rSelectionBox.left; x<info.rSelectionBox.right; x++){
#if CXIMAGE_SUPPORT_SELECTION
					if (SelectionIsInside(x,y))
#endif //CXIMAGE_SUPPORT_SELECTION
					{
						color = GetPixelColor(x,y);
						color.rgbRed = (BYTE)(255-color.rgbRed);
						color.rgbGreen = (BYTE)(255-color.rgbGreen);
						color.rgbBlue = (BYTE)(255-color.rgbBlue);
						SetPixelColor(x,y,color);
					}
				}
			}
		}
		//<DP> invert transparent color too
		info.nBkgndColor.rgbBlue = (BYTE)(255-info.nBkgndColor.rgbBlue);
		info.nBkgndColor.rgbGreen = (BYTE)(255-info.nBkgndColor.rgbGreen);
		info.nBkgndColor.rgbRed = (BYTE)(255-info.nBkgndColor.rgbRed);
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_BASICTRANSFORMATIONS
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_TRANSFORMATION
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
bool CxImage::Rotate(float angle, CxImage* iDst)
{
	if (!pDib) return false;

//  $Id: FilterRotate.cpp,v 1.10 2000/12/18 22:42:53 uzadow Exp $
//  Copyright (c) 1996-1998 Ulrich von Zadow

	// Negative the angle, because the y-axis is negative.
	double ang = -angle*acos((float)0)/90;
	int newWidth, newHeight;
	int nWidth = GetWidth();
	int nHeight= GetHeight();
	double cos_angle = cos(ang);
	double sin_angle = sin(ang);

	// Calculate the size of the new bitmap
	POINT p1={0,0};
	POINT p2={nWidth,0};
	POINT p3={0,nHeight};
	POINT p4={nWidth-1,nHeight};
	POINT newP1,newP2,newP3,newP4, leftTop, rightTop, leftBottom, rightBottom;

	newP1.x = p1.x;
	newP1.y = p1.y;
	newP2.x = (long)floor(p2.x*cos_angle - p2.y*sin_angle);
	newP2.y = (long)floor(p2.x*sin_angle + p2.y*cos_angle);
	newP3.x = (long)floor(p3.x*cos_angle - p3.y*sin_angle);
	newP3.y = (long)floor(p3.x*sin_angle + p3.y*cos_angle);
	newP4.x = (long)floor(p4.x*cos_angle - p4.y*sin_angle);
	newP4.y = (long)floor(p4.x*sin_angle + p4.y*cos_angle);

	leftTop.x = min(min(newP1.x,newP2.x),min(newP3.x,newP4.x));
	leftTop.y = min(min(newP1.y,newP2.y),min(newP3.y,newP4.y));
	rightBottom.x = max(max(newP1.x,newP2.x),max(newP3.x,newP4.x));
	rightBottom.y = max(max(newP1.y,newP2.y),max(newP3.y,newP4.y));
	leftBottom.x = leftTop.x;
	leftBottom.y = 2+rightBottom.y;
	rightTop.x = 2+rightBottom.x;
	rightTop.y = leftTop.y;

	newWidth = rightTop.x - leftTop.x;
	newHeight= leftBottom.y - leftTop.y;
	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(newWidth,newHeight,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

#if CXIMAGE_SUPPORT_ALPHA
	if(AlphaIsValid())	//MTA: Fix for rotation problem when the image has an alpha channel
	{
		imgDest.AlphaCreate();
		imgDest.AlphaClear();
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	int x,y,newX,newY,oldX,oldY;

	if (head.biClrUsed==0){ //RGB
		for (y = leftTop.y, newY = 0; y<=leftBottom.y; y++,newY++){
			info.nProgress = (long)(100*newY/newHeight);
			if (info.nEscape) break;
			for (x = leftTop.x, newX = 0; x<=rightTop.x; x++,newX++){
				oldX = (long)(x*cos_angle + y*sin_angle - 0.5);
				oldY = (long)(y*cos_angle - x*sin_angle - 0.5);
				imgDest.SetPixelColor(newX,newY,GetPixelColor(oldX,oldY));
#if CXIMAGE_SUPPORT_ALPHA
				imgDest.AlphaSet(newX,newY,AlphaGet(oldX,oldY));				//MTA: copy the alpha value
#endif //CXIMAGE_SUPPORT_ALPHA
			}
		}
	} else { //PALETTE
		for (y = leftTop.y, newY = 0; y<=leftBottom.y; y++,newY++){
			info.nProgress = (long)(100*newY/newHeight);
			if (info.nEscape) break;
			for (x = leftTop.x, newX = 0; x<=rightTop.x; x++,newX++){
				oldX = (long)(x*cos_angle + y*sin_angle - 0.5);
				oldY = (long)(y*cos_angle - x*sin_angle - 0.5);
				imgDest.SetPixelIndex(newX,newY,GetPixelIndex(oldX,oldY));
#if CXIMAGE_SUPPORT_ALPHA
				imgDest.AlphaSet(newX,newY,AlphaGet(oldX,oldY));				//MTA: copy the alpha value
#endif //CXIMAGE_SUPPORT_ALPHA
			}
		}
	}
	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CxImage::Rotate180(CxImage* iDst)
{
	if (!pDib) return false;

	long wid = GetWidth();
	long ht = GetHeight();

	CxImage imgDest;
	imgDest.CopyInfo(*this);
	imgDest.Create(wid,ht,GetBpp(),GetType());
	imgDest.SetPalette(GetPalette());

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid())	imgDest.AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA

	long x,y,y2;
	for (y = 0; y < ht; y++){
		info.nProgress = (long)(100*y/ht); //<Anatoly Ivasyuk>
		y2=ht-y-1;
		for (x = 0; x < wid; x++){
			if(head.biClrUsed==0)//RGB
				imgDest.SetPixelColor(wid-x-1, y2, GetPixelColor(x, y));
			else  //PALETTE
				imgDest.SetPixelIndex(wid-x-1, y2, GetPixelIndex(x, y));

#if CXIMAGE_SUPPORT_ALPHA
			if (AlphaIsValid())	imgDest.AlphaSet(wid-x-1, y2,AlphaGet(x, y));
#endif //CXIMAGE_SUPPORT_ALPHA

		}
	}

	//select the destination
	if (iDst) iDst->Transfer(imgDest);
	else Transfer(imgDest);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool CxImage::Resample(long newx, long newy, int mode, CxImage* iDst)
{
	if (newx==0 || newy==0) return false;

	if (head.biWidth==newx && head.biHeight==newy){
		if (iDst) iDst->Copy(*this);
		return true;
	}

	float xScale, yScale, fX, fY;
	xScale = (float)head.biWidth  / (float)newx;
	yScale = (float)head.biHeight / (float)newy;

	CxImage newImage;
	newImage.CopyInfo(*this);
	newImage.Create(newx,newy,head.biBitCount,GetType());
	newImage.SetPalette(GetPalette());
	if (!newImage.IsValid()) return false;

	if (head.biWidth==newx && head.biHeight==newy){
		Transfer(newImage);
		return true;
	}

	switch (mode) {
	case 1: // nearest pixel
	{ 
		for(long y=0; y<newy; y++){
			info.nProgress = (long)(100*y/newy);
			if (info.nEscape) break;
			fY = y * yScale;
			for(long x=0; x<newx; x++){
				fX = x * xScale;
				newImage.SetPixelColor(x,y,GetPixelColor((long)fX,(long)fY));
			}
		}
		break;
	}
	case 2: // bicubic interpolation by Blake L. Carlson <blake-carlson(at)uiowa(dot)edu
	{
		float f_x, f_y, a, b, rr, gg, bb, r1, r2;
		int   i_x, i_y, xx, yy;
		RGBQUAD rgb;
		BYTE* iDst;
		for(long y=0; y<newy; y++){
			info.nProgress = (long)(100*y/newy);
			if (info.nEscape) break;
			f_y = (float) y * yScale - 0.5f;
			i_y = (int) floor(f_y);
			a   = f_y - (float)floor(f_y);
			for(long x=0; x<newx; x++){
				f_x = (float) x * xScale - 0.5f;
				i_x = (int) floor(f_x);
				b   = f_x - (float)floor(f_x);

				rr = gg = bb = 0.0f;
				for(int m=-1; m<3; m++) {
					r1 = b3spline((float) m - a);
					yy = i_y+m;
					if (yy<0) yy=0;
					if (yy>=head.biHeight) yy = head.biHeight-1;
					for(int n=-1; n<3; n++) {
						r2 = r1 * b3spline(b - (float)n);
						xx = i_x+n;
						if (xx<0) xx=0;
						if (xx>=head.biWidth) xx=head.biWidth-1;

						if (head.biClrUsed){
							rgb = GetPixelColor(xx,yy);
						} else {
							iDst  = info.pImage + yy*info.dwEffWidth + xx*3;
							rgb.rgbBlue = *iDst++;
							rgb.rgbGreen= *iDst++;
							rgb.rgbRed  = *iDst;
						}

						rr += rgb.rgbRed * r2;
						gg += rgb.rgbGreen * r2;
						bb += rgb.rgbBlue * r2;
					}
				}

				if (head.biClrUsed)
					newImage.SetPixelColor(x,y,RGB(rr,gg,bb));
				else {
					iDst = newImage.info.pImage + y*newImage.info.dwEffWidth + x*3;
					*iDst++ = (BYTE)bb;
					*iDst++ = (BYTE)gg;
					*iDst   = (BYTE)rr;
				}

			}
		}
		break;
	}
	default: // bilinear interpolation
		if (!(head.biWidth>newx && head.biHeight>newy && head.biBitCount==24)) {
			//© 1999 Steve McMahon (steve@dogma.demon.co.uk)
			long ifX, ifY, ifX1, ifY1, xmax, ymax;
			float ir1, ir2, ig1, ig2, ib1, ib2, dx, dy;
			BYTE r,g,b;
			RGBQUAD rgb1, rgb2, rgb3, rgb4;
			xmax = head.biWidth-1;
			ymax = head.biHeight-1;
			for(long y=0; y<newy; y++){
				info.nProgress = (long)(100*y/newy);
				if (info.nEscape) break;
				fY = y * yScale;
				ifY = (int)fY;
				ifY1 = min(ymax, ifY+1);
				dy = fY - ifY;
				for(long x=0; x<newx; x++){
					fX = x * xScale;
					ifX = (int)fX;
					ifX1 = min(xmax, ifX+1);
					dx = fX - ifX;
					// Interpolate using the four nearest pixels in the source
					if (head.biClrUsed){
						rgb1=GetPaletteColor(GetPixelIndex(ifX,ifY));
						rgb2=GetPaletteColor(GetPixelIndex(ifX1,ifY));
						rgb3=GetPaletteColor(GetPixelIndex(ifX,ifY1));
						rgb4=GetPaletteColor(GetPixelIndex(ifX1,ifY1));
					}
					else {
						BYTE* iDst;
						iDst = info.pImage + ifY*info.dwEffWidth + ifX*3;
						rgb1.rgbBlue = *iDst++;	rgb1.rgbGreen= *iDst++;	rgb1.rgbRed =*iDst;
						iDst = info.pImage + ifY*info.dwEffWidth + ifX1*3;
						rgb2.rgbBlue = *iDst++;	rgb2.rgbGreen= *iDst++;	rgb2.rgbRed =*iDst;
						iDst = info.pImage + ifY1*info.dwEffWidth + ifX*3;
						rgb3.rgbBlue = *iDst++;	rgb3.rgbGreen= *iDst++;	rgb3.rgbRed =*iDst;
						iDst = info.pImage + ifY1*info.dwEffWidth + ifX1*3;
						rgb4.rgbBlue = *iDst++;	rgb4.rgbGreen= *iDst++;	rgb4.rgbRed =*iDst;
					}
					// Interplate in x direction:
					ir1 = rgb1.rgbRed   * (1 - dy) + rgb3.rgbRed   * dy;
					ig1 = rgb1.rgbGreen * (1 - dy) + rgb3.rgbGreen * dy;
					ib1 = rgb1.rgbBlue  * (1 - dy) + rgb3.rgbBlue  * dy;
					ir2 = rgb2.rgbRed   * (1 - dy) + rgb4.rgbRed   * dy;
					ig2 = rgb2.rgbGreen * (1 - dy) + rgb4.rgbGreen * dy;
					ib2 = rgb2.rgbBlue  * (1 - dy) + rgb4.rgbBlue  * dy;
					// Interpolate in y:
					r = (BYTE)(ir1 * (1 - dx) + ir2 * dx);
					g = (BYTE)(ig1 * (1 - dx) + ig2 * dx);
					b = (BYTE)(ib1 * (1 - dx) + ib2 * dx);
					// Set output
					newImage.SetPixelColor(x,y,RGB(r,g,b));
				}
			} 
		} else {
			//high resolution shrink, thanks to Henrik Stellmann <henrik.stellmann@volleynet.de>
			const long ACCURACY = 1000;
			long i,j; // index for faValue
			long x,y; // coordinates in  source image
			BYTE* pSource;
			BYTE* pDest = newImage.info.pImage;
			long* naAccu  = new long[3 * newx + 3];
			long* naCarry = new long[3 * newx + 3];
			long* naTemp;
			long  nWeightX,nWeightY;
			float fEndX;
			long nScale = (long)(ACCURACY * xScale * yScale);

			memset(naAccu,  0, sizeof(long) * 3 * newx);
			memset(naCarry, 0, sizeof(long) * 3 * newx);

			int u, v = 0; // coordinates in dest image
			float fEndY = yScale - 1.0f;
			for (y = 0; y < head.biHeight; y++){
				info.nProgress = (long)(100*y/head.biHeight); //<Anatoly Ivasyuk>
				if (info.nEscape) break;
				pSource = info.pImage + y * info.dwEffWidth;
				u = i = 0;
				fEndX = xScale - 1.0f;
				if ((float)y < fEndY) {       // complete source row goes into dest row
					for (x = 0; x < head.biWidth; x++){
						if ((float)x < fEndX){       // complete source pixel goes into dest pixel
							for (j = 0; j < 3; j++)	naAccu[i + j] += (*pSource++) * ACCURACY;
						} else {       // source pixel is splitted for 2 dest pixels
							nWeightX = (long)(((float)x - fEndX) * ACCURACY);
							for (j = 0; j < 3; j++){
								naAccu[i] += (ACCURACY - nWeightX) * (*pSource);
								naAccu[3 + i++] += nWeightX * (*pSource++);
							}
							fEndX += xScale;
							u++;
						}
					}
				} else {       // source row is splitted for 2 dest rows       
					nWeightY = (long)(((float)y - fEndY) * ACCURACY);
					for (x = 0; x < head.biWidth; x++){
						if ((float)x < fEndX){       // complete source pixel goes into 2 pixel
							for (j = 0; j < 3; j++){
								naAccu[i + j] += ((ACCURACY - nWeightY) * (*pSource));
								naCarry[i + j] += nWeightY * (*pSource++);
							}
						} else {       // source pixel is splitted for 4 dest pixels
							nWeightX = (int)(((float)x - fEndX) * ACCURACY);
							for (j = 0; j < 3; j++) {
								naAccu[i] += ((ACCURACY - nWeightY) * (ACCURACY - nWeightX)) * (*pSource) / ACCURACY;
								*pDest++ = (BYTE)(naAccu[i] / nScale);
								naCarry[i] += (nWeightY * (ACCURACY - nWeightX) * (*pSource)) / ACCURACY;
								naAccu[i + 3] += ((ACCURACY - nWeightY) * nWeightX * (*pSource)) / ACCURACY;
								naCarry[i + 3] = (nWeightY * nWeightX * (*pSource)) / ACCURACY;
								i++;
								pSource++;
							}
							fEndX += xScale;
							u++;
						}
					}
					if (u < newx){ // possibly not completed due to rounding errors
						for (j = 0; j < 3; j++) *pDest++ = (BYTE)(naAccu[i++] / nScale);
					}
					naTemp = naCarry;
					naCarry = naAccu;
					naAccu = naTemp;
					memset(naCarry, 0, sizeof(int) * 3);    // need only to set first pixel zero
					pDest = newImage.info.pImage + (++v * newImage.info.dwEffWidth);
					fEndY += yScale;
				}
			}
			if (v < newy){	// possibly not completed due to rounding errors
				for (i = 0; i < 3 * newx; i++) *pDest++ = (BYTE)(naAccu[i] / nScale);
			}
			delete [] naAccu;
			delete [] naCarry;
		}
	}

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){
		newImage.AlphaCreate();
		for(long y=0; y<newy; y++){
			fY = y * yScale;
			for(long x=0; x<newx; x++){
				fX = x * xScale;
				newImage.AlphaSet(x,y,AlphaGet((long)fX,(long)fY));
			}
		}
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	//select the destination
	if (iDst) iDst->Transfer(newImage);
	else Transfer(newImage);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::DecreaseBpp(DWORD nbit, bool errordiffusion, RGBQUAD* ppal, DWORD clrimportant)
{
	if (!pDib) return false;
	if (head.biBitCount <  nbit) return false;
	if (head.biBitCount == nbit){
		if (clrimportant==0) return true;
		if (head.biClrImportant && (head.biClrImportant<clrimportant)) return true;
	}

	long er,eg,eb;
	RGBQUAD c,ce;

	CxImage tmp;
	tmp.CopyInfo(*this);
	tmp.Create(head.biWidth,head.biHeight,(WORD)nbit,info.dwType);
	if (clrimportant) tmp.SetClrImportant(clrimportant);

#if CXIMAGE_SUPPORT_SELECTION
	tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
	tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

	switch (tmp.head.biBitCount){
	case 1:
		if (ppal) tmp.SetPalette(ppal,2);
		else {
			tmp.SetPaletteColor(0,0,0,0);
			tmp.SetPaletteColor(1,255,255,255);
		}
		break;
	case 4:
		if (ppal) tmp.SetPalette(ppal,16);
		else tmp.SetStdPalette();
		break;
	case 8:
		if (ppal) tmp.SetPalette(ppal);
		else tmp.SetStdPalette();
		break;
	default:
		return false;
	}

	for (long y=0;y<head.biHeight;y++){
		if (info.nEscape) break;
		info.nProgress = (long)(100*y/head.biHeight);
		for (long x=0;x<head.biWidth;x++){
			if (!errordiffusion){
				tmp.SetPixelColor(x,y,GetPixelColor(x,y));
			} else {
				c=GetPixelColor(x,y);
				tmp.SetPixelColor(x,y,c);

				ce=tmp.GetPixelColor(x,y);
				er=(long)c.rgbRed - (long)ce.rgbRed;
				eg=(long)c.rgbGreen - (long)ce.rgbGreen;
				eb=(long)c.rgbBlue - (long)ce.rgbBlue;

				c = GetPixelColor(x+1,y);
				c.rgbRed = (BYTE)min(255L,max(0L,(long)c.rgbRed + ((er*7)/16)));
				c.rgbGreen = (BYTE)min(255L,max(0L,(long)c.rgbGreen + ((eg*7)/16)));
				c.rgbBlue = (BYTE)min(255L,max(0L,(long)c.rgbBlue + ((eb*7)/16)));
				SetPixelColor(x+1,y,c);
				int coeff;
				for(int i=-1; i<2; i++){
					switch(i){
					case -1:
						coeff=2; break;
					case 0:
						coeff=4; break;
					case 1:
						coeff=1; break;
					}
					c = GetPixelColor(x+i,y+1);
					c.rgbRed = (BYTE)min(255L,max(0L,(long)c.rgbRed + ((er * coeff)/16)));
					c.rgbGreen = (BYTE)min(255L,max(0L,(long)c.rgbGreen + ((eg * coeff)/16)));
					c.rgbBlue = (BYTE)min(255L,max(0L,(long)c.rgbBlue + ((eb * coeff)/16)));
					SetPixelColor(x+i,y+1,c);
				}
			}
		}
	}

	if (head.biBitCount==1){
		tmp.SetPaletteColor(0,0,0,0);
		tmp.SetPaletteColor(1,255,255,255);
	}

	Transfer(tmp);
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::IncreaseBpp(DWORD nbit)
{
	if (!pDib) return false;
	switch (nbit){
	case 4:
		{
			if (head.biBitCount==4) return true;
			if (head.biBitCount>4) return false;

			CxImage tmp;
			tmp.CopyInfo(*this);
			tmp.Create(head.biWidth,head.biHeight,4,info.dwType);
			tmp.SetPalette(GetPalette(),GetNumColors());


#if CXIMAGE_SUPPORT_SELECTION
			tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
			tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

			for (long y=0;y<head.biHeight;y++){
				if (info.nEscape) break;
				for (long x=0;x<head.biWidth;x++){
					tmp.SetPixelIndex(x,y,GetPixelIndex(x,y));
				}
			}
			Transfer(tmp);
			return true;
		}
	case 8:
		{
			if (head.biBitCount==8) return true;
			if (head.biBitCount>8) return false;

			CxImage tmp;
			tmp.CopyInfo(*this);
			tmp.Create(head.biWidth,head.biHeight,8,info.dwType);
			tmp.SetPalette(GetPalette(),GetNumColors());

#if CXIMAGE_SUPPORT_SELECTION
			tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
			tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

			for (long y=0;y<head.biHeight;y++){
				if (info.nEscape) break;
				for (long x=0;x<head.biWidth;x++){
					tmp.SetPixelIndex(x,y,GetPixelIndex(x,y));
				}
			}
			Transfer(tmp);
			return true;
		}
	case 24:
		{
			if (head.biBitCount==24) return true;
			if (head.biBitCount>24) return false;

			CxImage tmp;
			tmp.CopyInfo(*this);
			tmp.Create(head.biWidth,head.biHeight,24,info.dwType);

			if (info.nBkgndIndex>=0) //translate transparency
				tmp.info.nBkgndColor=GetPaletteColor((BYTE)info.nBkgndIndex);

#if CXIMAGE_SUPPORT_SELECTION
			tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
			tmp.AlphaCopy(*this);
			if (AlphaPaletteIsValid() && !AlphaIsValid()) tmp.AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA

			for (long y=0;y<head.biHeight;y++){
				if (info.nEscape) break;
				for (long x=0;x<head.biWidth;x++){
					tmp.SetPixelColor(x,y,GetPixelColor(x,y),true);
				}
			}
			Transfer(tmp);
			return true;
		}
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Dither(long method)
{
	if (!pDib) return false;
	if (head.biBitCount == 1) return true;
	
	switch (method){
	case 1:
	{
		// Multi-Level Ordered-Dithering by Kenny Hoff (Oct. 12, 1995)
		#define NumRows 4
		#define NumCols 4
		#define NumIntensityLevels 2
		#define NumRowsLessOne (NumRows-1)
		#define NumColsLessOne (NumCols-1)
		#define RowsXCols (NumRows*NumCols)
		#define MaxIntensityVal 255
		#define MaxDitherIntensityVal (NumRows*NumCols*(NumIntensityLevels-1))

		int DitherMatrix[NumRows][NumCols] = {{0,8,2,10}, {12,4,14,6}, {3,11,1,9}, {15,7,13,5} };
		
		unsigned char Intensity[NumIntensityLevels] = { 0,1 };                       // 2 LEVELS B/W
		//unsigned char Intensity[NumIntensityLevels] = { 0,255 };                       // 2 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,127,255 };                   // 3 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,85,170,255 };                // 4 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,63,127,191,255 };            // 5 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,51,102,153,204,255 };        // 6 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,42,85,127,170,213,255 };     // 7 LEVELS
		//unsigned char Intensity[NumIntensityLevels] = { 0,36,73,109,145,182,219,255 }; // 8 LEVELS
		int DitherIntensity, DitherMatrixIntensity, Offset, DeviceIntensity;
		unsigned char DitherValue;
  
		GrayScale();

		CxImage tmp(head.biWidth,head.biHeight,1,info.dwType);

#if CXIMAGE_SUPPORT_SELECTION
			tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
			tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

		for (long y=0;y<head.biHeight;y++){
			info.nProgress = (long)(100*y/head.biHeight);
			if (info.nEscape) break;
			for (long x=0;x<head.biWidth;x++){

				DeviceIntensity = GetPixelIndex(x,y);
				DitherIntensity = DeviceIntensity*MaxDitherIntensityVal/MaxIntensityVal;
				DitherMatrixIntensity = DitherIntensity % RowsXCols;
				Offset = DitherIntensity / RowsXCols;
				if (DitherMatrix[y&NumRowsLessOne][x&NumColsLessOne] < DitherMatrixIntensity)
					DitherValue = Intensity[1+Offset];
				else
					DitherValue = Intensity[0+Offset];

				tmp.SetPixelIndex(x,y,DitherValue);
			}
		}
		tmp.SetPaletteColor(0,0,0,0);
		tmp.SetPaletteColor(1,255,255,255);
		Transfer(tmp);
		break;
	}
	default:
	{
		// Floyd-Steinberg error diffusion (Thanks to Steve McMahon)
		long error,nlevel,coeff;
		BYTE level;

		GrayScale();

		CxImage tmp(head.biWidth,head.biHeight,1,info.dwType);

#if CXIMAGE_SUPPORT_SELECTION
		tmp.SelectionCopy(*this);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
		tmp.AlphaCopy(*this);
#endif //CXIMAGE_SUPPORT_ALPHA

		for (long y=0;y<head.biHeight;y++){
			info.nProgress = (long)(100*y/head.biHeight);
			if (info.nEscape) break;
			for (long x=0;x<head.biWidth;x++){

				level=GetPixelIndex(x,y);
				if (level > 128){
					tmp.SetPixelIndex(x,y,1);
					error = level-255;
				} else {
					tmp.SetPixelIndex(x,y,0);
					error = level;
				}

				nlevel = GetPixelIndex(x+1,y) + (error * 7)/16;
				level = (BYTE)min(255,max(0,(int)nlevel));
				SetPixelIndex(x+1,y,level);
				for(int i=-1; i<2; i++){
					switch(i){
					case -1:
						coeff=3; break;
					case 0:
						coeff=5; break;
					case 1:
						coeff=1; break;
					}
					nlevel = GetPixelIndex(x+i,y+1) + (error * coeff)/16;
					level = (BYTE)min(255,max(0,(int)nlevel));
					SetPixelIndex(x+i,y+1,level);
				}
			}
		}
		tmp.SetPaletteColor(0,0,0,0);
		tmp.SetPaletteColor(1,255,255,255);
		Transfer(tmp);
	}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Crop(long left, long top, long right, long bottom, CxImage* iDst)
{
	if (!pDib) return false;

	long startx = max(0L,min(left,head.biWidth));
	long endx = max(0L,min(right,head.biWidth));
	long starty = head.biHeight - max(0L,min(top,head.biHeight));
	long endy = head.biHeight - max(0L,min(bottom,head.biHeight));

	if (startx==endx || starty==endy) return false;

	if (startx>endx) {long tmp=startx; startx=endx; endx=tmp;}
	if (starty>endy) {long tmp=starty; starty=endy; endy=tmp;}

	CxImage tmp(endx-startx,endy-starty,head.biBitCount,info.dwType);
	tmp.SetPalette(GetPalette(),head.biClrUsed);
	tmp.info.nBkgndIndex = info.nBkgndIndex;
	tmp.info.nBkgndColor = info.nBkgndColor;

	switch (head.biBitCount) {
	case 1:
	case 4:
	{
		for(long y=starty, yd=0; y<endy; y++, yd++){
			info.nProgress = (long)(100*y/endy); //<Anatoly Ivasyuk>
			for(long x=startx, xd=0; x<endx; x++, xd++){
				tmp.SetPixelIndex(xd,yd,GetPixelIndex(x,y));
			}
		}
		break;
	}
	case 8:
	case 24:
	{
		BYTE* pDest = tmp.info.pImage;
		BYTE* pSrc = info.pImage + starty * info.dwEffWidth + (startx*head.biBitCount >> 3);
		for(long y=starty; y<endy; y++){
			info.nProgress = (long)(100*y/endy); //<Anatoly Ivasyuk>
			memcpy(pDest,pSrc,tmp.info.dwEffWidth);
			pDest+=tmp.info.dwEffWidth;
			pSrc+=info.dwEffWidth;
		}
    }
	}

#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){ //<oboolo>
		tmp.AlphaCreate();
		BYTE* pDest = tmp.pAlpha;
		BYTE* pSrc = pAlpha + startx + starty*head.biWidth;
		for (long y=starty; y<endy; y++){
			memcpy(pDest,pSrc,endx-startx);
			pDest+=tmp.head.biWidth;
			pSrc+=head.biWidth;
		}
	}
#endif //CXIMAGE_SUPPORT_ALPHA

	//select the destination
	if (iDst) iDst->Transfer(tmp);
	else Transfer(tmp);

	return true;
}
////////////////////////////////////////////////////////////////////////////////
float CxImage::b3spline(float x)
{
	// thanks to Kristian Kratzenstein
	float a, b, c, d;
	float xm1 = x - 1.0f; // Was calculatet anyway cause the "if((x-1.0f) < 0)"
	float xp1 = x + 1.0f;
	float xp2 = x + 2.0f;

	if ((xp2) <= 0.0f) a = 0.0f; else a = xp2*xp2*xp2; // Only float, not float -> double -> float
	if ((xp1) <= 0.0f) b = 0.0f; else b = xp1*xp1*xp1;
	if (x <= 0) c = 0.0f; else c = x*x*x;  
	if ((xm1) <= 0.0f) d = 0.0f; else d = xm1*xm1*xm1;

	return (0.16666666666666666667f * (a - (4.0f * b) + (6.0f * c) - (4.0f * d)));
}

////////////////////////////////////////////////////////////////////////////////
bool CxImage::Skew(float xgain, float ygain, long xpivot, long ypivot)
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
				nx = x + (long)(xgain*(y - ypivot));
				ny = y + (long)(ygain*(x - xpivot));
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
bool CxImage::Expand(long left, long top, long right, long bottom, RGBQUAD canvascolor, CxImage* iDst)
{
	//thanks to <Colin Urquhart>

    if (!pDib) return false;

    if ((left < 0) || (right < 0) || (bottom < 0) || (top < 0)) return false;

    long newWidth = head.biWidth + left + right;
    long newHeight = head.biHeight + top + bottom;

    right = left + head.biWidth - 1;
    top = bottom + head.biHeight - 1;
    
    CxImage tmp(newWidth, newHeight, head.biBitCount, info.dwType);

    tmp.SetPalette(GetPalette(),head.biClrUsed);
    tmp.info.nBkgndIndex = info.nBkgndIndex;
    tmp.info.nBkgndColor = info.nBkgndColor;

    switch (head.biBitCount) {
    case 1:
    case 4:
    {
        BYTE pixel = tmp.GetNearestIndex(canvascolor);
        for(long y=0; y < newHeight; y++){
            info.nProgress = (long)(100*y/newHeight); //
            for(long x=0; x < newWidth; x++){
                if ((y < bottom) || (y > top) || (x < left) || (x > right)) {
                    tmp.SetPixelIndex(x,y, pixel);
                } else {
                    tmp.SetPixelIndex(x,y,GetPixelIndex(x-left,y-bottom));
                }
            }
        }
        break;
    }
    case 8:
    case 24:
    {
        if (head.biBitCount == 8) {
            BYTE pixel = tmp.GetNearestIndex( canvascolor);
            memset(tmp.info.pImage, pixel,  + (tmp.info.dwEffWidth * newHeight));
        } else {
            for (long y = 0; y < newHeight; ++y) {
                BYTE *pDest = tmp.info.pImage + (y * tmp.info.dwEffWidth);
                for (long x = 0; x < newWidth; ++x) {
                    *pDest++ = canvascolor.rgbBlue;
                    *pDest++ = canvascolor.rgbGreen;
                    *pDest++ = canvascolor.rgbRed;
                }
            }
        }

        BYTE* pDest = tmp.info.pImage + (tmp.info.dwEffWidth * bottom) + (left*(head.biBitCount >> 3));
        BYTE* pSrc = info.pImage;
        for(long y=bottom; y <= top; y++){
            info.nProgress = (long)(100*y/(1 + top - bottom));
            memcpy(pDest,pSrc,(head.biBitCount >> 3) * (right - left + 1));
            pDest+=tmp.info.dwEffWidth;
            pSrc+=info.dwEffWidth;
        }
    }
    }
    //select the destination
    if (iDst) iDst->Transfer(tmp);
    else Transfer(tmp);

    return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Expand(long newx, long newy, RGBQUAD canvascolor, CxImage* iDst)
{
	//thanks to <Colin Urquhart>

    if (!pDib) return false;

    if ((newx < head.biWidth) || (newy < head.biHeight)) return false;

    int nAddLeft = (newx - head.biWidth) / 2;
    int nAddTop = (newy - head.biHeight) / 2;

    return Expand(nAddLeft, nAddTop, newx - (head.biWidth + nAddLeft), newy - (head.biHeight + nAddTop), canvascolor, iDst);
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::Thumbnail(long newx, long newy, RGBQUAD canvascolor, CxImage* iDst)
{
	//thanks to <Colin Urquhart>

    if (!pDib) return false;

    if ((newx <= 0) || (newy <= 0)) return false;

    CxImage tmp(*this);

    // determine whether we need to shrink the image
    if ((head.biWidth > newx) || (head.biHeight > newy)) {
        float fScale;
        float fAspect = (float) newx / (float) newy;
        if (fAspect * head.biHeight > head.biWidth) {
            fScale = (float) newy / head.biHeight;
        } else {
            fScale = (float) newx / head.biWidth;
        }
        tmp.Resample((long) (fScale * head.biWidth), (long) (fScale * head.biHeight), 0);
    }

    // expand the frame
    tmp.Expand(newx, newy, canvascolor, iDst);

    //select the destination
    if (iDst) iDst->Transfer(tmp);
    else Transfer(tmp);
    return true;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_TRANSFORMATION
