#pragma once
#include "PlayList.h"

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
  virtual bool LoadData(std::istream &stream);  
protected:
  bool LoadAsxIniInfo(std::istream &stream);
};

class CPlayListRAM : public CPlayList
{
public:
  virtual bool LoadData(std::istream &stream);
};


}
