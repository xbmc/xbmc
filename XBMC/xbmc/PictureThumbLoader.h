#include "BackgroundInfoLoader.h"

#pragma once

class CPictureThumbLoader : public CBackgroundInfoLoader
{
public:
  CPictureThumbLoader();
  virtual ~CPictureThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
  void SetRegenerateThumbs(bool regenerate) { m_regenerateThumbs = regenerate; };
protected:
  virtual void OnLoaderFinish();
private:
  bool m_regenerateThumbs;
};
