#ifndef SCRAPER_URL_H
#define SCRAPER_URL_H

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>
#include <map>
#include <string>

class TiXmlElement;
namespace XFILE { class CCurlFile; }

class CScraperUrl
{
public:
  CScraperUrl(const std::string&);
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
    std::string m_spoof;
    std::string m_url;
    std::string m_cache;
    std::string m_aspect;
    URLTYPES m_type;
    bool m_post;
    bool m_isgz;
    int m_season;
  };

  bool Parse();
  bool ParseString(std::string); // copies by intention
  bool ParseElement(const TiXmlElement*);
  bool ParseEpisodeGuide(std::string strUrls); // copies by intention

  const SUrlEntry GetFirstThumb(const std::string &type = "") const;
  const SUrlEntry GetSeasonThumb(int season, const std::string &type = "") const;
  unsigned int GetMaxSeasonThumb() const;

  /*! \brief fetch the full URL (including referrer) of a thumb
   \param URL entry to use to create the full URL
   \return the full URL, including referrer
   */
  static std::string GetThumbURL(const CScraperUrl::SUrlEntry &entry);

  /*! \brief fetch the full URL (including referrer) of thumbs
   \param thumbs [out] vector of thumb URLs to fill
   \param type the type of thumb URLs to fetch, if empty (the default) picks any
   \param season number of season that we want thumbs for, -1 indicates no season (the default)
   */
  void GetThumbURLs(std::vector<std::string> &thumbs, const std::string &type = "", int season = -1) const;
  void Clear();
  static bool Get(const SUrlEntry&, std::string&, XFILE::CCurlFile& http,
                 const std::string& cacheContext);

  std::string m_xml;
  std::string m_spoof; // for backwards compatibility only!
  std::string strTitle;
  std::string strId;
  double relevance;
  std::vector<SUrlEntry> m_url;
};

#endif


