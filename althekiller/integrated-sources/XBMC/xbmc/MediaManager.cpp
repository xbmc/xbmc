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

#include "stdafx.h"
#include "MediaManager.h"
#include "xbox/IoSupport.h"
#include "URL.h"
#include "Util.h"
#ifdef _LINUX
#include "LinuxFileSystem.h"
#endif

using namespace std;

const char MEDIA_SOURCES_XML[] = { "P:\\mediasources.xml" };

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

void CMediaManager::GetLocalDrives(VECSOURCES &localDrives, bool includeQ)
{

#ifdef _WIN32PC
  CMediaSource share;
  if (includeQ)
  {
    CMediaSource share;
    share.strPath = _P("Q:\\");
    share.strName.Format(g_localizeStrings.Get(21438),'Q');
    share.m_ignore = true;
    localDrives.push_back(share) ;
  }

  char* pcBuffer= NULL;
  DWORD dwStrLength= GetLogicalDriveStrings( 0, pcBuffer );
  if( dwStrLength != 0 )
  {
    dwStrLength+= 1; 
    pcBuffer= new char [dwStrLength];
    GetLogicalDriveStrings( dwStrLength, pcBuffer );
    
    UINT uDriveType; 
    int iPos= 0, nResult; 
    char cVolumeName[100];
    do{
      cVolumeName[0]= '\0';
      nResult= GetVolumeInformation( pcBuffer + iPos, cVolumeName, 100, 0, 0, 0, NULL, 25);
      uDriveType= GetDriveType( pcBuffer + iPos  );
      share.strPath= share.strName= "";
      
      bool bUseDCD= false; // just for testing
      if( uDriveType > DRIVE_UNKNOWN && uDriveType == DRIVE_FIXED || uDriveType == DRIVE_REMOTE ||
          uDriveType == DRIVE_CDROM || uDriveType == DRIVE_REMOVABLE )
      {
        share.strPath= pcBuffer + iPos;
        if( cVolumeName[0] != '\0' ) share.strName= cVolumeName;
        if( uDriveType == DRIVE_CDROM && nResult)
        {
          share.strName.Format( "%s - %s (%s)", 
            g_localizeStrings.Get(218), share.strName, share.strPath );
          share.m_iDriveType= CMediaSource::SOURCE_TYPE_LOCAL;
          bUseDCD= true;
        }
        else 
        {
          // Lets show it, like Windows explorer do... TODO: sorting should depend on driver letter
          share.strName.Format( "%s (%s)", 
            ( uDriveType == DRIVE_CDROM )   ? g_localizeStrings.Get(218)  :
            ( uDriveType == DRIVE_REMOVABLE && 
              share.strName.IsEmpty() )     ? g_localizeStrings.Get(437)  :
            ( uDriveType == DRIVE_UNKNOWN ) ? g_localizeStrings.Get(13205): share.strName, share.strPath );
        }
        share.strName.Replace(":\\",":");
        share.m_ignore= true;
        if( !bUseDCD )
        {
          share.m_iDriveType= ( 
           ( uDriveType == DRIVE_FIXED  )    ? CMediaSource::SOURCE_TYPE_LOCAL :
           ( uDriveType == DRIVE_REMOTE )    ? CMediaSource::SOURCE_TYPE_REMOTE :
           ( uDriveType == DRIVE_CDROM  )    ? CMediaSource::SOURCE_TYPE_DVD :
           ( uDriveType == DRIVE_REMOVABLE ) ? CMediaSource::SOURCE_TYPE_REMOVABLE :
             CMediaSource::SOURCE_TYPE_UNKNOWN );
        }

        localDrives.push_back(share);
      }
      iPos += (strlen( pcBuffer + iPos) + 1 );
    } while( strlen( pcBuffer + iPos ) > 0 );
  }
  free( pcBuffer );
#else
#ifndef _LINUX
  // Local shares
  CMediaSource share;
  share.strPath = _P("C:\\");
  share.strName.Format(g_localizeStrings.Get(21438),'C');
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  localDrives.push_back(share);
#endif

#ifdef _LINUX
  // Home directory
  CMediaSource share;
  share.strPath = getenv("HOME");
  share.strName = g_localizeStrings.Get(21440);
  share.m_ignore = true;
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  localDrives.push_back(share);
#endif

  share.strPath = _P("D:\\");
  share.strName = g_localizeStrings.Get(218);
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_DVD;
  localDrives.push_back(share);

  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
#ifndef _LINUX
  share.strPath = _P("E:\\");
  share.strName.Format(g_localizeStrings.Get(21438),'E');
  localDrives.push_back(share);
  for (char driveletter=EXTEND_DRIVE_BEGIN; driveletter<=EXTEND_DRIVE_END; driveletter++)
  {
    if (CIoSupport::DriveExists(driveletter))
    {
      CMediaSource share;
      share.strPath.Format("%c:\\", driveletter);
      share.strName.Format(g_localizeStrings.Get(21438),driveletter);
      share.m_ignore = true;
      localDrives.push_back(share);
    }
  }
  if (includeQ)
  {
    CMediaSource share;
    share.strPath = _P("Q:\\");
    share.strName.Format(g_localizeStrings.Get(21438),'Q');
    share.m_ignore = true;
    localDrives.push_back(share) ;
  }
#endif

#ifdef _LINUX
#ifdef HAS_HAL
/* HalManager will autosource items so we only want the nonremovable
   devices here. removable devices will be added in CVirtualDirectory */
  vector<CStdString> result = CLinuxFileSystem::GetLocalDrives();
#else
  vector<CStdString> result = CLinuxFileSystem::GetLocalDrives();
#endif
  for (unsigned int i = 0; i < result.size(); i++)
  {
    CMediaSource share;
    share.strPath = result[i];
    share.strName = CUtil::GetFileName(result[i]);
    share.m_ignore = true;
    localDrives.push_back(share) ;
  }
#endif
#endif // Win32PC
}

void CMediaManager::GetNetworkLocations(VECSOURCES &locations)
{
  // Load our xml file
  LoadSources();
  for (unsigned int i = 0; i < m_locations.size(); i++)
  {
    CMediaSource share;
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

