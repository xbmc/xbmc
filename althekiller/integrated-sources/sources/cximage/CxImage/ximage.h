/*
 * File:	ximage.h
 * Purpose:	General Purpose Image Class 
 */
/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CxImage (c)  07/Aug/2001 <ing.davide.pizzolato@libero.it>
 * CxImage version 5.80 29/Sep/2003
 * See the file history.htm for the complete bugfix and news report.
 *
 * original CImage and CImageIterator implementation are:
 * Copyright: (c) 1995, Alejandro Aguilar Sierra <asierra(at)servidor(dot)unam(dot)mx>
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
#if !defined(__CXIMAGE_H)
#define __CXIMAGE_H

#if _MSC_VER > 1000
#pragma once
#endif 

/////////////////////////////////////////////////////////////////////////////
// CxImage supported features
#define CXIMAGE_SUPPORT_ALPHA          1
#define CXIMAGE_SUPPORT_SELECTION      1
#define CXIMAGE_SUPPORT_TRANSFORMATION 1
#define CXIMAGE_SUPPORT_DSP            1
#define CXIMAGE_SUPPORT_LAYERS		   1

#define CXIMAGE_SUPPORT_DECODE	1
#ifndef XBMC
	#define CXIMAGE_SUPPORT_ENCODE	1		//<vho><T.Peck>
	#define	CXIMAGE_SUPPORT_WINDOWS 1
	#define	CXIMAGE_SUPPORT_WINCE   0		//<T.Peck>
#else
	#define CXIMAGE_SUPPORT_ENCODE 1	
	#undef CXIMAGE_SUPPORT_WINDOWS 
	#undef CXIMAGE_SUPPORT_WINCE   
	#undef CXIMAGE_SUPPORT_LAYERS		   
//	#undef CXIMAGE_SUPPORT_ALPHA
	#undef CXIMAGE_SUPPORT_SELECTION
	#define CXIMAGE_SUPPORT_DSP 1
#endif

/////////////////////////////////////////////////////////////////////////////
// CxImage supported formats
#define CXIMAGE_SUPPORT_BMP 1
#define CXIMAGE_SUPPORT_GIF 1
#define CXIMAGE_SUPPORT_JPG 1
#define CXIMAGE_SUPPORT_PNG 1
#define CXIMAGE_SUPPORT_MNG 0
#define CXIMAGE_SUPPORT_ICO 1
#define CXIMAGE_SUPPORT_TIF 1
#define CXIMAGE_SUPPORT_TGA 1
#define CXIMAGE_SUPPORT_PCX 1
#define CXIMAGE_SUPPORT_WBMP 1
#define CXIMAGE_SUPPORT_WMF 1
#define CXIMAGE_SUPPORT_J2K 0		// Beta, use JP2
#define CXIMAGE_SUPPORT_JBG 0		// GPL'd see ../jbig/copying.txt & ../jbig/patents.htm

#define CXIMAGE_SUPPORT_JP2 1
#define CXIMAGE_SUPPORT_JPC 1
#define CXIMAGE_SUPPORT_PGX 1
#define CXIMAGE_SUPPORT_PNM 1
#define CXIMAGE_SUPPORT_RAS 1

/////////////////////////////////////////////////////////////////////////////
#include "xfile.h"
#include "xiofile.h"
#include "xmemfile.h"

#include "ximadefs.h"	//<vho> adjust some #define

/////////////////////////////////////////////////////////////////////////////
// CxImage formats enumerator
enum ENUM_CXIMAGE_FORMATS{
CXIMAGE_FORMAT_UNKNOWN,
#if CXIMAGE_SUPPORT_BMP
CXIMAGE_FORMAT_BMP,
#endif
#if CXIMAGE_SUPPORT_GIF
CXIMAGE_FORMAT_GIF,
#endif
#if CXIMAGE_SUPPORT_JPG
CXIMAGE_FORMAT_JPG,
#endif
#if CXIMAGE_SUPPORT_PNG
CXIMAGE_FORMAT_PNG,
#endif
#if CXIMAGE_SUPPORT_MNG
CXIMAGE_FORMAT_MNG,
#endif
#if CXIMAGE_SUPPORT_ICO
CXIMAGE_FORMAT_ICO,
#endif
#if CXIMAGE_SUPPORT_TIF
CXIMAGE_FORMAT_TIF,
#endif
#if CXIMAGE_SUPPORT_TGA
CXIMAGE_FORMAT_TGA,
#endif
#if CXIMAGE_SUPPORT_PCX
CXIMAGE_FORMAT_PCX,
#endif
#if CXIMAGE_SUPPORT_WBMP
CXIMAGE_FORMAT_WBMP,
#endif
#if CXIMAGE_SUPPORT_WMF
CXIMAGE_FORMAT_WMF,
#endif
#if CXIMAGE_SUPPORT_J2K
CXIMAGE_FORMAT_J2K,
#endif
#if CXIMAGE_SUPPORT_JBG
CXIMAGE_FORMAT_JBG,
#endif
#if CXIMAGE_SUPPORT_JP2
CXIMAGE_FORMAT_JP2,
#endif
#if CXIMAGE_SUPPORT_JPC
CXIMAGE_FORMAT_JPC,
#endif
#if CXIMAGE_SUPPORT_PGX
CXIMAGE_FORMAT_PGX,
#endif
#if CXIMAGE_SUPPORT_PNM
CXIMAGE_FORMAT_PNM,
#endif
#if CXIMAGE_SUPPORT_RAS
CXIMAGE_FORMAT_RAS,
#endif
CMAX_IMAGE_FORMATS
};

#ifdef XBMC

#define MAX_COMMENT 1000
#define MAX_SECTIONS 20

extern "C" {
  typedef struct tag_ExifInfo {
	  char  Version      [5];
    char  CameraMake   [32];
    char  CameraModel  [40];
    char  DateTime     [20];
    int   Height, Width;
    int   Orientation;
    int   IsColor;
    int   Process;
    int   FlashUsed;
    float FocalLength;
    float ExposureTime;
    float ApertureFNumber;
    float Distance;
    float CCDWidth;
    float ExposureBias;
    int   Whitebalance;
    int   MeteringMode;
    int   ExposureProgram;
    int   ISOequivalent;
    int   CompressionLevel;
	  float FocalplaneXRes;
	  float FocalplaneYRes;
	  float FocalplaneUnits;
	  float Xresolution;
	  float Yresolution;
	  float ResolutionUnit;
	  float Brightness;
    char  Comments[MAX_COMMENT];

    unsigned char * ThumbnailPointer;  /* Pointer at the thumbnail */
    unsigned ThumbnailSize;     /* Size of thumbnail. */

	  bool  IsExif;
  } EXIFINFO;
};

#endif

//color to grey mapping <H. Muelner> <jurgene>
//#define RGB2GRAY(r,g,b) (((b)*114 + (g)*587 + (r)*299)/1000)
#define RGB2GRAY(r,g,b) (((b)*117 + (g)*601 + (r)*306) >> 10)

struct rgb_color { BYTE r,g,b; };

/////////////////////////////////////////////////////////////////////////////
// CxImage class
/////////////////////////////////////////////////////////////////////////////
class DLL_EXP CxImage
{
//extensible information collector
typedef struct tagCxImageInfo {
	DWORD	dwEffWidth;			//DWORD aligned scan line width
	BYTE*	pImage;				//THE IMAGE BITS
	CxImage* pGhost;			//if this is a ghost, pGhost points to the body
	CxImage* pParent;			//if this is a layer, pParent points to the body
	DWORD	dwType;				//original image format
	char	szLastError[256];	//debugging
	long	nProgress;			//monitor
	long	nEscape;			//escape
	long	nBkgndIndex;		//used for GIF, PNG, MNG
	RGBQUAD nBkgndColor;		//used for RGB transparency
	BYTE	nQuality;			//used for JPEG
	long	nFrame;				//used for TIF, GIF, MNG : actual frame
	long	nNumFrames;			//used for TIF, GIF, MNG : total number of frames
	DWORD	dwFrameDelay;		//used for GIF, MNG
	long	xDPI;				//horizontal resolution
	long	yDPI;				//vertical resolution
	RECT	rSelectionBox;		//bounding rectangle
	BYTE	nAlphaMax;			//max opacity (fade)
	bool	bAlphaPaletteEnabled; //true if alpha values in the palette are enabled.
	bool	bEnabled;			//enables the painting functions
	long	xOffset;
	long	yOffset;
	DWORD	dwEncodeOption;		//for GIF, TIF : 0=def.1=unc,2=fax3,3=fax4,4=pack,5=jpg
	RGBQUAD last_c;				//for GetNearestIndex optimization
	BYTE	last_c_index;
	bool	last_c_isvalid;
	long	nNumLayers;
	DWORD	dwFlags;			// 0x??00000 = reserved, 0x00??0000 = blend mode, 0x0000???? = layer id - user flags
#ifdef XBMC
	EXIFINFO ExifInfo;
#endif
} CXIMAGEINFO;

public:
	//constructors
	CxImage(DWORD imagetype = 0);
	CxImage(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype = 0);
	CxImage(const CxImage &src, bool copypixels = true, bool copyselection = true, bool copyalpha = true);
	CxImage(const char * filename, DWORD imagetype);
	CxImage(FILE * stream, DWORD imagetype);
	CxImage(CxFile * stream, DWORD imagetype);
	CxImage(BYTE * buffer, DWORD size, DWORD imagetype);
	virtual ~CxImage() { Destroy(); };
	CxImage& operator = (const CxImage&);

	//initializzation
	void*	Create(DWORD dwWidth, DWORD dwHeight, DWORD wBpp, DWORD imagetype = 0);
	void	Destroy();
	void	Clear(BYTE bval=0);
	void	Copy(const CxImage &src, bool copypixels = true, bool copyselection = true, bool copyalpha = true);
	void	Transfer(CxImage &from);
	bool	CreateFromArray(BYTE* pArray,DWORD dwWidth,DWORD dwHeight,DWORD dwBitsperpixel, DWORD dwBytesperline, bool bFlipImage);
	bool	CreateFromMatrix(BYTE** ppMatrix,DWORD dwWidth,DWORD dwHeight,DWORD dwBitsperpixel, DWORD dwBytesperline, bool bFlipImage);

	//Attributes
	long	GetSize();
	BYTE*	GetBits(DWORD row = 0);
	BYTE	GetColorType();
	void*	GetDIB()		const {return pDib;}
	DWORD	GetHeight()		const {return head.biHeight;}
	DWORD	GetWidth()		const {return head.biWidth;}
	DWORD	GetEffWidth()	const {return info.dwEffWidth;}
	DWORD	GetNumColors()	const {return head.biClrUsed;}
	WORD	GetBpp()		const {return head.biBitCount;}
	DWORD	GetType()		const {return info.dwType;}
	char*	GetLastError()	{return info.szLastError;}
	const char* GetVersion();

	DWORD	GetFrameDelay() const {return info.dwFrameDelay;}
	void	SetFrameDelay(DWORD d) {info.dwFrameDelay=d;}

	void	GetOffset(long *x,long *y) {*x=info.xOffset; *y=info.yOffset;}
	void	SetOffset(long x,long y) {info.xOffset=x; info.yOffset=y;}

	BYTE	GetJpegQuality() const {return info.nQuality;}
	void	SetJpegQuality(BYTE q) {info.nQuality = q;}

	long	GetXDPI()		const {return info.xDPI;}
	long	GetYDPI()		const {return info.yDPI;}
	void	SetXDPI(long dpi);
	void	SetYDPI(long dpi);

	DWORD	GetClrImportant() const {return head.biClrImportant;}
	void	SetClrImportant(DWORD ncolors = 0);

	long	GetProgress()	const {return info.nProgress;}
	long	GetEscape()     const {return info.nEscape;}
	void	SetProgress(long p) {info.nProgress = p;}
	void	SetEscape(long i)   {info.nEscape = i;}

	long	GetTransIndex()	const {return info.nBkgndIndex;}
	RGBQUAD	GetTransColor();
	void	SetTransIndex(long idx) {info.nBkgndIndex = idx;}
	void	SetTransColor(RGBQUAD rgb) {rgb.rgbReserved=0; info.nBkgndColor = rgb;}
	bool	IsTransparent() const {return info.nBkgndIndex>=0;} // <vho>

	DWORD	GetEncodeOption() const {return info.dwEncodeOption;}
	void	SetEncodeOption(DWORD opt) {info.dwEncodeOption = opt;}

	DWORD	GetFlags() const {return info.dwFlags;}
	void	SetFlags(DWORD flags, bool bLockReservedFlags = true);

	//void*	GetUserData() const {return info.pUserData;}
	//void	SetUserData(void* pUserData) {info.pUserData = pUserData;}

#ifdef XBMC
	EXIFINFO *GetExifInfo() {return &info.ExifInfo;};
  bool  GetExifThumbnail(const char *filename, const char *outname);
  bool  RotateExif(int orientation = 0);
#endif
	//palette operations
	bool	IsGrayScale();
	bool	IsIndexed() {return head.biClrUsed!=0;}
	DWORD	GetPaletteSize();
	RGBQUAD* GetPalette() const;
	RGBQUAD GetPaletteColor(BYTE idx);
	bool	GetPaletteColor(BYTE i, BYTE* r, BYTE* g, BYTE* b);
	BYTE	GetNearestIndex(RGBQUAD c);
	void	BlendPalette(COLORREF cr,long perc);
	void	SetGrayPalette();
	void	SetPalette(DWORD n, BYTE *r, BYTE *g, BYTE *b);
	void	SetPalette(RGBQUAD* pPal,DWORD nColors=256);
	void	SetPalette(rgb_color *rgb,DWORD nColors=256);
	void	SetPaletteColor(BYTE idx, BYTE r, BYTE g, BYTE b, BYTE alpha=0);
	void	SetPaletteColor(BYTE idx, RGBQUAD c);
	void	SetPaletteColor(BYTE idx, COLORREF cr);
	void	SwapIndex(BYTE idx1, BYTE idx2);
	void	SetStdPalette();

	//pixel operations
	bool	IsInside(long x, long y);
	bool	IsTransparent(long x,long y);
	RGBQUAD GetPixelColor(long x,long y);
	BYTE	GetPixelIndex(long x,long y);
	BYTE	GetPixelGray(long x, long y);
	void	SetPixelColor(long x,long y,RGBQUAD c, bool bEditAlpha = false);
	void	SetPixelColor(long x,long y,COLORREF cr);
	void	SetPixelIndex(long x,long y,BYTE i);
	void	DrawLine(int StartX, int EndX, int StartY, int EndY, RGBQUAD color, bool bEditAlpha=false);
	void	DrawLine(int StartX, int EndX, int StartY, int EndY, COLORREF cr);


	//painting operations
#if CXIMAGE_SUPPORT_WINCE
	long	Blt(HDC pDC, long x=0, long y=0);
#endif
#if CXIMAGE_SUPPORT_WINDOWS
	HBITMAP MakeBitmap(HDC hdc = NULL);
	HANDLE	CopyToHandle();
	bool	CreateFromHANDLE(HANDLE hMem);		//Windows objects (clipboard)
	void	CreateFromHBITMAP(HBITMAP hbmp, HPALETTE hpal=0);	//Windows resource
	void	CreateFromHICON(HICON hico);
	long	Draw(HDC hdc, long x=0, long y=0, long cx = -1, long cy = -1, RECT* pClipRect = 0);
	long	Draw(HDC hdc, const RECT& rect, RECT* pClipRect=NULL) { return Draw(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, pClipRect); }
	long	Stretch(HDC hdc, long xoffset, long yoffset, long xsize, long ysize);
	long	Stretch(HDC hdc, const RECT& rect) { return Stretch(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top); }
	long	Tile(HDC hdc, RECT *rc);
	long	Draw2(HDC hdc, long x=0, long y=0, long cx = -1, long cy = -1);
	long	Draw2(HDC hdc, const RECT& rect) { return Draw2(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top); }
	long	DrawText(HDC hdc, long x, long y, const char* text, RGBQUAD color, const char* font, long lSize=0, long lWeight=400, BYTE bItalic=0, BYTE bUnderline=0, bool bEditAlpha=false);
#endif //CXIMAGE_SUPPORT_WINDOWS

	// file operations
#if CXIMAGE_SUPPORT_DECODE
#ifndef _XBOX
#ifdef WIN32
	bool Load(LPCWSTR filename, DWORD imagetype=0);
	bool LoadResource(HRSRC hRes, DWORD imagetype, HMODULE hModule=NULL);
#endif
#endif
#ifdef XBMC
	bool Load(const char * filename, DWORD imagetype, int &iWidth, int &iHeight);
	bool Decode(FILE * hFile, DWORD imagetype, int &iWidth, int &iHeight);
	bool Decode(CxFile * hFile, DWORD imagetype, int &iWidth, int &iHeight);
	bool Load(const char * filename, DWORD imagetype)
	{
		int iWidth=0;
		int iHeight=0;
		return Load(filename, imagetype, iWidth, iHeight);
	};
	bool Decode(FILE * hFile, DWORD imagetype)
	{
		int iWidth=0;
		int iHeight=0;
		return Decode(hFile, imagetype, iWidth, iHeight);
	};
	bool Decode(CxFile * hFile, DWORD imagetype)
	{
		int iWidth=0;
		int iHeight=0;
		return Decode(hFile, imagetype, iWidth, iHeight);
	};
#else
	bool Load(const char * filename, DWORD imagetype);
	bool Decode(FILE * hFile, DWORD imagetype);
	bool Decode(CxFile * hFile, DWORD imagetype);
#endif
	bool Decode(BYTE * buffer, DWORD size, DWORD imagetype);
#endif //CXIMAGE_SUPPORT_DECODE

#if CXIMAGE_SUPPORT_ENCODE
#ifdef WIN32	
#ifndef _XBOX
	bool Save(LPCWSTR filename, DWORD imagetype=0);
#endif
#endif
	bool Save(const char * filename, DWORD imagetype=0);
	bool Encode(FILE * hFile, DWORD imagetype);
	bool Encode(CxFile * hFile, DWORD imagetype);
	bool Encode(CxFile * hFile, CxImage ** pImages, int pagecount, DWORD imagetype);
	bool Encode(FILE *hFile, CxImage ** pImages, int pagecount, DWORD imagetype);
	bool Encode(BYTE * &buffer, long &size, DWORD imagetype);
#endif //CXIMAGE_SUPPORT_ENCODE

	//misc.
	bool IsValid() const {return pDib!=0;}
	bool IsEnabled() const {return info.bEnabled;}
	void Enable(bool enable=true){info.bEnabled=enable;}

	// frame operations
	long GetNumFrames() const {return info.nNumFrames;}
	long GetFrame() const {return info.nFrame;}
	void SetFrame(long nFrame) {info.nFrame=nFrame;}

#if CXIMAGE_SUPPORT_BASICTRANSFORMATIONS
	bool GrayScale();
	bool Flip();
	bool Mirror();
	bool Negative();
	bool RotateLeft(CxImage* iDst = NULL);
	bool RotateRight(CxImage* iDst = NULL);
#endif //CXIMAGE_SUPPORT_BASICTRANSFORMATIONS

#if CXIMAGE_SUPPORT_TRANSFORMATION
	// image operations
	bool Rotate(float angle, CxImage* iDst = NULL);
	bool Rotate180(CxImage* iDst = NULL);
	bool Resample(long newx, long newy, int mode = 1, CxImage* iDst = NULL);
	bool DecreaseBpp(DWORD nbit, bool errordiffusion, RGBQUAD* ppal = 0, DWORD clrimportant = 0);
	bool IncreaseBpp(DWORD nbit);
	bool Dither(long method = 0);
	bool Crop(long left, long top, long right, long bottom, CxImage* iDst = NULL);
	bool Crop(const RECT& rect, CxImage* iDst = NULL) { return Crop(rect.left, rect.top, rect.right, rect.bottom, iDst); }
	bool Skew(float xgain, float ygain, long xpivot=0, long ypivot=0);
	bool Expand(long left, long top, long right, long bottom, RGBQUAD canvascolor, CxImage* iDst = 0);
	bool Expand(long newx, long newy, RGBQUAD canvascolor, CxImage* iDst = 0);
	bool Thumbnail(long newx, long newy, RGBQUAD canvascolor, CxImage* iDst = 0);

protected:
	float b3spline(float x);
public:
#endif //CXIMAGE_SUPPORT_TRANSFORMATION

#if CXIMAGE_SUPPORT_DSP
	bool Contour();
	bool HistogramStretch();
	bool HistogramEqualize();
	bool HistogramNormalize();
	bool HistogramRoot();
	bool HistogramLog();
	long Histogram(long* red, long* green = 0, long* blue = 0, long* gray = 0, long colorspace = 0);
	bool Jitter(long radius=2);
	bool Repair(float radius = 0.25f, long niterations = 1, long colorspace = 0);
	bool Combine(CxImage* r,CxImage* g,CxImage* b,CxImage* a, long colorspace = 0);
	bool FFT2(CxImage* srcReal, CxImage* srcImag, CxImage* dstReal, CxImage* dstImag, long direction = 1, bool bForceFFT = true, bool bMagnitude = true);
	bool Noise(long level);
	bool Median(long Ksize=3);
	bool Gamma(float gamma);
	bool ShiftRGB(long r, long g, long b);
	bool Threshold(BYTE level);
	bool Colorize(BYTE hue, BYTE sat);
	bool Light(long brightness, long contrast = 0);
	float Mean();
	bool Filter(long* kernel, long Ksize, long Kfactor, long Koffset);
	bool Erode(long Ksize=2);
	bool Dilate(long Ksize=2);
	void HuePalette(float correction=1);
	enum ImageOpType { OpAdd, OpAnd, OpXor, OpOr, OpMask, OpSrcCopy, OpDstCopy, OpSub, OpSrcBlend };
	void Mix(CxImage & imgsrc2, ImageOpType op, long lXOffset = 0, long lYOffset = 0);
protected:
	bool IsPowerof2(long x);
	bool FFT(int dir,int m,double *x,double *y);
	bool DFT(int dir,long m,double *x1,double *y1,double *x2,double *y2);
	bool RepairChannel(CxImage *ch, float radius);
public:
	//color conversion utilities
	bool SplitRGB(CxImage* r,CxImage* g,CxImage* b);
	bool SplitYUV(CxImage* y,CxImage* u,CxImage* v);
	bool SplitHSL(CxImage* h,CxImage* s,CxImage* l);
	bool SplitYIQ(CxImage* y,CxImage* i,CxImage* q);
	bool SplitXYZ(CxImage* x,CxImage* y,CxImage* z);
	bool SplitCMYK(CxImage* c,CxImage* m,CxImage* y,CxImage* k);
	RGBQUAD HSLtoRGB(COLORREF cHSLColor);
	RGBQUAD RGBtoHSL(RGBQUAD lRGBColor);
	RGBQUAD HSLtoRGB(RGBQUAD lHSLColor);
	RGBQUAD YUVtoRGB(RGBQUAD lYUVColor);
	RGBQUAD RGBtoYUV(RGBQUAD lRGBColor);
	RGBQUAD YIQtoRGB(RGBQUAD lYIQColor);
	RGBQUAD RGBtoYIQ(RGBQUAD lRGBColor);
	RGBQUAD XYZtoRGB(RGBQUAD lXYZColor);
	RGBQUAD RGBtoXYZ(RGBQUAD lRGBColor);
#endif //CXIMAGE_SUPPORT_DSP
	RGBQUAD RGBtoRGBQUAD(COLORREF cr);
	COLORREF RGBQUADtoRGB (RGBQUAD c);

#if CXIMAGE_SUPPORT_SELECTION
	//selection
	bool SelectionClear();
	bool SelectionCreate();
	bool SelectionDelete();
	bool SelectionInvert();
	bool SelectionAddRect(RECT r);
	bool SelectionAddEllipse(RECT r);
	bool SelectionAddPolygon(POINT *points, long npoints);
	bool SelectionAddColor(RGBQUAD c);
	bool SelectionAddPixel(int x, int y);
	bool SelectionCopy(CxImage &from);
	bool SelectionIsInside(long x, long y);
	bool SelectionIsValid(){return pSelection!=0;}
	void SelectionGetBox(RECT& r){memcpy(&r,&info.rSelectionBox,sizeof(RECT));}
	bool SelectionToHRGN(HRGN& region);
#endif //CXIMAGE_SUPPORT_SELECTION

#if CXIMAGE_SUPPORT_ALPHA
	//Alpha
	void AlphaClear();
	void AlphaCreate();
	void AlphaDelete();
	void AlphaInvert();
	bool AlphaMirror();
	bool AlphaFlip();
	bool AlphaCopy(CxImage &from);
	bool AlphaSplit(CxImage *dest);
	void AlphaStrip();
	void AlphaSet(BYTE level);
	bool AlphaSet(CxImage &from);
	void AlphaSet(long x,long y,BYTE level);
	BYTE AlphaGet(long x,long y);
	BYTE AlphaGetMax() const {return info.nAlphaMax;}
	void AlphaSetMax(BYTE nAlphaMax) {info.nAlphaMax=nAlphaMax;}
	bool AlphaIsValid(){return pAlpha!=0;}
	BYTE* AlphaGetBits() const {return pAlpha;}

	void AlphaPaletteClear();
	void AlphaPaletteEnable(bool enable=true){info.bAlphaPaletteEnabled=enable;}
	bool AlphaPaletteIsEnabled(){return info.bAlphaPaletteEnabled;}
	bool AlphaPaletteIsValid();
	bool AlphaPaletteSplit(CxImage *dest);
#endif //CXIMAGE_SUPPORT_ALPHA

#if CXIMAGE_SUPPORT_LAYERS
	bool LayerCreate(long position = -1);
	bool LayerDelete(long position = -1);
	void LayerDeleteAll();
	CxImage* GetLayer(long position);
	CxImage* GetParent() const {return info.pParent;}
	long GetNumLayers() const {return info.nNumLayers;}
#endif //CXIMAGE_SUPPORT_LAYERS

protected:
	void Startup(DWORD imagetype = 0);
	void CopyInfo(const CxImage &src);
	void Ghost(CxImage *src);
	void RGBtoBGR(BYTE *buffer, int length);
	float HueToRGB(float n1,float n2, float hue);
	void Bitfield2RGB(BYTE *src, WORD redmask, WORD greenmask, WORD bluemask, BYTE bpp);
	static int CompareColors(const void *elem1, const void *elem2);
	bool EncodeSafeCheck(CxFile *hFile);

	void*				pDib; //contains the header, the palette, the pixels
    BITMAPINFOHEADER    head; //standard header
	CXIMAGEINFO			info; //extended information
	BYTE*				pSelection;	//selected region
	BYTE*				pAlpha; //alpha channel
	CxImage**			pLayers; //generic layers
};
////////////////////////////////////////////////////////////////////////////

#define	CXIMAGE_MAX_MEMORY 256000000

#define CXIMAGE_ERR_NOFILE "null file handler"
#define CXIMAGE_ERR_NOIMAGE "null image!!!"
////////////////////////////////////////////////////////////////////////////
#endif // !defined(__CXIMAGE_H)
