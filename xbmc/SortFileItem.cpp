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

#include "SortFileItem.h"
#include "video/VideoInfoTag.h"
#include "pvr/PVRChannel.h"
#include "pvr/epg/PVREpg.h"
#include "pvr/epg/PVREpgInfoTag.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "music/tags/MusicInfoTag.h"
#include "FileItem.h"
#include "URL.h"
#include "utils/log.h"

#define RETURN_IF_NULL(x,y) if ((x) == NULL) { CLog::Log(LOGWARNING, "%s, sort item is null", __FUNCTION__); return y; }

CStdString SSortFileItem::RemoveArticles(const CStdString &label)
{
  for (unsigned int i=0;i<g_advancedSettings.m_vecTokens.size();++i)
  {
    if (g_advancedSettings.m_vecTokens[i].size() < label.size() &&
        strnicmp(g_advancedSettings.m_vecTokens[i].c_str(), label.c_str(), g_advancedSettings.m_vecTokens[i].size()) == 0)
      return label.Mid(g_advancedSettings.m_vecTokens[i].size());
  }
  return label;
}

bool SSortFileItem::Ascending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore items that should sort on top or bottom
  if (left->SortsOnTop() != right->SortsOnTop())
    return left->SortsOnTop();
  if (left->SortsOnBottom() != right->SortsOnBottom())
    return !left->SortsOnBottom();
  if (left->SortsOnTop() || left->SortsOnBottom())
    return false; // both have either sort on top or sort on bottom -> leave as-is
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) < 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::Descending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore items that should sort on top or bottom
  if (left->SortsOnTop() != right->SortsOnTop())
    return left->SortsOnTop();
  if (left->SortsOnBottom() != right->SortsOnBottom())
    return !left->SortsOnBottom();
  if (left->SortsOnTop() || left->SortsOnBottom())
    return false; // both have either sort on top or sort on bottom -> leave as-is
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) > 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::IgnoreFoldersAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore items that should sort on top or bottom
  if (left->SortsOnTop() != right->SortsOnTop())
    return left->SortsOnTop();
  if (left->SortsOnBottom() != right->SortsOnBottom())
    return !left->SortsOnBottom();
  if (left->SortsOnTop() || left->SortsOnBottom())
    return false; // both have either sort on top or sort on bottom -> leave as-is
  return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) < 0;
}

bool SSortFileItem::IgnoreFoldersDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore items that should sort on top or bottom
  if (left->SortsOnTop() != right->SortsOnTop())
    return left->SortsOnTop();
  if (left->SortsOnBottom() != right->SortsOnBottom())
    return !left->SortsOnBottom();
  if (left->SortsOnTop() || left->SortsOnBottom())
    return false; // both have either sort on top or sort on bottom -> leave as-is
  return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) > 0;
}

void SSortFileItem::ByLabel(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetLabel());
}

void SSortFileItem::ByLabelNoThe(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(RemoveArticles(item->GetLabel()));
}

void SSortFileItem::ByFile(CFileItemPtr &item)
{
  if (!item) return;

  CURL url(item->m_strPath);
  CStdString label;
  label.Format("%s %d", url.GetFileNameWithoutPath().c_str(), item->m_lStartOffset);
  item->SetSortLabel(label);
}

void SSortFileItem::ByFullPath(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%s %d", item->m_strPath, item->m_lStartOffset);
  item->SetSortLabel(label);
}

void SSortFileItem::ByLastPlayed(CFileItemPtr &item)
{
  if (!item) return;

  if (item->HasVideoInfoTag())
    item->SetSortLabel(item->GetVideoInfoTag()->m_lastPlayed);
  else if (item->HasMusicInfoTag()) // TODO: No last played info in the fileitem for music
    item->SetSortLabel(item->GetMusicInfoTag()->GetTitle());
  else
    item->SetSortLabel(item->GetLabel());
}

void SSortFileItem::ByDate(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->IsEPG())
    label.Format("%s %s", item->GetEPGInfoTag()->Start().GetAsDBDateTime().c_str(), item->GetLabel().c_str());
  else if (item->IsPVRTimer())
    label.Format("%s %s", item->GetPVRTimerInfoTag()->Start().GetAsDBDateTime().c_str(), item->GetLabel().c_str());
  else
    label.Format("%s %s", item->m_dateTime.GetAsDBDateTime().c_str(), item->GetLabel().c_str());

  item->SetSortLabel(label);
}

void SSortFileItem::ByDateAdded(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%d", item->GetVideoInfoTag()->m_iFileId);
  item->SetSortLabel(label);
}

void SSortFileItem::BySize(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%"PRId64, item->m_dwSize);
  item->SetSortLabel(label);
}

void SSortFileItem::ByDriveType(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%d %s", item->m_iDriveType, item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::BySongTitle(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetMusicInfoTag()->GetTitle());
}

void SSortFileItem::BySongTitleNoThe(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(RemoveArticles(item->GetMusicInfoTag()->GetTitle()));
}

void SSortFileItem::BySongAlbum(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->HasMusicInfoTag())
    label = item->GetMusicInfoTag()->GetAlbum();
  else if (item->HasVideoInfoTag())
    label = item->GetVideoInfoTag()->m_strAlbum;

  CStdString artist;
  if (item->HasMusicInfoTag())
    artist = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    artist = item->GetVideoInfoTag()->m_strArtist;
  label += " " + artist;

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::BySongAlbumNoThe(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  if (item->HasMusicInfoTag())
    label = item->GetMusicInfoTag()->GetAlbum();
  else if (item->HasVideoInfoTag())
    label = item->GetVideoInfoTag()->m_strAlbum;
  label = RemoveArticles(label);

  CStdString artist;
  if (item->HasMusicInfoTag())
    artist = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    artist = item->GetVideoInfoTag()->m_strArtist;
  label += " " + RemoveArticles(artist);

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::BySongArtist(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->HasMusicInfoTag())
    label = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    label = item->GetVideoInfoTag()->m_strArtist;

  if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
  {
    int year = 0;
    if (item->HasMusicInfoTag())
      year = item->GetMusicInfoTag()->GetYear();
    else if (item->HasVideoInfoTag())
      year = item->GetVideoInfoTag()->m_iYear;
    label.AppendFormat(" %i", year);
  }

  CStdString album;
  if (item->HasMusicInfoTag())
    album = item->GetMusicInfoTag()->GetAlbum();
  else if (item->HasVideoInfoTag())
    album = item->GetVideoInfoTag()->m_strAlbum;
  label += " " + album;

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::BySongArtistNoThe(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->HasMusicInfoTag())
    label = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    label = item->GetVideoInfoTag()->m_strArtist;
  label = RemoveArticles(label);

  if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
  {
    int year = 0;
    if (item->HasMusicInfoTag())
      year = item->GetMusicInfoTag()->GetYear();
    else if (item->HasVideoInfoTag())
      year = item->GetVideoInfoTag()->m_iYear;
    label.AppendFormat(" %i", year);
  }

  CStdString album;
  if (item->HasMusicInfoTag())
    album = item->GetMusicInfoTag()->GetAlbum();
  else if (item->HasVideoInfoTag())
    album = item->GetVideoInfoTag()->m_strAlbum;
  label += " " + RemoveArticles(album);

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::BySongTrackNum(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  if (item->HasMusicInfoTag())
    label.Format("%i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());
  if (item->HasVideoInfoTag())
    label.Format("%i", item->GetVideoInfoTag()->m_iTrack);
  item->SetSortLabel(label);
}

void SSortFileItem::BySongDuration(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%i", item->GetMusicInfoTag()->GetDuration());
  item->SetSortLabel(label);
}

void SSortFileItem::BySongRating(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%c %s", item->GetMusicInfoTag()->GetRating(), item->GetMusicInfoTag()->GetTitle().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByProgramCount(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%i", item->m_iprogramCount);
  item->SetSortLabel(label);
}

void SSortFileItem::ByGenre(CFileItemPtr &item)
{
  if (!item) return;

  if (item->HasMusicInfoTag())
    item->SetSortLabel(item->GetMusicInfoTag()->GetGenre());
  else
    item->SetSortLabel(item->GetVideoInfoTag()->m_strGenre);
}

void SSortFileItem::ByCountry(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strCountry);
}


void SSortFileItem::ByYear(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->HasMusicInfoTag())
    label.Format("%i %s", item->GetMusicInfoTag()->GetYear(), item->GetLabel().c_str());
  else
    label.Format("%s %s %i %s", item->GetVideoInfoTag()->m_strPremiered.c_str(), item->GetVideoInfoTag()->m_strFirstAired, item->GetVideoInfoTag()->m_iYear, item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByMovieTitle(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strTitle);
}

void SSortFileItem::ByMovieSortTitle(CFileItemPtr &item)
{
  if (!item) return;
  if (!item->GetVideoInfoTag()->m_strSortTitle.IsEmpty())
    item->SetSortLabel(item->GetVideoInfoTag()->m_strSortTitle);
  else
    item->SetSortLabel(item->GetVideoInfoTag()->m_strTitle);
}

void SSortFileItem::ByMovieSortTitleNoThe(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  if (!item->GetVideoInfoTag()->m_strSortTitle.IsEmpty())
    label = item->GetVideoInfoTag()->m_strSortTitle;
  else
    label = item->GetVideoInfoTag()->m_strTitle;
  item->SetSortLabel(RemoveArticles(label));
}

void SSortFileItem::ByMovieRating(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%f %s", item->GetVideoInfoTag()->m_fRating, item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByMovieRuntime(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  if (item->GetVideoInfoTag()->m_streamDetails.GetVideoDuration() > 0)
    label.Format("%i %s", item->GetVideoInfoTag()->m_streamDetails.GetVideoDuration(), item->GetLabel().c_str());
  else
    label.Format("%s %s", item->GetVideoInfoTag()->m_strRuntime, item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByMPAARating(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strMPAARating + " " + item->GetLabel());
}

void SSortFileItem::ByStudio(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strStudio);
}

void SSortFileItem::ByStudioNoThe(CFileItemPtr &item)
{
  if (!item) return;
  CStdString studio = item->GetVideoInfoTag()->m_strStudio;
  item->SetSortLabel(RemoveArticles(studio));
}

void SSortFileItem::ByEpisodeNum(CFileItemPtr &item)
{
  if (!item) return;

  const CVideoInfoTag *tag = item->GetVideoInfoTag();

  // we calculate an offset number based on the episode's
  // sort season and episode values. in addition
  // we include specials 'episode' numbers to get proper
  // sorting of multiple specials in a row. each
  // of these are given their particular ranges to semi-ensure uniqueness.
  // theoretical problem: if a show has > 2^15 specials and two of these are placed
  // after each other they will sort backwards. if a show has > 2^32-1 seasons
  // or if a season has > 2^16-1 episodes strange things will happen (overflow)
  uint64_t num;
  if (tag->m_iSpecialSortEpisode > 0)
    num = ((uint64_t)tag->m_iSpecialSortSeason<<32)+(tag->m_iSpecialSortEpisode<<16)-((2<<15) - tag->m_iEpisode);
  else
    num = ((uint64_t)tag->m_iSeason<<32)+(tag->m_iEpisode<<16);

  // check filename as there can be duplicates now
  CURL file(tag->m_strFileNameAndPath);
  CStdString label;
  label.Format("%"PRIu64" %s", num, file.GetFileName().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByProductionCode(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strProductionCode);
}

void SSortFileItem::ByChannel(CFileItemPtr &item)
{
  if (!item) return;

  if (item->IsEPG() || item->IsPVRChannel())
  {
    CPVRChannel *channel = item->GetPVRChannelInfoTag();
    if (channel)
      item->SetSortLabel(channel->ChannelName());
  }
}

void SSortFileItem::ByBitrate(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%"PRId64, item->m_dwSize);
  item->SetSortLabel(label);
}

void SSortFileItem::ByListeners(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%i", item->GetMusicInfoTag()->GetListeners());
  item->SetSortLabel(label);
}
