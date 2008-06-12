// Place the code and data below here into the CXIMAGE section.
#ifndef _DLL
  #pragma code_seg( "CXIMAGE" )
  #pragma data_seg( "CXIMAGE_RW" )
  #pragma bss_seg( "CXIMAGE_RW" )
  #pragma const_seg( "CXIMAGE_RD" )
  #pragma comment(linker, "/merge:CXIMAGE_RW=CXIMAGE")
  #pragma comment(linker, "/merge:CXIMAGE_RD=CXIMAGE")
#endif
// ximage.cpp : main implementation file
/* 07/08/2001 v1.00 - ing.davide.pizzolato@libero.it
 * CxImage version 5.80 29/Sep/2003
 */

#include "ximage.h"

////////////////////////////////////////////////////////////////////////////////
// CxImage 
////////////////////////////////////////////////////////////////////////////////
// Initialize the internal structures
void CxImage::Startup(DWORD imagetype)
{
	//init pointers
	pDib = pSelection = pAlpha = NULL;
	pLayers = NULL;
	//init structures
	memset(&head,0,sizeof(BITMAPINFOHEADER));
	memset(&info,0,sizeof(CXIMAGEINFO));
	//init default attributes
    info.dwType = imagetype;
	info.nQuality = 75;
	info.nAlphaMax = 255;
	info.nBkgndIndex = -1;
	info.bEnabled = true;
	SetXDPI(96);
	SetYDPI(96);
}
////////////////////////////////////////////////////////////////////////////////
// Empty image constructor
// > imagetype: (optional) set the image format (for future use)
CxImage::CxImage(DWORD imagetype)
{
	Startup(imagetype);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::Destroy()
{
	//free this only if it's valid and it's not a ghost
	if (info.pGhost==NULL){
		if (pLayers) { 
			for(long n=0; n<info.nNumLayers;n++){ delete pLayers[n]; }
			free(pLayers); pLayers=0;
		}
		if (pSelection) {free(pSelection); pSelection=0;}
		if (pAlpha) {free(pAlpha); pAlpha=0;}
		if (pDib) {free(pDib); pDib=0;}
	}
}
////////////////////////////////////////////////////////////////////////////////
// Sized image constructor
// > dwWidth: width
// > dwHeight: height
// > wBpp: bit per pixel
// > imagetype: (optional) set the image format (for future use)
CxImage::CxImage(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype)
{
	Startup(imagetype);
	Create(dwWidth,dwHeight,wBpp,imagetype);
}
////////////////////////////////////////////////////////////////////////////////
// image constructor from existing source
// > src: source image.
// > copypixels: copy the pixels from the source image into the new image.
// > copyselection: copy the selection from src
CxImage::CxImage(const CxImage &src, bool copypixels, bool copyselection, bool copyalpha)
{
	Startup(src.GetType());
	Copy(src,copypixels,copyselection,copyalpha);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::Copy(const CxImage &src, bool copypixels, bool copyselection, bool copyalpha)
{
	//copy the attributes
	memcpy(&info,&src.info,sizeof(CXIMAGEINFO));
	//rebuild the image
	Create(src.GetWidth(),src.GetHeight(),src.GetBpp(),src.GetType());
	//copy the pixels and the palette, or at least copy the palette only.
	if (copypixels && pDib && src.pDib) memcpy(pDib,src.pDib,GetSize());
	else SetPalette(src.GetPalette());
	long nSize = head.biWidth * head.biHeight;
	//copy the selection
	if (copyselection && src.pSelection){
		if (pSelection) free(pSelection);
		pSelection = (BYTE*)malloc(nSize);
		memcpy(pSelection,src.pSelection,nSize);
	}
	//copy the alpha channel
	if (copyalpha && src.pAlpha){
		if (pAlpha) free(pAlpha);
		pAlpha = (BYTE*)malloc(nSize);
		memcpy(pAlpha,src.pAlpha,nSize);
	}
}
////////////////////////////////////////////////////////////////////////////////
// Use it before Create()
void CxImage::CopyInfo(const CxImage &src)
{
	if (pDib==NULL) memcpy(&info,&src.info,sizeof(CXIMAGEINFO));
}
////////////////////////////////////////////////////////////////////////////////
CxImage& CxImage::operator = (const CxImage& isrc)
{
	if (this != &isrc) Copy(isrc);
	return *this;
}
////////////////////////////////////////////////////////////////////////////////
// Initializes or rebuilds the image.
// Returns the pointer to the internal pDib object
// > dwWidth: width
// > dwHeight: height
// > wBpp: bit per pixel
// > imagetype: (optional) set the image format (for future use)
void* CxImage::Create(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype)
{
	// destroy the existing image (if any)
	Destroy();

    // Make sure bits per pixel is valid
    if		(wBpp <= 1)	wBpp = 1;
    else if (wBpp <= 4)	wBpp = 4;
    else if (wBpp <= 8)	wBpp = 8;
    else				wBpp = 24;

	// limit memory requirements (and also a check for bad parameters)
	if (((dwWidth*dwHeight*wBpp)>>8) > CXIMAGE_MAX_MEMORY){
		strcpy(info.szLastError,"CXIMAGE_MAX_MEMORY exceeded");
		return NULL;
	}

	// set the correct bpp value
    switch (wBpp){
        case 1:
            head.biClrUsed = 2;	break;
        case 4:
            head.biClrUsed = 16; break;
        case 8:
            head.biClrUsed = 256; break;
        default:
            head.biClrUsed = 0;
    }

	//set the common image informations
    info.dwEffWidth = ((((wBpp * dwWidth) + 31) / 32) * 4);
    info.dwType = imagetype;

    // initialize BITMAPINFOHEADER
	head.biSize = sizeof(BITMAPINFOHEADER); //<ralphw>
    head.biWidth = dwWidth;		// fill in width from parameter
    head.biHeight = dwHeight;	// fill in height from parameter
    head.biPlanes = 1;			// must be 1
    head.biBitCount = (WORD)wBpp;		// from parameter
    head.biCompression = BI_RGB;    
    head.biSizeImage = info.dwEffWidth * dwHeight;
//    head.biXPelsPerMeter = 0; See SetXDPI
//    head.biYPelsPerMeter = 0; See SetYDPI
    head.biClrImportant = 0;

	pDib = malloc(GetSize()); // alloc memory block to store our bitmap
    if (!pDib){
		strcpy(info.szLastError,"CxImage::Create can't allocate memory");
		return NULL;
	}

	//clear the palette
	RGBQUAD* pal=GetPalette();
	if (pal) memset(pal,0,GetPaletteSize());
	//Destroy the existing selection
#if CXIMAGE_SUPPORT_SELECTION
	if (pSelection) SelectionDelete();
#endif //CXIMAGE_SUPPORT_SELECTION
	//Destroy the existing alpha channel
#if CXIMAGE_SUPPORT_ALPHA
	if (pAlpha) AlphaDelete();
#endif //CXIMAGE_SUPPORT_ALPHA

    // use our bitmap info structure to fill in first part of
    // our DIB with the BITMAPINFOHEADER
    BITMAPINFOHEADER*  lpbi;
	lpbi = (BITMAPINFOHEADER*)(pDib);
    *lpbi = head;

	info.pImage=GetBits();

    return pDib; //return handle to the DIB
}
////////////////////////////////////////////////////////////////////////////////
// returns the pointer to the image pixels
BYTE* CxImage::GetBits(DWORD row)
{ 
	if (pDib){
		if (row) {
			if (row<(DWORD)head.biHeight){
				return ((BYTE*)pDib + *(DWORD*)pDib + GetPaletteSize() + (info.dwEffWidth * row));
			} else {
				return NULL;
			}
		} else {
			return ((BYTE*)pDib + *(DWORD*)pDib + GetPaletteSize());
		}
	}
	return NULL;
}
////////////////////////////////////////////////////////////////////////////////
// returns the whole pDib dimension
long CxImage::GetSize()
{
	return head.biSize + head.biSizeImage + GetPaletteSize();
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::IsInside(long x, long y)
{
  return (0<=y && y<head.biHeight && 0<=x && x<head.biWidth);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::Clear(BYTE bval)
{
	if (pDib) memset(info.pImage,bval,head.biSizeImage);
}
////////////////////////////////////////////////////////////////////////////////
// moves everything from (*from) to (this), (*from) become empty
void CxImage::Transfer(CxImage &from)
{
	Destroy();

	memcpy(&head,&from.head,sizeof(BITMAPINFOHEADER));
	memcpy(&info,&from.info,sizeof(CXIMAGEINFO));

	pDib = from.pDib;
	pSelection = from.pSelection;
	pAlpha = from.pAlpha;
	pLayers = from.pLayers;

	memset(&from.head,0,sizeof(BITMAPINFOHEADER));
	memset(&from.info,0,sizeof(CXIMAGEINFO));
	from.pDib = from.pSelection = from.pAlpha = NULL;
	from.pLayers = NULL;
}
////////////////////////////////////////////////////////////////////////////////
// (this) points to the same pDib owned by (*from), the image remains in (*from)
// but (this) has the access to the pixels. Use carefully !!!
void CxImage::Ghost(CxImage *from)
{
	if (from){
		memcpy(&head,&from->head,sizeof(BITMAPINFOHEADER));
		memcpy(&info,&from->info,sizeof(CXIMAGEINFO));
		pDib = from->pDib;
		pSelection = from->pSelection;
		pAlpha = from->pAlpha;
		pLayers = from->pLayers;
		info.pGhost=from;
	}
}
////////////////////////////////////////////////////////////////////////////////
// turns a 16 or 32 bit bitfield image into a RGB image
void CxImage::Bitfield2RGB(BYTE *src, WORD redmask, WORD greenmask, WORD bluemask, BYTE bpp)
{
	switch (bpp){
	case 16:
	{
		DWORD ns[3]={0,0,0};
		// compute the number of shift for each mask
		for (int i=0;i<16;i++){
			if ((redmask>>i)&0x01) ns[0]++;
			if ((greenmask>>i)&0x01) ns[1]++;
			if ((bluemask>>i)&0x01) ns[2]++;
		}
		ns[1]+=ns[0]; ns[2]+=ns[1];	ns[0]=8-ns[0]; ns[1]-=8; ns[2]-=8;
		// dword aligned width for 16 bit image
		long effwidth2=(((head.biWidth + 1) / 2) * 4);
		WORD w;
		long y2,y3,x2,x3;
		BYTE *p=info.pImage;
		// scan the buffer in reverse direction to avoid reallocations
		for (long y=head.biHeight-1; y>=0; y--){
			y2=effwidth2*y;
			y3=info.dwEffWidth*y;
			for (long x=head.biWidth-1; x>=0; x--){
				x2 = 2*x+y2;
				x3 = 3*x+y3;
				w = (WORD)(src[x2]+256*src[1+x2]);
				p[  x3]=(BYTE)((w & bluemask)<<ns[0]);
				p[1+x3]=(BYTE)((w & greenmask)>>ns[1]);
				p[2+x3]=(BYTE)((w & redmask)>>ns[2]);
			}
		}
		break;
	}
	case 32:
	{
		// dword aligned width for 32 bit image
		long effwidth4 = head.biWidth * 4;
		long y4,y3,x4,x3;
		BYTE *p=info.pImage;
		// scan the buffer in reverse direction to avoid reallocations
		for (long y=head.biHeight-1; y>=0; y--){
			y4=effwidth4*y;
			y3=info.dwEffWidth*y;
			for (long x=head.biWidth-1; x>=0; x--){
				x4 = 4*x+y4;
				x3 = 3*x+y3;
				p[  x3]=src[  x4];
				p[1+x3]=src[1+x4];
				p[2+x3]=src[2+x4];
			}
		}
	}

	}
	return;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::CreateFromArray(BYTE* pArray,DWORD dwWidth,DWORD dwHeight,DWORD dwBitsperpixel, DWORD dwBytesperline, bool bFlipImage)
{
	if (pArray==NULL) return false;
	if (!(dwBitsperpixel==1)||(dwBitsperpixel==4)||(dwBitsperpixel==8)||
		(dwBitsperpixel==24)||(dwBitsperpixel==32)) return false;

	if (!Create(dwWidth,dwHeight,dwBitsperpixel)) return false;

	if (dwBitsperpixel<24) SetGrayPalette();

#if CXIMAGE_SUPPORT_ALPHA
	if (dwBitsperpixel==32) AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA

	BYTE *dst,*src;

	for (DWORD y = 0; y<dwHeight; y++) {
		dst = info.pImage + (bFlipImage?(dwHeight-1-y):y) * info.dwEffWidth;
		src = pArray + y * dwBytesperline;
		if (dwBitsperpixel==32){
			for(DWORD x=0;x<dwWidth;x++){
				*dst++=src[0];
				*dst++=src[1];
				*dst++=src[2];
#if CXIMAGE_SUPPORT_ALPHA
				AlphaSet(x,y,src[3]);
#endif //CXIMAGE_SUPPORT_ALPHA
				src+=4;
			}
		} else {
			memcpy(dst,src,min(info.dwEffWidth,dwBytesperline));
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
bool CxImage::CreateFromMatrix(BYTE** ppMatrix,DWORD dwWidth,DWORD dwHeight,DWORD dwBitsperpixel, DWORD dwBytesperline, bool bFlipImage)
{
	if (ppMatrix==NULL) return false;
	if (!(dwBitsperpixel==1)||(dwBitsperpixel==4)||(dwBitsperpixel==8)||
		(dwBitsperpixel==24)||(dwBitsperpixel==32)) return false;

	if (!Create(dwWidth,dwHeight,dwBitsperpixel)) return false;

	if (dwBitsperpixel<24) SetGrayPalette();

#if CXIMAGE_SUPPORT_ALPHA
	if (dwBitsperpixel==32) AlphaCreate();
#endif //CXIMAGE_SUPPORT_ALPHA

	BYTE *dst,*src;

	for (DWORD y = 0; y<dwHeight; y++) {
		dst = info.pImage + (bFlipImage?(dwHeight-1-y):y) * info.dwEffWidth;
		src = ppMatrix[y];
		if (src){
			if (dwBitsperpixel==32){
				for(DWORD x=0;x<dwWidth;x++){
					*dst++=src[0];
					*dst++=src[1];
					*dst++=src[2];
#if CXIMAGE_SUPPORT_ALPHA
					AlphaSet(x,y,src[3]);
#endif //CXIMAGE_SUPPORT_ALPHA
					src+=4;
				}
			} else {
				memcpy(dst,src,min(info.dwEffWidth,dwBytesperline));
			}
		}
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////
int CxImage::CompareColors(const void *elem1, const void *elem2)
{
	RGBQUAD* c1 = (RGBQUAD*)elem1;
	RGBQUAD* c2 = (RGBQUAD*)elem2;

	int g1 = (int)RGB2GRAY(c1->rgbRed,c1->rgbGreen,c1->rgbBlue);
	int g2 = (int)RGB2GRAY(c2->rgbRed,c2->rgbGreen,c2->rgbBlue);
	
	return (g1-g2);
}
////////////////////////////////////////////////////////////////////////////////
RGBQUAD	CxImage::GetTransColor()
{
	if (head.biBitCount<24 && info.nBkgndIndex != -1) return GetPaletteColor((BYTE)info.nBkgndIndex);
	return info.nBkgndColor;
}
////////////////////////////////////////////////////////////////////////////////
BYTE CxImage::GetColorType()
{
	BYTE b = (BYTE)((head.biBitCount>8) ? 2 /*COLORTYPE_COLOR*/ : 1 /*COLORTYPE_PALETTE*/);
#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()) b = 4 /*COLORTYPE_ALPHA*/;
#endif //CXIMAGE_SUPPORT_ALPHA
	return b;
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetXDPI(long dpi)
{
	info.xDPI = dpi;
	head.biXPelsPerMeter = (long) floor(dpi * 10000.0 / 254.0 + 0.5);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetYDPI(long dpi)
{
	info.yDPI = dpi;
	head.biYPelsPerMeter = (long) floor(dpi * 10000.0 / 254.0 + 0.5);
}
////////////////////////////////////////////////////////////////////////////////
void CxImage::SetFlags(DWORD flags, bool bLockReservedFlags)
{
	if (bLockReservedFlags) info.dwFlags = flags & 0x0000ffff;
	else info.dwFlags = flags;
}
////////////////////////////////////////////////////////////////////////////////
const char* CxImage::GetVersion()
{
	static const char CxImageVersion[] = "CxImage 5.80";
	return (CxImageVersion);
}
//EOF
