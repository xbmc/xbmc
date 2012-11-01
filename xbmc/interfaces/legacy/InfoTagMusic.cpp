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

#include "InfoTagMusic.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    InfoTagMusic::InfoTagMusic() : AddonClass("InfoTagMusic")
    {
      infoTag = new MUSIC_INFO::CMusicInfoTag();
    }

    InfoTagMusic::InfoTagMusic(const MUSIC_INFO::CMusicInfoTag& tag) : AddonClass("InfoTagMusic")
    {
      infoTag = new MUSIC_INFO::CMusicInfoTag();
      *infoTag = tag;
    }

    InfoTagMusic::~InfoTagMusic()
    {
      delete infoTag;
    }

    String InfoTagMusic::getURL()
    {
      return infoTag->GetURL();
    }

    String InfoTagMusic::getTitle()
    {
      return infoTag->GetTitle();
    }

    String InfoTagMusic::getArtist()
    {
      return StringUtils::Join(infoTag->GetArtist(), g_advancedSettings.m_musicItemSeparator);
    }

    String InfoTagMusic::getAlbumArtist()
    {
      return StringUtils::Join(infoTag->GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator);
    }

    String InfoTagMusic::getAlbum()
    {
      return infoTag->GetAlbum();
    }

    String InfoTagMusic::getGenre()
    {
      return StringUtils::Join(infoTag->GetGenre(), g_advancedSettings.m_musicItemSeparator);
    }

    int InfoTagMusic::getDuration()
    {
      return infoTag->GetDuration();
    }

    int InfoTagMusic::getTrack()
    {
      return infoTag->GetTrackNumber();
    }

    int InfoTagMusic::getDisc()
    {
      return infoTag->GetDiscNumber();
    }

    String InfoTagMusic::getReleaseDate()
    {
      return infoTag->GetYearString();
    }

    int InfoTagMusic::getListeners()
    {
      return infoTag->GetListeners();
    }

    int InfoTagMusic::getPlayCount()
    {
      return infoTag->GetPlayCount();
    }

    String InfoTagMusic::getLastPlayed()
    {
      return infoTag->GetLastPlayed().GetAsLocalizedDate();
    }

    String InfoTagMusic::getComment()
    {
      return infoTag->GetComment();
    }

    String InfoTagMusic::getLyrics()
    {
      return infoTag->GetLyrics();
    }
  }
}

