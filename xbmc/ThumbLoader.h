#include "BackgroundInfoLoader.h"

#pragma once

class CThumbLoader : public CBackgroundInfoLoader
{
public:
  CThumbLoader();
  virtual ~CThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
};
