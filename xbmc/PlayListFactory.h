#pragma once

#include "playlist.h"

namespace PLAYLIST
{

class CPlayListFactory
{
public:
  static CPlayList* Create(const CStdString& filename);
  static CPlayList* Create(const CFileItem& item);
  static bool IsPlaylist(const CStdString& filename);
};
}
