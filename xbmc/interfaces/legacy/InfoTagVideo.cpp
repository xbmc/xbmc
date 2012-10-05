/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "InfoTagVideo.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    InfoTagVideo::InfoTagVideo() : AddonClass("InfoTagVideo")
    {
      infoTag = new CVideoInfoTag();
    }

    InfoTagVideo::InfoTagVideo(const CVideoInfoTag& tag) : AddonClass("InfoTagVideo")
    {
      infoTag = new CVideoInfoTag();
      *infoTag = tag;
    }

    InfoTagVideo::~InfoTagVideo()
    {
      delete infoTag;
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
      return StringUtils::Join(infoTag->m_genre, g_advancedSettings.m_videoItemSeparator).c_str();
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

    String InfoTagVideo::getTitle()
    {
      return infoTag->m_strTitle;
    }

    String InfoTagVideo::getVotes()
    {
      return infoTag->m_strVotes;
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
      return infoTag->m_strIMDBNumber;
    }

    int InfoTagVideo::getYear()
    {
      return infoTag->m_iYear;
    }

    double InfoTagVideo::getRating()
    {
      return infoTag->m_fRating;
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
      return infoTag->m_premiered.GetAsLocalizedDate();
    }

    String InfoTagVideo::getFirstAired()
    {
      return infoTag->m_firstAired.GetAsLocalizedDate();
    }
  }
}
