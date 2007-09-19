/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "LabelFormatter.h"
#include "../FileItem.h"
#include "../GUISettings.h"
#include "RegExp.h"
#include "../Util.h"

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
 *  %N - Track Number
 *  %S - Disc Number
 *  %A - Artist
 *  %T - Title
 *  %B - Album
 *  %G - Genre
 *  %Y - Year
 *  %F - FileName
 *  %L - existing Label
 *  %D - Duration
 *  %I - Size
 *  %J - Date
 *  %R - Movie rating
 *  %C - Programs count
 *  %K - Movie/Game title
 *  %M - number of episodes
 *  %E - episode number
 *  %P - production code
 *  %H - season*100+episode
 *  %Z - tvshow title
 *  %O - mpaa rating
 *  %U - studio
 */

#define MASK_CHARS "NSATBGYFLDIJRCKMEPHZOU"

CLabelFormatter::CLabelFormatter(const CStdString &mask, const CStdString &mask2)
{
  // assemble our label masks
  AssembleMask(0, mask);
  AssembleMask(1, mask2);
  // save a bool for faster lookups
  m_hideFileExtensions = g_guiSettings.GetBool("filelists.hideextensions");
}

CStdString CLabelFormatter::GetContent(unsigned int label, const CFileItem *item)
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

void CLabelFormatter::FormatLabel(CFileItem *item)
{
  CStdString maskedLabel = GetContent(0, item);
  if (!maskedLabel.IsEmpty())
    item->SetLabel(maskedLabel);
  else if (!item->m_bIsFolder && m_hideFileExtensions)
    item->RemoveExtension();
}

void CLabelFormatter::FormatLabel2(CFileItem *item)
{
  item->SetLabel2(GetContent(1, item));
}

CStdString CLabelFormatter::GetMaskContent(const CMaskString &mask, const CFileItem *item)
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
    break;
  case 'S':
    if (music && music->GetDiscNumber() > 0)
      value.Format("%02.2i", music->GetDiscNumber());
    break;
  case 'A':
    if (music && music->GetArtist().size())
      value = music->GetArtist();
    if (movie && movie->m_artist.size() > 0)
      value = movie->GetArtist();
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
  case 'B':
    if (music && music->GetAlbum().size())
      value = music->GetAlbum();
    else if (movie)
      value = movie->m_strAlbum;
    break;
  case 'G':
    if (music && music->GetGenre().size())
      value = music->GetGenre();
    if (movie && movie->m_strGenre.size())
      value = movie->m_strGenre;
    break;
  case 'Y':
    if (music)
      value = music->GetYear();
    if (movie)
    {
      if (!movie->m_strFirstAired.IsEmpty())
        value = movie->m_strFirstAired;
      else if (!movie->m_strPremiered.IsEmpty())
        value = movie->m_strPremiered;
      else if (movie->m_iYear > 0)
        value.Format("%i",movie->m_iYear);
    }
    break;
  case 'F': // filename
    value = CUtil::GetTitleFromPath(item->m_strPath, item->m_bIsFolder && !item->IsFileFolder());
    break;
  case 'L':
    value = item->GetLabel();
    // is the label the actual file or folder name?
    if (value == item->m_strPath.Right(value.GetLength()))
    { // label is the same as filename, clean it up as appropriate
      value = CUtil::GetTitleFromPath(item->m_strPath, item->m_bIsFolder && !item->IsFileFolder());
    }
    break;
  case 'D':
    { // duration
      int nDuration=0;
      if (music)
        nDuration = music->GetDuration();
      if (movie)
        nDuration = StringUtils::TimeStringToSeconds(movie->m_strRuntime);
      if (nDuration > 0)
        StringUtils::SecondsToTimeString(nDuration, value);
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
  case 'R': // rating
    if (music && music->GetRating() != '0')
      value = music->GetRating();
    else if (movie && movie->m_fRating != 0.f)
        value.Format("%2.2f", movie->m_fRating);
    else if (item->m_fRating != 0.f)
      value.Format("%2.2f", item->m_fRating);
    break;
  case 'C': // programs count
    value.Format("%i", item->m_iprogramCount);
    break;
  case 'K':
    value = item->m_strTitle;
  case 'M':
    if (movie && movie->m_iEpisode > 0)
      value.Format("%02.2i %s", movie->m_iEpisode,g_localizeStrings.Get(20360));
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
    if (movie && movie->m_strStudio)
    {// MPAA Rating
      value = movie ->m_strStudio;
    }
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
    m_dynamicContent[label].push_back(CMaskString("", *reg.GetReplaceString("\\1"), ""));
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
    SplitMask(label, work.Left(findStart) + reg.GetReplaceString("\\1"));
    m_dynamicContent[label].push_back(CMaskString(reg.GetReplaceString("\\2"), *reg.GetReplaceString("\\4"), reg.GetReplaceString("\\5")));
    work = work.Mid(findStart + reg.GetFindLen());
  }
  SplitMask(label, work);
  assert(m_staticContent[label].size() == m_dynamicContent[label].size() + 1);
}
