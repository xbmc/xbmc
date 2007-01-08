#pragma once
#include "playlist.h"
using namespace PLAYLIST;
namespace PLAYLIST
{

class CPlayListWPL :
      public CPlayList
{
public:
  CPlayListWPL(void);
  virtual ~CPlayListWPL(void);
  virtual bool Load(const CStdString& strFileName);
  virtual void Save(const CStdString& strFileName) const;
};
};
