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
#include "MediaManager.h"
#include "xbox/IoSupport.h"
#include "Util.h"
#ifdef _LINUX
#include "LinuxFileSystem.h"
#endif

const char MEDIA_SOURCES_XML[] = { "Q:\\system\\mediasources.xml" };

class CMediaManager g_mediaManager;

CMediaManager::CMediaManager()
{
}

bool CMediaManager::LoadSources()
{
  CStdString xmlFile = _P(MEDIA_SOURCES_XML);

  // clear our location list
  m_locations.clear();

  // load xml file...
  TiXmlDocument xmlDoc;
  if ( !xmlDoc.LoadFile( xmlFile.c_str() ) )
    return false;

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if ( !pRootElement || strcmpi(pRootElement->Value(), "mediasources") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d (%s)", xmlFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  // load the <network> block
  TiXmlNode *pNetwork = pRootElement->FirstChild("network");
  if (pNetwork)
  {
    TiXmlElement *pLocation = pNetwork->FirstChildElement("location");
    while (pLocation)
    {
      CNetworkLocation location;
      pLocation->Attribute("id", &location.id);
      if (pLocation->FirstChild())
      {
        location.path = pLocation->FirstChild()->Value();
        m_locations.push_back(location);
      }
      pLocation = pLocation->NextSiblingElement("location");
    }
  }
  return true;
}

bool CMediaManager::SaveSources()
{
  CStdString xmlFile = _P(MEDIA_SOURCES_XML);

  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("mediasources");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  TiXmlElement networkNode("network");
  TiXmlNode *pNetworkNode = pRoot->InsertEndChild(networkNode);
  if (pNetworkNode)
  {
    for (vector<CNetworkLocation>::iterator it = m_locations.begin(); it != m_locations.end(); it++)
    {
      TiXmlElement locationNode("location");
      locationNode.SetAttribute("id", (*it).id);
      TiXmlText value((*it).path);
      locationNode.InsertEndChild(value);
      pNetworkNode->InsertEndChild(locationNode);
    }
  }
  return xmlDoc.SaveFile(xmlFile.c_str());
}

void CMediaManager::GetLocalDrives(VECSHARES &localDrives, bool includeQ)
{
#ifndef _LINUX
  // Local shares
  CShare share;
  share.strPath = _P("C:\\");
  share.strName.Format(g_localizeStrings.Get(21438),'C');
  share.m_ignore = true;
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  localDrives.push_back(share);
#endif

  static int xx = 1;
#ifdef _LINUX
  // Home directory
  CShare share;
  share.strPath = getenv("HOME");
  share.strName = g_localizeStrings.Get(21440);
  share.strName += xx;
  xx++;
  share.m_ignore = true;
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  localDrives.push_back(share);
#endif

  share.strPath = _P("D:\\");
  share.strName = g_localizeStrings.Get(218);
  share.m_iDriveType = SHARE_TYPE_DVD;
  localDrives.push_back(share);

#ifndef _LINUX
  share.strPath = _P("E:\\");
  share.m_iDriveType = SHARE_TYPE_LOCAL;
  share.strName.Format(g_localizeStrings.Get(21438),'E');
  localDrives.push_back(share);
  for (char driveletter=EXTEND_DRIVE_BEGIN; driveletter<=EXTEND_DRIVE_END; driveletter++)
  {
    if (CIoSupport::DriveExists(driveletter))
    {
      CShare share;
      share.strPath.Format("%c:\\", driveletter);
      share.strName.Format(g_localizeStrings.Get(21438),driveletter);
      share.m_ignore = true;
      localDrives.push_back(share);
    }
  }
  if (includeQ)
  {
    CShare share;
    share.strPath = _P("Q:\\");
    share.strName.Format(g_localizeStrings.Get(21438),'Q');
    share.m_ignore = true;
    localDrives.push_back(share) ;
  }
#endif

#ifdef _LINUX
  vector<CStdString> result = CLinuxFileSystem::GetRemoveableDrives();
  for (unsigned int i = 0; i < result.size(); i++)
  {
     CShare share;
     share.strPath = result[i];
     share.strName = CUtil::GetFileName(result[i]);
     share.m_ignore = true;
     localDrives.push_back(share) ;
  }
#endif
}

void CMediaManager::GetNetworkLocations(VECSHARES &locations)
{
  // Load our xml file
  LoadSources();
  for (unsigned int i = 0; i < m_locations.size(); i++)
  {
    CShare share;
    share.strPath = m_locations[i].path;
    CURL url(share.strPath);
    url.GetURLWithoutUserDetails(share.strName);
    locations.push_back(share);
  }
}

bool CMediaManager::AddNetworkLocation(const CStdString &path)
{
  CNetworkLocation location;
  location.path = path;
  location.id = (int)m_locations.size();
  m_locations.push_back(location);
  return SaveSources();
}

bool CMediaManager::HasLocation(const CStdString& path)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == path)
      return true;
  }

  return false;
}


bool CMediaManager::RemoveLocation(const CStdString& path)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == path)
    {
      // prompt for sources, remove, cancel, 
      m_locations.erase(m_locations.begin()+i);
      return SaveSources();
    }
  }

  return false;
}

bool CMediaManager::SetLocationPath(const CStdString& oldPath, const CStdString& newPath)
{
  for (unsigned int i=0;i<m_locations.size();++i)
  {
    if (m_locations[i].path == oldPath)
    {
      m_locations[i].path = newPath;
      return SaveSources();
    }
  }

  return false;
}

