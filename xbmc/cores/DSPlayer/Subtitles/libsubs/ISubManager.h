#pragma once
#include "DShowUtil/smartptr.h"
#include "DShowUtil/DSGeometry.h"
#include "../subpic/ISubPic.h"
#include "../subtitles/STS.h"

#include "initguid.h"

struct SSubStyle 
{
  COLORREF color;
  BYTE alpha;
  double fontSize;
  int fontWeight;
  bool fItalic;
  int charSet;
  int borderStyle;
  double shadowDepthX;
  double shadowDepthY;
  double outlineWidthX;
  double outlineWidthY;
  wchar_t* fontName;
};

// subtitles clsid
DEFINE_GUID(RENDERED_TEXT_SUBTITLE, 0x537DCACA, 0x2812, 0x4a4f, 0xB2, 0xC6, 0x1A, 0x34, 0xC1, 0x7A, 0xDE, 0xB0);
DEFINE_GUID(VOBFILE_SUBTITLE, 0x998D4C9A, 0x460F, 0x4de6, 0xBD, 0xCD, 0x35, 0xAB, 0x24, 0xF9, 0x4A, 0xDF);

class ISubManager
{
public:
  virtual void SetEnable(bool enable) = 0;
  virtual void SetTimePerFrame(REFERENCE_TIME timePerFrame) = 0;
  virtual void SetTextureSize(Com::SmartSize& pSize) = 0;
  virtual void SetTime(REFERENCE_TIME rtNow) = 0;
  virtual void SetSubPicProvider(ISubStream* pSubStream) = 0;
  virtual void SetStyle(SSubStyle* style) = 0;
  virtual void StopThread() = 0;
  virtual void StartThread(IDirect3DDevice9* pD3DDevice) = 0;
  virtual HRESULT InsertPassThruFilter(IGraphBuilder* pGB) = 0;
  virtual HRESULT LoadExternalSubtitle(const wchar_t* subPath, ISubStream** pSubPic) = 0;
  virtual HRESULT SetSubPicProviderToInternal() = 0;
  virtual HRESULT GetTexture(Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest, Com::SmartRect& 
    renderRect) = 0;
  virtual void Free() = 0;
  virtual HRESULT GetStreamTitle(ISubStream* pSubStream, wchar_t **subTitle) = 0;
};