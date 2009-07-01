/*
 *      Copyright (C) 2005-2009 Team XBMC
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
#include "WINDetectDVDType.h"
#ifdef _WIN32PC
#include "WIN32Util.h"

using namespace MEDIA_DETECT;

CWINDetectDVDMedia* CWINDetectDVDMedia::m_instance = NULL;


CWINDetectDVDMedia::CWINDetectDVDMedia()
{
}

CWINDetectDVDMedia::~CWINDetectDVDMedia()
{
  RemoveAllMedia();
}
 
CWINDetectDVDMedia* CWINDetectDVDMedia::GetInstance()
{
  if( !m_instance )
    m_instance = new CWINDetectDVDMedia();
  return m_instance;
}

void CWINDetectDVDMedia::Destroy()
{
  if (m_instance)
  {
    delete m_instance;
    m_instance = NULL;
  }
}

void CWINDetectDVDMedia::AddMedia(CStdString& strDrive)
{
  CStdString strPath = strDrive;
  strPath.MakeLower();
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it == m_mapCdInfo.end())
  {
    CCdIoSupport cdio;
    CCdInfo* pCdInfo = cdio.GetCdInfo((char*)strPath.c_str());
    if (pCdInfo == NULL)
    {
      CLog::Log(LOGERROR, "Detection of DVD-ROM media failed.");
      return ;
    }
    else
      m_mapCdInfo.insert(std::pair<char,CCdInfo*>(strPath[0],pCdInfo));
  }
}

void CWINDetectDVDMedia::RemoveMedia(CStdString& strDrive)
{
  CStdString strPath = strDrive;
  strPath.MakeLower();
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it != m_mapCdInfo.end())
  {
    if(it->second != NULL)
      delete it->second;

    m_mapCdInfo.erase(it);
  }
}

void CWINDetectDVDMedia::RemoveAllMedia()
{
  std::map<char,CCdInfo*>::iterator it;
  for (it=m_mapCdInfo.begin();it!=m_mapCdInfo.end();++it)
  {
    if(it->second != NULL)
      delete it->second;
  }
  m_mapCdInfo.clear();
}

CCdInfo* CWINDetectDVDMedia::GetCdInfo(CStdString& strDrive)
{
  CSingleLock waitLock(m_critsec);
  CStdString strPath = strDrive;
  strPath.MakeLower();
  std::map<char,CCdInfo*>::iterator it;
  it = m_mapCdInfo.find(strPath[0]);
  if(it != m_mapCdInfo.end())
    return it->second;
  else
    return NULL;
}

bool CWINDetectDVDMedia::IsAudio(CStdString& strDrive)
{
  CCdInfo* pCdInfo = GetCdInfo(strDrive);
  if(pCdInfo != NULL)
    return pCdInfo->IsAudio(1);
  else
    return false;
}


#endif
