#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <map>
#include <vector>

#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"

class TiXmlNode;
class CAlbum;
class CMusicDatabase;

class CFace
{
public:
    long idFace;
    bool operator<(const CFace& a) const
    {
        if (strFace < a.strFace) return true;
        if (strFace > a.strFace) return false;
        if (strMusicBrainzFaceID < a.strMusicBrainzFaceID) return true;
        if (strMusicBrainzFaceID > a.strMusicBrainzFaceID) return false;
        return false;
    }
    
    void Reset()
    {
        strFace.Empty();
        location.clear();
        strBiography.Empty();
        styles.clear();
        moods.clear();
        instruments.clear();
        strBorn.Empty();
        strFormed.Empty();
        strDied.Empty();
        strDisbanded.Empty();
        yearsActive.clear();
        thumbURL.Clear();
        discography.clear();
        idFace = -1;
        strPath.Empty();
    }
    
    /*! \brief Load Face information from an XML file.
     See CVideoInfoTag::Load for a description of the types of elements we load.
     \param element    the root XML element to parse.
     \param append     whether information should be added to the existing tag, or whether it should be reset first.
     \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.
     \sa CVideoInfoTag::Load
     */
    bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
    bool Save(TiXmlNode *node, const CStdString &tag, const CStdString& strPath);
    
    CStdString strFace;
    CStdString strMusicBrainzFaceID;
    std::vector<std::string> location;
    CStdString strBiography;
    std::vector<std::string> styles;
    std::vector<std::string> moods;
    std::vector<std::string> instruments;
    CStdString strBorn;
    CStdString strFormed;
    CStdString strDied;
    CStdString strDisbanded;
    std::vector<std::string> yearsActive;
    CStdString strPath;
    CScraperUrl thumbURL;
    CFanart fanart;
    std::vector<std::pair<CStdString,CStdString> > discography;
};

class CFaceCredit
{
    friend class CAlbum;
    friend class CMusicDatabase;
    
public:
    CFaceCredit() { }
    CFaceCredit(std::string strFace, std::string strJoinPhrase) : m_strFace(strFace), m_strJoinPhrase(strJoinPhrase), m_boolFeatured(false) { }
    CFaceCredit(std::string strFace, std::string strMusicBrainzFaceID, std::string strJoinPhrase)
    : m_strFace(strFace), m_strMusicBrainzFaceID(strMusicBrainzFaceID), m_strJoinPhrase(strJoinPhrase), m_boolFeatured(false)  {  }
    bool operator<(const CFaceCredit& a) const
    {
        if (m_strFace < a.m_strFace) return true;
        if (m_strFace > a.m_strFace) return false;
        if (m_strMusicBrainzFaceID < a.m_strMusicBrainzFaceID) return true;
        if (m_strMusicBrainzFaceID > a.m_strMusicBrainzFaceID) return false;
        return false;
    }
    
    std::string GetFace() const                { return m_strFace; }
    std::string GetMusicBrainzFaceID() const   { return m_strMusicBrainzFaceID; }
    std::string GetJoinPhrase() const            { return m_strJoinPhrase; }
    void SetFace(const std::string &strFace) { m_strFace = strFace; }
    void SetMusicBrainzFaceID(const std::string &strMusicBrainzFaceID) { m_strMusicBrainzFaceID = strMusicBrainzFaceID; }
    void SetJoinPhrase(const std::string &strJoinPhrase) { m_strJoinPhrase = strJoinPhrase; }
    
private:
    long idFace;
    std::string m_strFace;
    std::string m_strMusicBrainzFaceID;
    std::string m_strJoinPhrase;
    bool m_boolFeatured;
};

typedef std::vector<CFace> VECFACES;
typedef std::vector<CFaceCredit> VECFACECREDITS;

