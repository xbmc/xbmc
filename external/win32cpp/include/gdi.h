// Win32++   Version 7.2
// Released: 5th AUgust 2011
//
//      David Nash
//      email: dnash@bigpond.net.au
//      url: https://sourceforge.net/projects/win32-framework
//
//
// Copyright (c) 2005-2011  David Nash
//
// Permission is hereby granted, free of charge, to
// any person obtaining a copy of this software and
// associated documentation files (the "Software"),
// to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
// gdi.h
//  Declaration of the CDC class, and CBitmapInfoPtr class

// The CDC class provides a device context, along with the various associated
//  objects such as Bitmaps, Brushes, Bitmaps, Fonts and Pens. This class
//  handles the creation, selection, de-selection and deletion of these objects
//  automatically. It also automatically deletes or releases the device context
//  itself as appropriate. Any failure to create the new GDI object throws an
//  exception.
//
// The CDC class is sufficient for most GDI programming needs. Sometimes
//  however we need to have the GDI object seperated from the device context.
//  Wrapper classes for GDI objects are provided for this purpose. The classes
//  are CBitmap, CBrush, CFont, CPalette, CPen and CRgn. These classes
//  automatically delete the GDI resouce assigned to them when their destructor
//  is called. These wrapper class objects can be attached to the CDC as
//  shown below.
//
// Coding Exampe without CDC ...
//  void DrawLine()
//  {
//	  HDC hdcClient = ::GetDC(m_hWnd);
//    HDC hdcMem = ::CreateCompatibleDC(hdcClient);
//    HBITMAP hBitmap = ::CreateCompatibleBitmap(hdcClient, cx, cy);
//	  HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hdcMem, hBitmap);
//    HPEN hPen = ::CreatePen(PS_SOLID, 1, RGB(255,0,0);
//    HPEN hOldPen = (HPEN)::SelectObject(hdcMem, hPen);
//	  ::MoveToEx(hdcMem, 0, 0, NULL);
//    ::LineTo(hdcMem, 50, 50);
//	  ::BitBlt(hdcClient, 0, 0, cx, cy, hdcMem, 0, 0);
//    ::SelectObject(hdcMem, hOldPen);
//    ::DeleteObject(hPen);
//    ::SelectObject(hdcMem, hOldBitmap);
//    ::DeleteObject(hBitmap);
//    ::DeleteDC(hdcMem);
//	  ::ReleaseDC(m_hWnd, hdcClient);
//  }
//
// Coding Example with CDC classes ...
//  void DrawLine()
//  {
//	  CClientDC dcClient(this)
//    CMemDC dcMem(&dcClient);
//	  CBitmap* pOldBitmap = dcMem.CreateCompatibleBitmap(&dcClient, cx, cy);
//    CPen* pOldPen = CMemDC.CreatePen(PS_SOLID, 1, RGB(255,0,0);
//	  CMemDC.MoveTo(0, 0);
//    CMemDC.LineTo(50, 50);
//	  dcClient.BitBlt(0, 0, cx, cy, &CMemDC, 0, 0);
//  }
//
// Coding Example with CDC classes and CPen ...
//  void DrawLine()
//  {
//	  CClientDC dcClient(this)
//    CMemDC CMemDC(&dcClient);
//	  CBitmap* pOldBitmap = dcMem.CreateCompatibleBitmap(&dcClient, cx, cy);
//    CPen MyPen(PS_SOLID, 1, RGB(255,0,0));
//    CPen* pOldPen = CMemDC.SelectObject(&MyPen);
//	  CMemDC.MoveTo(0, 0);
//    CMemDC.LineTo(50, 50);
//	  dcClient.BitBlt(0, 0, cx, cy, &CMemDC, 0, 0);
//  }

// Notes:
//  * When the CDC object drops out of scope, it's destructor is called, releasing
//     or deleting the device context as appropriate.
//  * When the destructor for CBitmap, CBrush, CPalette, CPen and CRgn are called,
//     the destructor is called deleting their GDI object.
//  * When the CDC object' destructor is called, any GDI objects created by one of
//     the CDC member functions (CDC::CreatePen for example) will be deleted.
//  * Bitmaps can only be selected into one device context at a time.
//  * Palettes use SelectPalatte to select them into device the context.
//  * Regions use SelectClipRgn to select them into the device context.
//  * The FromHandle function can be used to convert a GDI handle (HDC, HPEN,
//     HBITMAP etc) to a pointer of the appropriate GDI class (CDC, CPen CBitmap etc).
//     The FromHandle function creates a temporary object unless the HANDLE is already
//     assigned to a GDI class. Temporary objects don't delete their GDI object when
//     their destructor is called.
//  * All the GDI classes are reference counted. This allows functions to safely
//     pass these objects by value, as well as by pointer or by reference.

// The CBitmapInfoPtr class is a convienient wrapper for the BITMAPINFO structure.
// The size of the BITMAPINFO structure is dependant on the type of HBITMAP, and its
// space needs to be allocated dynamically. CBitmapInfoPtr automatically allocates
// and deallocates the memory for the structure. A CBitmapInfoPtr object can be
// used anywhere in place of a LPBITMAPINFO. LPBITMAPINFO is used in functions like
// GetDIBits and SetDIBits.
//
// Coding example ...
//  CDC MemDC = CreateCompatibleDC(NULL);
//  CBitmapInfoPtr pbmi(hBitmap);
//  MemDC.GetDIBits(hBitmap, 0, pbmi->bmiHeader.biHeight, NULL, pbmi, DIB_RGB_COLORS);

#ifndef _WIN32XX_GDI_H_
#define _WIN32XX_GDI_H_

#include "wincore.h"

// Disable macros from Windowsx.h
#undef CopyRgn

namespace Win32xx
{

	/////////////////////////////////////////////////////////////////
	// Declarations for some global functions in the Win32xx namespace
	//
#ifndef _WIN32_WCE
	void GrayScaleBitmap(CBitmap* pbmSource);
	void TintBitmap(CBitmap* pbmSource, int cRed, int cGreen, int cBlue);
	HIMAGELIST CreateDisabledImageList(HIMAGELIST himlNormal);
#endif

	///////////////////////////////////////////////
	// Declarations for the CGDIObject class
	//
	class CGDIObject
	{
		friend CBitmap* FromHandle(HBITMAP hBitmap);
		friend CBrush* FromHandle(HBRUSH hBrush);
		friend CDC* FromHandle(HDC hDC);
		friend CFont* FromHandle(HFONT hFont);
		friend CPalette* FromHandle(HPALETTE hPalette);
		friend CPen* FromHandle(HPEN hPen);
		friend CRgn* FromHandle(HRGN hRgn);

	public:
		struct DataMembers	// A structure that contains the data members for CGDIObject
		{
			HGDIOBJ hGDIObject;
			long	Count;
			BOOL	bRemoveObject;
		};
		CGDIObject();
		CGDIObject(const CGDIObject& rhs);
		virtual ~CGDIObject();
		CGDIObject& operator = ( const CGDIObject& rhs );
		void operator = (HGDIOBJ hObject);

		void	Attach(HGDIOBJ hObject);
		HGDIOBJ Detach();
		HGDIOBJ GetHandle() const;
		int		GetObject(int nCount, LPVOID pObject) const;

	protected:
		DataMembers* m_pData;

	private:
		void	AddToMap();
		BOOL	RemoveFromMap();
		void	Release();	
	};


	///////////////////////////////////////////////
	// Declarations for the CBitmap class
	//
	class CBitmap : public CGDIObject
	{
	  public:
		CBitmap();
		CBitmap(HBITMAP hBitmap);
		CBitmap(LPCTSTR lpstr);
		CBitmap(int nID);
		operator HBITMAP() const;
		~CBitmap();

		// Create and load methods
		BOOL LoadBitmap(LPCTSTR lpszName);
		BOOL LoadBitmap(int nID);
		BOOL LoadImage(LPCTSTR lpszName, int cxDesired, int cyDesired, UINT fuLoad);
		BOOL LoadImage(UINT nID, int cxDesired, int cyDesired, UINT fuLoad);
		BOOL LoadOEMBitmap(UINT nIDBitmap);
		HBITMAP CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitsPerPixel, LPCVOID lpBits);
		HBITMAP CreateCompatibleBitmap(CDC* pDC, int nWidth, int nHeight);
		HBITMAP CreateDIBSection(CDC* pDC, CONST BITMAPINFO* lpbmi, UINT uColorUse, LPVOID* ppvBits, HANDLE hSection, DWORD dwOffset);

#ifndef _WIN32_WCE
		HBITMAP CreateDIBitmap(CDC* pDC, CONST BITMAPINFOHEADER* lpbmih, DWORD dwInit, LPCVOID lpbInit, CONST BITMAPINFO* lpbmi, UINT uColorUse);
		HBITMAP CreateMappedBitmap(UINT nIDBitmap, UINT nFlags = 0, LPCOLORMAP lpColorMap = NULL, int nMapSize = 0);
		HBITMAP CreateBitmapIndirect(LPBITMAP lpBitmap);
		int GetDIBits(CDC* pDC, UINT uStartScan, UINT cScanLines,  LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT uColorUse) const;
		int SetDIBits(CDC* pDC, UINT uStartScan, UINT cScanLines, CONST VOID* lpvBits, CONST BITMAPINFO* lpbmi, UINT uColorUse);
		CSize GetBitmapDimensionEx() const;
		CSize SetBitmapDimensionEx(int nWidth, int nHeight);
#endif // !_WIN32_WCE

		// Attributes
		BITMAP GetBitmapData() const;
	};


	///////////////////////////////////////////////
	// Declarations for the CBrush class
	//
	class CBrush : public CGDIObject
	{
	  public:
		CBrush();
		CBrush(HBRUSH hBrush);
		CBrush(COLORREF crColor);
		operator HBRUSH() const;
		~CBrush();

		HBRUSH CreateSolidBrush(COLORREF crColor);
		HBRUSH CreatePatternBrush(CBitmap* pBitmap);
		LOGBRUSH GetLogBrush() const;

#ifndef _WIN32_WCE
		HBRUSH CreateHatchBrush(int nIndex, COLORREF crColor);
		HBRUSH CreateBrushIndirect(LPLOGBRUSH lpLogBrush);
		HBRUSH CreateDIBPatternBrush(HGLOBAL hglbDIBPacked, UINT fuColorSpec);
		HBRUSH CreateDIBPatternBrushPt(LPCVOID lpPackedDIB, UINT nUsage);
#endif // !defined(_WIN32_WCE)

	};


	///////////////////////////////////////////////
	// Declarations for the CFont class
	//
	class CFont : public CGDIObject
	{
	public:
		CFont();
		CFont(HFONT hFont);
		CFont(const LOGFONT* lpLogFont);
		operator HFONT() const;
		~CFont();

		// Create methods
		HFONT CreateFontIndirect(const LOGFONT* lpLogFont);
		HFONT CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, CDC* pDC = NULL, BOOL bBold = FALSE, BOOL bItalic = FALSE);
		HFONT CreatePointFontIndirect(const LOGFONT* lpLogFont, CDC* pDC = NULL);

#ifndef _WIN32_WCE
		HFONT CreateFont(int nHeight, int nWidth, int nEscapement,
				int nOrientation, int nWeight, DWORD dwItalic, DWORD dwUnderline,
				DWORD dwStrikeOut, DWORD dwCharSet, DWORD dwOutPrecision,
				DWORD dwClipPrecision, DWORD dwQuality, DWORD dwPitchAndFamily,
				LPCTSTR lpszFacename);
#endif // #ifndef _WIN32_WCE

		// Attributes
		LOGFONT GetLogFont() const;
	};


	///////////////////////////////////////////////
	// Declarations for the CPalette class
	//
	class CPalette : public CGDIObject
	{
	  public:
		CPalette();
		CPalette(HPALETTE hPalette);
		operator HPALETTE() const;
		~CPalette();

		// Create methods
		HPALETTE CreatePalette(LPLOGPALETTE lpLogPalette);

#ifndef _WIN32_WCE
		HPALETTE CreateHalftonePalette(CDC* pDC);
#endif // !_WIN32_WCE

		// Attributes
		int GetEntryCount() const;
		UINT GetPaletteEntries(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors) const;
		UINT SetPaletteEntries(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors);

		// Operations
#ifndef _WIN32_WCE
		BOOL ResizePalette(UINT nNumEntries);
		void AnimatePalette(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors);
#endif // !_WIN32_WCE

		UINT GetNearestPaletteIndex (COLORREF crColor) const;

	};


	///////////////////////////////////////////////
	// Declarations for the CPen class
	//
	class CPen : public CGDIObject
	{
	public:
		CPen();
		CPen(HPEN hPen);
		CPen(int nPenStyle, int nWidth, COLORREF crColor);
#ifndef _WIN32_WCE
		CPen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount = 0, const DWORD* lpStyle = NULL);
#endif // !_WIN32_WCE
		operator HPEN() const;
		~CPen();

		HPEN CreatePen(int nPenStyle, int nWidth, COLORREF crColor);
		HPEN CreatePenIndirect(LPLOGPEN lpLogPen);
		LOGPEN GetLogPen() const;

#ifndef _WIN32_WCE
		HPEN ExtCreatePen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount = 0, const DWORD* lpStyle = NULL);
		EXTLOGPEN GetExtLogPen() const;
#endif // !_WIN32_WCE

	};


	///////////////////////////////////////////////
	// Declarations for the CRgn class
	//
	class CRgn : public CGDIObject
	{
	  public:
		CRgn();
		CRgn(HRGN hRgn);
		operator HRGN() const;
		~CRgn ();

		// Create methods
		HRGN CreateRectRgn(int x1, int y1, int x2, int y2);
		HRGN CreateRectRgnIndirect(const RECT& rc);
		HRGN CreateFromData(const XFORM* lpXForm, int nCount, const RGNDATA* pRgnData);

#ifndef _WIN32_WCE
		HRGN CreateEllipticRgn(int x1, int y1, int x2, int y2);
		HRGN CreateEllipticRgnIndirect(const RECT& rc);
		HRGN CreatePolygonRgn(LPPOINT lpPoints, int nCount, int nMode);
		HRGN CreatePolyPolygonRgn(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount, int nPolyFillMode);
		HRGN CreateRoundRectRgn(int x1, int y1, int x2, int y2, int x3, int y3);
		HRGN CreateFromPath(HDC hDC);
#endif // !_WIN32_WCE

		// Operations
		void SetRectRgn(int x1, int y1, int x2, int y2);
		void SetRectRgn(const RECT& rc);
		int CombineRgn(CRgn* pRgnSrc1, CRgn* pRgnSrc2, int nCombineMode);
		int CombineRgn(CRgn* pRgnSrc, int nCombineMode);
		int CopyRgn(CRgn* pRgnSrc);
		BOOL EqualRgn(CRgn* pRgn) const;
		int OffsetRgn(int x, int y);
		int OffsetRgn(POINT& pt);
		int GetRgnBox(RECT& rc) const;
		BOOL PtInRegion(int x, int y) const;
		BOOL PtInRegion(POINT& pt) const;
		BOOL RectInRegion(const RECT& rc) const;
		int GetRegionData(LPRGNDATA lpRgnData, int nDataSize) const;
	};


	///////////////////////////////////////////////
	// Declarations for the CDC class
	//
	class CDC
	{
		friend class CWinApp;
		friend class CWnd;
		friend CDC* FromHandle(HDC hDC);

	public:
		struct DataMembers	// A structure that contains the data members for CDC
		{
			std::vector<GDIPtr> m_vGDIObjects;	// Smart pointers to internally created Bitmaps, Brushes, Fonts, Bitmaps and Regions
			HDC		hDC;			// The HDC belonging to this CDC
			long	Count;			// Reference count
			BOOL	bRemoveHDC;		// Delete/Release the HDC on destruction
			HWND	hWnd;			// The HWND of a Window or Client window DC
			int		nSavedDCState;	// The save state of the HDC.
		};

		CDC();									// Constructs a new CDC without assigning a HDC
		CDC(HDC hDC, HWND hWnd = 0);			// Assigns a HDC to a new CDC
		CDC(const CDC& rhs);					// Constructs a new copy of the CDC
		virtual ~CDC();
		operator HDC() const { return m_pData->hDC; }	// Converts a CDC to a HDC
		CDC& operator = (const CDC& rhs);		// Assigns a CDC to an existing CDC

		void Attach(HDC hDC, HWND hWnd = 0);
		void Destroy();
		HDC  Detach();
		HDC GetHDC() const { return m_pData->hDC; }
		CPalette* SelectPalette(const CPalette* pPalette, BOOL bForceBkgnd);
		CBitmap* SelectObject(const CBitmap* pBitmap);
		CBrush* SelectObject(const CBrush* pBrush);
		CFont* SelectObject(const CFont* pFont);
		CPalette* SelectObject(const CPalette* pPalette);
		CPen* SelectObject(const CPen* pPen);

#ifndef _WIN32_WCE
		void operator = (const HDC hDC);
#endif

		// Initialization
		BOOL CreateCompatibleDC(CDC* pDC);
		BOOL CreateDC(LPCTSTR lpszDriver, LPCTSTR lpszDevice, LPCTSTR lpszOutput, const DEVMODE* pInitData);
		int GetDeviceCaps(int nIndex) const;
#ifndef _WIN32_WCE
		BOOL CreateIC(LPCTSTR lpszDriver, LPCTSTR lpszDevice, LPCTSTR lpszOutput, const DEVMODE* pInitData);
#endif

		// Create and Select Bitmaps
		CBitmap* CreateBitmap(int cx, int cy, UINT Planes, UINT BitsPerPixel, LPCVOID pvColors);
		CBitmap* CreateCompatibleBitmap(CDC* pDC, int cx, int cy);
		CBitmap* CreateDIBSection(CDC* pDC, const BITMAPINFO& bmi, UINT iUsage, LPVOID *ppvBits,
										HANDLE hSection, DWORD dwOffset);
		BITMAP  GetBitmapData() const;
		CBitmap* LoadBitmap(UINT nID);
		CBitmap* LoadBitmap(LPCTSTR lpszName);
		CBitmap* LoadImage(UINT nID, int cxDesired, int cyDesired, UINT fuLoad);
		CBitmap* LoadImage(LPCTSTR lpszName, int cxDesired, int cyDesired, UINT fuLoad);
		CBitmap* LoadOEMBitmap(UINT nIDBitmap); // for OBM_/OCR_/OIC

#ifndef _WIN32_WCE
		CBitmap* CreateBitmapIndirect(LPBITMAP pBitmap);
		CBitmap* CreateDIBitmap(CDC* pDC, const BITMAPINFOHEADER& bmih, DWORD fdwInit, LPCVOID lpbInit,
										BITMAPINFO& bmi, UINT fuUsage);
		CBitmap* CreateMappedBitmap(UINT nIDBitmap, UINT nFlags /*= 0*/, LPCOLORMAP lpColorMap /*= NULL*/, int nMapSize /*= 0*/);
#endif

		// Create and Select Brushes
		CBrush* CreatePatternBrush(CBitmap* pBitmap);
		CBrush* CreateSolidBrush(COLORREF rbg);
		LOGBRUSH GetLogBrush() const;

#ifndef _WIN32_WCE
		CBrush* CreateBrushIndirect(LPLOGBRUSH pLogBrush);
		CBrush* CreateHatchBrush(int fnStyle, COLORREF rgb);
		CBrush* CreateDIBPatternBrush(HGLOBAL hglbDIBPacked, UINT fuColorSpec);
		CBrush* CreateDIBPatternBrushPt(LPCVOID lpPackedDIB, UINT iUsage);
#endif

		// Create and Select Fonts
		CFont* CreateFontIndirect(LPLOGFONT plf);
		LOGFONT GetLogFont() const;

#ifndef _WIN32_WCE
		CFont* CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight,
  							DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet,
  							DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality,
  							DWORD fdwPitchAndFamily, LPCTSTR lpszFace);
#endif

		// Create and Select Pens
		CPen* CreatePen(int nStyle, int nWidth, COLORREF rgb);
		CPen* CreatePenIndirect(LPLOGPEN pLogPen);
		LOGPEN GetLogPen() const;

		// Create Select Regions
		int CreateRectRgn(int left, int top, int right, int bottom);
		int CreateRectRgnIndirect(const RECT& rc);
		int CreateFromData(const XFORM* Xform, DWORD nCount, const RGNDATA *pRgnData);
#ifndef _WIN32_WCE
		int CreateEllipticRgn(int left, int top, int right, int bottom);
		int CreateEllipticRgnIndirect(const RECT& rc);
		int CreatePolygonRgn(LPPOINT ppt, int cPoints, int fnPolyFillMode);
		int CreatePolyPolygonRgn(LPPOINT ppt, LPINT pPolyCounts, int nCount, int fnPolyFillMode);
#endif

		// Wrappers for WinAPI functions

		// Point and Line Drawing Functions
		CPoint GetCurrentPosition() const;
		CPoint MoveTo(int x, int y) const;
		CPoint MoveTo(POINT pt) const;
		BOOL LineTo(int x, int y) const;
		BOOL LineTo(POINT pt) const;
		COLORREF GetPixel(int x, int y) const;
		COLORREF GetPixel(POINT pt) const;
		COLORREF SetPixel(int x, int y, COLORREF crColor) const;
		COLORREF SetPixel(POINT pt, COLORREF crColor) const;
#ifndef _WIN32_WCE
		BOOL Arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const;
		BOOL Arc(RECT& rc, POINT ptStart, POINT ptEnd) const;
		BOOL ArcTo(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const;
		BOOL ArcTo(RECT& rc, POINT ptStart, POINT ptEnd) const;
		BOOL AngleArc(int x, int y, int nRadius, float fStartAngle, float fSweepAngle) const;
		int  GetArcDirection() const;
		int  SetArcDirection(int nArcDirection) const;
		BOOL PolyDraw(const POINT* lpPoints, const BYTE* lpTypes, int nCount) const;
		BOOL Polyline(LPPOINT lpPoints, int nCount) const;
		BOOL PolyPolyline(const POINT* lpPoints, const DWORD* lpPolyPoints, int nCount) const;
		BOOL PolylineTo(const POINT* lpPoints, int nCount) const;
		BOOL PolyBezier(const POINT* lpPoints, int nCount) const;
		BOOL PolyBezierTo(const POINT* lpPoints, int nCount) const;
		BOOL SetPixelV(int x, int y, COLORREF crColor) const;
		BOOL SetPixelV(POINT pt, COLORREF crColor) const;
#endif

		// Shape Drawing Functions
		void DrawFocusRect(const RECT& rc) const;
		BOOL Ellipse(int x1, int y1, int x2, int y2) const;
		BOOL Ellipse(const RECT& rc) const;
		BOOL Polygon(LPPOINT lpPoints, int nCount) const;
		BOOL Rectangle(int x1, int y1, int x2, int y2) const;
		BOOL Rectangle(const RECT& rc) const;
		BOOL RoundRect(int x1, int y1, int x2, int y2, int nWidth, int nHeight) const;
		BOOL RoundRect(const RECT& rc, int nWidth, int nHeight) const;

#ifndef _WIN32_WCE
		BOOL Chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const;
		BOOL Chord(const RECT& rc, POINT ptStart, POINT ptEnd) const;
		BOOL Pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const;
		BOOL Pie(const RECT& rc, POINT ptStart, POINT ptEnd) const;
		BOOL PolyPolygon(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount) const;
#endif

		// Fill and Image Drawing functions
		BOOL FillRect(const RECT& rc, CBrush* pBrushr) const;
		BOOL InvertRect(const RECT& rc) const;
		BOOL DrawIconEx(int xLeft, int yTop, HICON hIcon, int cxWidth, int cyWidth, UINT istepIfAniCur, CBrush* pFlickerFreeDraw, UINT diFlags) const;
		BOOL DrawEdge(const RECT& rc, UINT nEdge, UINT nFlags) const;
		BOOL DrawFrameControl(const RECT& rc, UINT nType, UINT nState) const;
		BOOL FillRgn(CRgn* pRgn, CBrush* pBrush) const;
		void GradientFill(COLORREF Color1, COLORREF Color2, const RECT& rc, BOOL bVertical);
		void SolidFill(COLORREF Color, const RECT& rc);

#ifndef _WIN32_WCE
		BOOL DrawIcon(int x, int y, HICON hIcon) const;
		BOOL DrawIcon(POINT point, HICON hIcon) const;
		BOOL FrameRect(const RECT& rc, CBrush* pBrush) const;
		BOOL PaintRgn(CRgn* pRgn) const;
#endif

		// Bitmap Functions
		void DrawBitmap(int x, int y, int cx, int cy, CBitmap& Bitmap, COLORREF clrMask);
		int  StretchDIBits(int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth,
			           int nSrcHeight, CONST VOID *lpBits, BITMAPINFO& bi, UINT iUsage, DWORD dwRop) const;
		BOOL PatBlt(int x, int y, int nWidth, int nHeight, DWORD dwRop) const;
		BOOL BitBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC, int xSrc, int ySrc, DWORD dwRop) const;
		BOOL StretchBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop) const;

#ifndef _WIN32_WCE
		int  GetDIBits(CBitmap* pBitmap, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbi, UINT uUsage) const;
		int  SetDIBits(CBitmap* pBitmap, UINT uStartScan, UINT cScanLines, CONST VOID *lpvBits, LPBITMAPINFO lpbi, UINT fuColorUse) const;
		int  GetStretchBltMode() const;
		int  SetStretchBltMode(int iStretchMode) const;
		BOOL FloodFill(int x, int y, COLORREF crColor) const;
		BOOL ExtFloodFill(int x, int y, COLORREF crColor, UINT nFillType) const;
#endif

		// Brush Functions
#ifdef GetDCBrushColor
		COLORREF GetDCBrushColor() const;
		COLORREF SetDCBrushColor(COLORREF crColor) const;
#endif

		// Clipping Functions
		int  ExcludeClipRect(int Left, int Top, int Right, int BottomRect);
		int  ExcludeClipRect(const RECT& rc);
		int  GetClipBox(RECT& rc);
		int  GetClipRgn(HRGN hrgn);
		int  IntersectClipRect(int Left, int Top, int Right, int Bottom);
		int  IntersectClipRect(const RECT& rc);
		BOOL RectVisible(const RECT& rc);
		int  SelectClipRgn(CRgn* pRgn);

#ifndef _WIN32_WCE
		int  ExtSelectClipRgn(CRgn* pRgn, int fnMode);
		int  OffsetClipRgn(int nXOffset, int nYOffset);
		BOOL PtVisible(int X, int Y);
#endif

        // Co-ordinate Functions
#ifndef _WIN32_WCE
		BOOL DPtoLP(LPPOINT lpPoints, int nCount)  const;
		BOOL DPtoLP(RECT& rc)  const;
		BOOL LPtoDP(LPPOINT lpPoints, int nCount)  const;
		BOOL LPtoDP(RECT& rc)  const;
#endif

		// Layout Functions
		DWORD GetLayout() const;
		DWORD SetLayout(DWORD dwLayout) const;

		// Mapping functions
#ifndef _WIN32_WCE
		int  GetMapMode() const;
		int  SetMapMode(int nMapMode) const;
		BOOL GetViewportOrgEx(LPPOINT lpPoint)  const;
		BOOL SetViewportOrgEx(int x, int y, LPPOINT lpPoint = NULL) const;
		BOOL SetViewportOrgEx(POINT point, LPPOINT lpPointRet = NULL) const;
		BOOL OffsetViewportOrgEx(int nWidth, int nHeight, LPPOINT lpPoint = NULL) const;
		BOOL GetViewportExtEx(LPSIZE lpSize)  const;
		BOOL SetViewportExtEx(int x, int y, LPSIZE lpSize) const;
		BOOL SetViewportExtEx(SIZE size, LPSIZE lpSizeRet) const;
		BOOL ScaleViewportExtEx(int xNum, int xDenom, int yNum, int yDenom, LPSIZE lpSize) const;
		BOOL OffsetWindowOrg(int nWidth, int nHeight, LPPOINT lpPoint) const;
		BOOL GetWindowExtEx(LPSIZE lpSize)  const;
		BOOL SetWindowExtEx(int x, int y, LPSIZE lpSize) const;
		BOOL SetWindowExtEx(SIZE size, LPSIZE lpSizeRet) const;
		BOOL ScaleWindowExtEx(int xNum, int xDenom, int yNum, int yDenom, LPSIZE lpSize) const;
		BOOL GetWindowOrgEx(LPPOINT lpPoint)  const;
		BOOL SetWindowOrgEx(int x, int y, LPPOINT lpPoint) const;
		BOOL SetWindowOrgEx(POINT point, LPPOINT lpPointRet) const;
		BOOL OffsetWindowOrgEx(int nWidth, int nHeight, LPPOINT lpPoint) const;
#endif

		// Printer Functions
		int StartDoc(LPDOCINFO lpDocInfo) const;
		int EndDoc() const;
		int StartPage() const;
		int EndPage() const;
		int AbortDoc() const;
		int SetAbortProc(BOOL (CALLBACK* lpfn)(HDC, int)) const;

		// Text Functions
		int  DrawText(LPCTSTR lpszString, int nCount, LPRECT lprc, UINT nFormat) const;
		BOOL ExtTextOut(int x, int y, UINT nOptions, LPCRECT lprc, LPCTSTR lpszString, int nCount = -1, LPINT lpDxWidths = NULL) const;
		COLORREF GetBkColor() const;
		int  GetBkMode() const;
		UINT GetTextAlign() const;
		int  GetTextFace(int nCount, LPTSTR lpszFacename) const;
		COLORREF GetTextColor() const;
		BOOL  GetTextMetrics(TEXTMETRIC& Metrics) const;
		COLORREF SetBkColor(COLORREF crColor) const;
		int  SetBkMode(int iBkMode) const;
		UINT  SetTextAlign(UINT nFlags) const;
		COLORREF SetTextColor(COLORREF crColor) const;

#ifndef _WIN32_WCE
		int   DrawTextEx(LPTSTR lpszString, int nCount, LPRECT lprc, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams) const;
		CSize GetTabbedTextExtent(LPCTSTR lpszString, int nCount, int nTabPositions, LPINT lpnTabStopPositions) const;
		int   GetTextCharacterExtra() const;
		CSize GetTextExtentPoint32(LPCTSTR lpszString, int nCount) const;
		BOOL  GrayString(CBrush* pBrush, GRAYSTRINGPROC lpOutputFunc, LPARAM lpData, int nCount, int x, int y, int nWidth, int nHeight) const;
		int   SetTextCharacterExtra(int nCharExtra) const;
		int   SetTextJustification(int nBreakExtra, int nBreakCount) const;
		CSize TabbedTextOut(int x, int y, LPCTSTR lpszString, int nCount, int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin) const;
		BOOL  TextOut(int x, int y, LPCTSTR lpszString, int nCount = -1) const;
#endif

	private:
		void AddToMap();
		static CDC* AddTempHDC(HDC hDC, HWND hWnd);
		void Release();
		BOOL RemoveFromMap();

		DataMembers* m_pData;		// pointer to the class's data members
	};

	class CClientDC : public CDC
	{
	public:
		CClientDC(const CWnd* pWnd)
		{
			if (pWnd) assert(pWnd->IsWindow());
			HWND hWnd = pWnd? pWnd->GetHwnd() : GetDesktopWindow();
			Attach(::GetDC(hWnd), hWnd);
		}
		virtual ~CClientDC() {}
	};

	class CMemDC : public CDC
	{
	public:
		CMemDC(const CDC* pDC)
		{
			if (pDC) assert(pDC->GetHDC());
			HDC hDC = pDC? pDC->GetHDC() : NULL;
			Attach(::CreateCompatibleDC(hDC));
		}
		virtual ~CMemDC() {}
	};

	class CPaintDC : public CDC
	{
	public:
		CPaintDC(const CWnd* pWnd)
		{
			assert(pWnd->IsWindow());
			m_hWnd = pWnd->GetHwnd();
			Attach(::BeginPaint(pWnd->GetHwnd(), &m_ps), m_hWnd);
		}

		virtual ~CPaintDC()	{ ::EndPaint(m_hWnd, &m_ps); }

	private:
		HWND m_hWnd;
		PAINTSTRUCT m_ps;
	};

	class CWindowDC : public CDC
	{
	public:
		CWindowDC(const CWnd* pWnd)
		{
			if (pWnd) assert(pWnd->IsWindow());
			HWND hWnd = pWnd? pWnd->GetHwnd() : GetDesktopWindow();
			Attach(::GetWindowDC(hWnd), hWnd);
		}
		virtual ~CWindowDC() {}
	};

#ifndef _WIN32_WCE
	class CMetaFileDC : public CDC
	{
	public:
		CMetaFileDC() : m_hMF(0), m_hEMF(0) {}
		virtual ~CMetaFileDC()
		{
			if (m_hMF)
			{
				::CloseMetaFile(GetHDC());
				::DeleteMetaFile(m_hMF);
			}
			if (m_hEMF)
			{
				::CloseEnhMetaFile(GetHDC());
				::DeleteEnhMetaFile(m_hEMF);
			}
		}
		void Create(LPCTSTR lpszFilename = NULL) { Attach(::CreateMetaFile(lpszFilename)); }
		void CreateEnhanced(CDC* pDCRef, LPCTSTR lpszFileName, LPCRECT lpBounds, LPCTSTR lpszDescription)
		{
			HDC hDC = pDCRef? pDCRef->GetHDC() : NULL;
			::CreateEnhMetaFile(hDC, lpszFileName, lpBounds, lpszDescription);
			assert(GetHDC());
		}
		HMETAFILE Close() {	return ::CloseMetaFile(GetHDC()); }
		HENHMETAFILE CloseEnhanced() { return ::CloseEnhMetaFile(GetHDC()); }

	private:
		HMETAFILE m_hMF;
		HENHMETAFILE m_hEMF;
	};
#endif


	///////////////////////////////////////////////
	// Declarations for the CBitmapInfoPtr class
	// The CBitmapInfoPtr class is a convienient wrapper for the BITMAPINFO structure.
	class CBitmapInfoPtr
	{
	public:
		CBitmapInfoPtr(CBitmap* pBitmap)
		{
			BITMAP bmSource = pBitmap->GetBitmapData();

			// Convert the color format to a count of bits.
			WORD cClrBits = (WORD)(bmSource.bmPlanes * bmSource.bmBitsPixel);
			if (cClrBits == 1) 	     cClrBits = 1;
			else if (cClrBits <= 4)  cClrBits = 4;
			else if (cClrBits <= 8)  cClrBits = 8;
			else if (cClrBits <= 16) cClrBits = 16;
			else if (cClrBits <= 24) cClrBits = 24;
			else                     cClrBits = 32;

			// Allocate memory for the BITMAPINFO structure.
			UINT uQuadSize = (cClrBits == 24)? 0 : sizeof(RGBQUAD) * (int)(1 << cClrBits);
			m_bmi.assign(sizeof(BITMAPINFOHEADER) + uQuadSize, 0);
			m_pbmiArray = (LPBITMAPINFO) &m_bmi[0];

			m_pbmiArray->bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
			m_pbmiArray->bmiHeader.biHeight		= bmSource.bmHeight;
			m_pbmiArray->bmiHeader.biWidth		= bmSource.bmWidth;
			m_pbmiArray->bmiHeader.biPlanes		= bmSource.bmPlanes;
			m_pbmiArray->bmiHeader.biBitCount	= bmSource.bmBitsPixel;
			m_pbmiArray->bmiHeader.biCompression = BI_RGB;
			if (cClrBits < 24)
				m_pbmiArray->bmiHeader.biClrUsed = (1<<cClrBits);
		}
		LPBITMAPINFO get() const { return m_pbmiArray; }
		operator LPBITMAPINFO() const { return m_pbmiArray; }
		LPBITMAPINFO operator->() const { return m_pbmiArray; }

	private:
		CBitmapInfoPtr(const CBitmapInfoPtr&);				// Disable copy construction
		CBitmapInfoPtr& operator = (const CBitmapInfoPtr&);	// Disable assignment operator
		LPBITMAPINFO m_pbmiArray;
		std::vector<byte> m_bmi;
	};


	CBitmap* FromHandle(HBITMAP hBitmap);
	CBrush* FromHandle(HBRUSH hBrush);
	CFont* FromHandle(HFONT hFont);
	CPalette* FromHandle(HPALETTE hPalette);
	CPen* FromHandle(HPEN hPen);
	CRgn* FromHandle(HRGN hRgn);

}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


namespace Win32xx
{

	///////////////////////////////////////////////
	// Declarations for the CGDIObject class
	//

	inline CGDIObject::CGDIObject()
	// Constructs the CGDIObject
	{
		m_pData = new DataMembers;
		m_pData->hGDIObject = 0;
		m_pData->Count = 1L;
		m_pData->bRemoveObject = TRUE;
	}

	inline CGDIObject::CGDIObject(const CGDIObject& rhs)
	// Note: A copy of a CGDIObject is a clone of the original.
	//       Both objects manipulate the one HGDIOBJ.
	{
		m_pData = rhs.m_pData;
		InterlockedIncrement(&m_pData->Count);
	}

	inline CGDIObject::~CGDIObject()
	// Deconstructs the CGDIObject
	{
		Release();
	}

	inline CGDIObject& CGDIObject::operator = ( const CGDIObject& rhs )
	// Note: A copy of a CGDIObject is a clone of the original.
	//       Both objects manipulate the one HGDIOBJ.
	{
		if (this != &rhs)
		{
			InterlockedIncrement(&rhs.m_pData->Count);
			Release();
			m_pData = rhs.m_pData;
		}

		return *this;
	}

	inline void CGDIObject::operator = (HGDIOBJ hObject)
	{
		assert(m_pData);
		assert (m_pData->hGDIObject == NULL);
		m_pData->hGDIObject = hObject;
	}

	inline void CGDIObject::AddToMap()
	// Store the HDC and CDC pointer in the HDC map
	{
		assert( GetApp() );
		GetApp()->m_csMapLock.Lock();

		assert(m_pData->hGDIObject);
		assert(!GetApp()->GetCGDIObjectFromMap(m_pData->hGDIObject));

		GetApp()->m_mapGDI.insert(std::make_pair(m_pData->hGDIObject, this));
		GetApp()->m_csMapLock.Release();
	}

	inline void CGDIObject::Attach(HGDIOBJ hObject)
	// Attaches a GDI HANDLE to the CGDIObject.
	// The HGDIOBJ will be automatically deleted when the destructor is called unless it is detached.
	{
		assert(m_pData);

		if (m_pData->hGDIObject != NULL && m_pData->hGDIObject != hObject)
		{
			::DeleteObject(Detach());
		}

		CGDIObject* pObject = GetApp()->GetCGDIObjectFromMap(hObject);
		if (pObject)
		{
			delete m_pData;
			m_pData = pObject->m_pData;
			InterlockedIncrement(&m_pData->Count);
		}
		else
		{
			m_pData->hGDIObject = hObject;
			AddToMap();
		}
	}

	inline HGDIOBJ CGDIObject::Detach()
	// Detaches the HGDIOBJ from this object.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject);

		GetApp()->m_csMapLock.Lock();
		RemoveFromMap();
		HGDIOBJ hObject = m_pData->hGDIObject;
		m_pData->hGDIObject = 0;

		if (m_pData->Count)
		{
			if (InterlockedDecrement(&m_pData->Count) == 0)
			{
				delete m_pData;
			}
		}

		GetApp()->m_csMapLock.Release();

		// Assign values to our data members
		m_pData = new DataMembers;
		m_pData->hGDIObject = 0;
		m_pData->Count = 1L;
		m_pData->bRemoveObject = TRUE;

		return hObject;
	}

	inline HGDIOBJ CGDIObject::GetHandle() const
	{
		assert(m_pData);
		return m_pData->hGDIObject;
	}

	inline int CGDIObject::GetObject(int nCount, LPVOID pObject) const
	{
		assert(m_pData);
		return ::GetObject(m_pData->hGDIObject, nCount, pObject);
	}

	inline void CGDIObject::Release()
	{
		assert(m_pData);
		BOOL bSucceeded = TRUE;

		if (InterlockedDecrement(&m_pData->Count) == 0)
		{
			if (m_pData->hGDIObject != NULL)
			{
				if (m_pData->bRemoveObject)
					bSucceeded = ::DeleteObject(m_pData->hGDIObject);
				else
					bSucceeded = TRUE;
			}

			RemoveFromMap();
			delete m_pData;
			m_pData = 0;
		}

		assert(bSucceeded);
	}

	inline BOOL CGDIObject::RemoveFromMap()
	{
		BOOL Success = FALSE;

		if( GetApp() )
		{
			// Allocate an iterator for our HDC map
			std::map<HGDIOBJ, CGDIObject*, CompareGDI>::iterator m;

			CWinApp* pApp = GetApp();
			if (pApp)
			{
				// Erase the CGDIObject pointer entry from the map
				pApp->m_csMapLock.Lock();
				m = pApp->m_mapGDI.find(m_pData->hGDIObject);
				if (m != pApp->m_mapGDI.end())
				{
					pApp->m_mapGDI.erase(m);
					Success = TRUE;
				}

				pApp->m_csMapLock.Release();
			}
		}

		return Success;
	}


	///////////////////////////////////////////////
	// Declarations for the CBitmap class
	//
	inline CBitmap::CBitmap()
	{
	}

	inline CBitmap::CBitmap(HBITMAP hBitmap)
	{
		assert(m_pData);
		Attach(hBitmap);
	}

	inline CBitmap::CBitmap(LPCTSTR lpszName)
	{
		LoadBitmap(lpszName);
	}

	inline CBitmap::CBitmap(int nID)
	{
		LoadBitmap(nID);
	}

	inline CBitmap::operator HBITMAP() const
	{
		assert(m_pData);
		return (HBITMAP)m_pData->hGDIObject;
	}

	inline CBitmap::~CBitmap()
	{
	}

	inline BOOL CBitmap::LoadBitmap(int nID)
	// Loads a bitmap from a resource using the resource ID.
	{
		return LoadBitmap(MAKEINTRESOURCE(nID));
	}

	inline BOOL CBitmap::LoadBitmap(LPCTSTR lpszName)
	// Loads a bitmap from a resource using the resource string.
	{
		assert(GetApp());
		assert(m_pData);

		HBITMAP hBitmap = (HBITMAP)::LoadImage(GetApp()->GetResourceHandle(), lpszName, IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
		if (hBitmap)
		{
			Attach(hBitmap);
		}
		return (0 != hBitmap);	// boolean expression
	}

	inline BOOL CBitmap::LoadImage(UINT nID, int cxDesired, int cyDesired, UINT fuLoad)
	// Loads a bitmap from a resource using the resource ID.
	{
		return LoadImage(MAKEINTRESOURCE(nID), cxDesired, cyDesired, fuLoad);
	}

	inline BOOL CBitmap::LoadImage(LPCTSTR lpszName, int cxDesired, int cyDesired, UINT fuLoad)
	// Loads a bitmap from a resource using the resource string.
	{
		assert(GetApp());
		assert(m_pData);

		HBITMAP hBitmap = (HBITMAP)::LoadImage(GetApp()->GetResourceHandle(), lpszName, IMAGE_BITMAP, cxDesired, cyDesired, fuLoad);
		if (hBitmap)
		{
			Attach(hBitmap);
		}
		return (0 != hBitmap);	// boolean expression
	}

	inline BOOL CBitmap::LoadOEMBitmap(UINT nIDBitmap) // for OBM_/OCR_/OIC_
	// Loads a predefined bitmap
	// Predefined bitmaps include: OBM_BTNCORNERS, OBM_BTSIZE, OBM_CHECK, OBM_CHECKBOXES, OBM_CLOSE, OBM_COMBO
	//  OBM_DNARROW, OBM_DNARROWD, OBM_DNARROWI, OBM_LFARROW, OBM_LFARROWD, OBM_LFARROWI, OBM_MNARROW,OBM_OLD_CLOSE
	//  OBM_OLD_DNARROW, OBM_OLD_LFARROW, OBM_OLD_REDUCE, OBM_OLD_RESTORE, OBM_OLD_RGARROW, OBM_OLD_UPARROW
	//  OBM_OLD_ZOOM, OBM_REDUCE, OBM_REDUCED, OBM_RESTORE, OBM_RESTORED, OBM_RGARROW, OBM_RGARROWD, OBM_RGARROWI
	//  OBM_SIZE, OBM_UPARROW, OBM_UPARROWD, OBM_UPARROWI, OBM_ZOOM, OBM_ZOOMD
	{
		assert(m_pData);

		HBITMAP hBitmap = ::LoadBitmap(NULL, MAKEINTRESOURCE(nIDBitmap));
		if (hBitmap)
		{
			Attach( ::LoadBitmap(NULL, MAKEINTRESOURCE(nIDBitmap)) );
		}
		return (0 != hBitmap);	// boolean expression
	}

#ifndef _WIN32_WCE
		inline HBITMAP CBitmap::CreateMappedBitmap(UINT nIDBitmap, UINT nFlags /*= 0*/, LPCOLORMAP lpColorMap /*= NULL*/, int nMapSize /*= 0*/)
		// Creates a new bitmap using the bitmap data and colors specified by the bitmap resource and the color mapping information.
		{
			assert(GetApp());
			assert(m_pData);
			HBITMAP hBitmap = ::CreateMappedBitmap(GetApp()->GetResourceHandle(), nIDBitmap, (WORD)nFlags, lpColorMap, nMapSize);
			Attach(hBitmap);
			return hBitmap;
		}
#endif // !_WIN32_WCE

		inline HBITMAP CBitmap::CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitsPerPixel, LPCVOID lpBits)
		// Creates a bitmap with the specified width, height, and color format (color planes and bits-per-pixel).
		{
			assert(m_pData);
			HBITMAP hBitmap = ::CreateBitmap(nWidth, nHeight, nPlanes, nBitsPerPixel, lpBits);
			Attach(hBitmap);
			return hBitmap;
		}

#ifndef _WIN32_WCE
		inline HBITMAP CBitmap::CreateBitmapIndirect(LPBITMAP lpBitmap)
		// Creates a bitmap with the width, height, and color format specified in the BITMAP structure.
		{
			assert(m_pData);
			HBITMAP hBitmap = ::CreateBitmapIndirect(lpBitmap);
			Attach(hBitmap);
			return hBitmap;
		}
#endif // !_WIN32_WCE

		inline HBITMAP CBitmap::CreateCompatibleBitmap(CDC* pDC, int nWidth, int nHeight)
		// Creates a bitmap compatible with the device that is associated with the specified device context.
		{
			assert(m_pData);
			assert(pDC);
			HBITMAP hBitmap = ::CreateCompatibleBitmap(pDC->GetHDC(), nWidth, nHeight);
			Attach(hBitmap);
			return hBitmap;
		}

		// Attributes
		inline BITMAP CBitmap::GetBitmapData() const
		// Retrieves the BITMAP structure
		{
			assert(m_pData);
			assert(m_pData->hGDIObject != NULL);
			BITMAP bmp = {0};
			::GetObject(m_pData->hGDIObject, sizeof(BITMAP), &bmp);
			return bmp;
		}

#ifndef _WIN32_WCE
		inline CSize CBitmap::GetBitmapDimensionEx() const
		// Retrieves the dimensions of a compatible bitmap.
		// The retrieved dimensions must have been set by the SetBitmapDimensionEx function.
		{
			assert(m_pData);
			assert(m_pData->hGDIObject != NULL);
			CSize Size;
			::GetBitmapDimensionEx((HBITMAP)m_pData->hGDIObject, &Size);
			return Size;
		}

		inline CSize CBitmap::SetBitmapDimensionEx(int nWidth, int nHeight)
		// The SetBitmapDimensionEx function assigns preferred dimensions to a bitmap.
		// These dimensions can be used by applications; however, they are not used by the system.
		{
			assert(m_pData);
			assert(m_pData->hGDIObject != NULL);
			CSize Size;
			::SetBitmapDimensionEx((HBITMAP)m_pData->hGDIObject, nWidth, nHeight, Size);
			return Size;
		}

		// DIB support
		inline HBITMAP CBitmap::CreateDIBitmap(CDC* pDC, CONST BITMAPINFOHEADER* lpbmih, DWORD dwInit, CONST VOID* lpbInit, CONST BITMAPINFO* lpbmi, UINT uColorUse)
		// Creates a compatible bitmap (DDB) from a DIB and, optionally, sets the bitmap bits.
		{
			assert(m_pData);
			assert(pDC);
			HBITMAP hBitmap = ::CreateDIBitmap(pDC->GetHDC(), lpbmih, dwInit, lpbInit, lpbmi, uColorUse);
			Attach(hBitmap);
			return hBitmap;
		}
#endif // !_WIN32_WCE

		inline HBITMAP CBitmap::CreateDIBSection(CDC* pDC, CONST BITMAPINFO* lpbmi, UINT uColorUse, VOID** ppvBits, HANDLE hSection, DWORD dwOffset)
		// Creates a DIB that applications can write to directly. The function gives you a pointer to the location of the bitmap bit values.
		// You can supply a handle to a file-mapping object that the function will use to create the bitmap, or you can let the system allocate the memory for the bitmap.
		{
			assert(m_pData);
			assert(pDC);
			HBITMAP hBitmap = ::CreateDIBSection(pDC->GetHDC(), lpbmi, uColorUse, ppvBits, hSection, dwOffset);
			Attach(hBitmap);
			return hBitmap;
		}

#ifndef _WIN32_WCE
		inline int CBitmap::GetDIBits(CDC* pDC, UINT uStartScan, UINT cScanLines,  LPVOID lpvBits, LPBITMAPINFO lpbmi, UINT uColorUse) const
		// Retrieves the bits of the specified compatible bitmap and copies them into a buffer as a DIB using the specified format.
		{
			assert(m_pData);
			assert(pDC);
			assert(m_pData->hGDIObject != NULL);
			return ::GetDIBits(pDC->GetHDC(), (HBITMAP)m_pData->hGDIObject, uStartScan, cScanLines,  lpvBits, lpbmi, uColorUse);
		}

		inline int CBitmap::SetDIBits(CDC* pDC, UINT uStartScan, UINT cScanLines, CONST VOID* lpvBits, CONST BITMAPINFO* lpbmi, UINT uColorUse)
		// Sets the pixels in a compatible bitmap (DDB) using the color data found in the specified DIB.
		{
			assert(m_pData);
			assert(pDC);
			assert(m_pData->hGDIObject != NULL);
			return ::SetDIBits(pDC->GetHDC(), (HBITMAP)m_pData->hGDIObject, uStartScan, cScanLines, lpvBits, lpbmi, uColorUse);
		}
#endif // !_WIN32_WCE


	///////////////////////////////////////////////
	// Definitions of the CBrush class
	//
	inline CBrush::CBrush()
	{
	}

	inline CBrush::CBrush(HBRUSH hBrush)
	{
		assert(m_pData);
		Attach(hBrush);
	}

	inline CBrush::CBrush(COLORREF crColor)
	{
		Attach( ::CreateSolidBrush(crColor) );
		assert (m_pData->hGDIObject);
	}

	inline CBrush::operator HBRUSH() const
	{
		assert(m_pData);
		return (HBRUSH)m_pData->hGDIObject;
	}

	inline CBrush::~CBrush()
	{
	}

	inline HBRUSH CBrush::CreateSolidBrush(COLORREF crColor)
	// Creates a logical brush that has the specified solid color.
	{
		assert(m_pData);
		HBRUSH hBrush = ::CreateSolidBrush(crColor);
		Attach(hBrush);
		return hBrush;
	}

#ifndef _WIN32_WCE
	inline HBRUSH CBrush::CreateHatchBrush(int nIndex, COLORREF crColor)
	// Creates a logical brush that has the specified hatch pattern and color.
	{
		assert(m_pData);
		HBRUSH hBrush = ::CreateHatchBrush(nIndex, crColor);
		Attach(hBrush);
		return hBrush;
	}

	inline HBRUSH CBrush::CreateBrushIndirect(LPLOGBRUSH lpLogBrush)
	// Creates a logical brush from style, color, and pattern specified in the LOGPRUSH struct.
	{
		assert(m_pData);
		HBRUSH hBrush = ::CreateBrushIndirect(lpLogBrush);
		Attach(hBrush);
		return hBrush;
	}

	inline HBRUSH CBrush::CreateDIBPatternBrush(HGLOBAL hglbDIBPacked, UINT fuColorSpec)
	// Creates a logical brush that has the pattern specified by the specified device-independent bitmap (DIB).
	{
		assert(m_pData);
		HBRUSH hBrush = ::CreateDIBPatternBrush(hglbDIBPacked, fuColorSpec);
		Attach(hBrush);
		return hBrush;
	}

	inline HBRUSH CBrush::CreateDIBPatternBrushPt(LPCVOID lpPackedDIB, UINT nUsage)
	// Creates a logical brush that has the pattern specified by the device-independent bitmap (DIB).
	{
		assert(m_pData);
		HBRUSH hBrush = ::CreateDIBPatternBrushPt(lpPackedDIB, nUsage);
		Attach(hBrush);
		return hBrush;
	}

#endif // !defined(_WIN32_WCE)

	inline HBRUSH CBrush::CreatePatternBrush(CBitmap* pBitmap)
	// Creates a logical brush with the specified bitmap pattern. The bitmap can be a DIB section bitmap,
	// which is created by the CreateDIBSection function, or it can be a device-dependent bitmap.
	{
		assert(m_pData);
		assert(pBitmap);
		HBRUSH hBrush = ::CreatePatternBrush(*pBitmap);
		Attach(hBrush);
		return hBrush;
	}

	inline LOGBRUSH CBrush::GetLogBrush() const
	// Retrieves the LOGBRUSH structure that defines the style, color, and pattern of a physical brush.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		LOGBRUSH LogBrush = {0};
		::GetObject (m_pData->hGDIObject, sizeof(LOGBRUSH), &LogBrush);
		return LogBrush;
	}


	///////////////////////////////////////////////
	// Definitions of the CFont class
	//
	inline CFont::CFont()
	{
	}

	inline CFont::CFont(HFONT hFont)
	{
		assert(m_pData);
		Attach(hFont);
	}

	inline CFont::CFont(const LOGFONT* lpLogFont)
	{
		assert(m_pData);
		Attach( ::CreateFontIndirect(lpLogFont) );
	}

	inline CFont::operator HFONT() const
	{
		assert(m_pData);
		return (HFONT)m_pData->hGDIObject;
	}

	inline CFont::~CFont()
	{
	}

	inline HFONT CFont::CreateFontIndirect(const LOGFONT* lpLogFont)
	// Creates a logical font that has the specified characteristics.
	{
		assert(m_pData);
		HFONT hFont = ::CreateFontIndirect(lpLogFont);
		Attach(hFont);
		return hFont;
	}

	inline HFONT CFont::CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, CDC* pDC /*= NULL*/, BOOL bBold /*= FALSE*/, BOOL bItalic /*= FALSE*/)
	// Creates a font of a specified typeface and point size.
	{
		LOGFONT logFont = { 0 };
		logFont.lfCharSet = DEFAULT_CHARSET;
		logFont.lfHeight = nPointSize;

		lstrcpy(logFont.lfFaceName, lpszFaceName);

		if (bBold)
			logFont.lfWeight = FW_BOLD;
		if (bItalic)
			logFont.lfItalic = (BYTE)TRUE;

		return CreatePointFontIndirect(&logFont, pDC);
	}

	inline HFONT CFont::CreatePointFontIndirect(const LOGFONT* lpLogFont, CDC* pDC /* = NULL*/)
	// Creates a font of a specified typeface and point size.
	// This function automatically converts the height in lfHeight to logical units using the specified device context.
	{
		HDC hDC = pDC? pDC->GetHDC() : NULL;
		HDC hDC1 = (hDC != NULL) ? hDC : ::GetDC(HWND_DESKTOP);

		// convert nPointSize to logical units based on hDC
		LOGFONT logFont = *lpLogFont;

#ifndef _WIN32_WCE
		POINT pt = { 0, 0 };
		pt.y = ::MulDiv(::GetDeviceCaps(hDC1, LOGPIXELSY), logFont.lfHeight, 720);   // 72 points/inch, 10 decipoints/point
		::DPtoLP(hDC1, &pt, 1);

		POINT ptOrg = { 0, 0 };
		::DPtoLP(hDC1, &ptOrg, 1);

		logFont.lfHeight = -abs(pt.y - ptOrg.y);
#else // CE specific
		// DP and LP are always the same on CE
		logFont.lfHeight = -abs(((::GetDeviceCaps(hDC1, LOGPIXELSY)* logFont.lfHeight)/ 720));
#endif // _WIN32_WCE

		if (hDC == NULL)
			::ReleaseDC (NULL, hDC1);

		return CreateFontIndirect (&logFont);
	}

#ifndef _WIN32_WCE

	inline HFONT CFont::CreateFont(int nHeight, int nWidth, int nEscapement,
			int nOrientation, int nWeight, DWORD dwItalic, DWORD dwUnderline,
			DWORD dwStrikeOut, DWORD dwCharSet, DWORD dwOutPrecision,
			DWORD dwClipPrecision, DWORD dwQuality, DWORD dwPitchAndFamily,
			LPCTSTR lpszFacename)
	// Creates a logical font with the specified characteristics.
	{
		HFONT hFont = ::CreateFont(nHeight, nWidth, nEscapement,
			nOrientation, nWeight, dwItalic, dwUnderline, dwStrikeOut,
			dwCharSet, dwOutPrecision, dwClipPrecision, dwQuality,
			dwPitchAndFamily, lpszFacename);

		Attach(hFont);
		return hFont;
	}
#endif // #ifndef _WIN32_WCE

	inline LOGFONT CFont::GetLogFont() const
	// Retrieves the Logfont structure that contains font attributes.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		LOGFONT LogFont = {0};
		::GetObject(m_pData->hGDIObject, sizeof(LOGFONT), &LogFont);
		return LogFont;
	}


	///////////////////////////////////////////////
	// Definitions of the CPalette class
	//
	inline CPalette::CPalette()
	{
	}

	inline CPalette::CPalette(HPALETTE hPalette)
	{
		Attach(hPalette);
	}

	inline CPalette::operator HPALETTE() const
	{
		assert(m_pData);
		return (HPALETTE)m_pData->hGDIObject;
	}

	inline CPalette::~CPalette ()
	{
	}

	inline HPALETTE CPalette::CreatePalette(LPLOGPALETTE lpLogPalette)
	// Creates a logical palette from the information in the specified LOGPALETTE structure.
	{
		assert(m_pData);
		HPALETTE hPalette = ::CreatePalette (lpLogPalette);
		Attach(hPalette);
		return hPalette;
	}

#ifndef _WIN32_WCE
	inline HPALETTE CPalette::CreateHalftonePalette(CDC* pDC)
	// Creates a halftone palette for the specified device context (DC).
	{
		assert(m_pData);
		assert(pDC);
		HPALETTE hPalette = ::CreateHalftonePalette(pDC->GetHDC());
		Attach(hPalette);
		return hPalette;
	}
#endif // !_WIN32_WCE

	inline int CPalette::GetEntryCount() const
	// Retrieve the number of entries in the palette.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		WORD nEntries = 0;
		::GetObject(m_pData->hGDIObject, sizeof(WORD), &nEntries);
		return (int)nEntries;
	}

	inline UINT CPalette::GetPaletteEntries(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors) const
	// Retrieves a specified range of palette entries from the palette.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::GetPaletteEntries((HPALETTE)m_pData->hGDIObject, nStartIndex, nNumEntries, lpPaletteColors);
	}

	inline UINT CPalette::SetPaletteEntries(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors)
	// Sets RGB (red, green, blue) color values and flags in a range of entries in the palette.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::SetPaletteEntries((HPALETTE)m_pData->hGDIObject, nStartIndex, nNumEntries, lpPaletteColors);
	}

#ifndef _WIN32_WCE
	inline void CPalette::AnimatePalette(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors)
	// Replaces entries in the palette.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		::AnimatePalette((HPALETTE)m_pData->hGDIObject, nStartIndex, nNumEntries, lpPaletteColors);
	}

	inline BOOL CPalette::ResizePalette(UINT nNumEntries)
	//  Increases or decreases the size of the palette based on the specified value.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::ResizePalette((HPALETTE)m_pData->hGDIObject, nNumEntries);
	}
#endif // !_WIN32_WCE

	inline UINT CPalette::GetNearestPaletteIndex(COLORREF crColor) const
	// Retrieves the index for the entry in the palette most closely matching a specified color value.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::GetNearestPaletteIndex((HPALETTE)m_pData->hGDIObject, crColor);
	}


	///////////////////////////////////////////////
	// Declarations for the CPen class
	//
	inline CPen::CPen()
	{
	}

	inline CPen::CPen(HPEN hPen)
	{
		Attach(hPen);
	}

	inline CPen::CPen(int nPenStyle, int nWidth, COLORREF crColor)
	{
		assert(m_pData);
		Attach( ::CreatePen(nPenStyle, nWidth, crColor) );
	}

#ifndef _WIN32_WCE
	inline CPen::CPen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount /*= 0*/, const DWORD* lpStyle /*= NULL*/)
	{
		assert(m_pData);
		Attach( ::ExtCreatePen(nPenStyle, nWidth, pLogBrush, nStyleCount, lpStyle) );
	}
#endif // !_WIN32_WCE

	inline CPen::operator HPEN () const
	{
		assert(m_pData);
		return (HPEN)m_pData->hGDIObject;
	}

	inline CPen::~CPen()
	{
	}

	inline HPEN CPen::CreatePen(int nPenStyle, int nWidth, COLORREF crColor)
	// Creates a logical pen that has the specified style, width, and color.
	{
		assert(m_pData);
		HPEN hPen = ::CreatePen(nPenStyle, nWidth, crColor);
		Attach(hPen);
		return hPen;
	}

	inline HPEN CPen::CreatePenIndirect(LPLOGPEN lpLogPen)
	// Creates a logical cosmetic pen that has the style, width, and color specified in a structure.
	{
		assert(m_pData);
		HPEN hPen = ::CreatePenIndirect(lpLogPen);
		Attach(hPen);
		return hPen;
	}

	inline LOGPEN CPen::GetLogPen() const
	{
		// Retrieves the LOGPEN struct that specifies the pen's style, width, and color.
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);

		LOGPEN LogPen = {0};
		::GetObject(m_pData->hGDIObject, sizeof(LOGPEN), &LogPen);
		return LogPen;
	}

#ifndef _WIN32_WCE
	inline HPEN CPen::ExtCreatePen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount /* = 0*/, const DWORD* lpStyle /*= NULL*/)
	// Creates a logical cosmetic or geometric pen that has the specified style, width, and brush attributes.
	{
		assert(m_pData);
		HPEN hPen = ::ExtCreatePen(nPenStyle, nWidth, pLogBrush, nStyleCount, lpStyle);
		Attach(hPen);
		return hPen;
	}

	inline EXTLOGPEN CPen::GetExtLogPen() const
	// Retrieves the EXTLOGPEN struct that specifies the pen's style, width, color and brush attributes.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);

		EXTLOGPEN ExLogPen = {0};
		::GetObject(m_pData->hGDIObject, sizeof(EXTLOGPEN), &ExLogPen);
		return ExLogPen;
	}
#endif // !_WIN32_WCE


	///////////////////////////////////////////////
	// Definitions of the CRgn class
	//
	inline CRgn::CRgn()
	{
	}

	inline CRgn::CRgn(HRGN hRgn)
	{
		assert(m_pData);
		Attach(hRgn);
	}

	inline CRgn::operator HRGN() const
	{
		assert(m_pData);
		return (HRGN)m_pData->hGDIObject;
	}

	inline CRgn::~CRgn()
	{
	}

	inline HRGN CRgn::CreateRectRgn(int x1, int y1, int x2, int y2)
	// Creates a rectangular region.
	{
		assert(m_pData);
		HRGN hRgn = ::CreateRectRgn(x1, y1, x2, y2);
		Attach(hRgn);
		return hRgn;
	}

	inline HRGN CRgn::CreateRectRgnIndirect(const RECT& rc)
	// Creates a rectangular region.
	{
		assert(m_pData);
		HRGN hRgn = ::CreateRectRgnIndirect(&rc);
		Attach(hRgn);
		return hRgn;
	}

#ifndef _WIN32_WCE
	inline HRGN CRgn::CreateEllipticRgn(int x1, int y1, int x2, int y2)
	// Creates an elliptical region.
	{
		assert(m_pData);
		HRGN hRgn = ::CreateEllipticRgn(x1, y1, x2, y2);
		Attach(hRgn);
		return hRgn;
	}

	inline HRGN CRgn::CreateEllipticRgnIndirect(const RECT& rc)
	// Creates an elliptical region.
	{
		assert(m_pData);
		HRGN hRgn = ::CreateEllipticRgnIndirect(&rc);
		Attach(hRgn);
		return hRgn;
	}

	inline HRGN CRgn::CreatePolygonRgn(LPPOINT lpPoints, int nCount, int nMode)
	// Creates a polygonal region.
	{
		assert(m_pData);
		HRGN hRgn = ::CreatePolygonRgn(lpPoints, nCount, nMode);
		Attach(hRgn);
		return hRgn;
	}

	inline HRGN CRgn::CreatePolyPolygonRgn(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount, int nPolyFillMode)
	// Creates a region consisting of a series of polygons. The polygons can overlap.
	{
		assert(m_pData);
		HRGN hRgn = ::CreatePolyPolygonRgn(lpPoints, lpPolyCounts, nCount, nPolyFillMode);
		Attach(hRgn);
		return hRgn;
	}

	inline HRGN CRgn::CreateRoundRectRgn(int x1, int y1, int x2, int y2, int x3, int y3)
	// Creates a rectangular region with rounded corners.
	{
		assert(m_pData);
		HRGN hRgn = ::CreateRoundRectRgn(x1, y1, x2, y2, x3, y3);
		Attach(hRgn);
		return hRgn;
	}

	inline HRGN CRgn::CreateFromPath(HDC hDC)
	// Creates a region from the path that is selected into the specified device context.
	// The resulting region uses device coordinates.
	{
		assert(m_pData);
		assert(hDC != NULL);
		HRGN hRgn = ::PathToRegion(hDC);
		Attach(hRgn);
		return hRgn;
	}

#endif // !_WIN32_WCE

	inline HRGN CRgn::CreateFromData(const XFORM* lpXForm, int nCount, const RGNDATA* pRgnData)
	// Creates a region from the specified region and transformation data.
	{
		assert(m_pData);
		HRGN hRgn = ::ExtCreateRegion(lpXForm, nCount, pRgnData);
		Attach(hRgn);
		return hRgn;
	}

	inline void CRgn::SetRectRgn(int x1, int y1, int x2, int y2)
	// converts the region into a rectangular region with the specified coordinates.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		::SetRectRgn((HRGN)m_pData->hGDIObject, x1, y1, x2, y2);
	}

	inline void CRgn::SetRectRgn(const RECT& rc)
	// converts the region into a rectangular region with the specified coordinates.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		::SetRectRgn((HRGN)m_pData->hGDIObject, rc.left, rc.top, rc.right, rc.bottom);
	}

	inline int CRgn::CombineRgn(CRgn* pRgnSrc1, CRgn* pRgnSrc2, int nCombineMode)
	// Combines two sepcified regions and stores the result.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		assert(pRgnSrc1);
		assert(pRgnSrc2);
		return ::CombineRgn((HRGN)m_pData->hGDIObject, *pRgnSrc1, *pRgnSrc2, nCombineMode);
	}

	inline int CRgn::CombineRgn(CRgn* pRgnSrc, int nCombineMode)
	// Combines the sepcified region with the current region.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		assert(pRgnSrc);
		return ::CombineRgn((HRGN)m_pData->hGDIObject, (HRGN)m_pData->hGDIObject, *pRgnSrc, nCombineMode);
	}

	inline int CRgn::CopyRgn(CRgn* pRgnSrc)
	// Assigns the specified region to the current region.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject == NULL);
		assert(pRgnSrc);
		return ::CombineRgn((HRGN)m_pData->hGDIObject, *pRgnSrc, NULL, RGN_COPY);
	}

	inline BOOL CRgn::EqualRgn(CRgn* pRgn) const
	// Checks the two specified regions to determine whether they are identical.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		assert(pRgn);
		return ::EqualRgn((HRGN)m_pData->hGDIObject, *pRgn);
	}

	inline int CRgn::OffsetRgn(int x, int y)
	// Moves a region by the specified offsets.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::OffsetRgn((HRGN)m_pData->hGDIObject, x, y);
	}

	inline int CRgn::OffsetRgn(POINT& pt)
	// Moves a region by the specified offsets.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::OffsetRgn((HRGN)m_pData->hGDIObject, pt.x, pt.y);
	}

	inline int CRgn::GetRgnBox(RECT& rc) const
	// Retrieves the bounding rectangle of the region, and stores it in the specified RECT.
	// The return value indicates the region's complexity: NULLREGION;SIMPLEREGION; or COMPLEXREGION.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::GetRgnBox((HRGN)m_pData->hGDIObject, &rc);
	}

	inline int CRgn::GetRegionData(LPRGNDATA lpRgnData, int nDataSize) const
	// Fills the specified buffer with data describing a region.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return (int)::GetRegionData((HRGN)m_pData->hGDIObject, nDataSize, lpRgnData);
	}

	inline BOOL CRgn::PtInRegion(int x, int y) const
	// Determines whether the specified point is inside the specified region.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::PtInRegion((HRGN)m_pData->hGDIObject, x, y);
	}

	inline BOOL CRgn::PtInRegion(POINT& pt) const
	// Determines whether the specified point is inside the specified region.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::PtInRegion((HRGN)m_pData->hGDIObject, pt.x, pt.y);
	}

	inline BOOL CRgn::RectInRegion(const RECT& rc) const
	// Determines whether the specified rect is inside the specified region.
	{
		assert(m_pData);
		assert(m_pData->hGDIObject != NULL);
		return ::RectInRegion((HRGN)m_pData->hGDIObject, &rc);
	}


	///////////////////////////////////////////////
	// Definitions of the CDC class
	//
	inline CDC::CDC()
	{
		// Allocate memory for our data members
		m_pData = new DataMembers;

		// Assign values to our data members
		m_pData->hDC = 0;
		m_pData->Count = 1L;
		m_pData->bRemoveHDC = TRUE;
		m_pData->hWnd = 0;
	}

	inline CDC::CDC(HDC hDC, HWND hWnd /*= 0*/)
	// This constructor assigns an existing HDC to the CDC
	// The HDC WILL be released or deleted when the CDC object is destroyed
	// The hWnd paramter is only used in WindowsCE. It specifies the HWND of a Window or
	// Window Client DC

	// Note: this constructor permits a call like this:
	// CDC MyCDC = SomeHDC;
	//  or
	// CDC MyCDC = ::CreateCompatibleDC(SomeHDC);
	//  or
	// CDC MyCDC = ::GetDC(SomeHWND);
	{
		UNREFERENCED_PARAMETER(hWnd);
		assert(hDC);

		CDC* pDC = GetApp()->GetCDCFromMap(hDC);
		if (pDC)
		{
			m_pData = pDC->m_pData;
			InterlockedIncrement(&m_pData->Count);
		}
		else
		{
			// Allocate memory for our data members
			m_pData = new DataMembers;

			// Assign values to our data members
			m_pData->hDC = hDC;
			m_pData->Count = 1L;
			m_pData->bRemoveHDC = TRUE;
			m_pData->nSavedDCState = ::SaveDC(hDC);
#ifndef _WIN32_WCE
			m_pData->hWnd = ::WindowFromDC(hDC);
#else
			m_pData->hWnd = hWnd;
#endif
			if (m_pData->hWnd)
				AddToMap();
		}
	}

#ifndef _WIN32_WCE
	inline void CDC::operator = (const HDC hDC)
	// Note: this assignment operater permits a call like this:
	// CDC MyCDC;
	// MyCDC = SomeHDC;
	{
		Attach(hDC);
	}
#endif

	inline CDC::CDC(const CDC& rhs)	// Copy constructor
	// The copy constructor is called when a temporary copy of the CDC needs to be created.
	// This can happen when a CDC is passed by value in a function call. Each CDC copy manages
	// the same Device Context and GDI objects.
	{
		m_pData = rhs.m_pData;
		InterlockedIncrement(&m_pData->Count);
	}

	inline CDC& CDC::operator = (const CDC& rhs)
	// Note: A copy of a CDC is a clone of the original.
	//       Both objects manipulate the one HDC
	{
		if (this != &rhs)
		{
			InterlockedIncrement(&rhs.m_pData->Count);
			Release();
			m_pData = rhs.m_pData;
		}

		return *this;
	}

	inline CDC::~CDC ()
	{
		Release();
	}

	inline void CDC::AddToMap()
	// Store the HDC and CDC pointer in the HDC map
	{
		assert( GetApp() );
		assert(m_pData->hDC);
		GetApp()->m_csMapLock.Lock();

		assert(m_pData->hDC);
		assert(!GetApp()->GetCDCFromMap(m_pData->hDC));

		GetApp()->m_mapHDC.insert(std::make_pair(m_pData->hDC, this));
		GetApp()->m_csMapLock.Release();
	}

	inline void CDC::Attach(HDC hDC, HWND hWnd /* = 0*/)
	// Attaches a HDC to the CDC object.
	// The HDC will be automatically deleted or released when the destructor is called.
	// The hWnd parameter is only used on WindowsCE. It specifies the HWND of a Window or
	// Window Client DC
	{
		UNREFERENCED_PARAMETER(hWnd);
		assert(m_pData);
		assert(0 == m_pData->hDC);
		assert(hDC);

		CDC* pDC = GetApp()->GetCDCFromMap(hDC);
		if (pDC)
		{
			delete m_pData;
			m_pData = pDC->m_pData;
			InterlockedIncrement(&m_pData->Count);
		}
		else
		{
			m_pData->hDC = hDC;

#ifndef _WIN32_WCE
			m_pData->hWnd = ::WindowFromDC(hDC);
#else
			m_pData->hWnd = hWnd;
#endif

			if (m_pData->hWnd == 0)
				AddToMap();
			m_pData->nSavedDCState = ::SaveDC(hDC);
		}
	}

	inline HDC CDC::Detach()
	// Detaches the HDC from this object.
	{
		assert(m_pData);
		assert(m_pData->hDC);

		GetApp()->m_csMapLock.Lock();
		RemoveFromMap();
		HDC hDC = m_pData->hDC;
		m_pData->hDC = 0;

		if (m_pData->Count)
		{
			if (InterlockedDecrement(&m_pData->Count) == 0)
			{
				delete m_pData;
			}
		}

		GetApp()->m_csMapLock.Release();

		// Assign values to our data members
		m_pData = new DataMembers;
		m_pData->hDC = 0;
		m_pData->Count = 1L;
		m_pData->bRemoveHDC = TRUE;
		m_pData->hWnd = 0;

		return hDC;
	}

	// Initialization
	inline BOOL CDC::CreateCompatibleDC(CDC* pDC)
	// Returns a memory device context (DC) compatible with the specified device.
	{
		assert(m_pData->hDC == NULL);
		HDC hdcSource = (pDC == NULL)? NULL : pDC->GetHDC();
		HDC hDC = ::CreateCompatibleDC(hdcSource);
		if (hDC)
		{
			m_pData->hDC = hDC;
			AddToMap();
		}
		return (hDC != NULL);	// boolean expression
	}

	inline BOOL CDC::CreateDC(LPCTSTR lpszDriver, LPCTSTR lpszDevice, LPCTSTR lpszOutput, const DEVMODE* pInitData)
	// Returns a device context (DC) for a device using the specified name.
	{
		assert(m_pData->hDC == NULL);
		HDC hDC = ::CreateDC(lpszDriver, lpszDevice, lpszOutput, pInitData);
		if (hDC)
		{
			m_pData->hDC = hDC;
			AddToMap();
		}
		return (hDC != NULL);	// boolean expression
	}

#ifndef _WIN32_WCE
	inline BOOL CDC::CreateIC(LPCTSTR lpszDriver, LPCTSTR lpszDevice, LPCTSTR lpszOutput, const DEVMODE* pInitData)
	{
		assert(m_pData->hDC == NULL);
		HDC hDC = ::CreateIC(lpszDriver, lpszDevice, lpszOutput, pInitData);
		if (hDC)
		{
			m_pData->hDC = hDC;
			AddToMap();
		}
		return (hDC != NULL);	// boolean expression
	}
#endif

	inline void CDC::DrawBitmap(int x, int y, int cx, int cy, CBitmap& Bitmap, COLORREF clrMask)
	// Draws the specified bitmap to the specified DC using the mask colour provided as the transparent colour
	// Suitable for use with a Window DC or a memory DC
	{
		// Create the Image memory DC
		CMemDC dcImage(this);
		dcImage.SetBkColor(clrMask);
		dcImage.SelectObject(&Bitmap);

		// Create the Mask memory DC
		CMemDC dcMask(this);
        dcMask.CreateBitmap(cx, cy, 1, 1, NULL);
		dcMask.BitBlt(0, 0, cx, cy, &dcImage, 0, 0, SRCCOPY);

		// Mask the image to 'this' DC
		BitBlt(x, y, cx, cy, &dcImage, 0, 0, SRCINVERT);
		BitBlt(x, y, cx, cy, &dcMask, 0, 0, SRCAND);
		BitBlt(x, y, cx, cy, &dcImage, 0, 0, SRCINVERT);
	}

	inline CDC* CDC::AddTempHDC(HDC hDC, HWND hWnd)
	// Returns the CDC object associated with the device context handle
	// The HDC is removed when the CDC is destroyed
	{
		assert( GetApp() );
		CDC* pDC = new CDC;
		pDC->m_pData->hDC = hDC;
		GetApp()->AddTmpDC(pDC);
		pDC->m_pData->bRemoveHDC = TRUE;
		pDC->m_pData->hWnd = hWnd;
		return pDC;
	}

	inline void CDC::GradientFill(COLORREF Color1, COLORREF Color2, const RECT& rc, BOOL bVertical)
	// An efficient color gradient filler compatible with all Windows operating systems
	{
		int Width = rc.right - rc.left;
		int Height = rc.bottom - rc.top;

		int r1 = GetRValue(Color1);
		int g1 = GetGValue(Color1);
		int b1 = GetBValue(Color1);

		int r2 = GetRValue(Color2);
		int g2 = GetGValue(Color2);
		int b2 = GetBValue(Color2);

		COLORREF OldBkColor = GetBkColor();

		if (bVertical)
		{
			for(int i=0; i < Width; ++i)
			{
				int r = r1 + (i * (r2-r1) / Width);
				int g = g1 + (i * (g2-g1) / Width);
				int b = b1 + (i * (b2-b1) / Width);
				SetBkColor(RGB(r, g, b));
				CRect line( i + rc.left, rc.top, i + 1 + rc.left, rc.top+Height);
				ExtTextOut(0, 0, ETO_OPAQUE, line, NULL, 0, 0);
			}
		}
		else
		{
			for(int i=0; i < Height; ++i)
			{
				int r = r1 + (i * (r2-r1) / Height);
				int g = g1 + (i * (g2-g1) / Height);
				int b = b1 + (i * (b2-b1) / Height);
				SetBkColor(RGB(r, g, b));
				CRect line(rc.left, i + rc.top, rc.left+Width, i + 1 + rc.top);
				ExtTextOut(0, 0, ETO_OPAQUE, line, NULL, 0, 0);
			}
		}

		SetBkColor(OldBkColor);
	}

	inline void CDC::Release()
	{
		GetApp()->m_csMapLock.Lock();

		if (m_pData->Count)
		{
			if (InterlockedDecrement(&m_pData->Count) == 0)
			{
				Destroy();
				delete m_pData;
				m_pData = 0;
			}
		}

		GetApp()->m_csMapLock.Release();
	}

	inline BOOL CDC::RemoveFromMap()
	{
		BOOL Success = FALSE;

		if( GetApp() )
		{
			// Allocate an iterator for our HDC map
			std::map<HDC, CDC*, CompareHDC>::iterator m;

			CWinApp* pApp = GetApp();
			if (pApp)
			{
				// Erase the CDC pointer entry from the map
				pApp->m_csMapLock.Lock();
				m = pApp->m_mapHDC.find(m_pData->hDC);
				if (m != pApp->m_mapHDC.end())
				{
					pApp->m_mapHDC.erase(m);
					Success = TRUE;
				}

				pApp->m_csMapLock.Release();
			}
		}
		return Success;
	}

	inline void CDC::SolidFill(COLORREF Color, const RECT& rc)
	// Fills a rectangle with a solid color
	{
		COLORREF OldColor = SetBkColor(Color);
		ExtTextOut(0, 0, ETO_OPAQUE, &rc, NULL, 0, 0);
		SetBkColor(OldColor);
	}

	// Bitmap functions
	inline CBitmap* CDC::CreateCompatibleBitmap(CDC* pDC, int cx, int cy)
	// Creates a compatible bitmap and selects it into the device context.
	{
		assert(m_pData->hDC);
		assert(pDC);

		CBitmap* pBitmap = new CBitmap;
		pBitmap->CreateCompatibleBitmap(pDC, cx, cy);
		m_pData->m_vGDIObjects.push_back(pBitmap);
		return SelectObject(pBitmap);
	}

	inline CBitmap* CDC::CreateBitmap(int cx, int cy, UINT Planes, UINT BitsPerPixel, LPCVOID pvColors)
	// Creates a bitmap and selects it into the device context.
	// Returns a pointer to the old bitmap selected out of the device context
	{
		assert(m_pData->hDC);

		CBitmap* pBitmap = new CBitmap;
		pBitmap->CreateBitmap(cx, cy, Planes, BitsPerPixel, pvColors);
		m_pData->m_vGDIObjects.push_back(pBitmap);
		return SelectObject(pBitmap);
	}

#ifndef _WIN32_WCE
	inline CBitmap* CDC::CreateBitmapIndirect (LPBITMAP lpBitmap)
	// Creates a bitmap and selects it into the device context.
	// Returns a pointer to the old bitmap selected out of the device context
	{
		assert(m_pData->hDC);

		CBitmap* pBitmap = new CBitmap;
		pBitmap->CreateBitmapIndirect(lpBitmap);
		m_pData->m_vGDIObjects.push_back(pBitmap);
		return SelectObject(pBitmap);
	}

	inline CBitmap* CDC::CreateDIBitmap(CDC* pDC, const BITMAPINFOHEADER& bmih, DWORD fdwInit, LPCVOID lpbInit,
										BITMAPINFO& bmi,  UINT fuUsage)
	// Creates a bitmap and selects it into the device context.
	// Returns a pointer to the old bitmap selected out of the device context
	{
		assert(m_pData->hDC);
		assert(pDC);

		CBitmap* pBitmap = new CBitmap;
		pBitmap->CreateDIBitmap(pDC, &bmih, fdwInit, lpbInit, &bmi, fuUsage);
		m_pData->m_vGDIObjects.push_back(pBitmap);
		return SelectObject(pBitmap);
	}
#endif

	inline CBitmap* CDC::CreateDIBSection(CDC* pDC, const BITMAPINFO& bmi, UINT iUsage, LPVOID *ppvBits,
										HANDLE hSection, DWORD dwOffset)
	// Creates a bitmap and selects it into the device context.
	// Returns a pointer to the old bitmap selected out of the device context
	{
		assert(m_pData->hDC);
		assert(pDC);

		CBitmap* pBitmap = new CBitmap;
		pBitmap->CreateDIBSection(pDC, &bmi, iUsage, ppvBits, hSection, dwOffset);
		m_pData->m_vGDIObjects.push_back(pBitmap);
		return SelectObject(pBitmap);
	}

	inline void CDC::Destroy()
	// Deletes or releases the device context and returns the CDC object to its
	// default state, ready for reuse.
	{
		if (m_pData->hDC)
		{
			RemoveFromMap();
			if (m_pData->bRemoveHDC)
			{
				// Return the DC back to its initial state
				::RestoreDC(m_pData->hDC, m_pData->nSavedDCState);

				// We need to release a Window DC, and delete a memory DC
				if (m_pData->hWnd)
					::ReleaseDC(m_pData->hWnd, m_pData->hDC);
				else
					if (!::DeleteDC(m_pData->hDC))
						::ReleaseDC(NULL, m_pData->hDC);

				m_pData->hDC = 0;
				m_pData->hWnd = 0;
				m_pData->bRemoveHDC = TRUE;
			}
		}

	//	RemoveFromMap();
	}

	inline BITMAP CDC::GetBitmapData() const
	// Retrieves the BITMAP information for the current HBITMAP.
	{
		assert(m_pData->hDC);

		HBITMAP hbm = (HBITMAP)::GetCurrentObject(m_pData->hDC, OBJ_BITMAP);
		BITMAP bm = {0};
		::GetObject(hbm, sizeof(bm), &bm);
		return bm;
	}

	inline CBitmap* CDC::LoadBitmap(UINT nID)
	// Loads a bitmap from the resource and selects it into the device context
	{
		return LoadBitmap(MAKEINTRESOURCE(nID));
	}

	inline CBitmap* CDC::LoadBitmap(LPCTSTR lpszName)
	// Loads a bitmap from the resource and selects it into the device context
	// Returns a pointer to the old bitmap selected out of the device context
	{
		assert(m_pData->hDC);

		CBitmap* pBitmap = new CBitmap;
		BOOL bResult = pBitmap->LoadBitmap(lpszName);
		m_pData->m_vGDIObjects.push_back(pBitmap);

		return bResult? SelectObject(pBitmap) : NULL;
	}

	inline CBitmap* CDC::LoadImage(UINT nID, int cxDesired, int cyDesired, UINT fuLoad)
	// Loads a bitmap from the resource and selects it into the device context
	// Returns a pointer to the old bitmap selected out of the device context
	{
		return LoadImage(nID, cxDesired, cyDesired, fuLoad);
	}

	inline CBitmap* CDC::LoadImage(LPCTSTR lpszName, int cxDesired, int cyDesired, UINT fuLoad)
	// Loads a bitmap from the resource and selects it into the device context
	// Returns a pointer to the old bitmap selected out of the device context
	{
		assert(m_pData->hDC);

		CBitmap* pBitmap = new CBitmap;
		BOOL bResult = pBitmap->LoadImage(lpszName, cxDesired, cyDesired, fuLoad);
		m_pData->m_vGDIObjects.push_back(pBitmap);
		return bResult? SelectObject(pBitmap) : NULL;
	}

	inline CBitmap* CDC::LoadOEMBitmap(UINT nIDBitmap) // for OBM_/OCR_/OIC_
	// Loads a predefined system bitmap and selects it into the device context
	// Returns a pointer to the old bitmap selected out of the device context
	{
		assert(m_pData->hDC);

		CBitmap* pBitmap = new CBitmap;
		BOOL bResult = pBitmap->LoadOEMBitmap(nIDBitmap);
		m_pData->m_vGDIObjects.push_back(pBitmap);
		return bResult? SelectObject(pBitmap) : NULL;
	}

#ifndef _WIN32_WCE
	inline CBitmap* CDC::CreateMappedBitmap(UINT nIDBitmap, UINT nFlags /*= 0*/, LPCOLORMAP lpColorMap /*= NULL*/, int nMapSize /*= 0*/)
	// creates and selects a new bitmap using the bitmap data and colors specified by the bitmap resource and the color mapping information.
	// Returns a pointer to the old bitmap selected out of the device context
	{
		assert(m_pData->hDC);

		CBitmap* pBitmap = new CBitmap;
		pBitmap->CreateMappedBitmap(nIDBitmap, (WORD)nFlags, lpColorMap, nMapSize);
		m_pData->m_vGDIObjects.push_back(pBitmap);
		return SelectObject(pBitmap);
	}
#endif // !_WIN32_WCE


	// Brush functions
#ifndef _WIN32_WCE
	inline CBrush* CDC::CreateBrushIndirect(LPLOGBRUSH pLogBrush)
	// Creates the brush and selects it into the device context.
	// Returns a pointer to the old brush selected out of the device context.
	{
		assert(m_pData->hDC);

		CBrush* pBrush = new CBrush;
		pBrush->CreateBrushIndirect(pLogBrush);
		m_pData->m_vGDIObjects.push_back(pBrush);
		return SelectObject(pBrush);
	}

	inline CBrush* CDC::CreateHatchBrush(int fnStyle, COLORREF rgb)
	// Creates a brush with the specified hatch pattern and color, and selects it into the device context.
	// Returns a pointer to the old brush selected out of the device context.
	{
		assert(m_pData->hDC);

		CBrush* pBrush = new CBrush;
		pBrush->CreateHatchBrush(fnStyle, rgb);
		m_pData->m_vGDIObjects.push_back(pBrush);
		return SelectObject(pBrush);
	}

	inline CBrush* CDC::CreateDIBPatternBrush(HGLOBAL hglbDIBPacked, UINT fuColorSpec)
	// Creates a logical from the specified device-independent bitmap (DIB), and selects it into the device context.
	// Returns a pointer to the old brush selected out of the device context.
	{
		assert(m_pData->hDC);

		CBrush* pBrush = new CBrush;
		pBrush->CreateDIBPatternBrush(hglbDIBPacked, fuColorSpec);
		m_pData->m_vGDIObjects.push_back(pBrush);
		return SelectObject(pBrush);
	}
	
	inline CBrush* CDC::CreateDIBPatternBrushPt(LPCVOID lpPackedDIB, UINT iUsage)
	// Creates a logical from the specified device-independent bitmap (DIB), and selects it into the device context.
	// Returns a pointer to the old brush selected out of the device context.
	{
		assert(m_pData->hDC);

		CBrush* pBrush = new CBrush;
		pBrush->CreateDIBPatternBrushPt(lpPackedDIB, iUsage);
		m_pData->m_vGDIObjects.push_back(pBrush);
		return SelectObject(pBrush);
	}
#endif

	inline CBrush* CDC::CreatePatternBrush(CBitmap* pBitmap)
	// Creates the brush with the specified pattern, and selects it into the device context.
	// Returns a pointer to the old brush selected out of the device context.
	{
		assert(m_pData->hDC);
		assert(pBitmap);

		CBrush* pBrush = new CBrush;
		pBrush->CreatePatternBrush(pBitmap);
		m_pData->m_vGDIObjects.push_back(pBrush);
		return SelectObject(pBrush);
	}

	inline CBrush* CDC::CreateSolidBrush(COLORREF rgb)
	// Creates the brush with the specified color, and selects it into the device context.
	// Returns a pointer to the old brush selected out of the device context.
	{
		assert(m_pData->hDC);

		CBrush* pBrush = new CBrush;
		pBrush->CreateSolidBrush(rgb);
		m_pData->m_vGDIObjects.push_back(pBrush);
		return SelectObject(pBrush);
	}

	inline LOGBRUSH CDC::GetLogBrush() const
	// Retrieves the current brush information
	{
		assert(m_pData->hDC);

		HBRUSH hBrush = (HBRUSH)::GetCurrentObject(m_pData->hDC, OBJ_BRUSH);
		LOGBRUSH lBrush = {0};
		::GetObject(hBrush, sizeof(lBrush), &lBrush);
		return lBrush;
	}


	// Font functions
#ifndef _WIN32_WCE
	inline CFont* CDC::CreateFont (
					int nHeight,               // height of font
  					int nWidth,                // average character width
  					int nEscapement,           // angle of escapement
  					int nOrientation,          // base-line orientation angle
  					int fnWeight,              // font weight
  					DWORD fdwItalic,           // italic attribute option
  					DWORD fdwUnderline,        // underline attribute option
  					DWORD fdwStrikeOut,        // strikeout attribute option
  					DWORD fdwCharSet,          // character set identifier
  					DWORD fdwOutputPrecision,  // output precision
  					DWORD fdwClipPrecision,    // clipping precision
  					DWORD fdwQuality,          // output quality
  					DWORD fdwPitchAndFamily,   // pitch and family
  					LPCTSTR lpszFace           // typeface name
 					)

	// Creates a logical font with the specified characteristics.
	// Returns a pointer to the old font selected out of the device context.
	{
		assert(m_pData->hDC);

		CFont* pFont = new CFont;
		pFont->CreateFont (nHeight, nWidth, nEscapement, nOrientation, fnWeight,
								fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet,
								fdwOutputPrecision, fdwClipPrecision, fdwQuality,
								fdwPitchAndFamily, lpszFace);
		m_pData->m_vGDIObjects.push_back(pFont);
		return SelectObject(pFont);
	}
#endif

	inline CFont* CDC::CreateFontIndirect(LPLOGFONT plf)
	// Creates a logical font and selects it into the device context.
	// Returns a pointer to the old font selected out of the device context.
	{
		assert(m_pData->hDC);

		CFont* pFont = new CFont;
		pFont->CreateFontIndirect(plf);
		m_pData->m_vGDIObjects.push_back(pFont);
		return SelectObject(pFont);
	}

	inline LOGFONT CDC::GetLogFont() const
	// Retrieves the current font information.
	{
		assert(m_pData->hDC);

		HFONT hFont = (HFONT)::GetCurrentObject(m_pData->hDC, OBJ_FONT);
		LOGFONT lFont = {0};
		::GetObject(hFont, sizeof(lFont), &lFont);
		return lFont;
	}

	// Pen functions
	inline CPen* CDC::CreatePen (int nStyle, int nWidth, COLORREF rgb)
	// Creates the pen and selects it into the device context.
	// Returns a pointer to the old pen selected out of the device context.
	{
		assert(m_pData->hDC);

		CPen* pPen = new CPen;
		pPen->CreatePen(nStyle, nWidth, rgb);
		m_pData->m_vGDIObjects.push_back(pPen);
		return SelectObject(pPen);
	}

	inline CPen* CDC::CreatePenIndirect (LPLOGPEN pLogPen)
	// Creates the pen and selects it into the device context.
	// Returns a pointer to the old pen selected out of the device context.
	{
		assert(m_pData->hDC);

		CPen* pPen = new CPen;
		pPen->CreatePenIndirect(pLogPen);
		m_pData->m_vGDIObjects.push_back(pPen);
		return SelectObject(pPen);
	}

	inline LOGPEN CDC::GetLogPen() const
	// Retrieves the current pen information as a LOGPEN
	{
		assert(m_pData->hDC);

		HPEN hPen = (HPEN)::GetCurrentObject(m_pData->hDC, OBJ_PEN);
		LOGPEN lPen = {0};
		::GetObject(hPen, sizeof(lPen), &lPen);
		return lPen;
	}

	// Region functions
	inline int CDC::CreateRectRgn(int left, int top, int right, int bottom)
	// Creates a rectangular region from the rectangle co-ordinates.
	// The return value specifies the region's complexity: NULLREGION;SIMPLEREGION;COMPLEXREGION;ERROR.
	{
		assert(m_pData->hDC);

		CRgn* pRgn = new CRgn;
		pRgn->CreateRectRgn(left, top, right, bottom);
		m_pData->m_vGDIObjects.push_back(pRgn);
		return SelectClipRgn(pRgn);
	}

	inline int CDC::CreateRectRgnIndirect(const RECT& rc)
	// Creates a rectangular region from the rectangle co-ordinates.
	// The return value specifies the region's complexity: NULLREGION;SIMPLEREGION;COMPLEXREGION;ERROR.
	{
		assert(m_pData->hDC);

		CRgn* pRgn = new CRgn;
		pRgn->CreateRectRgnIndirect(rc);
		m_pData->m_vGDIObjects.push_back(pRgn);
		return SelectClipRgn(pRgn);
	}

	inline int CDC::CreateFromData(const XFORM* Xform, DWORD nCount, const RGNDATA *pRgnData)
	// Creates a region from the specified region data and tranformation data.
	// The return value specifies the region's complexity: NULLREGION;SIMPLEREGION;COMPLEXREGION;ERROR.
	// Notes: GetRegionData can be used to get a region's data
	//        If the XFROM pointer is NULL, the identity transformation is used.
	{
		assert(m_pData->hDC);

		CRgn* pRgn = new CRgn;
		pRgn->CreateFromData(Xform, nCount, pRgnData);
		m_pData->m_vGDIObjects.push_back(pRgn);
		return SelectClipRgn(pRgn);
	}


#ifndef _WIN32_WCE
	inline int CDC::CreateEllipticRgn(int left, int top, int right, int bottom)
	// Creates the ellyiptical region from the bounding rectangle co-ordinates
	// and selects it into the device context.
	// The return value specifies the region's complexity: NULLREGION;SIMPLEREGION;COMPLEXREGION;ERROR.
	{
		assert(m_pData->hDC);

		CRgn* pRgn = new CRgn;
		pRgn->CreateEllipticRgn(left, top, right, bottom);
		m_pData->m_vGDIObjects.push_back(pRgn);
		return SelectClipRgn(pRgn);
	}

	inline int CDC::CreateEllipticRgnIndirect(const RECT& rc)
	// Creates the ellyiptical region from the bounding rectangle co-ordinates
	// and selects it into the device context.
	// The return value specifies the region's complexity: NULLREGION;SIMPLEREGION;COMPLEXREGION;ERROR.
	{
		assert(m_pData->hDC);

		CRgn* pRgn = new CRgn;
		pRgn->CreateEllipticRgnIndirect(rc);
		m_pData->m_vGDIObjects.push_back(pRgn);
		return SelectClipRgn(pRgn);
	}

	inline int CDC::CreatePolygonRgn(LPPOINT ppt, int cPoints, int fnPolyFillMode)
	// Creates the polygon region from the array of points and selects it into
	// the device context. The polygon is presumed closed.
	// The return value specifies the region's complexity: NULLREGION;SIMPLEREGION;COMPLEXREGION;ERROR.
	{
		assert(m_pData->hDC);

		CRgn* pRgn = new CRgn;
		pRgn->CreatePolygonRgn(ppt, cPoints, fnPolyFillMode);
		m_pData->m_vGDIObjects.push_back(pRgn);
		return SelectClipRgn(pRgn);
	}

	inline int CDC::CreatePolyPolygonRgn(LPPOINT ppt, LPINT pPolyCounts, int nCount, int fnPolyFillMode)
	// Creates the polygon region from a series of polygons.The polygons can overlap.
	// The return value specifies the region's complexity: NULLREGION;SIMPLEREGION;COMPLEXREGION;ERROR.
	{
		assert(m_pData->hDC);

		CRgn* pRgn = new CRgn;
		pRgn->CreatePolyPolygonRgn(ppt, pPolyCounts, nCount, fnPolyFillMode);
		m_pData->m_vGDIObjects.push_back(pRgn);
		return SelectClipRgn(pRgn);
	}
#endif


	// Wrappers for WinAPI functions

	inline int CDC::GetDeviceCaps (int nIndex) const
	// Retrieves device-specific information for the specified device.
	{
		assert(m_pData->hDC);
		return ::GetDeviceCaps(m_pData->hDC, nIndex);
	}

	// Brush Functions
#ifdef GetDCBrushColor
	inline COLORREF CDC::GetDCBrushColor() const
	{
		assert(m_pData->hDC);
		return ::GetDCBrushColor(m_pData->hDC);
	}

	inline COLORREF CDC::SetDCBrushColor(COLORREF crColor) const
	{
		assert(m_pData->hDC);
		return ::SetDCBrushColor(m_pData->hDC, crColor);
	}
#endif

	// Clipping functions
	inline int CDC::ExcludeClipRect(int Left, int Top, int Right, int BottomRect)
	// Creates a new clipping region that consists of the existing clipping region minus the specified rectangle.
	{
		assert(m_pData->hDC);
		return ::ExcludeClipRect(m_pData->hDC, Left, Top, Right, BottomRect);
	}

	inline int CDC::ExcludeClipRect(const RECT& rc)
	// Creates a new clipping region that consists of the existing clipping region minus the specified rectangle.
	{
		assert(m_pData->hDC);
		return ::ExcludeClipRect(m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom);
	}

	inline int CDC::GetClipBox (RECT& rc)
	// Retrieves the dimensions of the tightest bounding rectangle that can be drawn around the current visible area on the device.
	{
		assert(m_pData->hDC);
		return ::GetClipBox(m_pData->hDC, &rc);
	}

	inline int CDC::GetClipRgn(HRGN hrgn)
	// Retrieves a handle identifying the current application-defined clipping region for the specified device context.
	// hrgn: A handle to an existing region before the function is called.
	//       After the function returns, this parameter is a handle to a copy of the current clipping region.
	{
		assert(m_pData->hDC);
		return ::GetClipRgn(m_pData->hDC, hrgn);
	}

	inline int CDC::IntersectClipRect(int Left, int Top, int Right, int Bottom)
	// Creates a new clipping region from the intersection of the current clipping region and the specified rectangle.
	{
		assert(m_pData->hDC);
		return ::IntersectClipRect(m_pData->hDC, Left, Top, Right, Bottom);
	}

	inline int CDC::IntersectClipRect(const RECT& rc)
	// Creates a new clipping region from the intersection of the current clipping region and the specified rectangle.
	{
		assert(m_pData->hDC);
		return ::IntersectClipRect(m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom);
	}

	inline BOOL CDC::RectVisible(const RECT& rc)
	// Determines whether any part of the specified rectangle lies within the clipping region of a device context.
	{
		assert(m_pData->hDC);
		return ::RectVisible (m_pData->hDC, &rc);
	}

	inline int CDC::SelectClipRgn(CRgn* pRgn)
	// Selects a region as the current clipping region for the specified device context.
	// Note: Only a copy of the selected region is used.
	//       To remove a device-context's clipping region, specify a NULL region handle.
	{
		assert(m_pData->hDC);
		return ::SelectClipRgn(m_pData->hDC, pRgn? (HRGN)pRgn->GetHandle() : 0);
	}

#ifndef _WIN32_WCE
	inline int CDC::ExtSelectClipRgn(CRgn* pRgn, int fnMode)
	// Combines the specified region with the current clipping region using the specified mode.
	{
		assert(m_pData->hDC);
		assert(pRgn);
		return ::ExtSelectClipRgn(m_pData->hDC, *pRgn, fnMode);
	}
#endif

	inline CBitmap* CDC::SelectObject(const CBitmap* pBitmap)
	// Use this to attach an existing bitmap.
	{
		assert(m_pData->hDC);
		assert(pBitmap);

		return FromHandle( (HBITMAP)::SelectObject(m_pData->hDC, *pBitmap) );
	}

	inline CBrush* CDC::SelectObject(const CBrush* pBrush)
	// Use this to attach an existing brush.
	{
		assert(m_pData->hDC);
		assert(pBrush);

		return FromHandle( (HBRUSH)::SelectObject(m_pData->hDC, *pBrush) );
	}

	inline CFont* CDC::SelectObject(const CFont* pFont)
	// Use this to attach an existing font.
	{
		assert(m_pData->hDC);
		assert(pFont);

		return FromHandle( (HFONT)::SelectObject(m_pData->hDC, *pFont) );
	}

	inline CPalette* CDC::SelectObject(const CPalette* pPalette)
	// Use this to attach an existing Palette.
	{
		assert(m_pData->hDC);
		assert(pPalette);

		return FromHandle( (HPALETTE)::SelectObject(m_pData->hDC, *pPalette) );
	}

	inline CPen* CDC::SelectObject(const CPen* pPen)
	// Use this to attach an existing pen.
	{
		assert(m_pData->hDC);
		assert(pPen);

		return FromHandle( (HPEN)::SelectObject(m_pData->hDC, *pPen) );
	}

	inline CPalette* CDC::SelectPalette(const CPalette* pPalette, BOOL bForceBkgnd)
	// Use this to attach an existing palette.
	{
		assert(m_pData->hDC);
		assert(pPalette);

		return FromHandle( (HPALETTE)::SelectPalette(m_pData->hDC, *pPalette, bForceBkgnd) );
	}
#ifndef _WIN32_WCE
	inline BOOL CDC::PtVisible(int X, int Y)
	// Determines whether the specified point is within the clipping region of a device context.
	{
		assert(m_pData->hDC);
		return ::PtVisible (m_pData->hDC, X, Y);
	}

	inline int CDC::OffsetClipRgn(int nXOffset, int nYOffset)
	// Moves the clipping region of a device context by the specified offsets.
	{
		assert(m_pData->hDC);
		return ::OffsetClipRgn (m_pData->hDC, nXOffset, nYOffset);
	}
#endif

	// Point and Line Drawing Functions
	inline CPoint CDC::GetCurrentPosition() const
	// Returns the current "MoveToEx" position.
	{
		assert(m_pData->hDC);
		CPoint pt;
		::MoveToEx(m_pData->hDC, 0, 0, &pt);
		::MoveToEx(m_pData->hDC, pt.x, pt.y, NULL);
		return pt;
	}

	inline CPoint CDC::MoveTo(int x, int y) const
	// Updates the current position to the specified point.
	{
		assert(m_pData->hDC);
		return ::MoveToEx(m_pData->hDC, x, y, NULL);
	}

	inline CPoint CDC::MoveTo(POINT pt) const
	// Updates the current position to the specified point
	{
		assert(m_pData->hDC);
		return ::MoveToEx(m_pData->hDC, pt.x, pt.y, NULL);
	}

	inline BOOL CDC::LineTo(int x, int y) const
	// Draws a line from the current position up to, but not including, the specified point.
	{
		assert(m_pData->hDC);
		return ::LineTo(m_pData->hDC, x, y);
	}

	inline BOOL CDC::LineTo(POINT pt) const
	// Draws a line from the current position up to, but not including, the specified point.
	{
		assert(m_pData->hDC);
		return ::LineTo(m_pData->hDC, pt.x, pt.y);
	}

#ifndef _WIN32_WCE
	inline BOOL CDC::Arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const
	// Draws an elliptical arc.
	{
		assert(m_pData->hDC);
		return ::Arc(m_pData->hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	}

	inline BOOL CDC::Arc(RECT& rc, POINT ptStart, POINT ptEnd) const
	// Draws an elliptical arc.
	{
		assert(m_pData->hDC);
		return ::Arc(m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom,
			ptStart.x, ptStart.y, ptEnd.x, ptEnd.y);
	}

	inline BOOL CDC::ArcTo(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const
	// Draws an elliptical arc.
	{
		assert(m_pData->hDC);
		return ::ArcTo(m_pData->hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	}

	inline BOOL CDC::ArcTo(RECT& rc, POINT ptStart, POINT ptEnd) const
	// Draws an elliptical arc.
	{
		assert(m_pData->hDC);
		return ::ArcTo (m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom,
			ptStart.x, ptStart.y, ptEnd.x, ptEnd.y);
	}

	inline BOOL CDC::AngleArc(int x, int y, int nRadius, float fStartAngle, float fSweepAngle) const
	// Draws a line segment and an arc.
	{
		assert(m_pData->hDC);
		return ::AngleArc(m_pData->hDC, x, y, nRadius, fStartAngle, fSweepAngle);
	}

	inline int CDC::GetArcDirection() const
	// Retrieves the current arc direction ( AD_COUNTERCLOCKWISE or AD_CLOCKWISE ).
	{
		assert(m_pData->hDC);
		return ::GetArcDirection(m_pData->hDC);
	}

	inline int CDC::SetArcDirection(int nArcDirection) const
	// Sets the current arc direction ( AD_COUNTERCLOCKWISE or AD_CLOCKWISE ).
	{
		assert(m_pData->hDC);
		return ::SetArcDirection(m_pData->hDC, nArcDirection);
	}

	inline BOOL CDC::PolyDraw(const POINT* lpPoints, const BYTE* lpTypes, int nCount) const
	// Draws a set of line segments and Bzier curves.
	{
		assert(m_pData->hDC);
		return ::PolyDraw(m_pData->hDC, lpPoints, lpTypes, nCount);
	}

	inline BOOL CDC::Polyline(LPPOINT lpPoints, int nCount) const
	// Draws a series of line segments by connecting the points in the specified array.
	{
		assert(m_pData->hDC);
		return ::Polyline(m_pData->hDC, lpPoints, nCount);
	}

	inline BOOL CDC::PolyPolyline(const POINT* lpPoints, const DWORD* lpPolyPoints, int nCount) const
	// Draws multiple series of connected line segments.
	{
		assert(m_pData->hDC);
		return ::PolyPolyline(m_pData->hDC, lpPoints, lpPolyPoints, nCount);
	}

	inline BOOL CDC::PolylineTo(const POINT* lpPoints, int nCount) const
	// Draws one or more straight lines.
	{
		assert(m_pData->hDC);
		return ::PolylineTo(m_pData->hDC, lpPoints, nCount);
	}
	inline BOOL CDC::PolyBezier(const POINT* lpPoints, int nCount) const
	// Draws one or more Bzier curves.
	{
		assert(m_pData->hDC);
		return ::PolyBezier(m_pData->hDC, lpPoints, nCount);
	}

	inline BOOL CDC::PolyBezierTo(const POINT* lpPoints, int nCount) const
	// Draws one or more Bzier curves.
	{
		assert(m_pData->hDC);
		return ::PolyBezierTo(m_pData->hDC, lpPoints, nCount );
	}

	inline COLORREF CDC::GetPixel(int x, int y) const
	// Retrieves the red, green, blue (RGB) color value of the pixel at the specified coordinates.
	{
		assert(m_pData->hDC);
		return ::GetPixel(m_pData->hDC, x, y);
	}

	inline COLORREF CDC::GetPixel(POINT pt) const
	// Retrieves the red, green, blue (RGB) color value of the pixel at the specified coordinates.
	{
		assert(m_pData->hDC);
		return ::GetPixel(m_pData->hDC, pt.x, pt.y);
	}

	inline COLORREF CDC::SetPixel (int x, int y, COLORREF crColor) const
	// Sets the pixel at the specified coordinates to the specified color.
	{
		assert(m_pData->hDC);
		return ::SetPixel(m_pData->hDC, x, y, crColor);
	}

	inline COLORREF CDC::SetPixel(POINT pt, COLORREF crColor) const
	// Sets the pixel at the specified coordinates to the specified color.
	{
		assert(m_pData->hDC);
		return ::SetPixel(m_pData->hDC, pt.x, pt.y, crColor);
	}

	inline BOOL CDC::SetPixelV(int x, int y, COLORREF crColor) const
	// Sets the pixel at the specified coordinates to the closest approximation of the specified color.
	{
		assert(m_pData->hDC);
		return ::SetPixelV(m_pData->hDC, x, y, crColor);
	}

	inline BOOL CDC::SetPixelV(POINT pt, COLORREF crColor) const
	// Sets the pixel at the specified coordinates to the closest approximation of the specified color.
	{
		assert(m_pData->hDC);
		return ::SetPixelV(m_pData->hDC, pt.x, pt.y, crColor);
	}
#endif

	// Shape Drawing Functions
	inline void CDC::DrawFocusRect(const RECT& rc) const
	// Draws a rectangle in the style used to indicate that the rectangle has the focus.
	{
		assert(m_pData->hDC);
		::DrawFocusRect(m_pData->hDC, &rc);
	}

	inline BOOL CDC::Ellipse(int x1, int y1, int x2, int y2) const
	// Draws an ellipse. The center of the ellipse is the center of the specified bounding rectangle.
	{
		assert(m_pData->hDC);
		return ::Ellipse(m_pData->hDC, x1, y1, x2, y2);
	}

	inline BOOL CDC::Ellipse(const RECT& rc) const
	// Draws an ellipse. The center of the ellipse is the center of the specified bounding rectangle.
	{
		assert(m_pData->hDC);
		return ::Ellipse(m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom);
	}

	inline BOOL CDC::Polygon(LPPOINT lpPoints, int nCount) const
	// Draws a polygon consisting of two or more vertices connected by straight lines.
	{
		assert(m_pData->hDC);
		return ::Polygon(m_pData->hDC, lpPoints, nCount);
	}

	inline BOOL CDC::Rectangle(int x1, int y1, int x2, int y2) const
	// Draws a rectangle. The rectangle is outlined by using the current pen and filled by using the current brush.
	{
		assert(m_pData->hDC);
		return ::Rectangle(m_pData->hDC, x1, y1, x2, y2);
	}

	inline BOOL CDC::Rectangle(const RECT& rc) const
	// Draws a rectangle. The rectangle is outlined by using the current pen and filled by using the current brush.
	{
		assert(m_pData->hDC);
		return ::Rectangle(m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom);
	}

	inline BOOL CDC::RoundRect(int x1, int y1, int x2, int y2, int nWidth, int nHeight) const
	// Draws a rectangle with rounded corners.
	{
		assert(m_pData->hDC);
		return ::RoundRect(m_pData->hDC, x1, y1, x2, y2, nWidth, nHeight);
	}
	inline BOOL CDC::RoundRect(const RECT& rc, int nWidth, int nHeight) const
	// Draws a rectangle with rounded corners.
	{
		assert(m_pData->hDC);
		return ::RoundRect(m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom, nWidth, nHeight );
	}

#ifndef _WIN32_WCE
	inline BOOL CDC::Chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const
	// Draws a chord (a region bounded by the intersection of an ellipse and a line segment, called a secant).
	{
		assert(m_pData->hDC);
		return ::Chord(m_pData->hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	}

	inline BOOL CDC::Chord(const RECT& rc, POINT ptStart, POINT ptEnd) const
	// Draws a chord (a region bounded by the intersection of an ellipse and a line segment, called a secant).
	{
		assert(m_pData->hDC);
		return ::Chord(m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom,
			ptStart.x, ptStart.y, ptEnd.x, ptEnd.y);
	}

	inline BOOL CDC::Pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) const
	// Draws a pie-shaped wedge bounded by the intersection of an ellipse and two radials.
	{
		assert(m_pData->hDC);
		return ::Pie(m_pData->hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	}

	inline BOOL CDC::Pie(const RECT& rc, POINT ptStart, POINT ptEnd) const
	// Draws a pie-shaped wedge bounded by the intersection of an ellipse and two radials.
	{
		assert(m_pData->hDC);
		return ::Pie(m_pData->hDC, rc.left, rc.top, rc.right, rc.bottom,
			ptStart.x, ptStart.y, ptEnd.x, ptEnd.y);
	}

	inline BOOL CDC::PolyPolygon(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount) const
	// Draws a series of closed polygons.
	{
		assert(m_pData->hDC);
		return ::PolyPolygon(m_pData->hDC, lpPoints, lpPolyCounts, nCount);
	}
#endif

	// Fill and 3D Drawing functions
	inline BOOL CDC::FillRect(const RECT& rc, CBrush* pBrush) const
	// Fills a rectangle by using the specified brush.
	{
		assert(m_pData->hDC);
		assert(pBrush);
		return (BOOL)::FillRect(m_pData->hDC, &rc, *pBrush);
	}

	inline BOOL CDC::InvertRect(const RECT& rc) const
	// Inverts a rectangle in a window by performing a logical NOT operation on the color values for each pixel in the rectangle's interior.
	{
		assert(m_pData->hDC);
		return ::InvertRect( m_pData->hDC, &rc);
	}

	inline BOOL CDC::DrawIconEx(int xLeft, int yTop, HICON hIcon, int cxWidth, int cyWidth, UINT istepIfAniCur, CBrush* pFlickerFreeDraw, UINT diFlags) const
	// draws an icon or cursor, performing the specified raster operations, and stretching or compressing the icon or cursor as specified.
	{
		assert(m_pData->hDC);
		HBRUSH hFlickerFreeDraw = pFlickerFreeDraw? (HBRUSH)pFlickerFreeDraw->GetHandle() : NULL;
		return ::DrawIconEx(m_pData->hDC, xLeft, yTop, hIcon, cxWidth, cyWidth, istepIfAniCur, hFlickerFreeDraw, diFlags);
	}

	inline BOOL CDC::DrawEdge(const RECT& rc, UINT nEdge, UINT nFlags) const
	// Draws one or more edges of rectangle.
	{
		assert(m_pData->hDC);
		return ::DrawEdge(m_pData->hDC, (LPRECT)&rc, nEdge, nFlags);
	}

	inline BOOL CDC::DrawFrameControl(const RECT& rc, UINT nType, UINT nState) const
	// Draws a frame control of the specified type and style.
	{
		assert(m_pData->hDC);
		return ::DrawFrameControl(m_pData->hDC, (LPRECT)&rc, nType, nState);
	}

	inline BOOL CDC::FillRgn(CRgn* pRgn, CBrush* pBrush) const
	// Fills a region by using the specified brush.
	{
		assert(m_pData->hDC);
		assert(pRgn);
		assert(pBrush);
		return ::FillRgn(m_pData->hDC, *pRgn, *pBrush);
	}

#ifndef _WIN32_WCE
	inline BOOL CDC::DrawIcon(int x, int y, HICON hIcon) const
	// Draws an icon or cursor.
	{
		assert(m_pData->hDC);
		return ::DrawIcon(m_pData->hDC, x, y, hIcon);
	}

	inline BOOL CDC::DrawIcon(POINT pt, HICON hIcon) const
	// Draws an icon or cursor.
	{
		assert(m_pData->hDC);
		return ::DrawIcon(m_pData->hDC, pt.x, pt.y, hIcon);
	}

	inline BOOL CDC::FrameRect(const RECT& rc, CBrush* pBrush) const
	// Draws a border around the specified rectangle by using the specified brush.
	{
		assert(m_pData->hDC);
		assert(pBrush);
		return (BOOL)::FrameRect(m_pData->hDC, &rc, *pBrush);
	}

	inline BOOL CDC::PaintRgn(CRgn* pRgn) const
	// Paints the specified region by using the brush currently selected into the device context.
	{
		assert(m_pData->hDC);
		assert(pRgn);
		return ::PaintRgn(m_pData->hDC, *pRgn);
	}
#endif

	// Bitmap Functions
	inline int CDC::StretchDIBits(int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth,
		           int nSrcHeight, CONST VOID *lpBits, BITMAPINFO& bi, UINT iUsage, DWORD dwRop) const
	// Copies the color data for a rectangle of pixels in a DIB to the specified destination rectangle.
	{
		assert(m_pData->hDC);
		return ::StretchDIBits(m_pData->hDC, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, &bi, iUsage, dwRop);
	}

	inline BOOL CDC::PatBlt(int x, int y, int nWidth, int nHeight, DWORD dwRop) const
	// Paints the specified rectangle using the brush that is currently selected into the device context.
	{
		assert(m_pData->hDC);
		return ::PatBlt(m_pData->hDC, x, y, nWidth, nHeight, dwRop);
	}

	inline BOOL CDC::BitBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC, int xSrc, int ySrc, DWORD dwRop) const
	// Performs a bit-block transfer of the color data corresponding to a rectangle of pixels from the specified source device context into a destination device context.
	{
		assert(m_pData->hDC);
		assert(pSrcDC);
		return ::BitBlt(m_pData->hDC, x, y, nWidth, nHeight, pSrcDC->GetHDC(), xSrc, ySrc, dwRop);
	}

	inline BOOL CDC::StretchBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop) const
	// Copies a bitmap from a source rectangle into a destination rectangle, stretching or compressing the bitmap to fit the dimensions of the destination rectangle, if necessary.
	{
		assert(m_pData->hDC);
		assert(pSrcDC);
		return ::StretchBlt(m_pData->hDC, x, y, nWidth, nHeight, pSrcDC->GetHDC(), xSrc, ySrc, nSrcWidth, nSrcHeight, dwRop);
	}

#ifndef _WIN32_WCE
	inline int CDC::GetDIBits(CBitmap* pBitmap, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbi, UINT uUsage) const
	// Retrieves the bits of the specified compatible bitmap and copies them into a buffer as a DIB using the specified format.
	{
		assert(m_pData->hDC);
		assert(pBitmap);
		return ::GetDIBits(m_pData->hDC, *pBitmap, uStartScan, cScanLines, lpvBits, lpbi, uUsage);
	}

	inline int CDC::SetDIBits(CBitmap* pBitmap, UINT uStartScan, UINT cScanLines, CONST VOID *lpvBits, LPBITMAPINFO lpbi, UINT fuColorUse) const
	// Sets the pixels in a compatible bitmap (DDB) using the color data found in the specified DIB.
	{
		assert(m_pData->hDC);
		return ::SetDIBits(m_pData->hDC, *pBitmap, uStartScan, cScanLines, lpvBits, lpbi, fuColorUse);
	}

	inline int CDC::GetStretchBltMode() const
	// Retrieves the current stretching mode.
	// Possible modes: BLACKONWHITE, COLORONCOLOR, HALFTONE, STRETCH_ANDSCANS, STRETCH_DELETESCANS, STRETCH_HALFTONE, STRETCH_ORSCANS, WHITEONBLACK
	{
		assert(m_pData->hDC);
		return ::GetStretchBltMode(m_pData->hDC);
	}

	inline int CDC::SetStretchBltMode(int iStretchMode) const
	// Sets the stretching mode.
	// Possible modes: BLACKONWHITE, COLORONCOLOR, HALFTONE, STRETCH_ANDSCANS, STRETCH_DELETESCANS, STRETCH_HALFTONE, STRETCH_ORSCANS, WHITEONBLACK
	{
		assert(m_pData->hDC);
		return ::SetStretchBltMode(m_pData->hDC, iStretchMode);
	}

	inline BOOL CDC::FloodFill(int x, int y, COLORREF crColor) const
	// Fills an area of the display surface with the current brush.
	{
		assert(m_pData->hDC);
		return ::FloodFill(m_pData->hDC, x, y, crColor);
	}

	inline BOOL CDC::ExtFloodFill(int x, int y, COLORREF crColor, UINT nFillType) const
	// Fills an area of the display surface with the current brush.
	// Fill type: FLOODFILLBORDER or FLOODFILLSURFACE
	{
		assert(m_pData->hDC);
		return ::ExtFloodFill(m_pData->hDC, x, y, crColor, nFillType );
	}

	// co-ordinate functions
	inline BOOL CDC::DPtoLP(LPPOINT lpPoints, int nCount) const
	// Converts device coordinates into logical coordinates.
	{
		assert(m_pData->hDC);
		return ::DPtoLP(m_pData->hDC, lpPoints, nCount);
	}

	inline BOOL CDC::DPtoLP(RECT& rc) const
	// Converts device coordinates into logical coordinates.
	{
		assert(m_pData->hDC);
		return ::DPtoLP(m_pData->hDC, (LPPOINT)&rc, 2);
	}

	inline BOOL CDC::LPtoDP(LPPOINT lpPoints, int nCount) const
	// Converts logical coordinates into device coordinates.
	{
		assert(m_pData->hDC);
		return ::LPtoDP(m_pData->hDC, lpPoints, nCount);
	}

	inline BOOL CDC::LPtoDP(RECT& rc) const
	// Converts logical coordinates into device coordinates.
	{
		assert(m_pData->hDC);
		return ::LPtoDP(m_pData->hDC, (LPPOINT)&rc, 2);
	}

#endif

	// Layout Functions
	inline DWORD CDC::GetLayout() const
	// Returns the layout of a device context (LAYOUT_RTL and LAYOUT_BITMAPORIENTATIONPRESERVED).
	{
#if defined(WINVER) && defined(GetLayout) && (WINVER >= 0x0500)
		return ::GetLayout(m_pData->hDC);
#else
		return 0;
#endif
	}

	inline DWORD CDC::SetLayout(DWORD dwLayout) const
	// changes the layout of a device context (DC).
	// dwLayout values:  LAYOUT_RTL or LAYOUT_BITMAPORIENTATIONPRESERVED
	{
#if defined(WINVER) && defined (SetLayout) && (WINVER >= 0x0500)
		// Sets the layout of a device context
		return ::SetLayout(m_pData->hDC, dwLayout);
#else
		UNREFERENCED_PARAMETER(dwLayout); // no-op
		return 0;
#endif
	}

	// Mapping Functions
#ifndef _WIN32_WCE
	inline int CDC::GetMapMode()  const
	// Rretrieves the current mapping mode.
	// Possible modes: MM_ANISOTROPIC, MM_HIENGLISH, MM_HIMETRIC, MM_ISOTROPIC, MM_LOENGLISH, MM_LOMETRIC, MM_TEXT, and MM_TWIPS.
	{
		assert(m_pData->hDC);
		return ::GetMapMode(m_pData->hDC);
	}

	inline BOOL CDC::GetViewportOrgEx(LPPOINT lpPoint)  const
	// Retrieves the x-coordinates and y-coordinates of the viewport origin for the device context.
	{
		assert(m_pData->hDC);
		return ::GetViewportOrgEx(m_pData->hDC, lpPoint);
	}

	inline int CDC::SetMapMode(int nMapMode) const
	// Sets the mapping mode of the specified device context.
	{
		assert(m_pData->hDC);
		return ::SetMapMode(m_pData->hDC, nMapMode);
	}

	inline BOOL CDC::SetViewportOrgEx(int x, int y, LPPOINT lpPoint /* = NULL */) const
	// Specifies which device point maps to the window origin (0,0).
	{
		assert(m_pData->hDC);
		return ::SetViewportOrgEx(m_pData->hDC, x, y, lpPoint);
	}

	inline BOOL CDC::SetViewportOrgEx(POINT point, LPPOINT lpPointRet /* = NULL */) const
	// Specifies which device point maps to the window origin (0,0).
	{
		assert(m_pData->hDC);
		return SetViewportOrgEx(point.x, point.y, lpPointRet);
	}

	inline BOOL CDC::OffsetViewportOrgEx(int nWidth, int nHeight, LPPOINT lpPoint /* = NULL */) const
	// Modifies the viewport origin for the device context using the specified horizontal and vertical offsets.
	{
		assert(m_pData->hDC);
		return ::OffsetViewportOrgEx(m_pData->hDC, nWidth, nHeight, lpPoint);
	}

	inline BOOL CDC::GetViewportExtEx(LPSIZE lpSize)  const
	// Retrieves the x-extent and y-extent of the current viewport for the device context.
	{
		assert(m_pData->hDC);
		return ::GetViewportExtEx(m_pData->hDC, lpSize);
	}

	inline BOOL CDC::SetViewportExtEx(int x, int y, LPSIZE lpSize ) const
	// Sets the horizontal and vertical extents of the viewport for the device context by using the specified values.
	{
		assert(m_pData->hDC);
		return ::SetViewportExtEx(m_pData->hDC, x, y, lpSize);
	}

	inline BOOL CDC::SetViewportExtEx(SIZE size, LPSIZE lpSizeRet ) const
	// Sets the horizontal and vertical extents of the viewport for the device context by using the specified values.
	{
		assert(m_pData->hDC);
		return SetViewportExtEx(size.cx, size.cy, lpSizeRet);
	}

	inline BOOL CDC::ScaleViewportExtEx(int xNum, int xDenom, int yNum, int yDenom, LPSIZE lpSize ) const
	// Modifies the viewport for the device context using the ratios formed by the specified multiplicands and divisors.
	{
		assert(m_pData->hDC);
		return ::ScaleViewportExtEx(m_pData->hDC, xNum, xDenom, yNum, yDenom, lpSize);
	}

	inline BOOL CDC::GetWindowOrgEx(LPPOINT lpPoint) const
	// Retrieves the x-coordinates and y-coordinates of the window origin for the device context.
	{
		assert(m_pData->hDC);
		return ::GetWindowOrgEx(m_pData->hDC, lpPoint);
	}

	inline BOOL CDC::SetWindowOrgEx(int x, int y, LPPOINT lpPoint ) const
	// Specifies which window point maps to the viewport origin (0,0).
	{
		assert(m_pData->hDC);
		return ::SetWindowOrgEx(m_pData->hDC, x, y, lpPoint);
	}

	inline BOOL CDC::SetWindowOrgEx(POINT point, LPPOINT lpPointRet ) const
	// Specifies which window point maps to the viewport origin (0,0).
	{
		assert(m_pData->hDC);
		return SetWindowOrgEx(point.x, point.y, lpPointRet);
	}

	inline BOOL CDC::OffsetWindowOrgEx(int nWidth, int nHeight, LPPOINT lpPoint ) const
	// Modifies the window origin for the device context using the specified horizontal and vertical offsets.
	{
		assert(m_pData->hDC);
		return ::OffsetWindowOrgEx(m_pData->hDC, nWidth, nHeight, lpPoint);
	}

	inline BOOL CDC::GetWindowExtEx(LPSIZE lpSize)  const
	// Retrieves the x-extent and y-extent of the window for the device context.
	{
		assert(m_pData->hDC);
		return ::GetWindowExtEx(m_pData->hDC, lpSize);
	}

	inline BOOL CDC::SetWindowExtEx(int x, int y, LPSIZE lpSize ) const
	// Sets the horizontal and vertical extents of the window for the device context by using the specified values.
	{
		assert(m_pData->hDC);
		return ::SetWindowExtEx(m_pData->hDC, x, y, lpSize);
	}

	inline BOOL CDC::SetWindowExtEx(SIZE size, LPSIZE lpSizeRet) const
	// Sets the horizontal and vertical extents of the window for the device context by using the specified values.
	{
		assert(m_pData->hDC);
		return SetWindowExtEx(size.cx, size.cy, lpSizeRet);
	}

	inline BOOL CDC::ScaleWindowExtEx(int xNum, int xDenom, int yNum, int yDenom, LPSIZE lpSize) const
	// Modifies the window for the device context using the ratios formed by the specified multiplicands and divisors.
	{
		assert(m_pData->hDC);
		return ::ScaleWindowExtEx(m_pData->hDC, xNum, xDenom, yNum, yDenom, lpSize);
	}
#endif

	// Printer Functions
	inline int CDC::StartDoc(LPDOCINFO lpDocInfo) const
	// Starts a print job.
	{
		assert(m_pData->hDC);
		return ::StartDoc(m_pData->hDC, lpDocInfo);
	}

	inline int CDC::EndDoc() const
	// Ends a print job.
	{
		assert(m_pData->hDC);
		return ::EndDoc(m_pData->hDC);
	}

	inline int CDC::StartPage() const
	// Prepares the printer driver to accept data.
	{
		assert(m_pData->hDC);
		return ::StartPage(m_pData->hDC);
	}

	inline int CDC::EndPage() const
	// Notifies the device that the application has finished writing to a page.
	{
		assert(m_pData->hDC);
		return ::EndPage(m_pData->hDC);
	}

	inline int CDC::AbortDoc() const
	// Stops the current print job and erases everything drawn since the last call to the StartDoc function.
	{
		assert(m_pData->hDC);
		return ::AbortDoc(m_pData->hDC);
	}

	inline int CDC::SetAbortProc(BOOL (CALLBACK* lpfn)(HDC, int)) const
	// Sets the application-defined abort function that allows a print job to be canceled during spooling.
	{
		assert(m_pData->hDC);
		return ::SetAbortProc(m_pData->hDC, lpfn);
	}

	// Text Functions
	inline BOOL CDC::ExtTextOut(int x, int y, UINT nOptions, LPCRECT lprc, LPCTSTR lpszString, int nCount /*= -1*/, LPINT lpDxWidths /*=NULL*/) const
	// Draws text using the currently selected font, background color, and text color
	{
		assert(m_pData->hDC);

		if (nCount == -1)
			nCount = lstrlen (lpszString);

		return ::ExtTextOut(m_pData->hDC, x, y, nOptions, lprc, lpszString, nCount, lpDxWidths );
	}

	inline int CDC::DrawText(LPCTSTR lpszString, int nCount, LPRECT lprc, UINT nFormat) const
	// Draws formatted text in the specified rectangle
	{
		assert(m_pData->hDC);
		return ::DrawText(m_pData->hDC, lpszString, nCount, lprc, nFormat );
	}

	inline UINT CDC::GetTextAlign() const
	// Retrieves the text-alignment setting
	// Values: TA_BASELINE, TA_BOTTOM, TA_TOP, TA_CENTER, TA_LEFT, TA_RIGHT, TA_RTLREADING, TA_NOUPDATECP, TA_UPDATECP
	{
		assert(m_pData->hDC);
		return ::GetTextAlign(m_pData->hDC);
	}

	inline UINT CDC::SetTextAlign(UINT nFlags) const
	// Sets the text-alignment setting
	// Values: TA_BASELINE, TA_BOTTOM, TA_TOP, TA_CENTER, TA_LEFT, TA_RIGHT, TA_RTLREADING, TA_NOUPDATECP, TA_UPDATECP
	{
		assert(m_pData->hDC);
		return ::SetTextAlign(m_pData->hDC, nFlags);
	}

	inline int CDC::GetTextFace(int nCount, LPTSTR lpszFacename) const
	// Retrieves the typeface name of the font that is selected into the device context
	{
		assert(m_pData->hDC);
		return ::GetTextFace(m_pData->hDC, nCount, lpszFacename);
	}

	inline BOOL CDC::GetTextMetrics(TEXTMETRIC& Metrics) const
	// Fills the specified buffer with the metrics for the currently selected font
	{
		assert(m_pData->hDC);
		return ::GetTextMetrics(m_pData->hDC, &Metrics);
	}

	inline COLORREF CDC::GetBkColor() const
	// Returns the current background color
	{
		assert(m_pData->hDC);
		return ::GetBkColor(m_pData->hDC);
	}

	inline COLORREF CDC::SetBkColor(COLORREF crColor) const
	// Sets the current background color to the specified color value
	{
		assert(m_pData->hDC);
		return ::SetBkColor(m_pData->hDC, crColor);
	}

	inline COLORREF CDC::GetTextColor() const
	// Retrieves the current text color
	{
		assert(m_pData->hDC);
		return ::GetTextColor(m_pData->hDC);
	}

	inline COLORREF CDC::SetTextColor(COLORREF crColor) const
	// Sets the current text color
	{
		assert(m_pData->hDC);
		return ::SetTextColor(m_pData->hDC, crColor);
	}

	inline int CDC::GetBkMode() const
	// returns the current background mix mode (OPAQUE or TRANSPARENT)
	{
		assert(m_pData->hDC);
		return ::GetBkMode(m_pData->hDC);
	}

	inline int CDC::SetBkMode(int iBkMode) const
	// Sets the current background mix mode (OPAQUE or TRANSPARENT)
	{
		assert(m_pData->hDC);
		return ::SetBkMode(m_pData->hDC, iBkMode);
	}

#ifndef _WIN32_WCE
	inline int CDC::DrawTextEx(LPTSTR lpszString, int nCount, LPRECT lprc, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams) const
	// Draws formatted text in the specified rectangle with more formatting options
	{
		assert(m_pData->hDC);
		return ::DrawTextEx(m_pData->hDC, lpszString, nCount, lprc, nFormat, lpDTParams);
	}

	inline CSize CDC::GetTextExtentPoint32(LPCTSTR lpszString, int nCount) const
	// Computes the width and height of the specified string of text
	{
		assert(m_pData->hDC);
		CSize sz;
		::GetTextExtentPoint32(m_pData->hDC, lpszString, nCount, &sz);
		return sz;
	}

	inline CSize CDC::GetTabbedTextExtent(LPCTSTR lpszString, int nCount, int nTabPositions, LPINT lpnTabStopPositions) const
	// Computes the width and height of a character string
	{
		assert(m_pData->hDC);
		DWORD dwSize = ::GetTabbedTextExtent(m_pData->hDC, lpszString, nCount, nTabPositions, lpnTabStopPositions );
		CSize sz(dwSize);
		return sz;
	}

	inline BOOL CDC::GrayString(CBrush* pBrush, GRAYSTRINGPROC lpOutputFunc, LPARAM lpData, int nCount, int x, int y, int nWidth, int nHeight) const
	// Draws gray text at the specified location
	{
		assert(m_pData->hDC);
		assert(pBrush);
		return ::GrayString(m_pData->hDC, *pBrush, lpOutputFunc, lpData, nCount, x, y, nWidth, nHeight);
	}

	inline int CDC::SetTextJustification(int nBreakExtra, int nBreakCount) const
	// Specifies the amount of space the system should add to the break characters in a string of text
	{
		assert(m_pData->hDC);
		return ::SetTextJustification(m_pData->hDC, nBreakExtra, nBreakCount);
	}

	inline int CDC::GetTextCharacterExtra() const
	// Retrieves the current intercharacter spacing for the device context
	{
		assert(m_pData->hDC);
		return ::GetTextCharacterExtra(m_pData->hDC);
	}

	inline int CDC::SetTextCharacterExtra(int nCharExtra) const
	// Sets the intercharacter spacing
	{
		assert(m_pData->hDC);
		return ::SetTextCharacterExtra(m_pData->hDC, nCharExtra);
	}

	inline CSize CDC::TabbedTextOut(int x, int y, LPCTSTR lpszString, int nCount, int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin) const
	// Writes a character string at a specified location, expanding tabs to the values specified in an array of tab-stop positions
	{
		assert(m_pData->hDC);
		DWORD dwSize = ::TabbedTextOut(m_pData->hDC, x, y, lpszString, nCount, nTabPositions, lpnTabStopPositions, nTabOrigin );
		CSize sz(dwSize);
		return sz;
	}

	inline BOOL CDC::TextOut(int x, int y, LPCTSTR lpszString, int nCount/* = -1*/) const
	// Writes a character string at the specified location
	{
		assert(m_pData->hDC);
		if (nCount == -1)
			nCount = lstrlen (lpszString);

		return ::TextOut(m_pData->hDC, x, y, lpszString, nCount);
	}

#endif



	/////////////////////////////////////////////////////////////////
	// Definitions for some global functions in the Win32xx namespace
	//

#ifndef _WIN32_WCE
	inline void TintBitmap (CBitmap* pbmSource, int cRed, int cGreen, int cBlue)
	// Modifies the colour of the supplied Device Dependant Bitmap, by the colour
	// correction values specified. The correction values can range from -255 to +255.
	// This function gains its speed by accessing the bitmap colour information
	// directly, rather than using GetPixel/SetPixel.
	{
		// Create our LPBITMAPINFO object
		CBitmapInfoPtr pbmi(pbmSource);
		pbmi->bmiHeader.biBitCount = 24;

		// Create the reference DC for GetDIBits to use
		CMemDC MemDC(NULL);

		// Use GetDIBits to create a DIB from our DDB, and extract the colour data
		MemDC.GetDIBits(pbmSource, 0, pbmi->bmiHeader.biHeight, NULL, pbmi, DIB_RGB_COLORS);
		std::vector<byte> vBits(pbmi->bmiHeader.biSizeImage, 0);
		byte* pByteArray = &vBits[0];

		MemDC.GetDIBits(pbmSource, 0, pbmi->bmiHeader.biHeight, pByteArray, pbmi, DIB_RGB_COLORS);
		UINT nWidthBytes = pbmi->bmiHeader.biSizeImage/pbmi->bmiHeader.biHeight;

		// Ensure sane colour correction values
		cBlue  = MIN(cBlue, 255);
		cBlue  = MAX(cBlue, -255);
		cRed   = MIN(cRed, 255);
		cRed   = MAX(cRed, -255);
		cGreen = MIN(cGreen, 255);
		cGreen = MAX(cGreen, -255);

		// Pre-calculate the RGB modification values
		int b1 = 256 - cBlue;
		int g1 = 256 - cGreen;
		int r1 = 256 - cRed;

		int b2 = 256 + cBlue;
		int g2 = 256 + cGreen;
		int r2 = 256 + cRed;

		// Modify the colour
		int yOffset = 0;
		int xOffset;
		int Index;
		for (int Row=0; Row < pbmi->bmiHeader.biHeight; Row++)
		{
			xOffset = 0;

			for (int Column=0; Column < pbmi->bmiHeader.biWidth; Column++)
			{
				// Calculate Index
				Index = yOffset + xOffset;

				// Adjust the colour values
				if (cBlue > 0)
					pByteArray[Index]   = (BYTE)(cBlue + (((pByteArray[Index] *b1)) >>8));
				else if (cBlue < 0)
					pByteArray[Index]   = (BYTE)((pByteArray[Index] *b2) >>8);

				if (cGreen > 0)
					pByteArray[Index+1] = (BYTE)(cGreen + (((pByteArray[Index+1] *g1)) >>8));
				else if (cGreen < 0)
					pByteArray[Index+1] = (BYTE)((pByteArray[Index+1] *g2) >>8);

				if (cRed > 0)
					pByteArray[Index+2] = (BYTE)(cRed + (((pByteArray[Index+2] *r1)) >>8));
				else if (cRed < 0)
					pByteArray[Index+2] = (BYTE)((pByteArray[Index+2] *r2) >>8);

				// Increment the horizontal offset
				xOffset += pbmi->bmiHeader.biBitCount >> 3;
			}

			// Increment vertical offset
			yOffset += nWidthBytes;
		}

		// Save the modified colour back into our source DDB
		MemDC.SetDIBits(pbmSource, 0, pbmi->bmiHeader.biHeight, pByteArray, pbmi, DIB_RGB_COLORS);
	}

	inline void GrayScaleBitmap(CBitmap* pbmSource)
	{
		// Create our LPBITMAPINFO object
		CBitmapInfoPtr pbmi(pbmSource);

		// Create the reference DC for GetDIBits to use
		CMemDC MemDC(NULL);

		// Use GetDIBits to create a DIB from our DDB, and extract the colour data
		MemDC.GetDIBits(pbmSource, 0, pbmi->bmiHeader.biHeight, NULL, pbmi, DIB_RGB_COLORS);
		std::vector<byte> vBits(pbmi->bmiHeader.biSizeImage, 0);
		byte* pByteArray = &vBits[0];

		MemDC.GetDIBits(pbmSource, 0, pbmi->bmiHeader.biHeight, pByteArray, pbmi, DIB_RGB_COLORS);
		UINT nWidthBytes = pbmi->bmiHeader.biSizeImage/pbmi->bmiHeader.biHeight;

		int yOffset = 0;
		int xOffset;
		int Index;

		for (int Row=0; Row < pbmi->bmiHeader.biHeight; Row++)
		{
			xOffset = 0;

			for (int Column=0; Column < pbmi->bmiHeader.biWidth; Column++)
			{
				// Calculate Index
				Index = yOffset + xOffset;

				BYTE byGray = (BYTE) ((pByteArray[Index] + pByteArray[Index+1]*6 + pByteArray[Index+2] *3)/10);
				pByteArray[Index]   = byGray;
				pByteArray[Index+1] = byGray;
				pByteArray[Index+2] = byGray;

				// Increment the horizontal offset
				xOffset += pbmi->bmiHeader.biBitCount >> 3;
			}

			// Increment vertical offset
			yOffset += nWidthBytes;
		}

		// Save the modified colour back into our source DDB
		MemDC.SetDIBits(pbmSource, 0, pbmi->bmiHeader.biHeight, pByteArray, pbmi, DIB_RGB_COLORS);
	}

	inline HIMAGELIST CreateDisabledImageList(HIMAGELIST himlNormal)
	// Returns a greyed image list, created from hImageList
	{
		int cx, cy;
		int nCount = ImageList_GetImageCount(himlNormal);
		if (0 == nCount)
			return NULL;

		ImageList_GetIconSize(himlNormal, &cx, &cy);

		// Create the disabled ImageList
		HIMAGELIST himlDisabled = ImageList_Create(cx, cy, ILC_COLOR24 | ILC_MASK, nCount, 0);

		// Process each image in the ImageList
		for (int i = 0 ; i < nCount; ++i)
		{
			CClientDC DesktopDC(NULL);
			CMemDC MemDC(NULL);
			CBitmap* pOldBitmap = MemDC.CreateCompatibleBitmap(&DesktopDC, cx, cx);
			CRect rc;
			rc.SetRect(0, 0, cx, cx);

			// Set the mask color to grey for the new ImageList
			COLORREF crMask = RGB(200, 199, 200);
			if ( GetDeviceCaps(DesktopDC, BITSPIXEL) < 24)
			{
				HPALETTE hPal = (HPALETTE)GetCurrentObject(DesktopDC, OBJ_PAL);
				UINT Index = GetNearestPaletteIndex(hPal, crMask);
				if (Index != CLR_INVALID) crMask = PALETTEINDEX(Index);
			}

			MemDC.SolidFill(crMask, rc);

			// Draw the image on the memory DC
			ImageList_SetBkColor(himlNormal, crMask);
			ImageList_Draw(himlNormal, i, MemDC, 0, 0, ILD_NORMAL);

			// Convert colored pixels to gray
			for (int x = 0 ; x < cx; ++x)
			{
				for (int y = 0; y < cy; ++y)
				{
					COLORREF clr = ::GetPixel(MemDC, x, y);

					if (clr != crMask)
					{
						BYTE byGray = (BYTE) (95 + (GetRValue(clr) *3 + GetGValue(clr)*6 + GetBValue(clr))/20);
						MemDC.SetPixel(x, y, RGB(byGray, byGray, byGray));
					}

				}
			}

			// Detach the bitmap so we can use it.
			CBitmap* pBitmap = MemDC.SelectObject(pOldBitmap);
			ImageList_AddMasked(himlDisabled, *pBitmap, crMask);
		}

		return himlDisabled;
	}
#endif

	////////////////////////////////////////////
	// Global Function Definitions
	//

	inline CDC* FromHandle(HDC hDC)
	// Returns the CDC object associated with the device context handle
	// If a CDC object doesn't already exist, a temporary CDC object is created.
	// The HDC belonging to a temporary CDC is not released or destroyed when the
	//  temporary CDC is deconstructed.
	{
		assert( GetApp() );
		CDC* pDC = GetApp()->GetCDCFromMap(hDC);
		if (hDC != 0 && pDC == 0)
		{
			pDC = new CDC;
			GetApp()->AddTmpDC(pDC);
			pDC->m_pData->hDC = hDC;
			pDC->m_pData->bRemoveHDC = FALSE;
		}
		return pDC;
	}

	inline CBitmap* FromHandle(HBITMAP hBitmap)
	// Returns the CBitmap associated with the Bitmap handle
	// If a CBitmap object doesn't already exist, a temporary CBitmap object is created.
	// The HBITMAP belonging to a temporary CBitmap is not released or destroyed
	//  when the temporary CBitmap is deconstructed.
	{
		assert( GetApp() );
		CBitmap* pBitmap = (CBitmap*)GetApp()->GetCGDIObjectFromMap(hBitmap);
		if (hBitmap != 0 && pBitmap == 0)
		{
			pBitmap = new CBitmap;
			GetApp()->AddTmpGDI(pBitmap);
			pBitmap->m_pData->hGDIObject = hBitmap;
			pBitmap->m_pData->bRemoveObject = FALSE;
		}
		return pBitmap;
	}

	inline CBrush* FromHandle(HBRUSH hBrush)
	// Returns the CBrush associated with the Brush handle
	// If a CBrush object doesn't already exist, a temporary CBrush object is created.
	// The HBRUSH belonging to a temporary CBrush is not released or destroyed
	//  when the temporary CBrush is deconstructed.
	{
		assert( GetApp() );
		CBrush* pBrush = (CBrush*)GetApp()->GetCGDIObjectFromMap(hBrush);
		if (hBrush != 0 && pBrush == 0)
		{
			pBrush = new CBrush;
			GetApp()->AddTmpGDI(pBrush);
			pBrush->m_pData->hGDIObject = hBrush;
			pBrush->m_pData->bRemoveObject = FALSE;
		}
		return pBrush;
	}

	inline CFont* FromHandle(HFONT hFont)
	// Returns the CFont associated with the Font handle
	// If a CFont object doesn't already exist, a temporary CFont object is created.
	// The HFONT belonging to a temporary CFont is not released or destroyed
	//  when the temporary CFont is deconstructed.
	{
		assert( GetApp() );
		CFont* pFont = (CFont*)GetApp()->GetCGDIObjectFromMap(hFont);
		if (hFont != 0 && pFont == 0)
		{
			pFont = new CFont;
			GetApp()->AddTmpGDI(pFont);
			pFont->m_pData->hGDIObject = hFont;
			pFont->m_pData->bRemoveObject = FALSE;
		}
		return pFont;
	}

	inline CPalette* FromHandle(HPALETTE hPalette)
	// Returns the CPalette associated with the palette handle
	// If a CPalette object doesn't already exist, a temporary CPalette object is created.
	// The HPALETTE belonging to a temporary CPalette is not released or destroyed
	//  when the temporary CPalette is deconstructed.
	{
		assert( GetApp() );
		CPalette* pPalette = (CPalette*)GetApp()->GetCGDIObjectFromMap(hPalette);
		if (hPalette != 0 && pPalette == 0)
		{
			pPalette = new CPalette;
			GetApp()->AddTmpGDI(pPalette);
			pPalette->m_pData->hGDIObject = hPalette;
			pPalette->m_pData->bRemoveObject = FALSE;
		}
		return pPalette;
	}

	inline CPen* FromHandle(HPEN hPen)
	// Returns the CPen associated with the HPEN.
	// If a CPen object doesn't already exist, a temporary CPen object is created.
	// The HPEN belonging to a temporary CPen is not released or destroyed
	//  when the temporary CPen is deconstructed.
	{
		assert( GetApp() );
		CPen* pPen = (CPen*)GetApp()->GetCGDIObjectFromMap(hPen);
		if (hPen != 0 && pPen == 0)
		{
			pPen = new CPen;
			GetApp()->AddTmpGDI(pPen);
			pPen->m_pData->hGDIObject = hPen;
			pPen->m_pData->bRemoveObject = FALSE;
		}
		return pPen;
	}

	inline CRgn* FromHandle(HRGN hRgn)
	// Returns the CRgn associated with the HRGN.
	// If a CRgn object doesn't already exist, a temporary CRgn object is created.
	// The HRGN belonging to a temporary CRgn is not released or destroyed
	//  when the temporary CRgn is deconstructed.
	{
		assert( GetApp() );
		CRgn* pRgn = (CRgn*)GetApp()->GetCGDIObjectFromMap(hRgn);
		if (hRgn != 0 && pRgn == 0)
		{
			pRgn = new CRgn;
			GetApp()->AddTmpGDI(pRgn);
			pRgn->m_pData->hGDIObject = hRgn;
			pRgn->m_pData->bRemoveObject = FALSE;
		}
		return pRgn;
	}



} // namespace Win32xx

#endif // _WIN32XX_GDI_H_

