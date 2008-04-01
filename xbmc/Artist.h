#pragma once

#include <map>
#include <vector>

#include "utils/ScraperParser.h"
#include "utils/ScraperUrl.h"

class CArtist
{
public:
  long idArtist;
  bool operator<(const CArtist& a) const
  {
    return strArtist < a.strArtist;
  }

  void Reset()
  {
    strArtist.Empty();
    strGenre.Empty();
    strBiography.Empty();
    strStyles.Empty();
    strMoods.Empty();
    strInstruments.Empty();
    strBorn.Empty();
    strFormed.Empty();
    strDied.Empty();
    strDisbanded.Empty();
    strYearsActive.Empty();
    thumbURL.Clear();
    discography.clear();
    idArtist = -1;
  }

  bool Load(const TiXmlElement *movie);
  bool Save(TiXmlNode *node, const CStdString &tag);

  CStdString strArtist;
  CStdString strGenre;
  CStdString strBiography;
  CStdString strStyles;
  CStdString strMoods;
  CStdString strInstruments;
  CStdString strBorn;
  CStdString strFormed;
  CStdString strDied;
  CStdString strDisbanded;
  CStdString strYearsActive;
  CScraperUrl thumbURL;
  std::vector<std::pair<CStdString,CStdString> > discography;
};

typedef std::vector<CArtist> VECARTISTS;
