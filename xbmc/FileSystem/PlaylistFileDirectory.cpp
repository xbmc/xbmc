#include "../stdafx.h"
#include "PlaylistFileDirectory.h"
#include "../utils/log.h"
#include "../PlaylistFactory.h"
#include "../Util.h"

namespace DIRECTORY
{
  CPlaylistFileDirectory::CPlaylistFileDirectory()
  {
  }

  CPlaylistFileDirectory::~CPlaylistFileDirectory()
  {
  }

  bool CPlaylistFileDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {
    CPlayListFactory factory;
    auto_ptr<CPlayList> pPlayList (factory.Create(strPath));
    if ( NULL != pPlayList.get())
    {
      // load it
      if (!pPlayList->Load(strPath, false))
        return false; //hmmm unable to load playlist?

      CPlayList playlist = *pPlayList;
      // convert playlist items to songs
      for (int i = 0; i < (int)playlist.size(); ++i)
      {
        CFileItem *item = new CFileItem(playlist[i]);
        item->m_iprogramCount = i;  // hack for playlist order
        item->m_musicInfoTag.SetDuration(playlist[i].GetDuration());
        items.Add(item);
      }
    }
    return true;
  }

  bool CPlaylistFileDirectory::ContainsFiles(const CStdString& strPath)
  {
    CPlayListFactory factory;
    auto_ptr<CPlayList> pPlayList (factory.Create(strPath));
    if ( NULL != pPlayList.get())
    {
      // load it
      if (!pPlayList->Load(strPath, false))
        return false; //hmmm unable to load playlist?

      return (pPlayList->size() > 1);
    }
    return false;
  }
}