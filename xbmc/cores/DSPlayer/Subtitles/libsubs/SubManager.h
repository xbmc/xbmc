#pragma once

#include "ISubManager.h"
#include "../subpic/ISubPic.h"
#include "../subtitles/STS.h"

class CRenderedTextSubtitle;

extern BOOL g_overrideUserStyles;

class CSubManager: public ISubManager
{
public:
  CSubManager(IDirect3DDevice9* d3DDev, SIZE size, SSubSettings settings, HRESULT& hr);
  ~CSubManager(void);

  void SetEnable(bool enable);

  void SetTimePerFrame(REFERENCE_TIME timePerFrame);
  void SetTime(REFERENCE_TIME rtNow);

  void SetStyle(SSubStyle* style);
  void SetSubPicProvider(ISubStream* pSubStream);
  void SetTextureSize(Com::SmartSize& pSize);
  void SetSizes(Com::SmartRect window, Com::SmartRect video);

  void StopThread();
  void StartThread(IDirect3DDevice9* pD3DDevice);

  //load internal subtitles through TextPassThruFilter
  HRESULT InsertPassThruFilter(IGraphBuilder* pGB);
  HRESULT LoadExternalSubtitle(const wchar_t* subPath, ISubStream** pSubPic);
  HRESULT SetSubPicProviderToInternal();

  HRESULT GetTexture(Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest, Com::SmartRect& renderRect);

  void Free();

  HRESULT GetStreamTitle(ISubStream* pSubStream, wchar_t **subTitle);

private:
  friend class CTextPassThruInputPin;
  friend class CTextPassThruFilter;
  //SetTextPassThruSubStream, InvalidateSubtitle are called from CTextPassThruInputPin
  void SetTextPassThruSubStream(ISubStream* pSubStreamNew);
  void InvalidateSubtitle(DWORD_PTR nSubtitleId, REFERENCE_TIME rtInvalidate);

  void UpdateSubtitle();
  void ApplyStyle(CRenderedTextSubtitle* pRTS);
  void ApplyStyleSubStream(ISubStream* pSubStream);

  CCritSec m_csSubLock; 
  int m_iSubtitleSel; // if(m_iSubtitleSel&(1<<31)): disabled
  REFERENCE_TIME m_rtNow; //current time
  double m_fps;
  
  STSStyle m_style;
  bool m_useDefaultStyle;

  SSubSettings m_settings;
  Com::SmartSize m_lastSize;

  boost::shared_ptr<ISubPicQueue> m_pSubPicQueue;
  Com::SmartPtr<ISubPicAllocator> m_pAllocator;

  Com::SmartQIPtr<IAMStreamSelect> m_pSS; //graph filter with subtitles
  IDirect3DDevice9* m_d3DDev;
  Com::SmartPtr<ISubStream> m_pInternalSubStream;
  ISubPicProvider* m_pSubPicProvider; // save when changing d3d device

  REFERENCE_TIME m_rtTimePerFrame;

  //CSubresync m_subresync;
};