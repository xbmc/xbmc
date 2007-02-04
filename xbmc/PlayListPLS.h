#pragma once
#include "playlist.h"

namespace PLAYLIST
{
class CPlayListPLS :
      public CPlayList
{
public:
  CPlayListPLS(void);
  virtual ~CPlayListPLS(void);
  virtual bool Load(const CStdString& strFileName);
  virtual void Save(const CStdString& strFileName) const;
};

class CPlayListASX : public CPlayList
{
public:
  virtual bool LoadData(const CStdString& strData);
protected:
  bool LoadAsxIniInfo(const CStdString& strData);
};

class CPlayListRAM : public CPlayList
{
public:
  virtual bool LoadData(const CStdString& strData);
};


};
