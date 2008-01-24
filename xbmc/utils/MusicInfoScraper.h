#pragma once
#include "MusicAlbumInfo.h"
#include "MusicArtistInfo.h"
#include "HTTP.h"

struct SScraperInfo;

using namespace MUSIC_GRABBER;

namespace MUSIC_GRABBER
{
class CMusicInfoScraper : public CThread
{
public:
  CMusicInfoScraper(const SScraperInfo& info);
  virtual ~CMusicInfoScraper(void);
  void FindAlbuminfo(const CStdString& strAlbum, const CStdString& strArtist = "");
  void LoadAlbuminfo(int iAlbum);
  void FindArtistinfo(const CStdString& strArtist);
  void LoadArtistinfo(int iArtist);
  bool Completed();
  bool Successfull();
  void Cancel();
  bool IsCanceled();
  int GetAlbumCount() const;
  int GetArtistCount() const;
  CMusicAlbumInfo& GetAlbum(int iAlbum);
  CMusicArtistInfo& GetArtist(int iArtist);
protected:
  void FindAlbuminfo();
  void LoadAlbuminfo();
  void FindArtistinfo();
  void LoadArtistinfo();
  virtual void OnStartup();
  virtual void Process();
  vector<CMusicAlbumInfo> m_vecAlbums;
  vector<CMusicArtistInfo> m_vecArtists;
  CStdString m_strAlbum;
  CStdString m_strArtist;
  int m_iAlbum;
  int m_iArtist;
  bool m_bSuccessfull;
  bool m_bCanceled;
  CHTTP m_http;
  SScraperInfo m_info;
};

}
