#pragma once
#include "DShowUtil/smartptr.h"
#include "DShowUtil/DSGeometry.h"
#include "../subpic/ISubPic.h"
#include "../subtitles/STS.h"

class ISubManager
{
public:
  virtual void SetEnable(bool enable) = 0;
  virtual void SetTimePerFrame(REFERENCE_TIME timePerFrame) = 0;
  virtual void SetTextureSize(Com::SmartSize& pSize) = 0;
  virtual void SetTime(REFERENCE_TIME rtNow) = 0;
  virtual void SetSubPicProvider(ISubStream* pSubStream) = 0;
  virtual void SetStyle(STSStyle &style) = 0;
  virtual HRESULT InsertPassThruFilter(IGraphBuilder* pGB) = 0;
  virtual HRESULT LoadExternalSubtitle(const wchar_t* subPath, ISubStream** pSubPic) = 0;
  virtual HRESULT SetSubPicProviderToInternal() = 0;
  virtual HRESULT GetTexture(Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest, Com::SmartRect& renderRect) = 0;
  virtual void Free() = 0;
};