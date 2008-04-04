/*!
 \file Album.h
\brief
*/
#pragma once

#include <map>
#include <vector>

#include "Song.h"
#include "utils/ScraperParser.h"

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
    thumbURL.Clear();;  
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

  bool Load(const TiXmlElement *movie);
  bool Save(TiXmlNode *node, const CStdString &tag);

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
