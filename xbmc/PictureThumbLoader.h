#include "BackgroundInfoLoader.h"

#pragma once

class CPictureThumbLoader : public CBackgroundInfoLoader
{
public:
  CPictureThumbLoader();
  virtual ~CPictureThumbLoader();

protected:
  virtual bool LoadItem(CFileItem* pItem);
};
