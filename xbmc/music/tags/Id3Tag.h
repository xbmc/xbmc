#pragma once
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
#include "MusicInfoTag.h"
#include "DllLibid3tag.h"

namespace MUSIC_INFO
{

class CID3Tag
{
public:
  CID3Tag(void);
  virtual ~CID3Tag(void);
  virtual bool Write(const CStdString& strFile);
  void SetMusicInfoTag(CMusicInfoTag& tag) { m_musicInfoTag=tag; }

protected:
  void SetArtist(const CStdString& strValue);
  void SetAlbum(const CStdString& strValue);
  void SetAlbumArtist(const CStdString& strValue);
  void SetTitle(const CStdString& strValue);
  void SetTrack(int n);
  void SetPartOfSet(int n);
  void SetYear(const CStdString& strValue);
  void SetGenre(const CStdString& strValue);
  void SetEncodedBy(const CStdString& strValue);
  void SetComment(const CStdString& strValue);
  void SetRating(char rating);
  void SetCompilation(bool compilation);

  CStdString ToStringCharset(const id3_ucs4_t* ucs4, id3_field_textencoding encoding) const;
  id3_ucs4_t* StringCharsetToUcs4(const CStdString& str) const;

  mutable DllLibID3Tag m_dll;
  CMusicInfoTag m_musicInfoTag;
  id3_tag* m_tag;
};
}
