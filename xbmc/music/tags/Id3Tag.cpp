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

#include "Id3Tag.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

#include <set>

using namespace std;
using namespace MUSIC_INFO;

CID3Tag::CID3Tag()
{
  m_tag=NULL;
}

CID3Tag::~CID3Tag()
{
}

id3_ucs4_t* CID3Tag::StringCharsetToUcs4(const CStdString& str) const
{
  // our StringCharset is UTF-8
  return m_dll.id3_utf8_ucs4duplicate((id3_utf8_t*)str.c_str());
}

bool CID3Tag::Write(const CStdString& strFile)
{
  m_dll.Load();

  id3_file* id3file = m_dll.id3_file_open(strFile.c_str(), ID3_FILE_MODE_READWRITE);
  if (!id3file)
    return false;

  m_tag = m_dll.id3_file_tag(id3file);
  if (!m_tag)
  {
    m_dll.id3_file_close(id3file);
    return false;
  }

  SetTitle(m_musicInfoTag.GetTitle());
  SetArtist(StringUtils::Join(m_musicInfoTag.GetArtist(), g_advancedSettings.m_musicItemSeparator));
  SetAlbum(m_musicInfoTag.GetAlbum());
  SetAlbumArtist(StringUtils::Join(m_musicInfoTag.GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator));
  SetTrack(m_musicInfoTag.GetTrackNumber());
  SetGenre(StringUtils::Join(m_musicInfoTag.GetGenre(), g_advancedSettings.m_musicItemSeparator));
  SetYear(m_musicInfoTag.GetYearString());
  SetEncodedBy("XBMC");

  m_dll.id3_tag_options(m_tag, ID3_TAG_OPTION_COMPRESSION, 0);
  m_dll.id3_tag_options(m_tag, ID3_TAG_OPTION_CRC, 0);
  m_dll.id3_tag_options(m_tag, ID3_TAG_OPTION_UNSYNCHRONISATION, 0);
  m_dll.id3_tag_options(m_tag, ID3_TAG_OPTION_ID3V1, 1);

  bool success=(m_dll.id3_file_update(id3file)!=-1) ? true : false;

  m_dll.id3_file_close(id3file);

  return success;
}



void CID3Tag::SetArtist(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setartist(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetAlbum(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setalbum(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetAlbumArtist(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setalbumartist(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetTitle(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_settitle(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetTrack(int n)
{
  CStdString strValue;
  strValue.Format("%d", n);
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_settrack(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetPartOfSet(int n)
{
  CStdString strValue;
  strValue.Format("%d", n);
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setpartofset(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetYear(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setyear(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetGenre(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setgenre(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetEncodedBy(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setencodedby(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetComment(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setcomment(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetRating(char rating)
{
  m_dll.id3_metadata_setrating(m_tag, rating);
}

void CID3Tag::SetCompilation(bool compilation)
{
  CStdString strValue = compilation ? "1" : "0";
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setcompilation(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

