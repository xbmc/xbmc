#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "utils/StringUtils.h"
#include "utils/Fanart.h"
#include "utils/ScraperUrl.h"
#include "XBDateTime.h"

class TiXmlNode;
class CAlbum;
class CMusicDatabase;

class CArtist
{
public:
  long idArtist;
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
    discography.clear();
    idArtist = -1;
    strPath.clear();
    dateAdded.Reset();
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
  CScraperUrl thumbURL;
  CFanart fanart;
  std::vector<std::pair<std::string,std::string> > discography;
  CDateTime dateAdded;
};

class CArtistCredit
{
  friend class CAlbum;
  friend class CMusicDatabase;

public:
  CArtistCredit() : idArtist(-1) { }
  CArtistCredit(std::string strArtist) : m_strArtist(strArtist) { }
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
  void SetArtist(const std::string &strArtist) { m_strArtist = strArtist; }
  void SetSortName(const std::string &strSortName) { m_strSortName = strSortName; }
  void SetMusicBrainzArtistID(const std::string &strMusicBrainzArtistID) { m_strMusicBrainzArtistID = strMusicBrainzArtistID; }
  void SetArtistId(int idArtist)               { this->idArtist = idArtist; }

private:
  long idArtist;
  std::string m_strArtist;
  std::string m_strSortName;
  std::string m_strMusicBrainzArtistID;
};

typedef std::vector<CArtist> VECARTISTS;
typedef std::vector<CArtistCredit> VECARTISTCREDITS;

const std::string BLANKARTIST_FAKEMUSICBRAINZID = "Artist Tag Missing";
const std::string BLANKARTIST_NAME = "[Missing Tag]";
const long BLANKARTIST_ID = 1;

#define ROLE_ARTIST 1  //Default role

class CMusicRole
{
public:
  CMusicRole() { }
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



