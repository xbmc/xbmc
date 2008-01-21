#pragma once

#include "BackgroundInfoLoader.h"


class CVideoThumbLoader : public CBackgroundInfoLoader
{
public:
  CVideoThumbLoader();
  virtual ~CVideoThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
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
