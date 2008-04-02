#pragma once
#include "BackgroundInfoLoader.h"
#include "FileItem.h"

class CPictureInfoLoader : public CBackgroundInfoLoader
{
public:
  CPictureInfoLoader();
  virtual ~CPictureInfoLoader();

  void UseCacheOnHD(const CStdString& strFileName);
  virtual bool LoadItem(CFileItem* pItem);

protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();
protected:
  CFileItemList m_mapFileItems;
  unsigned int m_tagReads;
  bool m_loadTags;
};

