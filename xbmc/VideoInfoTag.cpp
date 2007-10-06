#include "VideoInfoTag.h"
#include "XMLUtils.h"
#include "LocalizeStrings.h"
#include "Settings.h"

#include <sstream>

void CVideoInfoTag::Reset()
{
  m_strDirector = "";
  m_strWritingCredits = "";
  m_strGenre = "";
  m_strTagLine = "";
  m_strPlotOutline = "";
  m_strPlot = "";
  m_strPictureURL.Clear();
  m_strTitle = "";
  m_strOriginalTitle = "";
  m_strVotes = "";
  m_cast.clear();
  m_artist.clear();
  m_strFile = "";
  m_strPath = "";
  m_strIMDBNumber = "";
  m_strMPAARating = "";
  m_strPremiered= "";
  m_strStatus= "";
  m_strProductionCode= "";
  m_strFirstAired= "";
  m_strStudio = "";
  m_strAlbum = "";
  m_iTop250 = 0;
  m_iYear = 0;
  m_iSeason = -1;
  m_iEpisode = -1;
  m_iSpecialSortSeason = -1;
  m_iSpecialSortEpisode = -1;
  m_fRating = 0.0f;
  m_iDbId = -1;

  m_bWatched = false;
}

bool CVideoInfoTag::Save(TiXmlNode *node, const CStdString &tag)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement movieElement(tag.c_str());
  TiXmlNode *movie = node->InsertEndChild(movieElement);

  if (!movie) return false;

  XMLUtils::SetString(movie, "title", m_strTitle);
  if (!m_strOriginalTitle.IsEmpty())
    XMLUtils::SetString(movie, "originaltitle", m_strOriginalTitle);
  XMLUtils::SetFloat(movie, "rating", m_fRating);
  XMLUtils::SetInt(movie, "year", m_iYear);
  XMLUtils::SetInt(movie, "top250", m_iTop250);
  XMLUtils::SetInt(movie, "season", m_iSeason);
  XMLUtils::SetInt(movie, "episode", m_iEpisode);
  XMLUtils::SetInt(movie, "displayseason",m_iSpecialSortSeason);
  XMLUtils::SetInt(movie, "displayepisode",m_iSpecialSortEpisode);
  XMLUtils::SetString(movie, "votes", m_strVotes);
  XMLUtils::SetString(movie, "outline", m_strPlotOutline);
  XMLUtils::SetString(movie, "plot", m_strPlot);
  XMLUtils::SetString(movie, "tagline", m_strTagLine);
  XMLUtils::SetString(movie, "runtime", m_strRuntime);
  XMLUtils::SetString(movie, "thumb", m_strPictureURL.m_xml);
  XMLUtils::SetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::SetBoolean(movie, "watched", m_bWatched);
  XMLUtils::SetString(movie, "file", m_strFile);
  XMLUtils::SetString(movie, "path", m_strPath);
  XMLUtils::SetString(movie, "id", m_strIMDBNumber);
  XMLUtils::SetString(movie, "filenameandpath", m_strFileNameAndPath);
  XMLUtils::SetString(movie, "genre", m_strGenre);
  XMLUtils::SetString(movie, "credits", m_strWritingCredits);
  XMLUtils::SetString(movie, "director", m_strDirector);
  XMLUtils::SetString(movie, "premiered", m_strPremiered);
  XMLUtils::SetString(movie, "status", m_strStatus);
  XMLUtils::SetString(movie, "code", m_strProductionCode);
  XMLUtils::SetString(movie, "aired", m_strFirstAired);
  XMLUtils::SetString(movie, "studio", m_strStudio);
  XMLUtils::SetString(movie, "album", m_strAlbum);
  if (m_strEpisodeGuide.IsEmpty())
    XMLUtils::SetString(movie, "episodeguide", m_strEpisodeGuide);

  // cast
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    // add a <actor> tag
    TiXmlElement cast("actor");
    TiXmlNode *node = movie->InsertEndChild(cast);
    TiXmlElement actor("name");
    TiXmlNode *actorNode = node->InsertEndChild(actor);
    TiXmlText name(it->strName);
    actorNode->InsertEndChild(name);
    TiXmlElement role("role");
    TiXmlNode *roleNode = node->InsertEndChild(role);
    TiXmlText character(it->strRole);
    roleNode->InsertEndChild(character);
    TiXmlElement thumb("thumb");
    TiXmlNode *thumbNode = node->InsertEndChild(thumb);
    TiXmlText th(it->thumbUrl.m_xml);
    thumbNode->InsertEndChild(th);
  }
  // artists
  for (std::vector<CStdString>::const_iterator it = m_artist.begin(); it != m_artist.end(); ++it)
  {
    // add a <actor> tag
    TiXmlElement cast("artist");
    TiXmlNode *node = movie->InsertEndChild(cast);
    TiXmlElement actor("name");
    TiXmlNode *actorNode = node->InsertEndChild(actor);
    TiXmlText name(*it);
    actorNode->InsertEndChild(name);
  }

  return true;
}

bool CVideoInfoTag::Load(const TiXmlElement *movie, bool chained /* = false */)
{
  if (!movie) return false;

  // reset our details if we aren't chained.
  if (!chained) Reset();

  XMLUtils::GetString(movie, "title", m_strTitle);
  XMLUtils::GetString(movie, "originaltitle", m_strOriginalTitle);
  XMLUtils::GetFloat(movie, "rating", m_fRating);
  XMLUtils::GetInt(movie, "year", m_iYear);
  XMLUtils::GetInt(movie, "top250", m_iTop250);
  XMLUtils::GetInt(movie, "season", m_iSeason);
  XMLUtils::GetInt(movie, "episode", m_iEpisode);
  XMLUtils::GetInt(movie, "displayseason", m_iSpecialSortSeason);
  XMLUtils::GetInt(movie, "displayepisode", m_iSpecialSortEpisode);
  int after=0;
  XMLUtils::GetInt(movie, "displayafterseason",after);
  if (after > 0)
  {
    m_iSpecialSortSeason = after;
    m_iSpecialSortEpisode = 2^13; // should be more than any realistic episode number
  }
  XMLUtils::GetString(movie, "votes", m_strVotes);
  XMLUtils::GetString(movie, "outline", m_strPlotOutline);
  XMLUtils::GetString(movie, "plot", m_strPlot);
  XMLUtils::GetString(movie, "tagline", m_strTagLine);
  XMLUtils::GetString(movie, "runtime", m_strRuntime);
  XMLUtils::GetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::GetBoolean(movie, "watched", m_bWatched);
  XMLUtils::GetString(movie, "file", m_strFile);
  XMLUtils::GetString(movie, "path", m_strPath);
  XMLUtils::GetString(movie, "id", m_strIMDBNumber);
  XMLUtils::GetString(movie, "filenameandpath", m_strFileNameAndPath);
  XMLUtils::GetString(movie, "premiered", m_strPremiered);
  XMLUtils::GetString(movie, "status", m_strStatus);
  XMLUtils::GetString(movie, "code", m_strProductionCode);
  XMLUtils::GetString(movie, "aired", m_strFirstAired);
  XMLUtils::GetString(movie, "album", m_strAlbum);

  m_strPictureURL.ParseElement(movie->FirstChildElement("thumbs"));
  if (m_strPictureURL.m_url.size() == 0)
  {
    if (movie->FirstChildElement("thumb") && !movie->FirstChildElement("thumb")->FirstChildElement())
    {
      if (movie->FirstChildElement("thumb")->FirstChild() && strncmp(movie->FirstChildElement("thumb")->FirstChild()->Value(),"<thumb>",7) == 0)
      {
        CStdString strValue = movie->FirstChildElement("thumb")->FirstChild()->Value();
        TiXmlDocument doc;
        doc.Parse(strValue.c_str());
        if (doc.FirstChildElement("thumbs"))
          m_strPictureURL.ParseElement(doc.FirstChildElement("thumbs"));
        else
          m_strPictureURL.ParseElement(doc.FirstChildElement("thumb"));
      }
      else
        m_strPictureURL.ParseElement(movie->FirstChildElement("thumb"));
    }
    else
      m_strPictureURL.ParseElement(movie->FirstChildElement("thumb"));
  }

  CStdString strTemp;
  const TiXmlNode *node = movie->FirstChild("genre");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_strGenre.IsEmpty())
        m_strGenre = strTemp;
      else
        m_strGenre += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("genre");
  }

  node = movie->FirstChild("credits");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_strWritingCredits.IsEmpty())
        m_strWritingCredits = strTemp;
      else
        m_strWritingCredits += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("credits");
  }
  
  node = movie->FirstChild("director");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_strDirector.IsEmpty())
        m_strDirector = strTemp;
      else
        m_strDirector += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("director");
  }
  // cast
  node = movie->FirstChild("actor");
  while (node)
  {
    const TiXmlNode *actor = node->FirstChild("name");
    if (actor && actor->FirstChild())
    {
      SActorInfo info;
      info.strName = actor->FirstChild()->Value();
      const TiXmlNode *roleNode = node->FirstChild("role");
      if (roleNode && roleNode->FirstChild())
        info.strRole = roleNode->FirstChild()->Value();
      const TiXmlElement *thumbNode = node->FirstChildElement("thumbs");
      if (thumbNode && thumbNode->FirstChild())
        info.thumbUrl.ParseElement(thumbNode);
      else
      {
        thumbNode = node->FirstChildElement("thumb");
        if (thumbNode && thumbNode->FirstChild())
          info.thumbUrl.ParseElement(thumbNode);
      }
      m_cast.push_back(info);
    }
    node = node->NextSibling("actor");
  }
  // studios
  node = movie->FirstChild("studio");
  while (node)
  {
    if (node->FirstChild())
    {
      strTemp = node->FirstChild()->Value();
      if (m_strStudio.IsEmpty())
        m_strStudio = strTemp;
      else
        m_strStudio += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("studio");
  }
  // artists
  node = movie->FirstChild("artist");
  while (node)
  {
    const TiXmlNode* pNode = node->FirstChild("name");
    if (pNode && pNode->FirstChild())
    {
      const char *pValue = pNode->FirstChild()->Value();
      if (pValue)
        m_artist.push_back(pValue);
    }
    node = node->NextSibling("artist");
  }

  const TiXmlElement *epguide = movie->FirstChildElement("episodeguide");
  if (epguide)
  {
    if (epguide->FirstChild() && strncmp(epguide->FirstChild()->Value(),"<episodeguide>",14) == 0)
      m_strEpisodeGuide = epguide->FirstChild()->Value();
    else if (epguide->FirstChild() && strlen(epguide->FirstChild()->Value()) > 0)
    {
      std::stringstream stream;
      stream << *epguide;
      m_strEpisodeGuide = stream.str();
    }
  }

  return true;
}

void CVideoInfoTag::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_strDirector;
    ar << m_strWritingCredits;
    ar << m_strGenre;
    ar << m_strTagLine;
    ar << m_strPlotOutline;
    ar << m_strPlot;
    ar << m_strPictureURL.m_spoof;
    ar << m_strPictureURL.m_xml;
    ar << m_strTitle;
    ar << m_strVotes;
    ar << m_strStudio;
    ar << (int)m_cast.size();
    for (unsigned int i=0;i<m_cast.size();++i)
    {
      ar << m_cast[i].strName;
      ar << m_cast[i].strRole;
      ar << m_cast[i].thumbUrl.m_xml;
    }
    ar << (int)m_artist.size();
    for (unsigned int i=0;i<m_artist.size();++i)
      ar << m_artist[i];

    ar << m_strRuntime;
    ar << m_strFile;
    ar << m_strPath;
    ar << m_strIMDBNumber;
    ar << m_strMPAARating;
    ar << m_strFileNameAndPath;
    ar << m_strOriginalTitle;
    ar << m_strEpisodeGuide;
    ar << m_strPremiered;
    ar << m_strStatus;
    ar << m_strProductionCode;
    ar << m_strFirstAired;
    ar << m_strShowTitle;
    ar << m_strAlbum;
    ar << m_bWatched;
    ar << m_iTop250;
    ar << m_iYear;
    ar << m_iSeason;
    ar << m_iEpisode;
    ar << m_fRating;
    ar << m_iDbId;
    ar << m_iSpecialSortSeason;
    ar << m_iSpecialSortEpisode;
  }
  else
  {
    ar >> m_strDirector;
    ar >> m_strWritingCredits;
    ar >> m_strGenre;
    ar >> m_strTagLine;
    ar >> m_strPlotOutline;
    ar >> m_strPlot;
    ar >> m_strPictureURL.m_spoof;
    ar >> m_strPictureURL.m_xml;
    m_strPictureURL.Parse();
    ar >> m_strTitle;
    ar >> m_strVotes;
    ar >> m_strStudio;
    int iCastSize;
    ar >> iCastSize;
    for (int i=0;i<iCastSize;++i)
    {
      SActorInfo info;
      ar >> info.strName;
      ar >> info.strRole;
      CStdString strXml;
      ar >> strXml;
      info.thumbUrl.ParseString(strXml);
      m_cast.push_back(info);
    }
    int iArtistSize;
    ar >> iArtistSize;
    for (int i=0;i<iArtistSize;++i)
    {
      CStdString strFirst;
      ar >> strFirst;
      m_artist.push_back(strFirst);
    }

    ar >> m_strRuntime;
    ar >> m_strFile;
    ar >> m_strPath;
    ar >> m_strIMDBNumber;
    ar >> m_strMPAARating;
    ar >> m_strFileNameAndPath;
    ar >> m_strOriginalTitle;
    ar >> m_strEpisodeGuide;
    ar >> m_strPremiered;
    ar >> m_strStatus;
    ar >> m_strProductionCode;
    ar >> m_strFirstAired;
    ar >> m_strShowTitle;
    ar >> m_strAlbum;
    ar >> m_bWatched;
    ar >> m_iTop250;
    ar >> m_iYear;
    ar >> m_iSeason;
    ar >> m_iEpisode;
    ar >> m_fRating;
    ar >> m_iDbId;
    ar >> m_iSpecialSortSeason;
    ar >> m_iSpecialSortEpisode;
  }
}

const CStdString CVideoInfoTag::GetArtist() const
{
  CStdString result;
  for (unsigned int i=0;i<m_artist.size();++i)
    result += m_artist[i]+g_advancedSettings.m_videoItemSeparator;
  result.TrimRight(g_advancedSettings.m_videoItemSeparator);

  return result;
}

const CStdString CVideoInfoTag::GetCast(bool bIncludeRole /*= false*/) const
{
  CStdString strLabel;
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    CStdString character;
    if (it->strRole.IsEmpty() || !bIncludeRole)
      character.Format("%s\n", it->strName.c_str());
    else
      character.Format("%s %s %s\n", it->strName.c_str(), g_localizeStrings.Get(20347).c_str(), it->strRole.c_str());
    strLabel += character;
  }
  return strLabel.TrimRight("\n");
}
