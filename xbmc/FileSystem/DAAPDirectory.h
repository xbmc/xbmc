#pragma once
#include "IDirectory.h"

extern "C"
{
#include "../lib/libXDAAP/client.h"
 #include "../lib/libXDAAP/private.h"
}

namespace DIRECTORY
{
class CDAAPDirectory :

      public IDirectory
{
public:
  CDAAPDirectory(void);
  virtual ~CDAAPDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  //virtual void CloseDAAP(void);
  int GetCurrLevel(CStdString strPath);

private:
  void free_albums(albumPTR *alb);
  void free_artists();
  void AddToArtistAlbum(char *artist_s, char *album_s);

  DAAP_ClientHost_DatabaseItem *m_currentSongItems;
  int m_currentSongItemCount;

  DAAP_SClientHost *m_thisHost;
  int m_currLevel;

  artistPTR *m_artisthead;
  CStdString m_selectedPlaylist;
  CStdString m_selectedArtist;
  CStdString m_selectedAlbum;
};
}
