// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
// xImaWnd.cpp : Windows functions
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximage.h"


////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_WINCE
////////////////////////////////////////////////////////////////////////////////
long CxImage::Blt(HDC pDC, long x, long y)
{
	if((pDib==0)||(pDC==0)||(!info.bEnabled)) return 0;

    HBRUSH brImage = CreateDIBPatternBrushPt(pDib, DIB_RGB_COLORS);
    HBRUSH brOld = (HBRUSH) SelectObject(pDC, brImage);
    PatBlt(pDC, 0, 0, head.biWidth, head.biHeight, PATCOPY);
    SelectObject(pDC, brOld);
    DeleteObject(brImage);
	return 1;
}
#endif //CXIMAGE_SUPPORT_WINCE
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_WINDOWS
////////////////////////////////////////////////////////////////////////////////
// Transfer the image in a global bitmap handle (clipboard copy)
HANDLE CxImage::CopyToHandle()
{
	HANDLE hMem=NULL;
	if (pDib){
		hMem= GlobalAlloc(GHND, GetSize());
		BYTE* pDst=(BYTE*)GlobalLock(hMem);
		memcpy(pDst,pDib,GetSize());
		GlobalUnlock(hMem);
	}
	return hMem;
}
////////////////////////////////////////////////////////////////////////////////
// Global object (clipboard paste) constructor
// § the clipboard format must be CF_DIB.
// > hMem: source bitmap object
bool CxImage::CreateFromHANDLE(HANDLE hMem)
{
	if (pDib) {free(pDib); pDib=NULL;}

	BYTE *lpVoid;						//pointer to the bitmap
	lpVoid = (BYTE *)GlobalLock(hMem);
	BITMAPINFOHEADER *pHead;			//pointer to the bitmap header
	pHead = (BITMAPINFOHEADER *)lpVoid;
	if (lpVoid){
		//copy the bitmap header
		memcpy(&head,pHead,sizeof(BITMAPINFOHEADER));
		//create the image
		if(!Create(head.biWidth,head.biHeight,head.biBitCount)){
			GlobalUnlock(lpVoid);
			return false;
		}
		//preserve DPI
		if (head.biXPelsPerMeter) SetXDPI((long)floor(head.biXPelsPerMeter * 254.0 / 10000.0 + 0.5)); else SetXDPI(96);
		if (head.biYPelsPerMeter) SetYDPI((long)floor(head.biYPelsPerMeter * 254.0 / 10000.0 + 0.5)); else SetYDPI(96);
		//copy the pixels
		if((pHead->biCompression != BI_RGB) || (pHead->biBitCount == 32)){ //<Jörgen Alfredsson>
			// BITFIELD case
			// set the internal header in the dib
			memcpy(pDib,&head,sizeof(head));
			// get the bitfield masks
			DWORD bf[3];
			memcpy(bf,lpVoid+pHead->biSize,12);
			// transform into RGB
			Bitfield2RGB(lpVoid+pHead->biSize+12,(WORD)bf[0],(WORD)bf[1],(WORD)bf[2],(BYTE)pHead->biBitCount);
		} else { //normal bitmap
			memcpy(pDib,lpVoid,GetSize());
		}
		GlobalUnlock(lpVoid);
		return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////
// Transfer the image in a  bitmap handle
HBITMAP CxImage::MakeBitmap(HDC hdc)
{
	if (!pDib)
		return NULL;

	if (!hdc){
		// this call to CreateBitmap doesn't create a DIB <jaslet>
		// // Create a device-independent bitmap <CSC>
		//  return CreateBitmap(head.biWidth,head.biHeight,	1, head.biBitCount, GetBits());
		// use instead this code
		HDC hMemDC = CreateCompatibleDC(NULL);
		LPVOID pBit32;
		HBITMAP bmp = CreateDIBSection(hMemDC,(LPBITMAPINFO)pDib,DIB_RGB_COLORS, &pBit32, NULL, 0);
		if (pBit32) memcpy(pBit32, GetBits(), head.biSizeImage);
		DeleteDC(hMemDC);
		return bmp;
	}

	// this single line seems to work very well
	HBITMAP bmp = CreateDIBitmap(hdc, (LPBITMAPINFOHEADER)pDib, CBM_INIT,
		GetBits(), (LPBITMAPINFO)pDib, DIB_RGB_COLORS);

	return bmp;
}
////////////////////////////////////////////////////////////////////////////////
// Bitmap resource constructor
// > hbmp: bitmap resource handle
void CxImage::CreateFromHBITMAP(HBITMAP hbmp, HPALETTE hpal)
{
	if (pDib) {free(pDib); pDib=NULL;}

	if (hbmp) { 
        BITMAP bm;
		// get informations about the bitmap
        GetObject(hbmp, sizeof(BITMAP), (LPSTR) &bm);
		// create the image
        Create(bm.bmWidth, bm.bmHeight, bm.bmBitsPixel, 0);
		// create a device context for the bitmap
        HDC dc = ::GetDC(NULL);

		if (hpal){
			SelectObject(dc,hpal); //the palette you should get from the user or have a stock one
			RealizePalette(dc);
		}

		// copy the pixels
        if (GetDIBits(dc, hbmp, 0, head.biHeight, info.pImage,
			(LPBITMAPINFO)pDib, DIB_RGB_COLORS) == 0){ //replace &head with pDib <Wil Stark>
            strcpy(info.szLastError,"GetDIBits failed");
			::ReleaseDC(NULL, dc);
			return;
        }
        ::ReleaseDC(NULL, dc);
    }
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::CreateFromHICON(HICON hico)
{
	Destroy();
	if (hico) { 
		ICONINFO iinfo;
		GetIconInfo(hico,&iinfo);
		CreateFromHBITMAP(iinfo.hbmColor);
#if CXIMAGE_SUPPORT_ALPHA
		CxImage mask;
		mask.CreateFromHBITMAP(iinfo.hbmMask);
		mask.GrayScale();
		mask.Negative();
		AlphaSet(mask);
#endif
		DeleteObject(iinfo.hbmColor); //<Sims>
		DeleteObject(iinfo.hbmMask);  //<Sims>
    }
}
////////////////////////////////////////////////////////////////////////////////
// Draws (stretch) the image with transparency & alpha support
// > hdc: destination device context
// > x,y: (optional) offset
// > cx,cy: (optional) size.
long CxImage::Draw(HDC hdc, long x, long y, long cx, long cy, RECT* pClipRect)
{
	if((pDib==0)||(hdc==0)||(cx==0)||(cy==0)||(!info.bEnabled)) return 0;

	if (cx < 0) cx = head.biWidth;
	if (cy < 0) cy = head.biHeight;
	bool bTransparent = info.nBkgndIndex != -1;
	bool bAlpha = pAlpha != 0;

	RECT mainbox; // (experimental) 
	if (pClipRect){
		GetClipBox(hdc,&mainbox);
		HRGN rgn = CreateRectRgnIndirect(pClipRect);
		ExtSelectClipRgn(hdc,rgn,RGN_AND);
		DeleteObject(rgn);
	}

	if (!(bTransparent || bAlpha || info.bAlphaPaletteEnabled)){
		if (cx==head.biWidth && cy==head.biHeight){ //NORMAL
			SetStretchBltMode(hdc,COLORONCOLOR);	
			SetDIBitsToDevice(hdc, x, y, cx, cy, 0, 0, 0, cy,
						info.pImage,(BITMAPINFO*)pDib,DIB_RGB_COLORS);
		} else { //STRETCH
			RECT clipbox,paintbox;
			GetClipBox(hdc,&clipbox);
			paintbox.left = min(clipbox.right,max(clipbox.left,x));
			paintbox.right = max(clipbox.left,min(clipbox.right,x+cx));
			paintbox.top = min(clipbox.bottom,max(clipbox.top,y));
			paintbox.bottom = max(clipbox.top,min(clipbox.bottom,y+cy));
			long destw = paintbox.right - paintbox.left;
			long desth = paintbox.bottom - paintbox.top;
			//pixel informations
			RGBQUAD c={0,0,0,0};
			//Preparing Bitmap Info
			BITMAPINFO bmInfo;
			memset(&bmInfo.bmiHeader,0,sizeof(BITMAPINFOHEADER));
			bmInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth=destw;
			bmInfo.bmiHeader.biHeight=desth;
			bmInfo.bmiHeader.biPlanes=1;
			bmInfo.bmiHeader.biBitCount=24;
			BYTE *pbase;	//points to the final dib
			BYTE *pdst;		//current pixel from pbase
			BYTE *ppix;		//current pixel from image
			//get the background
			HDC TmpDC=CreateCompatibleDC(hdc);
			HBITMAP TmpBmp=CreateDIBSection(hdc,&bmInfo,DIB_RGB_COLORS,(void**)&pbase,0,0);
			HGDIOBJ TmpObj=SelectObject(TmpDC,TmpBmp);

			if (pbase){
				long xx,yy,yoffset;
				long ew = ((((24 * destw) + 31) / 32) * 4);
				long ymax = paintbox.bottom;
				long xmin = paintbox.left;
				float fx=(float)head.biWidth/(float)cx;
				float fy=(float)head.biHeight/(float)cy;
				long sx,sy;
				for(yy=0;yy<desth;yy++){
					sy=max(0L,head.biHeight-(long)ceil(((ymax-yy-y)*fy)));
					yoffset=sy*head.biWidth;
					pdst=pbase+yy*ew;
					for(xx=0;xx<destw;xx++){
						sx=(long)floor(((xx+xmin-x)*fx));
						if (head.biClrUsed){
							c=GetPaletteColor(GetPixelIndex(sx,sy));
						} else {
							ppix=info.pImage+sy*info.dwEffWidth+sx*3;
							c.rgbBlue = *ppix++;
							c.rgbGreen= *ppix++;
							c.rgbRed  = *ppix;
						}
						*pdst++=c.rgbBlue;
						*pdst++=c.rgbGreen;
						*pdst++=c.rgbRed;
					}
				}
			}
			//paint the image & cleanup
			SetDIBitsToDevice(hdc,paintbox.left,paintbox.top,destw,desth,0,0,0,desth,pbase,&bmInfo,0);
			DeleteObject(SelectObject(TmpDC,TmpObj));
			DeleteDC(TmpDC);
		}
	} else {	// draw image with transparent/alpha blending
	//////////////////////////////////////////////////////////////////
		//Alpha blend - Thanks to Florian Egel

		//find the smallest area to paint
		RECT clipbox,paintbox;
		GetClipBox(hdc,&clipbox);
		paintbox.left = min(clipbox.right,max(clipbox.left,x));
		paintbox.right = max(clipbox.left,min(clipbox.right,x+cx));
		paintbox.top = min(clipbox.bottom,max(clipbox.top,y));
		paintbox.bottom = max(clipbox.top,min(clipbox.bottom,y+cy));
		long destw = paintbox.right - paintbox.left;
		long desth = paintbox.bottom - paintbox.top;

		//pixel informations
		RGBQUAD c={0,0,0,0};
		RGBQUAD ct = GetTransColor();
		long* pc = (long*)&c;
		long* pct= (long*)&ct;

		//Preparing Bitmap Info
		BITMAPINFO bmInfo;
		memset(&bmInfo.bmiHeader,0,sizeof(BITMAPINFOHEADER));
		bmInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
		bmInfo.bmiHeader.biWidth=destw;
		bmInfo.bmiHeader.biHeight=desth;
		bmInfo.bmiHeader.biPlanes=1;
		bmInfo.bmiHeader.biBitCount=24;

		BYTE *pbase;	//points to the final dib
		BYTE *pdst;		//current pixel from pbase
		BYTE *ppix;		//current pixel from image

		//get the background
		HDC TmpDC=CreateCompatibleDC(hdc);
		HBITMAP TmpBmp=CreateDIBSection(hdc,&bmInfo,DIB_RGB_COLORS,(void**)&pbase,0,0);
		HGDIOBJ TmpObj=SelectObject(TmpDC,TmpBmp);
		BitBlt(TmpDC,0,0,destw,desth,hdc,paintbox.left,paintbox.top,SRCCOPY);

		if (pbase){
			long xx,yy,yoffset,ix,iy;
			BYTE a,a1;
			long ew = ((((24 * destw) + 31) / 32) * 4);
			long ymax = paintbox.bottom;
			long xmin = paintbox.left;

			if (cx!=head.biWidth || cy!=head.biHeight){
				//STRETCH
				float fx=(float)head.biWidth/(float)cx;
				float fy=(float)head.biHeight/(float)cy;
				long sx,sy;
				
				for(yy=0;yy<desth;yy++){
					sy=max(0L,head.biHeight-(long)ceil(((ymax-yy-y)*fy)));
					yoffset=sy*head.biWidth;
					pdst=pbase+yy*ew;
					for(xx=0;xx<destw;xx++){
						sx=(long)floor(((xx+xmin-x)*fx));

						if (bAlpha) a=pAlpha[yoffset+sx]; else a=255;
						a =(BYTE)((a*(1+info.nAlphaMax))>>8);

						if (head.biClrUsed){
							c=GetPaletteColor(GetPixelIndex(sx,sy));
							if (info.bAlphaPaletteEnabled){
								a= (BYTE)((a*(1+c.rgbReserved))>>8);
							}
						} else {
							ppix=info.pImage+sy*info.dwEffWidth+sx*3;
							c.rgbBlue = *ppix++;
							c.rgbGreen= *ppix++;
							c.rgbRed  = *ppix;
						}
						if (*pc!=*pct || !bTransparent){
							// DJT, assume many pixels are fully transparent or opaque and thus avoid multiplication
							if (a == 0) {			// Transparent, retain dest 
								pdst+=3; 
							} else if (a == 255) {	// opaque, ignore dest 
								*pdst++= c.rgbBlue; 
								*pdst++= c.rgbGreen; 
								*pdst++= c.rgbRed; 
							} else {				// semi transparent 
								a1=(BYTE)~a;
								*pdst++=(BYTE)((*pdst * a1 + a * c.rgbBlue)>>8); 
								*pdst++=(BYTE)((*pdst * a1 + a * c.rgbGreen)>>8); 
								*pdst++=(BYTE)((*pdst * a1 + a * c.rgbRed)>>8); 
							} 
						} else {
							pdst+=3;
						}
					}
				}
			} else {
				//NORMAL
				iy=head.biHeight-ymax+y;
				for(yy=0;yy<desth;yy++,iy++){
					yoffset=iy*head.biWidth;
					ix=xmin-x;
					pdst=pbase+yy*ew;
					ppix=info.pImage+iy*info.dwEffWidth+ix*3;
					for(xx=0;xx<destw;xx++,ix++){

						if (bAlpha) a=pAlpha[yoffset+ix]; else a=255;
						a = (BYTE)((a*(1+info.nAlphaMax))>>8);

						if (head.biClrUsed){
							c=GetPaletteColor(GetPixelIndex(ix,iy));
							if (info.bAlphaPaletteEnabled){
								a= (BYTE)((a*(1+c.rgbReserved))>>8);
							}
						} else {
							c.rgbBlue = *ppix++;
							c.rgbGreen= *ppix++;
							c.rgbRed  = *ppix++;
						}

						if (*pc!=*pct || !bTransparent){
							// DJT, assume many pixels are fully transparent or opaque and thus avoid multiplication
							if (a == 0) {			// Transparent, retain dest 
								pdst+=3; 
							} else if (a == 255) {	// opaque, ignore dest 
								*pdst++= c.rgbBlue; 
								*pdst++= c.rgbGreen; 
								*pdst++= c.rgbRed; 
							} else {				// semi transparent 
								a1=(BYTE)~a;
								*pdst++=(BYTE)((*pdst * a1 + a * c.rgbBlue)>>8); 
								*pdst++=(BYTE)((*pdst * a1 + a * c.rgbGreen)>>8); 
								*pdst++=(BYTE)((*pdst * a1 + a * c.rgbRed)>>8); 
							} 
						} else {
							pdst+=3;
						}
					}
				}
			}
		}
		//paint the image & cleanup
		SetDIBitsToDevice(hdc,paintbox.left,paintbox.top,destw,desth,0,0,0,desth,pbase,&bmInfo,0);
		DeleteObject(SelectObject(TmpDC,TmpObj));
		DeleteDC(TmpDC);
	}

	if (pClipRect){  // (experimental)
		HRGN rgn = CreateRectRgnIndirect(&mainbox);
		ExtSelectClipRgn(hdc,rgn,RGN_OR);
		DeleteObject(rgn);
	}

	return 1;
}
////////////////////////////////////////////////////////////////////////////////
// Draws (stretch) the image with single transparency support
// > hdc: destination device context
// > x,y: offset
// > cx,cy: (optional) size.
long CxImage::Draw2(HDC hdc, long x, long y, long cx, long cy)
{
	if((pDib==0)||(hdc==0)||(cx==0)||(cy==0)||(!info.bEnabled)) return 0;
	if (cx < 0) cx = head.biWidth;
	if (cy < 0) cy = head.biHeight;
	bool bTransparent = (info.nBkgndIndex != -1);

	if (!bTransparent){
		SetStretchBltMode(hdc,COLORONCOLOR);	
		StretchDIBits(hdc, x, y, cx, cy, 0, 0, head.biWidth, head.biHeight,
						info.pImage,(BITMAPINFO*)pDib, DIB_RGB_COLORS,SRCCOPY);
	} else {
		// draw image with transparent background
		const int safe = 0; // or else GDI fails in the following - sometimes 
		RECT rcDst = {x+safe, y+safe, x+cx, y+cy};
		if (RectVisible(hdc, &rcDst)){
		/////////////////////////////////////////////////////////////////
			// True Mask Method - Thanks to Paul Reynolds and Ron Gery
			int nWidth = head.biWidth;
			int nHeight = head.biHeight;
			// Create two memory dcs for the image and the mask
			HDC dcImage=CreateCompatibleDC(hdc);
			HDC dcTrans=CreateCompatibleDC(hdc);
			// Select the image into the appropriate dc
			HBITMAP bm = CreateCompatibleBitmap(hdc, nWidth, nHeight);
			HBITMAP pOldBitmapImage = (HBITMAP)SelectObject(dcImage,bm);
			SetStretchBltMode(dcImage,COLORONCOLOR);
			StretchDIBits(dcImage, 0, 0, nWidth, nHeight, 0, 0, nWidth, nHeight,
							info.pImage,(BITMAPINFO*)pDib,DIB_RGB_COLORS,SRCCOPY);

			// Create the mask bitmap
			HBITMAP bitmapTrans = CreateBitmap(nWidth, nHeight, 1, 1, NULL);
			// Select the mask bitmap into the appropriate dc
			HBITMAP pOldBitmapTrans = (HBITMAP)SelectObject(dcTrans, bitmapTrans);
			// Build mask based on transparent colour
			RGBQUAD rgbBG;
			if (head.biBitCount<24) rgbBG = GetPaletteColor((BYTE)info.nBkgndIndex);
			else rgbBG = info.nBkgndColor;
			COLORREF crColour = RGB(rgbBG.rgbRed, rgbBG.rgbGreen, rgbBG.rgbBlue);
			COLORREF crOldBack = SetBkColor(dcImage,crColour);
			BitBlt(dcTrans,0, 0, nWidth, nHeight, dcImage, 0, 0, SRCCOPY);

			// Do the work - True Mask method - cool if not actual display
			StretchBlt(hdc,x, y,cx,cy, dcImage, 0, 0, nWidth, nHeight, SRCINVERT);
			StretchBlt(hdc,x, y,cx,cy, dcTrans, 0, 0, nWidth, nHeight, SRCAND);
			StretchBlt(hdc,x, y,cx,cy, dcImage, 0, 0, nWidth, nHeight, SRCINVERT);

			// Restore settings
			SelectObject(dcImage,pOldBitmapImage);
			SelectObject(dcTrans,pOldBitmapTrans);
			SetBkColor(hdc,crOldBack);
			DeleteObject( bitmapTrans );  // RG 29/01/2002
			DeleteDC(dcImage);
			DeleteDC(dcTrans);
			DeleteObject(bm);
		}
	}
	return 1;
}
////////////////////////////////////////////////////////////////////////////////
// Stretch the image. Obsolete: use Draw() or Draw2()
long CxImage::Stretch(HDC hdc, long xoffset, long yoffset, long xsize, long ysize)
{
	if((pDib)&&(hdc)) {
		//palette must be correctly filled
		SetStretchBltMode(hdc,COLORONCOLOR);	
		StretchDIBits(hdc, xoffset, yoffset,
					xsize, ysize, 0, 0, head.biWidth, head.biHeight,
					info.pImage,(BITMAPINFO*)pDib,DIB_RGB_COLORS,SRCCOPY);
		return 1;
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
// Tiles the device context in the specified rectangle with the image.
long CxImage::Tile(HDC hdc, RECT *rc)
{
	if((pDib)&&(hdc)&&(rc)) {
		int w = rc->right - rc->left;
		int h = rc->bottom - rc->top;
		int x,y,z;
		int bx=head.biWidth;
		int by=head.biHeight;
		for (y = 0 ; y < h ; y += by){
			if ((y+by)>h) by=h-y;
			z=bx;
			for (x = 0 ; x < w ; x += z){
				if ((x+z)>w) z=w-x;
				RECT r = {rc->left + x,rc->top + y,rc->left + x + z,rc->top + y + by};
				Draw(hdc,rc->left + x, rc->top + y,-1,-1,&r);
			}
		}
		return 1;
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
long CxImage::DrawText(HDC hdc, long x, long y, const char* text, RGBQUAD color, const char* font, long lSize, long lWeight, BYTE bItalic, BYTE bUnderline, bool bEditAlpha)
{
	if (IsValid()){
		//get the background
		HDC TmpDC=CreateCompatibleDC(hdc);
		//choose the font
		HFONT m_Font;
		LOGFONT* m_pLF;
		m_pLF=(LOGFONT*)calloc(1,sizeof(LOGFONT));
		strncpy(m_pLF->lfFaceName,font,31);
		m_pLF->lfHeight=lSize;
		m_pLF->lfWeight=lWeight;
		m_pLF->lfItalic=bItalic;
		m_pLF->lfUnderline=bUnderline;
		m_Font=CreateFontIndirect(m_pLF);
		//select the font in the dc
		HFONT pOldFont=NULL;
		if (m_Font)
			pOldFont = (HFONT)SelectObject(TmpDC,m_Font);
		else
			pOldFont = (HFONT)SelectObject(TmpDC,GetStockObject(DEFAULT_GUI_FONT));

		//Set text color
		SetTextColor(TmpDC,RGB(255,255,255));
		SetBkColor(TmpDC,RGB(0,0,0));
		//draw the text
		SetBkMode(TmpDC,OPAQUE);
		//Set text position;
		RECT pos = {0,0,0,0};
		long len = (long)strlen(text);
		::DrawText(TmpDC,text,len,&pos,DT_CALCRECT);
		pos.right+=pos.bottom; //for italics

		//Preparing Bitmap Info
		long width=pos.right;
		long height=pos.bottom;
		BITMAPINFO bmInfo;
		memset(&bmInfo.bmiHeader,0,sizeof(BITMAPINFOHEADER));
		bmInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
		bmInfo.bmiHeader.biWidth=width;
		bmInfo.bmiHeader.biHeight=height;
		bmInfo.bmiHeader.biPlanes=1;
		bmInfo.bmiHeader.biBitCount=24;
		BYTE *pbase; //points to the final dib

		HBITMAP TmpBmp=CreateDIBSection(TmpDC,&bmInfo,DIB_RGB_COLORS,(void**)&pbase,0,0);
		HGDIOBJ TmpObj=SelectObject(TmpDC,TmpBmp);
		memset(pbase,0,height*((((24 * width) + 31) / 32) * 4));

		::DrawText(TmpDC,text,len,&pos,0);

		CxImage itext;
		itext.CreateFromHBITMAP(TmpBmp);

		y=head.biHeight-y-1;
		for (long ix=0;ix<width;ix++){
			for (long iy=0;iy<height;iy++){
				if (itext.GetPixelColor(ix,iy).rgbBlue) SetPixelColor(x+ix,y+iy,color,bEditAlpha);
			}
		}

		//cleanup
		if (pOldFont) SelectObject(TmpDC,pOldFont);
		DeleteObject(m_Font);
		free(m_pLF);
		DeleteObject(SelectObject(TmpDC,TmpObj));
		DeleteDC(TmpDC);
	}

	return 1;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_WINDOWS
////////////////////////////////////////////////////////////////////////////////
