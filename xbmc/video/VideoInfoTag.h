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
  std::string m_strVotes;
  std::vector<std::string> m_artist;
  std::vector< SActorInfo > m_cast;
  typedef std::vector< SActorInfo >::const_iterator iCast;
  std::string m_strSet;
  int m_iSetId;
  std::vector<std::string> m_tags;
  std::string m_strFile;
  std::string m_strPath;
  std::string m_strIMDBNumber;
  std::string m_strMPAARating;
  std::string m_strFileNameAndPath;
  std::string m_strOriginalTitle;
  std::string m_strEpisodeGuide;
  CDateTime m_premiered;
  std::string m_strStatus;
  std::string m_strProductionCode;
  CDateTime m_firstAired;
  std::string m_strShowTitle;
  std::vector<std::string> m_studio;
  std::string m_strAlbum;
  CDateTime m_lastPlayed;
  std::vector<std::string> m_showLink;
  int m_playCount;
  int m_iTop250;
  int m_iYear;
  int m_iSeason;
  int m_iEpisode;
  std::string m_strUniqueId;
  int m_iDbId;
  int m_iFileId;
  int m_iSpecialSortSeason;
  int m_iSpecialSortEpisode;
  int m_iTrack;
  float m_fRating;
  float m_fEpBookmark;
  int m_iBookmarkId;
  int m_iIdShow;
  int m_iIdSeason;
  CFanart m_fanart;
  CStreamDetails m_streamDetails;
  CBookmark m_resumePoint;
  CDateTime m_dateAdded;
  MediaType m_type;
  int m_duration; ///< duration in seconds

private:
  /* \brief Parse our native XML format for video info.
   See Load for a description of the available tag types.

   \param element    the root XML element to parse.
   \param prioritise whether additive tags should be replaced (or prepended) by the content of the tags, or appended to.
   \sa Load
   */
  void ParseNative(const TiXmlElement* element, bool prioritise);
};

typedef std::vector<CVideoInfoTag> VECMOVIES;
