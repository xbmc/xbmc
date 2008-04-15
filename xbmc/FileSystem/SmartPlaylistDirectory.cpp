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
    bool success = false, success2 = false;
    if (playlist.GetType().Equals("tvshows"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetTvShowsByWhere("videodb://2/2/", whereOrder, items);
      items.SetContent("tvshows");
      db.Close();
      return success;
    }
    else if (playlist.GetType().Equals("episodes"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetEpisodesByWhere("videodb://2/2/", whereOrder, items);
      items.SetContent("episodes");
      db.Close();
      return success;
    }
    else if (playlist.GetType().Equals("movies"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetMoviesByWhere("videodb://1/2/", whereOrder, items);
      items.SetContent("movies");
      db.Close();
      return success;
    }
    if (playlist.GetType().Equals("music") || playlist.GetType().Equals("mixed") || playlist.GetType().IsEmpty())
    {
      CMusicDatabase db;
      db.Open();
      CStdString type=playlist.GetType();
      if (type.IsEmpty())
        type = "music";
      if (playlist.GetType().Equals("mixed"))
        playlist.SetType("music");

      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      success = db.GetSongsByWhere("", whereOrder, items);
      db.Close();
      playlist.SetType(type);
    }
    if (playlist.GetType().Equals("musicvideos") || playlist.GetType().Equals("mixed"))
    {
      CVideoDatabase db;
      db.Open();
      CStdString type=playlist.GetType();
      if (playlist.GetType().Equals("mixed"))
        playlist.SetType("video");
      CStdString whereOrder = playlist.GetWhereClause() + " " + playlist.GetOrderClause();
      CFileItemList items2;
      success2 = db.GetMusicVideosByWhere("videodb://3/2/", whereOrder, items2, false); // TODO: SMARTPLAYLISTS Don't check locks???
      db.Close();
      items.Append(items2);
      playlist.SetType(type);
    }
    if (playlist.GetType().Equals("mixed"))
      return success || success2;
    else if (playlist.GetType().Equals("musicvideos"))
      return success2;
    else
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


