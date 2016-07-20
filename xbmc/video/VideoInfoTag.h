#pragma once
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

#include <string>
#include <vector>
#include "XBDateTime.h"
#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"
#include "utils/ISortable.h"
#include "utils/StreamDetails.h"
#include "video/Bookmark.h"

class CArchive;
class TiXmlNode;
class TiXmlElement;
class CVariant;

struct SActorInfo
{
  SActorInfo() : order(-1) {};
  bool operator<(const SActorInfo &right) const
  {
    return order < right.order;
  }
  std::string strName;
  std::string strRole;
  CScraperUrl thumbUrl;
  std::string thumb;
  int        order;
};

class CRating
{
public:
  CRating(): rating(0.0f), votes(0) {}
  CRating(float r): rating(r), votes(0) {}
  CRating(float r, int v): rating(r), votes(v) {}
  float rating;
  int votes;
};
typedef std::map<std::string, CRating> RatingMap;

class CVideoInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CVideoInfoTag() { Reset(); };
  void Reset();
  /* \brief Load information to a videoinfotag from an XML element
   There are three types of tags supported:
    1. Single-value tags, such as <title>.  These are set if available, else are left untouched.
    2. Additive tags, such as <set> or <genre>.  These are appended to or replaced (if available) based on the value
       of the prioritise parameter.  In addition, a clear attribute is available in the XML to clear the current value prior
       to appending.
    3. Image tags such as <thumb> and <fanart>.  If the prioritise value is specified, any additional values are prepended
       to the existing values.

   \param element    the root XML element to parse.
   \param append     whether information should be added to the existing tag, or whether it should be reset first.
   \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.

   \sa ParseNative
   */
  bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
  bool Save(TiXmlNode *node, const std::string &tag, bool savePathInfo = true, const TiXmlElement *additionalNode = NULL);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual void ToSortable(SortItem& sortable, Field field) const;
  const CRating GetRating(std::string type = "") const;
  const std::string& GetDefaultRating() const;
  const std::string GetUniqueID(std::string type = "") const;
  const std::map<std::string, std::string>& GetUniqueIDs() const;
  const std::string& GetDefaultUniqueID() const;
  const bool HasUniqueID() const;
  const bool HasYear() const;
  const int GetYear() const;
  const bool HasPremiered() const;
  const CDateTime& GetPremiered() const;
  const std::string GetCast(bool bIncludeRole = false) const;
  bool HasStreamDetails() const;
  bool IsEmpty() const;

  const std::string& GetPath() const
  {
    if (m_strFileNameAndPath.empty())
      return m_strPath;
    return m_strFileNameAndPath;
  };

  /*! \brief retrieve the duration in seconds.
   Prefers the duration from stream details if available.
   */
  unsigned int GetDuration() const;

  /*! \brief get the duration in seconds from a minute string
   \param runtime the runtime string from a scraper or similar
   \return the time in seconds, if decipherable.
   */
  static unsigned int GetDurationFromMinuteString(const std::string &runtime);

  void SetBasePath(std::string basePath);
  void SetDirector(std::vector<std::string> director);
  void SetWritingCredits(std::vector<std::string> writingCredits);
  void SetGenre(std::vector<std::string> genre);
  void SetCountry(std::vector<std::string> country);
  void SetTagLine(std::string tagLine);
  void SetPlotOutline(std::string plotOutline);
  void SetTrailer(std::string trailer);
  void SetPlot(std::string plot);
  void SetTitle(std::string title);
  void SetSortTitle(std::string sortTitle);
  void SetPictureURL(CScraperUrl &pictureURL);
  void SetRating(float rating, int votes, const std::string& type = "", bool def = false);
  void SetRating(CRating rating, const std::string& type = "", bool def = false);
  void SetRating(float rating, const std::string& type = "", bool def = false);
  void RemoveRating(const std::string& type);
  void SetRatings(RatingMap ratings);
  void SetVotes(int votes, const std::string& type = "");
  void SetUniqueIDs(std::map<std::string, std::string> uniqueIDs);
  void SetPremiered(CDateTime premiered);
  void SetPremieredFromDBDate(std::string premieredString);
  void SetYear(int year);
  void SetArtist(std::vector<std::string> artist);
  void SetSet(std::string set);
  void SetSetOverview(std::string setOverview);
  void SetTags(std::vector<std::string> tags);
  void SetFile(std::string file);
  void SetPath(std::string path);
  void SetMPAARating(std::string mpaaRating);
  void SetFileNameAndPath(std::string fileNameAndPath);
  void SetOriginalTitle(std::string originalTitle);
  void SetEpisodeGuide(std::string episodeGuide);
  void SetStatus(std::string status);
  void SetProductionCode(std::string productionCode);
  void SetShowTitle(std::string showTitle);
  void SetStudio(std::vector<std::string> studio);
  void SetAlbum(std::string album);
  void SetShowLink(std::vector<std::string> showLink);
  void SetUniqueID(const std::string& uniqueid, const std::string& type = "", bool def = false);
  void RemoveUniqueID(const std::string& type);
  void SetNamedSeasons(std::map<int, std::string> namedSeasons);
  void SetUserrating(int userrating);

  std::string m_basePath; // the base path of the video, for folder-based lookups
  int m_parentPathID;      // the parent path id where the base path of the video lies
  std::vector<std::string> m_director;
  std::vector<std::string> m_writingCredits;
  std::vector<std::string> m_genre;
  std::vector<std::string> m_country;
  std::string m_strTagLine;
  std::string m_strPlotOutline;
  std::string m_strTrailer;
  std::string m_strPlot;
  CScraperUrl m_strPictureURL;
  std::string m_strTitle;
  std::string m_strSortTitle;
  std::vector<std::string> m_artist;
  std::vector< SActorInfo > m_cast;
  typedef std::vector< SActorInfo >::const_iterator iCast;
  std::string m_strSet;
  int m_iSetId;
  std::string m_strSetOverview;
  std::vector<std::string> m_tags;
  std::string m_strFile;
  std::string m_strPath;
  std::string m_strMPAARating;
  std::string m_strFileNameAndPath;
  std::string m_strOriginalTitle;
  std::string m_strEpisodeGuide;
  CDateTime m_premiered;
  bool m_bHasPremiered;
  std::string m_strStatus;
  std::string m_strProductionCode;
  CDateTime m_firstAired;
  std::string m_strShowTitle;
  std::vector<std::string> m_studio;
  std::string m_strAlbum;
  CDateTime m_lastPlayed;
  std::vector<std::string> m_showLink;
  std::map<int, std::string> m_namedSeasons;
  int m_playCount;
  int m_iTop250;
  int m_iSeason;
  int m_iEpisode;
  int m_iIdUniqueID;
  int m_iDbId;
  int m_iFileId;
  int m_iSpecialSortSeason;
  int m_iSpecialSortEpisode;
  int m_iTrack;
  RatingMap m_ratings;
  int m_iIdRating;
  int m_iUserRating;
  CBookmark m_EpBookmark;
  int m_iBookmarkId;
  int m_iIdShow;
  int m_iIdSeason;
  CFanart m_fanart;
  CStreamDetails m_streamDetails;
  CBookmark m_resumePoint;
  CDateTime m_dateAdded;
  MediaType m_type;
  int m_duration; ///< duration in seconds
  int m_relevance; // Used for actors' number of appearances
  int m_parsedDetails;

private:
  /* \brief Parse our native XML format for video info.
   See Load for a description of the available tag types.

   \param element    the root XML element to parse.
   \param prioritise whether additive tags should be replaced (or prepended) by the content of the tags, or appended to.
   \sa Load
   */
  void ParseNative(const TiXmlElement* element, bool prioritise);

  std::string m_strDefaultRating;
  std::string m_strDefaultUniqueID;
  std::map<std::string, std::string> m_uniqueIDs;
  std::string Trim(std::string &&value);
  std::vector<std::string> Trim(std::vector<std::string> &&items);
};

typedef std::vector<CVideoInfoTag> VECMOVIES;
