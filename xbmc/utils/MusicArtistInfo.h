#pragma once

class TiXmlDocument;
class CScraperUrl;
struct SScraperInfo;

namespace MUSIC_GRABBER
{
class CMusicArtistInfo
{
public:
  CMusicArtistInfo(void);
  CMusicArtistInfo(const CStdString& strArtist, const CScraperUrl& strArtistURL);
  virtual ~CMusicArtistInfo(void);
  bool Loaded() const;
  void SetLoaded(bool bOnOff);
  void SetArtist(const CArtist& artist);
  const CArtist& GetArtist() const;
  CArtist& GetArtist();
  const CScraperUrl& GetArtistURL() const;
  bool Load(CHTTP& http, const SScraperInfo& info, const CStdString& strFunction="GetArtistDetails", const CScraperUrl* url=NULL);
  bool Parse(const TiXmlElement* artist, bool bChained=false);
protected:
  CArtist m_artist;
  CScraperUrl m_artistURL;
  bool m_bLoaded;
};
};
