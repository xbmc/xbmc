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

#include "InfoTagVideo.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    InfoTagVideo::InfoTagVideo()
    {
      infoTag = new CVideoInfoTag();
    }

    InfoTagVideo::InfoTagVideo(const CVideoInfoTag& tag)
    {
      infoTag = new CVideoInfoTag();
      *infoTag = tag;
    }

    InfoTagVideo::~InfoTagVideo()
    {
      delete infoTag;
    }

    int InfoTagVideo::getDbId()
    {
      return infoTag->m_iDbId;
    }

    String InfoTagVideo::getDirector()
    {
      return StringUtils::Join(infoTag->m_director, g_advancedSettings.m_videoItemSeparator);
    }

    String InfoTagVideo::getWritingCredits()
    {
      return StringUtils::Join(infoTag->m_writingCredits, g_advancedSettings.m_videoItemSeparator);
    }

    String InfoTagVideo::getGenre()
    {
      return StringUtils::Join(infoTag->m_genre, g_advancedSettings.m_videoItemSeparator);
    }

    String InfoTagVideo::getTagLine()
    {
      return infoTag->m_strTagLine;
    }

    String InfoTagVideo::getPlotOutline()
    {
      return infoTag->m_strPlotOutline;
    }

    String InfoTagVideo::getPlot()
    {
      return infoTag->m_strPlot;
    }

    String InfoTagVideo::getPictureURL()
    {
      return infoTag->m_strPictureURL.GetFirstThumb().m_url;
    }

    String InfoTagVideo::getTVShowTitle()
    {
      return infoTag->m_strShowTitle;
    }

    String InfoTagVideo::getTitle()
    {
      return infoTag->m_strTitle;
    }

    String InfoTagVideo::getMediaType()
    {
      return infoTag->m_type;
    }

    String InfoTagVideo::getVotes()
    {
      return StringUtils::Format("%i", infoTag->GetRating().votes);
    }

    String InfoTagVideo::getCast()
    {
      return infoTag->GetCast(true);
    }

    String InfoTagVideo::getFile()
    {
      return infoTag->m_strFile;
    }

    String InfoTagVideo::getPath()
    {
      return infoTag->m_strPath;
    }

    String InfoTagVideo::getIMDBNumber()
    {
      return infoTag->GetUniqueID();
    }

    int InfoTagVideo::getSeason()
    {
      return infoTag->m_iSeason;
    }

    int InfoTagVideo::getEpisode()
    {
      return infoTag->m_iEpisode;
    }

    int InfoTagVideo::getYear()
    {
      return infoTag->GetYear();
    }

    double InfoTagVideo::getRating()
    {
      return infoTag->GetRating().rating;
    }

    int InfoTagVideo::getUserRating()
    {
      return infoTag->m_iUserRating;
    }

    int InfoTagVideo::getPlayCount()
    {
      return infoTag->m_playCount;
    }

    String InfoTagVideo::getLastPlayed()
    {
      return infoTag->m_lastPlayed.GetAsLocalizedDateTime();
    }

    String InfoTagVideo::getOriginalTitle()
    {
      return infoTag->m_strOriginalTitle;
    }

    String InfoTagVideo::getPremiered()
    {
      return infoTag->GetPremiered().GetAsLocalizedDate();
    }

    String InfoTagVideo::getFirstAired()
    {
      return infoTag->m_firstAired.GetAsLocalizedDate();
    }
  }
}
