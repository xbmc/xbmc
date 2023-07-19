/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

class TiXmlElement;
namespace XFILE
{
class CCurlFile;
}

class CScraperUrl
{
public:
  enum class UrlType
  {
    General = 1,
    Season = 2
  };

  struct SUrlEntry
  {
    explicit SUrlEntry(std::string url = "") : m_url(std::move(url)) {}

    std::string m_spoof;
    std::string m_url;
    std::string m_cache;
    std::string m_aspect;
    std::string m_preview;
    UrlType m_type = UrlType::General;
    bool m_post = false;
    bool m_isgz = false;
    int m_season = -1;
  };

  CScraperUrl();
  explicit CScraperUrl(const std::string& strUrl);
  explicit CScraperUrl(const TiXmlElement* element);
  ~CScraperUrl();

  void Clear();

  bool HasData() const { return !m_data.empty(); }
  const std::string& GetData() const { return m_data; }
  void SetData(std::string data);

  const std::string& GetTitle() const { return m_title; }
  void SetTitle(std::string title) { m_title = std::move(title); }

  const std::string& GetId() const { return m_id; }
  void SetId(std::string id) { m_id = std::move(id); }

  double GetRelevance() const { return m_relevance; }
  void SetRelevance(double relevance) { m_relevance = relevance; }

  bool HasUrls() const { return !m_urls.empty(); }
  const std::vector<SUrlEntry>& GetUrls() const { return m_urls; }
  void SetUrls(std::vector<SUrlEntry> urls) { m_urls = std::move(urls); }
  void AppendUrl(SUrlEntry url) { m_urls.push_back(std::move(url)); }

  const SUrlEntry GetFirstUrlByType(const std::string& type = "") const;
  const SUrlEntry GetSeasonUrl(int season, const std::string& type = "") const;
  unsigned int GetMaxSeasonUrl() const;

  std::string GetFirstThumbUrl() const;

  /*! \brief fetch the full URLs (including referrer) of thumbs
   \param thumbs [out] vector of thumb URLs to fill
   \param type the type of thumb URLs to fetch, if empty (the default) picks any
   \param season number of season that we want thumbs for, -1 indicates no season (the default)
   \param unique avoid adding duplicate URLs when adding to a thumbs vector with existing items
   */
  void GetThumbUrls(std::vector<std::string>& thumbs,
                    const std::string& type = "",
                    int season = -1,
                    bool unique = false) const;

  bool Parse();
  bool ParseFromData(const std::string& data); // copies by intention
  bool ParseAndAppendUrl(const TiXmlElement* element);
  bool ParseAndAppendUrlsFromEpisodeGuide(const std::string& episodeGuide); // copies by intention
  void AddParsedUrl(const std::string& url,
                    const std::string& aspect = "",
                    const std::string& preview = "",
                    const std::string& referrer = "",
                    const std::string& cache = "",
                    bool post = false,
                    bool isgz = false,
                    int season = -1);

  /*! \brief fetch the full URL (including referrer) of a thumb
   \param URL entry to use to create the full URL
   \return the full URL, including referrer
   */
  static std::string GetThumbUrl(const CScraperUrl::SUrlEntry& entry);

  static bool Get(const SUrlEntry& scrURL,
                  std::string& strHTML,
                  XFILE::CCurlFile& http,
                  const std::string& cacheContext);

  // ATTENTION: this member MUST NOT be used directly except from databases
  std::string m_data;

private:
  std::string m_title;
  std::string m_id;
  double m_relevance;
  std::vector<SUrlEntry> m_urls;
  bool m_parsed;
};
