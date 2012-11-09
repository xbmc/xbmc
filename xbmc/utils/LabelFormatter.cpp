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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "LabelFormatter.h"
#include "settings/GUISettings.h"
#include "settings/AdvancedSettings.h"
#include "RegExp.h"
#include "Util.h"
#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "FileItem.h"
#include "StringUtils.h"
#include "URIUtils.h"
#include "guilib/LocalizeStrings.h"

using namespace MUSIC_INFO;

/* LabelFormatter
 * ==============
 *
 * The purpose of this class is to parse a mask string of the form
 *
 *  [%N. ][%T] - [%A][ (%Y)]
 *
 * and provide methods to format up a CFileItem's label(s).
 *
 * The %N/%A/%B masks are replaced with the corresponding metadata (if available).
 *
 * Square brackets are treated as a metadata block.  Anything inside the block other
 * than the metadata mask is treated as either a prefix or postfix to the metadata. This
 * information is only included in the formatted string when the metadata is non-empty.
 *
 * Any metadata tags not enclosed with square brackets are treated as if it were immediately
 * enclosed - i.e. with no prefix or postfix.
 *
 * The special characters %, [, and ] can be produced using %%, %[, and %] respectively.
 *
 * Any static text outside of the metadata blocks is only shown if the blocks on either side
 * (or just one side in the case of an end) are both non-empty.
 *
 * Examples (using the above expression):
 *
 *   Track  Title  Artist  Year     Resulting Label
 *   -----  -----  ------  ----     ---------------
 *     10    "40"    U2    1983     10. "40" - U2 (1983)
 *           "40"    U2    1983     "40" - U2 (1983)
 *     10            U2    1983     10. U2 (1983)
 *     10    "40"          1983     "40" (1983)
 *     10    "40"    U2             10. "40" - U2
 *     10    "40"                   10. "40"
 *
 * Available metadata masks:
 *
 *  %A - Artist
 *  %B - Album
 *  %C - Programs count
 *  %D - Duration
 *  %E - episode number
 *  %F - FileName
 *  %G - Genre
 *  %H - season*100+episode
 *  %I - Size
 *  %J - Date
 *  %K - Movie/Game title
 *  %L - existing Label
 *  %M - number of episodes
 *  %N - Track Number
 *  %O - mpaa rating
 *  %P - production code
 *  %Q - file time
 *  %R - Movie rating
 *  %S - Disc Number
 *  %T - Title
 *  %U - studio
 *  %V - Playcount
 *  %W - Listeners
 *  %X - Bitrate
 *  %Y - Year
 *  %Z - tvshow title
 *  %a - Date Added
 */

#define MASK_CHARS "NSATBGYFLDIJRCKMEPHZOQUVXWa"

CLabelFormatter::CLabelFormatter(const CStdString &mask, const CStdString &mask2)
{
  // assemble our label masks
  AssembleMask(0, mask);
  AssembleMask(1, mask2);
  // save a bool for faster lookups
  m_hideFileExtensions = !g_guiSettings.GetBool("filelists.showextensions");
}

CStdString CLabelFormatter::GetContent(unsigned int label, const CFileItem *item) const
{
  assert(label < 2);
  assert(m_staticContent[label].size() == m_dynamicContent[label].size() + 1);

  if (!item) return "";

  CStdString strLabel, dynamicLeft, dynamicRight;
  for (unsigned int i = 0; i < m_dynamicContent[label].size(); i++)
  {
    dynamicRight = GetMaskContent(m_dynamicContent[label][i], item);
    if ((i == 0 || !dynamicLeft.IsEmpty()) && !dynamicRight.IsEmpty())
      strLabel += m_staticContent[label][i];
    strLabel += dynamicRight;
    dynamicLeft = dynamicRight;
  }
  if (!dynamicLeft.IsEmpty())
    strLabel += m_staticContent[label][m_dynamicContent[label].size()];

  return strLabel;
}

void CLabelFormatter::FormatLabel(CFileItem *item) const
{
  CStdString maskedLabel = GetContent(0, item);
  if (!maskedLabel.IsEmpty())
    item->SetLabel(maskedLabel);
  else if (!item->m_bIsFolder && m_hideFileExtensions)
    item->RemoveExtension();
}

void CLabelFormatter::FormatLabel2(CFileItem *item) const
{
  item->SetLabel2(GetContent(1, item));
}

CStdString CLabelFormatter::GetMaskContent(const CMaskString &mask, const CFileItem *item) const
{
  if (!item) return "";
  const CMusicInfoTag *music = item->GetMusicInfoTag();
  const CVideoInfoTag *movie = item->GetVideoInfoTag();
  CStdString value;
  switch (mask.m_content)
  {
  case 'N':
    if (music && music->GetTrackNumber() > 0)
      value.Format("%02.2i", music->GetTrackNumber());
    if (movie&& movie->m_iTrack > 0)
      value.Format("%02.2i", movie->m_iTrack);
    break;
  case 'S':
    if (music && music->GetDiscNumber() > 0)
      value.Format("%02.2i", music->GetDiscNumber());
    break;
  case 'A':
    if (music && music->GetArtist().size())
      value = StringUtils::Join(music->GetArtist(), g_advancedSettings.m_musicItemSeparator);
    if (movie && movie->m_artist.size())
      value = StringUtils::Join(movie->m_artist, g_advancedSettings.m_videoItemSeparator);
    break;
  case 'T':
    if (music && music->GetTitle().size())
      value = music->GetTitle();
    if (movie && movie->m_strTitle.size())
      value = movie->m_strTitle;
    break;
  case 'Z':
    if (movie && !movie->m_strShowTitle.IsEmpty())
      value = movie->m_strShowTitle;
    break;
  case 'B':
    if (music && music->GetAlbum().size())
      value = music->GetAlbum();
    else if (movie)
      value = movie->m_strAlbum;
    break;
  case 'G':
    if (music && music->GetGenre().size())
      value = StringUtils::Join(music->GetGenre(), g_advancedSettings.m_musicItemSeparator);
    if (movie && movie->m_genre.size())
      value = StringUtils::Join(movie->m_genre, g_advancedSettings.m_videoItemSeparator);
    break;
  case 'Y':
    if (music)
      value = music->GetYearString();
    if (movie)
    {
      if (movie->m_firstAired.IsValid())
        value = movie->m_firstAired.GetAsLocalizedDate();
      else if (movie->m_premiered.IsValid())
        value = movie->m_premiered.GetAsLocalizedDate();
      else if (movie->m_iYear > 0)
        value.Format("%i",movie->m_iYear);
    }
    break;
  case 'F': // filename
    value = CUtil::GetTitleFromPath(item->GetPath(), item->m_bIsFolder && !item->IsFileFolder());
    break;
  case 'L':
    value = item->GetLabel();
    // is the label the actual file or folder name?
    if (value == URIUtils::GetFileName(item->GetPath()))
    { // label is the same as filename, clean it up as appropriate
      value = CUtil::GetTitleFromPath(item->GetPath(), item->m_bIsFolder && !item->IsFileFolder());
    }
    break;
  case 'D':
    { // duration
      int nDuration=0;
      if (music)
        nDuration = music->GetDuration();
      if (movie)
      {
        if (movie->m_streamDetails.GetVideoDuration() > 0)
          nDuration = movie->m_streamDetails.GetVideoDuration();
        else if (!movie->m_strRuntime.IsEmpty())
          nDuration = StringUtils::TimeStringToSeconds(movie->m_strRuntime);
      }
      if (nDuration > 0)
        value = StringUtils::SecondsToTimeString(nDuration);
      else if (item->m_dwSize > 0)
        value = StringUtils::SizeToString(item->m_dwSize);
    }
    break;
  case 'I': // size
    if( !item->m_bIsFolder || item->m_dwSize != 0 )
      value = StringUtils::SizeToString(item->m_dwSize);
    break;
  case 'J': // date
    if (item->m_dateTime.IsValid())
      value = item->m_dateTime.GetAsLocalizedDate();
    break;
  case 'Q': // time
    if (item->m_dateTime.IsValid())
      value = item->m_dateTime.GetAsLocalizedTime("", false);
    break;
  case 'R': // rating
    if (music && music->GetRating() != '0')
      value = music->GetRating();
    else if (movie && movie->m_fRating != 0.f)
      value.Format("%.1f", movie->m_fRating);
    break;
  case 'C': // programs count
    value.Format("%i", item->m_iprogramCount);
    break;
  case 'K':
    value = item->m_strTitle;
    break;
  case 'M':
    if (movie && movie->m_iEpisode > 0)
      value.Format("%i %s", movie->m_iEpisode,g_localizeStrings.Get(movie->m_iEpisode == 1 ? 20452 : 20453));
    break;
  case 'E':
    if (movie && movie->m_iEpisode > 0)
    { // episode number
      if (movie->m_iSpecialSortEpisode > 0)
        value.Format("S%02.2i", movie->m_iEpisode);
      else
        value.Format("%02.2i", movie->m_iEpisode);
    }
    break;
  case 'P':
    if (movie) // tvshow production code
      value = movie->m_strProductionCode;
    break;
  case 'H':
    if (movie && movie->m_iEpisode > 0)
    { // season*100+episode number
      if (movie->m_iSpecialSortSeason > 0)
        value.Format("Sx%02.2i", movie->m_iEpisode);
      else
        value.Format("%ix%02.2i", movie->m_iSeason,movie->m_iEpisode);
    }
    break;
  case 'O':
    if (movie && movie->m_strMPAARating)
    {// MPAA Rating
      value = movie->m_strMPAARating;
    }
    break;
  case 'U':
    if (movie && movie->m_studio.size() > 0)
    {// Studios
      value = StringUtils::Join(movie ->m_studio, g_advancedSettings.m_videoItemSeparator);
    }
    break;
  case 'V': // Playcount
    if (music)
      value.Format("%i", music->GetPlayCount());
    if (movie)
      value.Format("%i", movie->m_playCount);
    break;
  case 'X': // Bitrate
    if( !item->m_bIsFolder && item->m_dwSize != 0 )
      value.Format("%i kbps", item->m_dwSize);
    break;
   case 'W': // Listeners
    if( !item->m_bIsFolder && music && music->GetListeners() != 0 )
     value.Format("%i %s", music->GetListeners(), g_localizeStrings.Get(music->GetListeners() == 1 ? 20454 : 20455));
    break;
  case 'a': // Date Added
    if (movie && movie->m_dateAdded.IsValid())
      value = movie->m_dateAdded.GetAsLocalizedDate();
    break;
  }
  if (!value.IsEmpty())
    return mask.m_prefix + value + mask.m_postfix;
  return "";
}

void CLabelFormatter::SplitMask(unsigned int label, const CStdString &mask)
{
  assert(label < 2);
  CRegExp reg;
  reg.RegComp("%([" MASK_CHARS "])");
  CStdString work(mask);
  int findStart = -1;
  while ((findStart = reg.RegFind(work.c_str())) >= 0)
  { // we've found a match
    m_staticContent[label].push_back(work.Left(findStart));
    m_dynamicContent[label].push_back(CMaskString("", 
          reg.GetReplaceString("\\1")[0], ""));
    work = work.Mid(findStart + reg.GetFindLen());
  }
  m_staticContent[label].push_back(work);
}

void CLabelFormatter::AssembleMask(unsigned int label, const CStdString& mask)
{
  assert(label < 2);
  m_staticContent[label].clear();
  m_dynamicContent[label].clear();

  // we want to match [<prefix>%A<postfix]
  // but allow %%, %[, %] to be in the prefix and postfix.  Anything before the first [
  // could be a mask that's not surrounded with [], so pass to SplitMask.
  CRegExp reg;
  reg.RegComp("(^|[^%])\\[(([^%]|%%|%\\]|%\\[)*)%([" MASK_CHARS "])(([^%]|%%|%\\]|%\\[)*)\\]");
  CStdString work(mask);
  int findStart = -1;
  while ((findStart = reg.RegFind(work.c_str())) >= 0)
  { // we've found a match for a pre/postfixed string
    // send anything
    SplitMask(label, work.Left(findStart) + reg.GetReplaceString("\\1").c_str());
    m_dynamicContent[label].push_back(CMaskString(
            reg.GetReplaceString("\\2"),
            reg.GetReplaceString("\\4")[0],
            reg.GetReplaceString("\\5")));
    work = work.Mid(findStart + reg.GetFindLen());
  }
  SplitMask(label, work);
  assert(m_staticContent[label].size() == m_dynamicContent[label].size() + 1);
}

bool CLabelFormatter::FillMusicTag(const CStdString &fileName, CMusicInfoTag *tag) const
{
  // run through and find static content to split the string up
  int pos1 = fileName.Find(m_staticContent[0][0], 0);
  if (pos1 == (int)CStdString::npos)
    return false;
  for (unsigned int i = 1; i < m_staticContent[0].size(); i++)
  {
    int pos2 = m_staticContent[0][i].size() ? fileName.Find(m_staticContent[0][i], pos1) : fileName.size();
    if (pos2 == (int)CStdString::npos)
      return false;
    // found static content - thus we have the dynamic content surrounded
    FillMusicMaskContent(m_dynamicContent[0][i - 1].m_content, fileName.Mid(pos1, pos2 - pos1), tag);
    pos1 = pos2 + m_staticContent[0][i].size();
  }
  return true;
}

void CLabelFormatter::FillMusicMaskContent(const char mask, const CStdString &value, CMusicInfoTag *tag) const
{
  if (!tag) return;
  switch (mask)
  {
  case 'N':
    tag->SetTrackNumber(atol(value.c_str()));
    break;
  case 'S':
    tag->SetPartOfSet(atol(value.c_str()));
    break;
  case 'A':
    tag->SetArtist(value);
    break;
  case 'T':
    tag->SetTitle(value);
    break;
  case 'B':
    tag->SetAlbum(value);
    break;
  case 'G':
    tag->SetGenre(value);
    break;
  case 'Y':
    tag->SetYear(atol(value.c_str()));
    break;
  case 'D':
    tag->SetDuration(StringUtils::TimeStringToSeconds(value));
    break;
  case 'R': // rating
    tag->SetRating(value[0]);
    break;
  }
}

