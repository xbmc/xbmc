#pragma once
#include "Subresync.h"
#include "ISubManager.h"

class CRenderedTextSubtitle;

extern BOOL g_overrideUserStyles;
extern int g_subPicsBufferAhead;
extern bool g_pow2tex;
extern BOOL g_disableAnim;

class CSubManager: public ISubManager
{
public:
	CSubManager(IDirect3DDevice9* d3DDev, SIZE size, HRESULT& hr);
	~CSubManager(void);

	void SetEnable(bool enable);
	int GetDelay(); 
	void SetDelay(int delay);
	bool IsModified() { return m_subresync.IsModified(); };

  void SetTimePerFrame(REFERENCE_TIME timePerFrame);
  void SetTime(REFERENCE_TIME rtNow);

  void SetStyle(STSStyle& style);
  void SetSubPicProvider(ISubStream* pSubStream);
  void SetTextureSize(Com::SmartSize& pSize);

  //load internal subtitles through TextPassThruFilter
  HRESULT InsertPassThruFilter(IGraphBuilder* pGB);
  HRESULT LoadExternalSubtitle(const wchar_t* subPath, ISubStream** pSubPic);
  HRESULT SetSubPicProviderToInternal();

  HRESULT GetTexture(Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest, Com::SmartRect& renderRect);

  void Free();

private:
	friend class CTextPassThruInputPin;
	friend class CTextPassThruFilter;
	//ReplaceSubtitle, InvalidateSubtitle are called from CTextPassThruInputPin
	void SetTextPassThruSubStream(ISubStream* pSubStreamOld, ISubStream* pSubStreamNew);
	void InvalidateSubtitle(DWORD_PTR nSubtitleId, REFERENCE_TIME rtInvalidate);

	void UpdateSubtitle();
	void ApplyStyle(CRenderedTextSubtitle* pRTS);
	void ApplyStyleSubStream(ISubStream* pSubStream);

	CCritSec m_csSubLock; 
	int m_iSubtitleSel; // if(m_iSubtitleSel&(1<<31)): disabled
	REFERENCE_TIME m_rtNow; //current time
	double m_fps;
	REFERENCE_TIME m_delay;
  
  STSStyle m_style;
  bool m_useDefaultStyle;

  Com::SmartSize m_textureSize;
	Com::SmartSize m_lastSize;

  boost::shared_ptr<ISubPicQueue> m_pSubPicQueue;
  Com::SmartPtr<ISubPicAllocator> m_pAllocator;

	Com::SmartQIPtr<IAMStreamSelect> m_pSS; //graph filter with subtitles
  Com::SmartPtr<IDirect3DDevice9> m_d3DDev;
  Com::SmartPtr<ISubStream> m_pInternalSubStream;

  REFERENCE_TIME m_rtTimePerFrame;

	CSubresync m_subresync;
};