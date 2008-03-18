#pragma once
#include "PlayList.h"

namespace PLAYLIST
{
class CPlayListM3U :
      public CPlayList
{
public:
  CPlayListM3U(void);
  virtual ~CPlayListM3U(void);
  virtual bool Load(const CStdString& strFileName);
  virtual void Save(const CStdString& strFileName) const;
};
}
