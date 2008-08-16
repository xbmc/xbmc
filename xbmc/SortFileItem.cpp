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

#include "stdafx.h"
#include "Settings.h"
#include "SortFileItem.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "FileItem.h"
#include "URL.h"
#include "utils/log.h"

#define RETURN_IF_NULL(x,y) if ((x) == NULL) { CLog::Log(LOGWARNING, "%s, sort item is null", __FUNCTION__); return y; }

inline int StartsWithToken(const CStdString& strLabel)
{
  for (unsigned int i=0;i<g_advancedSettings.m_vecTokens.size();++i)
  {
    if (g_advancedSettings.m_vecTokens[i].size() < strLabel.size() &&
        strnicmp(g_advancedSettings.m_vecTokens[i].c_str(), strLabel.c_str(), g_advancedSettings.m_vecTokens[i].size()) == 0)
      return g_advancedSettings.m_vecTokens[i].size();
  }
  return 0;
}

bool SSortFileItem::Ascending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) < 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::Descending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) > 0;
  return left->m_bIsFolder;
}

void SSortFileItem::ByLabel(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetLabel());
}

void SSortFileItem::ByLabelNoThe(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetLabel().Mid(StartsWithToken(item->GetLabel())));
}

void SSortFileItem::ByFile(CFileItemPtr &item)
{
  if (!item) return;

  CURL url(item->m_strPath);
  CStdString label;
  label.Format("%s %d", url.GetFileNameWithoutPath().c_str(), item->m_lStartOffset);
  item->SetSortLabel(label);
}

void SSortFileItem::ByDate(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%s %s", item->m_dateTime.GetAsDBDateTime().c_str(), item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::BySize(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%lu", item->m_dwSize);
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
  int start = StartsWithToken(item->GetMusicInfoTag()->GetTitle());
  item->SetSortLabel(item->GetMusicInfoTag()->GetTitle().Mid(start));
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
  label = label.Mid(StartsWithToken(label));

  CStdString artist;
  if (item->HasMusicInfoTag())
    artist = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    artist = item->GetVideoInfoTag()->m_strArtist;
  artist = artist.Mid(StartsWithToken(artist));
  label += " " + artist;

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
  label = label.Mid(StartsWithToken(label));

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
  album = album.Mid(StartsWithToken(album));
  label += " " + album;

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::BySongTrackNum(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());
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
  item->SetSortLabel(item->GetVideoInfoTag()->m_strRuntime);
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
  item->SetSortLabel(studio.Mid(StartsWithToken(studio)));
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
  // theoretical problem: if a show has > 128 specials and two of these are placed
  // after each other they will sort backwards. if a show has > 2^8-1 seasons
  // or if a season has > 2^16-1 episodes strange things will happen (overflow)
  unsigned int num;
  if (tag->m_iSpecialSortEpisode > 0)
    num = (tag->m_iSpecialSortSeason<<24)+(tag->m_iSpecialSortEpisode<<8)-(128-tag->m_iEpisode);
  else
    num = (tag->m_iSeason<<24)+(tag->m_iEpisode<<8);

  // check filename as there can be duplicates now
  CURL file(tag->m_strFileNameAndPath);
  CStdString label;
  label.Format("%u %s", num, file.GetFileName().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByProductionCode(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strProductionCode);
}

