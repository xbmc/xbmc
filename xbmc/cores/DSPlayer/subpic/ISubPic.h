/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <atlbase.h>
#include <atlcoll.h>
#include "dshowutil/DSGeometry.h"
#include "CoordGeom.h"
#include "streams.h"
#pragma pack(push, 1)
struct SubPicDesc
{
	int type;
	int w, h, bpp, pitch, pitchUV;
	void* bits;
	BYTE* bitsU;
	BYTE* bitsV;
	RECT vidrect; // video rectangle

	struct SubPicDesc() {type = 0; w = h = bpp = pitch = pitchUV = 0; bits = NULL; bitsU = bitsV = NULL;}
};
#pragma pack(pop)

//
// ISubPic
//

[uuid("449E11F3-52D1-4a27-AA61-E2733AC92CC0")]
interface ISubPic : public IUnknown
{
	STDMETHOD_(void*, GetObject) () PURE;

	STDMETHOD_(REFERENCE_TIME, GetStart) () PURE;
	STDMETHOD_(REFERENCE_TIME, GetStop) () PURE;
	STDMETHOD_(void, SetStart) (REFERENCE_TIME rtStart) PURE;
	STDMETHOD_(void, SetStop) (REFERENCE_TIME rtStop) PURE;

	STDMETHOD (GetDesc) (SubPicDesc& spd /*[out]*/) PURE;
	STDMETHOD (CopyTo) (ISubPic* pSubPic /*[in]*/) PURE;

	STDMETHOD (ClearDirtyRect) (DWORD color /*[in]*/) PURE;
	STDMETHOD (GetDirtyRect) (RECT* pDirtyRect /*[out]*/) PURE;
	STDMETHOD (SetDirtyRect) (RECT* pDirtyRect /*[in]*/) PURE;

	STDMETHOD (GetMaxSize) (SIZE* pMaxSize /*[out]*/) PURE;
	STDMETHOD (SetSize) (SIZE pSize /*[in]*/, RECT vidrect /*[in]*/) PURE;

	STDMETHOD (Lock) (SubPicDesc& spd /*[out]*/) PURE;
	STDMETHOD (Unlock) (RECT* pDirtyRect /*[in]*/) PURE;

	STDMETHOD (AlphaBlt) (RECT* pSrc, RECT* pDst, SubPicDesc* pTarget = NULL /*[in]*/) PURE;
	STDMETHOD (GetSourceAndDest) (SIZE* pSize /*[in]*/, RECT* pRcSource /*[out]*/, RECT* pRcDest /*[out]*/) PURE;
	STDMETHOD (SetVirtualTextureSize) (const SIZE pSize, const POINT pTopLeft) PURE;

	STDMETHOD_(REFERENCE_TIME, GetSegmentStart) () PURE;
	STDMETHOD_(REFERENCE_TIME, GetSegmentStop) () PURE;
	STDMETHOD_(void, SetSegmentStart) (REFERENCE_TIME rtStart) PURE;
	STDMETHOD_(void, SetSegmentStop) (REFERENCE_TIME rtStop) PURE;
};

class ISubPicImpl : public CUnknown, public ISubPic
{
protected:
	REFERENCE_TIME m_rtStart, m_rtStop;
	REFERENCE_TIME m_rtSegmentStart, m_rtSegmentStop;
	tagRECT	m_rcDirty;
	tagSIZE	m_maxsize;
	tagSIZE	m_size;
	tagRECT	m_vidrect;
	tagSIZE	m_VirtualTextureSize;
	tagPOINT	m_VirtualTextureTopLeft;

/*

                          Texture
		+-------+---------------------------------+ 
		|       .                                 |   . 
		|       .             m_maxsize           |       .
 TextureTopLeft .<=============================== |======>    .           Video
		| . . . +-------------------------------- | -----+       +-----------------------------------+
		|       |                         .       |      |       |  m_vidrect                        |
		|       |                         .       |      |       |                                   |
		|       |                         .       |      |       |                                   |
		|       |        +-----------+    .       |      |       |                                   |
		|       |        | m_rcDirty |    .       |      |       |                                   |
		|       |        |           |    .       |      |       |                                   |
		|       |        +-----------+    .       |      |       |                                   |
		|       +-------------------------------- | -----+       |                                   |
		|                    m_size               |              |                                   |
		|       <=========================>       |              |                                   |
		|                                         |              |                                   |
		|                                         |              +-----------------------------------+
		|                                         |          . 
		|                                         |      .     
		|                                         |   .   
		+-----------------------------------------+  
                   m_VirtualTextureSize 
        <=========================================>

*/


public:
	ISubPicImpl();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPic

	STDMETHODIMP_(REFERENCE_TIME) GetStart();
	STDMETHODIMP_(REFERENCE_TIME) GetStop();
	STDMETHODIMP_(void) SetStart(REFERENCE_TIME rtStart);
	STDMETHODIMP_(void) SetStop(REFERENCE_TIME rtStop);

	STDMETHODIMP GetDesc(SubPicDesc& spd) = 0;
	STDMETHODIMP CopyTo(ISubPic* pSubPic);

	STDMETHODIMP ClearDirtyRect(DWORD color) = 0;
	STDMETHODIMP GetDirtyRect(RECT* pDirtyRect);
	STDMETHODIMP SetDirtyRect(RECT* pDirtyRect);

	STDMETHODIMP GetMaxSize(SIZE* pMaxSize);
	STDMETHODIMP SetSize(SIZE size, RECT vidrect);

	STDMETHODIMP Lock(SubPicDesc& spd) = 0;
	STDMETHODIMP Unlock(RECT* pDirtyRect) = 0;

	STDMETHODIMP AlphaBlt(RECT* pSrc, RECT* pDst, SubPicDesc* pTarget) = 0;

	STDMETHODIMP SetVirtualTextureSize (const SIZE pSize, const POINT pTopLeft);
	STDMETHODIMP GetSourceAndDest(SIZE* pSize, RECT* pRcSource, RECT* pRcDest);

	STDMETHODIMP_(REFERENCE_TIME) GetSegmentStart();
	STDMETHODIMP_(REFERENCE_TIME) GetSegmentStop();
	STDMETHODIMP_(void) SetSegmentStart(REFERENCE_TIME rtStart);
	STDMETHODIMP_(void) SetSegmentStop(REFERENCE_TIME rtStop);

};

//
// ISubPicAllocator
//

[uuid("CF7C3C23-6392-4a42-9E72-0736CFF793CB")]
interface ISubPicAllocator : public IUnknown
{
	STDMETHOD (SetCurSize) (SIZE size /*[in]*/) PURE;
	STDMETHOD (SetCurVidRect) (RECT curvidrect) PURE;

	STDMETHOD (GetStatic) (ISubPic** ppSubPic /*[out]*/) PURE;
	STDMETHOD (AllocDynamic) (ISubPic** ppSubPic /*[out]*/) PURE;

	STDMETHOD_(bool, IsDynamicWriteOnly) () PURE;

	STDMETHOD (ChangeDevice) (IUnknown* pDev) PURE;
	STDMETHOD (SetMaxTextureSize) (SIZE MaxTextureSize) PURE;
};


class ISubPicAllocatorImpl : public CUnknown, public ISubPicAllocator
{
	CComPtr<ISubPic> m_pStatic;

private:
	tagSIZE m_cursize;
	tagRECT m_curvidrect;
	bool m_fDynamicWriteOnly;

	virtual bool Alloc(bool fStatic, ISubPic** ppSubPic) = 0;

protected:
	bool m_fPow2Textures;

public:
	ISubPicAllocatorImpl(SIZE cursize, bool fDynamicWriteOnly, bool fPow2Textures);

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicAllocator

	STDMETHODIMP SetCurSize(SIZE cursize);
	STDMETHODIMP SetCurVidRect(RECT curvidrect);
	STDMETHODIMP GetStatic(ISubPic** ppSubPic);
	STDMETHODIMP AllocDynamic(ISubPic** ppSubPic);
	STDMETHODIMP_(bool) IsDynamicWriteOnly();
	STDMETHODIMP ChangeDevice(IUnknown* pDev);
	STDMETHODIMP SetMaxTextureSize(SIZE MaxTextureSize) { return E_NOTIMPL; };
};

//
// ISubPicProvider
//

[uuid("D62B9A1A-879A-42db-AB04-88AA8F243CFD")]
interface ISubPicProvider : public IUnknown
{
	STDMETHOD (Lock) () PURE;
	STDMETHOD (Unlock) () PURE;

	STDMETHOD_(POSITION, GetStartPosition) (REFERENCE_TIME rt, double fps) PURE;
	STDMETHOD_(POSITION, GetNext) (POSITION pos) PURE;

	STDMETHOD_(REFERENCE_TIME, GetStart) (POSITION pos, double fps) PURE;
	STDMETHOD_(REFERENCE_TIME, GetStop) (POSITION pos, double fps) PURE;

	STDMETHOD_(bool, IsAnimated) (POSITION pos) PURE;

	STDMETHOD (Render) (SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox) PURE;
	STDMETHOD (GetTextureSize) (POSITION pos, SIZE& MaxTextureSize, SIZE& VirtualSize, POINT& VirtualTopLeft) PURE;
};

class ISubPicProviderImpl : public CUnknown, public ISubPicProvider
{
protected:
	CCritSec* m_pLock;

public:
	ISubPicProviderImpl(CCritSec* pLock);
	virtual ~ISubPicProviderImpl();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicProvider

	STDMETHODIMP Lock();
	STDMETHODIMP Unlock();

	STDMETHODIMP_(POSITION) GetStartPosition(REFERENCE_TIME rt, double fps) = 0;
	STDMETHODIMP_(POSITION) GetNext(POSITION pos) = 0;

	STDMETHODIMP_(REFERENCE_TIME) GetStart(POSITION pos, double fps) = 0;
	STDMETHODIMP_(REFERENCE_TIME) GetStop(POSITION pos, double fps) = 0;

	STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox) = 0;
	STDMETHODIMP GetTextureSize (POSITION pos, SIZE& MaxTextureSize, SIZE& VirtualSize, POINT& VirtualTopLeft) { return E_NOTIMPL; };
};

//
// ISubPicQueue
//

[uuid("C8334466-CD1E-4ad1-9D2D-8EE8519BD180")]
interface ISubPicQueue : public IUnknown
{
	STDMETHOD (SetSubPicProvider) (ISubPicProvider* pSubPicProvider /*[in]*/) PURE;
	STDMETHOD (GetSubPicProvider) (ISubPicProvider** pSubPicProvider /*[out]*/) PURE;

	STDMETHOD (SetFPS) (double fps /*[in]*/) PURE;
	STDMETHOD (SetTime) (REFERENCE_TIME rtNow /*[in]*/) PURE;

	STDMETHOD (Invalidate) (REFERENCE_TIME rtInvalidate = -1) PURE;
	STDMETHOD_(bool, LookupSubPic) (REFERENCE_TIME rtNow /*[in]*/, CComPtr<ISubPic> &pSubPic /*[out]*/) PURE;

	STDMETHOD (GetStats) (int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop /*[out]*/) PURE;
	STDMETHOD (GetStats) (int nSubPic /*[in]*/, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop /*[out]*/) PURE;
};

class ISubPicQueueImpl : public CUnknown, public ISubPicQueue
{
	CCritSec m_csSubPicProvider;
	CComPtr<ISubPicProvider> m_pSubPicProvider;

protected:
	double m_fps;
	REFERENCE_TIME m_rtNow;
	REFERENCE_TIME m_rtNowLast;

	CComPtr<ISubPicAllocator> m_pAllocator;

	HRESULT RenderTo(ISubPic* pSubPic, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, double fps, BOOL bIsAnimated);

public:
	ISubPicQueueImpl(ISubPicAllocator* pAllocator, HRESULT* phr);
	virtual ~ISubPicQueueImpl();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicQueue

	STDMETHODIMP SetSubPicProvider(ISubPicProvider* pSubPicProvider);
	STDMETHODIMP GetSubPicProvider(ISubPicProvider** pSubPicProvider);

	STDMETHODIMP SetFPS(double fps);
	STDMETHODIMP SetTime(REFERENCE_TIME rtNow);
/*
	STDMETHODIMP Invalidate(REFERENCE_TIME rtInvalidate = -1) = 0;
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, ISubPic** ppSubPic) = 0;

	STDMETHODIMP GetStats(int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop) = 0;
	STDMETHODIMP GetStats(int nSubPics, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop) = 0;
*/
};

class CSubPicQueue : public ISubPicQueueImpl, private CAMThread
{
	int m_nMaxSubPic;
	BOOL m_bDisableAnim;

	CInterfaceList<ISubPic> m_Queue;

	CCritSec m_csQueueLock; // for protecting CInterfaceList<ISubPic>
	REFERENCE_TIME UpdateQueue();
	void AppendQueue(ISubPic* pSubPic);
	int GetQueueCount();

	REFERENCE_TIME m_rtQueueMin;
	REFERENCE_TIME m_rtQueueMax;
	REFERENCE_TIME m_rtInvalidate;

	// CAMThread

	bool m_fBreakBuffering;
	enum {EVENT_EXIT, EVENT_TIME, EVENT_COUNT}; // IMPORTANT: _EXIT must come before _TIME if we want to exit fast from the destructor
	HANDLE m_ThreadEvents[EVENT_COUNT];
    DWORD ThreadProc();

public:
	CSubPicQueue(int nMaxSubPic, BOOL bDisableAnim, ISubPicAllocator* pAllocator, HRESULT* phr);
	virtual ~CSubPicQueue();

	// ISubPicQueue

	STDMETHODIMP SetFPS(double fps);
	STDMETHODIMP SetTime(REFERENCE_TIME rtNow);

	STDMETHODIMP Invalidate(REFERENCE_TIME rtInvalidate = -1);
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic> &pSubPic);

	STDMETHODIMP GetStats(int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	STDMETHODIMP GetStats(int nSubPic, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
};

class CSubPicQueueNoThread : public ISubPicQueueImpl
{
	CCritSec m_csLock;
	CComPtr<ISubPic> m_pSubPic;

public:
	CSubPicQueueNoThread(ISubPicAllocator* pAllocator, HRESULT* phr);
	virtual ~CSubPicQueueNoThread();

	// ISubPicQueue

	STDMETHODIMP Invalidate(REFERENCE_TIME rtInvalidate = -1);
	STDMETHODIMP_(bool) LookupSubPic(REFERENCE_TIME rtNow, CComPtr<ISubPic> &pSubPic);

	STDMETHODIMP GetStats(int& nSubPics, REFERENCE_TIME& rtNow, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
	STDMETHODIMP GetStats(int nSubPic, REFERENCE_TIME& rtStart, REFERENCE_TIME& rtStop);
};

//
// ISubPicAllocatorPresenter
//

[uuid("CF75B1F0-535C-4074-8869-B15F177F944E")]
interface ISubPicAllocatorPresenter : public IUnknown
{
	STDMETHOD (CreateRenderer) (IUnknown** ppRenderer) PURE;

	STDMETHOD_(SIZE, GetVideoSize) (bool fCorrectAR = true) PURE;
	STDMETHOD_(void, SetPosition) (RECT w, RECT v) PURE;
	STDMETHOD_(bool, Paint) (bool fAll) PURE;

	STDMETHOD_(void, SetTime) (REFERENCE_TIME rtNow) PURE;
	STDMETHOD_(void, SetSubtitleDelay) (int delay_ms) PURE;
	STDMETHOD_(int, GetSubtitleDelay) () PURE;
	STDMETHOD_(double, GetFPS) () PURE;

	STDMETHOD_(void, SetSubPicProvider) (ISubPicProvider* pSubPicProvider) PURE;
	STDMETHOD_(void, Invalidate) (REFERENCE_TIME rtInvalidate = -1) PURE;

	STDMETHOD (GetDIB) (BYTE* lpDib, DWORD* size) PURE;

	STDMETHOD (SetVideoAngle) (Vector v, bool fRepaint = true) PURE;
	STDMETHOD (SetPixelShader) (LPCSTR pSrcData, LPCSTR pTarget) PURE;
};

[uuid("767AEBA8-A084-488a-89C8-F6B74E53A90F")]
interface ISubPicAllocatorPresenter2 : public ISubPicAllocatorPresenter
{
	STDMETHOD (SetPixelShader2) (LPCSTR pSrcData, LPCSTR pTarget, bool bScreenSpace) PURE;
};

class ISubPicAllocatorPresenterImpl 
	: public CUnknown
	, public CCritSec
	, public ISubPicAllocatorPresenter2
{
protected:
	HWND m_hWnd;
	tagSIZE m_spMaxSize; // TODO:
	int m_spMaxQueued; // TODO:
	REFERENCE_TIME m_rtSubtitleDelay;

	tagSIZE m_NativeVideoSize, m_AspectRatio;
	tagRECT m_VideoRect, m_WindowRect;

	REFERENCE_TIME m_rtNow;
	double m_fps;

	CComPtr<ISubPicProvider> m_SubPicProvider;
	CComPtr<ISubPicAllocator> m_pAllocator;
	CComPtr<ISubPicQueue> m_pSubPicQueue;

	void AlphaBltSubPic(tagSIZE size, SubPicDesc* pTarget = NULL);

    XForm m_xform;
	void Transform(tagRECT r, Vector v[4]);

public:
	ISubPicAllocatorPresenterImpl(HWND hWnd, HRESULT& hr, CStdString *_pError);
	virtual ~ISubPicAllocatorPresenterImpl();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicAllocatorPresenter

	STDMETHODIMP CreateRenderer(IUnknown** ppRenderer) = 0;

	STDMETHODIMP_(SIZE) GetVideoSize(bool fCorrectAR = true);
	STDMETHODIMP_(void) SetPosition(RECT w, RECT v);
	STDMETHODIMP_(bool) Paint(bool fAll) = 0;

	STDMETHODIMP_(void) SetTime(REFERENCE_TIME rtNow);
	STDMETHODIMP_(void) SetSubtitleDelay(int delay_ms);
	STDMETHODIMP_(int) GetSubtitleDelay();
	STDMETHODIMP_(double) GetFPS();

	STDMETHODIMP_(void) SetSubPicProvider(ISubPicProvider* pSubPicProvider);	
	STDMETHODIMP_(void) Invalidate(REFERENCE_TIME rtInvalidate = -1);

	STDMETHODIMP GetDIB(BYTE* lpDib, DWORD* size) {return E_NOTIMPL;}

	STDMETHODIMP SetVideoAngle(Vector v, bool fRepaint = true);
	STDMETHODIMP SetPixelShader(LPCSTR pSrcData, LPCSTR pTarget) {return E_NOTIMPL;}
	STDMETHODIMP SetPixelShader2(LPCSTR pSrcData, LPCSTR pTarget, bool bScreenSpace) 
	{
		if (!bScreenSpace)
			return SetPixelShader(pSrcData, pTarget);
		return E_NOTIMPL;
	}
};

//
// ISubStream
//

[uuid("DE11E2FB-02D3-45e4-A174-6B7CE2783BDB")]
interface ISubStream : public IPersist
{
	STDMETHOD_(int, GetStreamCount) () PURE;
	STDMETHOD (GetStreamInfo) (int i, WCHAR** ppName, LCID* pLCID) PURE;
	STDMETHOD_(int, GetStream) () PURE;
	STDMETHOD (SetStream) (int iStream) PURE;
	STDMETHOD (Reload) () PURE;

	// TODO: get rid of IPersist to identify type and use only 
	// interface functions to modify the settings of the substream
};




