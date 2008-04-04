#ifndef SCRAPER_URL_H
#define SCRAPER_URL_H

#include "../../guilib/tinyXML/tinyxml.h"
#include "../../guilib/StdString.h"

#include <vector>

class CHTTP;

class CScraperUrl
{
public:
  CScraperUrl(const CStdString&);
  CScraperUrl(const TiXmlElement*);
  CScraperUrl();
  ~CScraperUrl();

  enum URLTYPES
  {
    URL_TYPE_GENERAL = 1,
    URL_TYPE_SEASON = 2
  };

  struct SUrlEntry 
  {
    CStdString m_spoof;
    CStdString m_url;
    CStdString m_cache;
    URLTYPES m_type;
    bool m_post;
    int m_season;
  };

  bool Parse();
  bool ParseString(CStdString); // copies by intention
  bool ParseElement(const TiXmlElement*);
  bool ParseEpisodeGuide(CStdString strUrls); // copies by intention

  const SUrlEntry GetFirstThumb() const;
  const SUrlEntry GetSeasonThumb(int) const;
  void Clear();
  static bool Get(const SUrlEntry&, std::string&, CHTTP& http);
  static bool DownloadThumbnail(const CStdString &thumb, const SUrlEntry& entry);
  static void ClearCache();

  CStdString m_xml;
  CStdString m_spoof; // for backwards compatibility only!
  CStdString strTitle;
  CStdString strId;
  std::vector<SUrlEntry> m_url;
};

#endif


