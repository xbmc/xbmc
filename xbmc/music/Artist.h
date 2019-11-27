/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBDateTime.h"
#include "utils/Fanart.h"
#include "utils/ScraperUrl.h"
#include "utils/StringUtils.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class TiXmlNode;
class CAlbum;
class CMusicDatabase;

class CArtist
{
public:
  long idArtist = -1;
  bool operator<(const CArtist& a) const
  {
    if (strMusicBrainzArtistID.empty() && a.strMusicBrainzArtistID.empty())
    {
      if (strArtist < a.strArtist) return true;
      if (strArtist > a.strArtist) return false;
      return false;
    }

    if (strMusicBrainzArtistID < a.strMusicBrainzArtistID) return true;
    if (strMusicBrainzArtistID > a.strMusicBrainzArtistID) return false;
    return false;
  }

  void MergeScrapedArtist(const CArtist& source, bool override = true);

  void Reset()
  {
    strArtist.clear();
    strSortName.clear();
    strType.clear();
    strGender.clear();
    strDisambiguation.clear();
    genre.clear();
    strBiography.clear();
    styles.clear();
    moods.clear();
    instruments.clear();
    strBorn.clear();
    strFormed.clear();
    strDied.clear();
    strDisbanded.clear();
    yearsActive.clear();
    thumbURL.Clear();
    art.clear();
    discography.clear();
    idArtist = -1;
    strPath.clear();
    dateAdded.Reset();
    bScrapedMBID = false;
    strLastScraped.clear();
  }

  /*! \brief Load artist information from an XML file.
   See CVideoInfoTag::Load for a description of the types of elements we load.
   \param element    the root XML element to parse.
   \param append     whether information should be added to the existing tag, or whether it should be reset first.
   \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.
   \sa CVideoInfoTag::Load
   */
  bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
  bool Save(TiXmlNode *node, const std::string &tag, const std::string& strPath);

  void SetDateAdded(const std::string& strDateAdded);

  std::string strArtist;
  std::string strSortName;
  std::string strMusicBrainzArtistID;
  std::string strType;
  std::string strGender;
  std::string strDisambiguation;
  std::vector<std::string> genre;
  std::string strBiography;
  std::vector<std::string> styles;
  std::vector<std::string> moods;
  std::vector<std::string> instruments;
  std::string strBorn;
  std::string strFormed;
  std::string strDied;
  std::string strDisbanded;
  std::vector<std::string> yearsActive;
  std::string strPath;
  CScraperUrl thumbURL; // Data for available thumbs
  CFanart fanart;  // Data for available fanart, urls etc.
  std::map<std::string, std::string> art;  // Current artwork - thumb, fanart etc.
  std::vector<std::pair<std::string,std::string> > discography;
  CDateTime dateAdded;
  bool bScrapedMBID = false;
  std::string strLastScraped;
};

class CArtistCredit
{
  friend class CAlbum;
  friend class CMusicDatabase;

public:
  CArtistCredit() = default;
  explicit CArtistCredit(std::string strArtist) : m_strArtist(strArtist) { }
  CArtistCredit(std::string strArtist, std::string strMusicBrainzArtistID)
    : m_strArtist(strArtist), m_strMusicBrainzArtistID(strMusicBrainzArtistID) {  }
  CArtistCredit(std::string strArtist, std::string strSortName, std::string strMusicBrainzArtistID)
    : m_strArtist(strArtist), m_strSortName(strSortName), m_strMusicBrainzArtistID(strMusicBrainzArtistID) {  }

  bool operator<(const CArtistCredit& a) const
  {
    if (m_strMusicBrainzArtistID.empty() && a.m_strMusicBrainzArtistID.empty())
    {
      if (m_strArtist < a.m_strArtist) return true;
      if (m_strArtist > a.m_strArtist) return false;
      return false;
    }

    if (m_strMusicBrainzArtistID < a.m_strMusicBrainzArtistID) return true;
    if (m_strMusicBrainzArtistID > a.m_strMusicBrainzArtistID) return false;
    return false;
  }

  std::string GetArtist() const                { return m_strArtist; }
  std::string GetSortName() const              { return m_strSortName; }
  std::string GetMusicBrainzArtistID() const   { return m_strMusicBrainzArtistID; }
  int         GetArtistId() const              { return idArtist; }
  bool HasScrapedMBID() const { return m_bScrapedMBID; }
  void SetArtist(const std::string &strArtist) { m_strArtist = strArtist; }
  void SetSortName(const std::string &strSortName) { m_strSortName = strSortName; }
  void SetMusicBrainzArtistID(const std::string &strMusicBrainzArtistID) { m_strMusicBrainzArtistID = strMusicBrainzArtistID; }
  void SetArtistId(int idArtist)               { this->idArtist = idArtist; }
  void SetScrapedMBID(bool scrapedMBID) { this->m_bScrapedMBID = scrapedMBID; }

private:
  long idArtist = -1;
  std::string m_strArtist;
  std::string m_strSortName;
  std::string m_strMusicBrainzArtistID;
  bool m_bScrapedMBID = false; // Flag that mbid is from album merge of scarper results not derived from tags
};

typedef std::vector<CArtist> VECARTISTS;
typedef std::vector<CArtistCredit> VECARTISTCREDITS;

const std::string BLANKARTIST_FAKEMUSICBRAINZID = "Artist Tag Missing";
const std::string BLANKARTIST_NAME = "[Missing Tag]";
const long BLANKARTIST_ID = 1;
const std::string VARIOUSARTISTS_MBID = "89ad4ac3-39f7-470e-963a-56509c546377";

#define ROLE_ARTIST 1  //Default role

class CMusicRole
{
public:
  CMusicRole() = default;
  CMusicRole(std::string strRole, std::string strArtist) : idRole(-1), m_strRole(strRole), m_strArtist(strArtist), idArtist(-1) { }
  CMusicRole(int role, std::string strRole, std::string strArtist, long ArtistId) : idRole(role), m_strRole(strRole), m_strArtist(strArtist), idArtist(ArtistId) { }
  std::string GetArtist() const { return m_strArtist; }
  std::string GetRoleDesc() const { return m_strRole; }
  int GetRoleId() const { return idRole; }
  long GetArtistId() const { return idArtist; }
  void SetArtistId(long iArtistId) { idArtist = iArtistId;  }

  bool operator==(const CMusicRole& a) const
  {
    if (StringUtils::EqualsNoCase(m_strRole, a.m_strRole))
      return StringUtils::EqualsNoCase(m_strArtist, a.m_strArtist);
    else
      return false;
  }
private:
  int idRole;
  std::string m_strRole;
  std::string m_strArtist;
  long idArtist;
};

typedef std::vector<CMusicRole> VECMUSICROLES;



