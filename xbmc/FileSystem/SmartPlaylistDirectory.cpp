#include "stdafx.h"
#include "SmartPlaylistDirectory.h"
#include "../utils/log.h"
#include "../SmartPlaylist.h"
#include "../MusicDatabase.h"
#include "../VideoDatabase.h"
#include "../Util.h"

using namespace PLAYLIST;

namespace DIRECTORY
{
  CSmartPlaylistDirectory::CSmartPlaylistDirectory()
  {
  }

  CSmartPlaylistDirectory::~CSmartPlaylistDirectory()
  {
  }

  bool CSmartPlaylistDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {
    // Load in the SmartPlaylist and get the WHERE query
    CSmartPlaylist playlist;
    if (!playlist.Load(strPath))
      return false;
    bool success;
    if (playlist.GetType().Equals("music"))
    {
      CMusicDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetSongsByWhere("", whereOrder, items);
      db.Close();
    }
    if (playlist.GetType().Equals("video"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetMusicVideosByWhere("videodb://3/2/", whereOrder, items);
      db.Close();
    }

    return success;
  }

  bool CSmartPlaylistDirectory::ContainsFiles(const CStdString& strPath)
  {
    // smart playlists always have files??
    return true;
  }

  CStdString CSmartPlaylistDirectory::GetPlaylistByName(const CStdString& name)
  {
    CFileItemList list;
    if (CDirectory::GetDirectory("special://musicplaylists/", list, "*.xsp"))
    {
      for (int i = 0; i < list.Size(); i++)
      {
        CFileItem *item = list[i];
        if (item->GetLabel().CompareNoCase(name) == 0)
        { // found :)
          return item->m_strPath;
        } 
      }
    }
    return "";
  }
}