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

#include "dsconfig.h"
#include "Utils/log.h"
#include "DShowUtil/DShowUtil.h"
#include "CharsetConverter.h"
using namespace std;

CDSConfig::CDSConfig()
{
}

CDSConfig::~CDSConfig()
{
}

HRESULT CDSConfig::LoadGraph(CComPtr<IGraphBuilder2> pGB)
{
  HRESULT hr = S_OK;
  m_pGraphBuilder = pGB.Detach();
  LoadFilters();

  return hr;
}

void CDSConfig::LoadFilters()
{
  BeginEnumFilters(m_pGraphBuilder, pEF, pBF)
  {
    GetStreamSelector(pBF);
	GetMpcVideoDec(pBF);
	GetMpaDec(pBF);
  }
  EndEnumFilters
}

bool CDSConfig::GetStreamSelector(IBaseFilter* pBF)
{
  if (m_pIAMStreamSelect)
    return true;
  m_pIAMStreamSelect = pBF;
  if(m_pIAMStreamSelect)
  {
    DWORD nStreams = 0, flags, group, prevgroup = -1;
    LCID lcid;
	CStdString destname;
	WCHAR* wname = NULL;
	CComPtr<IUnknown> pObj, pUnk;
    m_pIAMStreamSelect->Count(&nStreams);
    flags = 0;
    group = 0;
    wname = NULL;
    for(DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL)
    {
      m_pIAMStreamSelect->Info(i, NULL, &flags, &lcid, &group, &wname, &pObj, &pUnk);
      g_charsetConverter.wToUTF8(wname,destname);
      CLog::Log(LOGDEBUG,"%s",destname.c_str());
	}
  }
  return false;
}

int CDSConfig::GetAudioStreamCount()
{
  if (!m_pIAMStreamSelect)
    return 0;
  DWORD nStreams = 0;
  m_pIAMStreamSelect->Count(&nStreams);
  return (int)nStreams;
}

int CDSConfig::GetAudioStream()
{
  if (!m_pIAMStreamSelect)
    return -1;
  DWORD nStreams = 0, flags, group, prevgroup = -1;
  LCID lcid;
  WCHAR* wname = NULL;
  CComPtr<IUnknown> pObj, pUnk;
  m_pIAMStreamSelect->Count(&nStreams);
  flags = 0;
  group = 0;
  wname = NULL;
  for(DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL)
  {
    m_pIAMStreamSelect->Info(i, NULL, &flags, &lcid, &group, &wname, &pObj, &pUnk);
    if ( ((int)flags) == 1 )//AMSTREAMSELECTENABLE_ENABLE = 1
      return (int)i;
  }
}

void CDSConfig::GetAudioStreamName(int iStream, CStdString &strStreamName)
{
  if (!m_pIAMStreamSelect)
    return;
  DWORD nStreams = 0, flags, group, prevgroup = -1;
  LCID lcid;
  WCHAR* wname = NULL;
  CComPtr<IUnknown> pObj, pUnk;
  flags = 0;
  group = 0;
  wname = NULL;
  m_pIAMStreamSelect->Info(iStream, NULL, &flags, &lcid, &group, &wname, &pObj, &pUnk);
  CStdString strDest;
  g_charsetConverter.wToUTF8(wname,strDest);
  

}

void CDSConfig::SetAudioStream(int iStream)
{
  if (!m_pIAMStreamSelect)
    return;
  DWORD nStreams = 0, flags, group, prevgroup = -1;
  LCID lcid;
  WCHAR* wname = NULL;
  CComPtr<IUnknown> pObj, pUnk;
  m_pIAMStreamSelect->Count(&nStreams);
  flags = 0;
  group = 0;
  wname = NULL;
  for(DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL)
  {
    m_pIAMStreamSelect->Enable(i ,0);//0 for disable all streams
  }
  if (SUCCEEDED(m_pIAMStreamSelect->Enable(iStream ,AMSTREAMSELECTENABLE_ENABLE)))
    CLog::Log(LOGDEBUG,"%s Sucessfully selected audio stream",__FUNCTION__);
}

bool CDSConfig::GetMpcVideoDec(IBaseFilter* pBF)
{
  if (m_pIMpcDecFilter)
    return false;
  m_pIMpcDecFilter = pBF;
  m_pStdDxva.Format("");
  if ( m_pIMpcDecFilter )
  {
    m_pStdDxva = DShowUtil::GetDXVAMode(m_pIMpcDecFilter->GetDXVADecoderGuid());
    CLog::Log(LOGDEBUG,"Got IMPCVideoDecFilter");
  }

  return true;
}

bool CDSConfig::GetMpaDec(IBaseFilter* pBF)
{
  if (m_pIMpaDecFilter)
    return false;
  m_pIMpaDecFilter = pBF;
  if (m_pIMpaDecFilter)
  {
    CLog::Log(LOGDEBUG,"Got mpa decoder");
  }
  return true;
}