
#pragma once

#include "BackgroundInfoLoader.h"

#include "cores/ffmpeg/DllAvFormat.h"
#include "cores/ffmpeg/DllAvCodec.h"
#include "cores/ffmpeg/DllSwScale.h"

class CVideoThumbLoader : public CBackgroundInfoLoader
{
public:
  CVideoThumbLoader();
  virtual ~CVideoThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
  bool ExtractThumb(const CStdString &strPath, const CStdString &strTarget);

protected:
  virtual void OnLoaderStart() ;
  virtual void OnLoaderFinish() ;

  DllAvFormat m_dllAvFormat;
  DllAvCodec  m_dllAvCodec;
  DllAvUtil   m_dllAvUtil;
  DllSwScale  m_dllSwScale;
};

class CProgramThumbLoader : public CBackgroundInfoLoader
{
public:
  CProgramThumbLoader();
  virtual ~CProgramThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
};

class CMusicThumbLoader : public CBackgroundInfoLoader
{
public:
  CMusicThumbLoader();
  virtual ~CMusicThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
};
