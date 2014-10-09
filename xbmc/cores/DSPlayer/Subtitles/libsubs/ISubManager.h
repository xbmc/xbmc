#pragma once

#include "initguid.h"
#include "system.h"


struct SSubStyle 
{
  COLORREF colors[4];
  BYTE alpha[4];
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

  SSubStyle()
  {
    SetDefault();
  }

  void SetDefault()
  {
    borderStyle = 0;
    outlineWidthX = outlineWidthY = 2;
    shadowDepthX = shadowDepthY = 3;
    colors[0] = 0x00ffffff;
    colors[1] = 0x0000ffff;
    colors[2] = 0x00000000;
    colors[3] = 0x00000000;
    alpha[0] = 0x00;
    alpha[1] = 0x00;
    alpha[2] = 0x00;
    alpha[3] = 0x80;
    charSet = DEFAULT_CHARSET;
    fontName = NULL;
    fontSize = 18;
    fontWeight = FW_BOLD;
    fItalic = false;
  }
};

struct SSubSettings
{
public:
  bool forcePowerOfTwoTextures;
  bool disableAnimations;
  uint32_t bufferAhead;
  SIZE textureSize;

  SSubSettings()
  {
    forcePowerOfTwoTextures = false;
    disableAnimations = true;
    bufferAhead = 3;
    textureSize.cx = 1024;
    textureSize.cy = 768;
  }
};

// subtitles clsid
DEFINE_GUID(RENDERED_TEXT_SUBTITLE, 0x537DCACA, 0x2812, 0x4a4f, 0xB2, 0xC6, 0x1A, 0x34, 0xC1, 0x7A, 0xDE, 0xB0);
DEFINE_GUID(VOBFILE_SUBTITLE, 0x998D4C9A, 0x460F, 0x4de6, 0xBD, 0xCD, 0x35, 0xAB, 0x24, 0xF9, 0x4A, 0xDF);

interface ISubStream;
struct SSubStyle;
namespace Com {
  class SmartRect;
  class SmartSize;
  template<class T> class SmartPtr;
}
interface IDirect3DDevice9;
interface IDirect3DTexture9;
interface IGraphBuilder;
typedef LONGLONG REFERENCE_TIME;

class ISubManager
{
public:
  virtual void SetEnable(bool enable) = 0;
  virtual void SetTimePerFrame(REFERENCE_TIME timePerFrame) = 0;
  virtual void SetTextureSize(Com::SmartSize& pSize) = 0;
  virtual void SetSizes(Com::SmartRect window, Com::SmartRect video) = 0;
  virtual void SetTime(REFERENCE_TIME rtNow) = 0;
  virtual void SetSubPicProvider(ISubStream* pSubStream) = 0;
  virtual void SetStyle(SSubStyle* style, bool bOverride) = 0;
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