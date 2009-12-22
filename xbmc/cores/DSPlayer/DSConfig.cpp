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

//audio settings
// required setting XBAudioConfig::HasDigitalOutput
//
//#include "XBAudioConfig.h"
#include "GuiSettings.h"
using namespace std;

CDSConfig::CDSConfig()
{
}

CDSConfig::~CDSConfig()
{
  if (m_pIMpaDecFilter)
    m_pIMpaDecFilter.Release();
  
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
//definition of AC3 VALUE DEFINITION
//A52_CHANNEL 0
//A52_MONO 1
//A52_STEREO 2
//A52_3F 3
//A52_2F1R 4
//A52_3F1R 5
//A52_2F2R 6
//A52_3F2R 7
//A52_CHANNEL1 8
//A52_CHANNEL2 9
//A52_DOLBY 10
//A52_CHANNEL_MASK 15
//LFE checked is 16

//DTS VALUE DEFINITION
//DCA_MONO 0
//DCA_CHANNEL 1
//DCA_STEREO 2
//DCA_STEREO_SUMDIFF 3
//DCA_STEREO_TOTAL 4
//DCA_3F 5
//DCA_2F1R 6
//DCA_3F1R 7
//DCA_2F2R 8
//DCA_3F2R 9
//DCA_4F2R 10
//LFE checked DCA_LFE 0x80-->128 decimal
  if (m_pIMpaDecFilter)
  {
    int pSpkConfig;
    bool audioisdigital,audiodowntostereo;
    //0 analog
    audiodowntostereo = g_guiSettings.GetSetting("audiooutput.downmixmultichannel")->ToString().Equals("true",false);
    audioisdigital = g_guiSettings.GetSetting("audiooutput.mode")->ToString().Equals("1",false);
    if (audioisdigital)
    {
      //1 digital
      //Well didnt really searched on exactly how the spdif config is setted mathematically
      //So just took the lazy way and took the value in the windows registry for spdif
      //ac3 stereo        -->ffffffee
      //ac3 3front 2 rear -->ffffffe9
      if (audiodowntostereo)
        pSpkConfig = 0xFFFFFFEE;
      else
        pSpkConfig = 0xFFFFFFE9;
      //ac3 = 0 for setting speaker
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)0, pSpkConfig);
      
      //dts stereo        -->ffffff7e
      //dts 3front 2 rear -->ffffff77 
      
      if (audiodowntostereo)
        pSpkConfig = 0xFFFFFF7E;
      else
        pSpkConfig = 0xFFFFFF77;
      
      //dts = 1 for setting speaker
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)1, pSpkConfig);
      m_pIMpaDecFilter->SaveSettings();
    }
    else
    {
      //0 analog
      if (audiodowntostereo)
        pSpkConfig = 18;
      else
        pSpkConfig = 23;
      //ac3 = 0 for setting speaker
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)0, pSpkConfig);

      if (audiodowntostereo)
        pSpkConfig = 130;
      else
        pSpkConfig = 137;

      //dts = 1 for setting speaker
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)1, pSpkConfig);
      
      //aac to stereo
      m_pIMpaDecFilter->SetSpeakerConfig((IMpaDecFilter::enctype)2, audiodowntostereo ? 1 : 0);
      m_pIMpaDecFilter->SaveSettings();
    }
    
    CLog::Log(LOGNOTICE,"%s %s",__FUNCTION__,audiodowntostereo ? "dts and ac3 is now on stereo" : "dts and ac3 is now on 3 front, 2 rear");
    CLog::Log(LOGNOTICE,"%s %s",__FUNCTION__,audioisdigital ? "SPDIF" : "not SPDIF");
                                                
  }
  return true;
}