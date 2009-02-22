#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "../guilib/system.h"
#include "utils/Archive.h"
#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"

#include <vector>

struct SActorInfo
{
  CStdString strName;
  CStdString strRole;
  CScraperUrl thumbUrl;
};

class CVideoInfoTag : public ISerializable
{
public:
  CVideoInfoTag() { Reset(); };
  void Reset();
  bool Load(const TiXmlElement *movie, bool chained = false);
  bool Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo = true);
  virtual void Serialize(CArchive& ar);
  const CStdString GetCast(bool bIncludeRole = false) const;
  bool HasStreamDetails() const;

  CStdString m_strDirector;
  CStdString m_strWritingCredits;
  CStdString m_strGenre;
  CStdString m_strTagLine;
  CStdString m_strPlotOutline;
  CStdString m_strTrailer;
  CStdString m_strPlot;
  CScraperUrl m_strPictureURL;
  CStdString m_strTitle;
  CStdString m_strVotes;
  CStdString m_strArtist;
  std::vector< SActorInfo > m_cast;
  typedef std::vector< SActorInfo >::const_iterator iCast;

  CStdString m_strRuntime;
  CStdString m_strFile;
  CStdString m_strPath;
  CStdString m_strIMDBNumber;
  CStdString m_strMPAARating;
  CStdString m_strFileNameAndPath;
  CStdString m_strOriginalTitle;
  CStdString m_strEpisodeGuide;
  CStdString m_strPremiered;
  CStdString m_strStatus;
  CStdString m_strProductionCode;
  CStdString m_strFirstAired;
  CStdString m_strShowTitle;
  CStdString m_strStudio;
  CStdString m_strAlbum;
  int m_playCount;
  int m_iTop250;
  int m_iYear;
  int m_iSeason;
  int m_iEpisode;
  int m_iDbId;
  int m_iSpecialSortSeason;
  int m_iSpecialSortEpisode;
  int m_iTrack;
  float m_fRating;
  int m_iBookmarkId;
  CFanart m_fanart;

  // StreamDetails
  CStdString m_strVideoCodec;
  CStdString m_strAudioCodec;
  CStdString m_strAudioLanguage;
  CStdString m_strSubtitleLanguage;
  CStdString m_strVideoAspect;
  int m_iVideoWidth;
  int m_iVideoHeight;
  int m_iAudioChannels;
private:
  void ParseNative(const TiXmlElement* movie);
  void ParseMyMovies(const TiXmlElement* movie);
};

typedef std::vector<CVideoInfoTag> VECMOVIES;
