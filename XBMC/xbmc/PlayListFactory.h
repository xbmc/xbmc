#pragma once

#include "playlist.h"
using namespace PLAYLIST;
namespace PLAYLIST
{

class CPlayListFactory
{
public:
  CPlayListFactory(void);
  virtual ~CPlayListFactory(void);
  CPlayList* Create(const CStdString& strFileName) const;
};
};
