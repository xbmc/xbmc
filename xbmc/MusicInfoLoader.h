#include "BackgroundInfoLoader.h"

#pragma once

#include "MusicDatabase.h"

namespace MUSIC_INFO
{
class CMusicInfoLoader : public CBackgroundInfoLoader
{
public:
  CMusicInfoLoader();
  virtual ~CMusicInfoLoader();

  void UseCacheOnHD(const CStdString& strFileName);
  virtual bool LoadItem(CFileItem* pItem);

protected:
  virtual void OnLoaderStart();
  virtual void OnLoaderFinish();
  void LoadCache(const CStdString& strFileName, MAPFILEITEMS& items);
  void SaveCache(const CStdString& strFileName, CFileItemList& items);
protected:
  CStdString m_strCacheFileName;
  MAPFILEITEMS m_mapFileItems;
  IMAPFILEITEMS it;
  MAPSONGS m_songsMap;
  CStdString m_strPrevPath;
  CMusicDatabase m_musicDatabase;
};
};
