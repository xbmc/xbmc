/*!
 \file Album.h
\brief
*/
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

#include <map>
#include <vector>

#include "Song.h"
#include "utils/ScraperUrl.h"

class TiXmlNode;

class CAlbum
{
public:
  CAlbum() { idAlbum = 0; iRating = 0; iYear = 0; };
  bool operator<(const CAlbum &a) const
  {
    return strAlbum + strArtist < a.strAlbum + a.strArtist;
  }

  void Reset()
  {
    idAlbum = -1;
    strAlbum.Empty();
    strArtist.Empty();
    strGenre.Empty();
    thumbURL.Clear();
    strMoods.Empty();
    strStyles.Empty();
    strThemes.Empty();
    strReview.Empty();
    strLabel.Empty();
    strType.Empty();
    m_strDateOfRelease.Empty();
    iRating=-1;
    iYear=-1;
    songs.clear();
  }

  bool Load(const TiXmlElement *movie, bool chained=false);
  bool Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath);

  long idAlbum;
  CStdString strAlbum;
  CStdString strArtist;
  CStdString strGenre;
  CScraperUrl thumbURL;
  CStdString strMoods;
  CStdString strStyles;
  CStdString strThemes;
  CStdString strReview;
  CStdString strLabel;
  CStdString strType;
  CStdString m_strDateOfRelease;
  int iRating;
  int iYear;
  VECSONGS songs;
};

typedef std::vector<CAlbum> VECALBUMS;
