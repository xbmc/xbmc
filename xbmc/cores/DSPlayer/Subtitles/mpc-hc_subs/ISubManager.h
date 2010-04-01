#pragma once
#include "DShowUtil/smartptr.h"
#include "DShowUtil/DSGeometry.h"
#include "../subpic/ISubPic.h"

class ISubManager
{
public:
  virtual void InsertPassThruFilter(IGraphBuilder* pGB) = 0;
  virtual void LoadExternalSubtitles(const wchar_t* fn, const wchar_t* paths) = 0;
  //virtual void LoadSubtitlesForFile(const wchar_t* fn, IGraphBuilder* pGB, const wchar_t* paths) = 0;
  virtual void SetEnable(bool enable) = 0;
  virtual void Render(int x, int y, int width, int height) = 0;
  virtual int GetDelay() = 0;
  virtual void SetDelay(int delay) = 0;
  virtual bool IsModified() = 0;
  virtual void SetTimePerFrame(REFERENCE_TIME timePerFrame) = 0;
  virtual void SetSegmentStart(REFERENCE_TIME segmentStart) = 0;
  virtual void SetSampleStart(REFERENCE_TIME sampleStart) = 0;
  virtual void SetSubPicProvider(ISubStream* pSubStream) = 0;
  virtual HRESULT GetTexture(Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest) = 0;
};