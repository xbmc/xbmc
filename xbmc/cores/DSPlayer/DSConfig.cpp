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
#include "RegExp.h"
//audio settings
// required setting XBAudioConfig::HasDigitalOutput
//
//#include "XBAudioConfig.h"
#include "GuiSettings.h"
using namespace std;

CDSConfig::CDSConfig()
{
  m_pGraphBuilder = NULL;
  m_pIMpcDecFilter = NULL;
  m_pIMpaDecFilter = NULL;
  m_pIAMStreamSelect = NULL;
  m_pSplitter = NULL;
}

CDSConfig::~CDSConfig()
{
  SAFE_RELEASE(m_pIMpaDecFilter);
  SAFE_RELEASE(m_pIMpcDecFilter);
  SAFE_RELEASE(m_pIAMStreamSelect);
  SAFE_RELEASE(m_pSplitter);

  for (std::map<long, IAMStreamSelectInfos *>::iterator it = m_pAudioStreams.begin();
    it != m_pAudioStreams.end(); ++it)
  {
    SAFE_RELEASE(it->second->pObj);
    SAFE_RELEASE(it->second->pUnk);
    delete it->second;
  }

  for (std::map<long, IAMStreamSelectInfos *>::iterator it = m_pEmbedSubtitles.begin();
    it != m_pEmbedSubtitles.end(); ++it)
  {
    SAFE_RELEASE(it->second->pObj);
    SAFE_RELEASE(it->second->pUnk);
    delete it->second;
  }
}

HRESULT CDSConfig::LoadGraph(IFilterGraph2* pGB, IBaseFilter * splitter)
{
  HRESULT hr = S_OK;
  m_pGraphBuilder = pGB;
  m_pSplitter = splitter;
  pGB = NULL;
  LoadFilters();

  return hr;
}

void CDSConfig::LoadFilters()
{
  BeginEnumFilters(m_pGraphBuilder, pEF, pBF)
  {
	  GetMpcVideoDec(pBF);
	  GetMpaDec(pBF);
  }
  EndEnumFilters(pEF, pBF)
  LoadAudioStreams();
}

HRESULT CDSConfig::UnloadGraph()
{
  HRESULT hr = S_OK;

  IEnumFilters *pEnum = NULL;
  IBaseFilter *pBF = NULL;

  m_pGraphBuilder->EnumFilters(&pEnum);

  // Disconnect all the pins
  while (S_OK == pEnum->Next(1, &pBF, 0))
  {
    pBF->Stop();
    CStdString filterName;
    g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(pBF), filterName);
    //hr = RemoveFilter(m_pGraphBuilder, pBF);
    m_pGraphBuilder->RemoveFilter(pBF);
    if (SUCCEEDED(hr))
    {
      //pBF->JoinFilterGraph(NULL, NULL); // Notify the filter we remove it from the graph - DONE IN REMOVE FILTER
      CLog::Log(LOGNOTICE, "%s Successfully removed \"%s\" from the graph", __FUNCTION__, filterName.c_str());
    } else 
      CLog::Log(LOGERROR, "%s Failed to remove \"%s\" from the graph", __FUNCTION__, filterName.c_str());
    SAFE_RELEASE(pBF);
    pEnum->Reset();
  }

  SAFE_RELEASE(pEnum);
  return hr;
}

bool CDSConfig::LoadAudioStreams()
{
  CStdString splitterName;
  g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(m_pSplitter), splitterName);

  CLog::Log(LOGDEBUG, "%s Looking for audio streams in %s splitter", __FUNCTION__, splitterName.c_str());

  /* Regex to rename audio stream */
  std::vector<CRegExp *> regex;

  CRegExp *reg = new CRegExp(true);
  reg->RegComp("(.*?)(\\(audio.*?\\)|\\(subtitle.*?\\))"); // mkv source audio / subtitle
  regex.push_back(reg);

  reg = new CRegExp(true);
  reg->RegComp(".* - (.*),.*\\(.*\\)"); // mpeg source audio / subtitle
  regex.push_back(reg);

  /* Does the splitter support IAMStreamSelect ?*/
  HRESULT hr = m_pSplitter->QueryInterface(__uuidof(m_pIAMStreamSelect), (void **) &m_pIAMStreamSelect);
  if (SUCCEEDED(hr))
  {
    /* Yes */
    CLog::Log(LOGDEBUG, "%s Get IAMStreamSelect interface from %s", __FUNCTION__, splitterName.c_str());

    DWORD nStreams = 0, flags = 0;
    WCHAR* wname = NULL;
    IAMStreamSelectInfos *infos;

    m_pIAMStreamSelect->Count(&nStreams);

    AM_MEDIA_TYPE * mediaType = NULL;
    for(unsigned int i = 0; i < nStreams; i++)
    {
      infos = new IAMStreamSelectInfos();
      infos->group = 0;  infos->lcid = 0; infos->pObj = 0; infos->pUnk = 0; infos->flags = 0;

      m_pIAMStreamSelect->Info(i, &mediaType, &infos->flags, &infos->lcid, &infos->group, &wname, &infos->pObj, &infos->pUnk);
      g_charsetConverter.wToUTF8(wname, infos->name);
      CoTaskMemFree(wname);

      /* Apply regex */
      for (std::vector<CRegExp *>::const_iterator it = regex.begin(); it != regex.end(); ++it)
      {
        if ( (*it)->RegFind(infos->name) > -1 )
        {
          infos->name = (*it)->GetMatch(1);
          break;
        }
      }

      if (mediaType->majortype == MEDIATYPE_Audio)
      {
        /* Audio stream */
        m_pAudioStreams.insert( std::pair<long, IAMStreamSelectInfos *>(i, infos) );
        CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());

      } else if (mediaType->majortype == MEDIATYPE_Subtitle)
      {
        /* Embed subtitles */
        m_pEmbedSubtitles.insert( std::pair<long, IAMStreamSelectInfos *>(i, infos) );
        CLog::Log(LOGNOTICE, "%s Embed subtitle found : %s", __FUNCTION__, infos->name.c_str());
      }

      DeleteMediaType(mediaType);
    }
  } else {
    /* No */  
  
    // Enumerate output pins
    PIN_DIRECTION dir;
    int i = 0, j = 0;
    CStdStringW pinNameW;
    CStdString pinName;
    IAMStreamSelectInfos *infos;
    bool pinConnected = FALSE;
    bool audioPinAlreadyConnected = FALSE, subtitlePinAlreadyConnected = FALSE;

    int nIn = 0, nOut = 0, nInC = 0, nOutC = 0;
    DShowUtil::CountPins(m_pSplitter, nIn, nOut, nInC, nOutC);
    CLog::Log(LOGDEBUG, "%s The splitter has %d output pins", __FUNCTION__, nOut);

    BeginEnumPins(m_pSplitter, pEP, pPin)
    {
      if (SUCCEEDED(pPin->QueryDirection(&dir)) && ( dir == PINDIR_OUTPUT ))
      {

        pinNameW = DShowUtil::GetPinName(pPin);
        g_charsetConverter.wToUTF8(pinNameW, pinName);
        CLog::Log(LOGDEBUG, "%s Output pin found : %s", __FUNCTION__, pinName.c_str());

        /* TODO: Ne pas utiliser ConnectionMediaType. Renvois false si la pin n'est pas connecté -> problème.
         Utiliser BeginEnumMediaTypes comme en dessous pour TOUTES les pins */

        BeginEnumMediaTypes(pPin, pET, pMediaType)
        {

          CLog::Log(LOGDEBUG, "%s \tOutput pin major type : %s", __FUNCTION__, GuidNames[pMediaType->majortype]);
          CLog::Log(LOGDEBUG, "%s \tOutput pin sub type : %s", __FUNCTION__, GuidNames[pMediaType->subtype]);
          CLog::Log(LOGDEBUG, "%s \tOutput pin format type : %s", __FUNCTION__, GuidNames[pMediaType->formattype]);

          if (pMediaType->majortype != MEDIATYPE_Audio &&
            pMediaType->majortype != MEDIATYPE_Subtitle)
            continue;

          infos = new IAMStreamSelectInfos();
          infos->group = 0;  infos->lcid = 0; infos->pObj = 0; infos->pUnk = 0; infos->flags = 0;
          pinConnected = FALSE;

          infos->name = pinName;

          /* Apply regex */
          for (std::vector<CRegExp *>::const_iterator it = regex.begin(); it != regex.end(); ++it)
          {
            if ( (*it)->RegFind(infos->name) > -1 )
            {
              infos->name = (*it)->GetMatch(1);
              break;
            }
          }

          if (infos->name.Equals("Undetermined"))
            infos->name.Format("Audio %02d", i);

          infos->pObj = pPin;

          if (pMediaType->majortype == MEDIATYPE_Audio)
          {
            if (SUCCEEDED(DShowUtil::IsPinConnected(pPin)))
            {
              if (audioPinAlreadyConnected)
              { // Prevent multiple audio stream at the same time
                IPin *pin = NULL;
                pPin->ConnectedTo(&pin);
                m_pGraphBuilder->Disconnect(pPin);
                m_pGraphBuilder->Disconnect(pin);
                SAFE_RELEASE(pin);
              } 
              else 
              {
                infos->flags = AMSTREAMSELECTINFO_ENABLED;
                audioPinAlreadyConnected = TRUE;
              }
            }

            m_pAudioStreams.insert( std::pair<long, IAMStreamSelectInfos *>(i, infos) );
            CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());
            i++;
          } else {

            if (SUCCEEDED(DShowUtil::IsPinConnected(pPin)))
            {
              if (subtitlePinAlreadyConnected)
              { // Prevent multiple audio stream at the same time
                IPin *pin = NULL;
                pPin->ConnectedTo(&pin);
                m_pGraphBuilder->Disconnect(pPin);
                m_pGraphBuilder->Disconnect(pin);
                SAFE_RELEASE(pin);
              } 
              else 
              {
                infos->flags = AMSTREAMSELECTINFO_ENABLED;
                subtitlePinAlreadyConnected = TRUE;
              }
            }

            m_pEmbedSubtitles.insert( std::pair<long, IAMStreamSelectInfos *>(j, infos) );
            CLog::Log(LOGNOTICE, "%s Embed subtitle found : %s", __FUNCTION__, infos->name.c_str());
            j++;

          }

        }
        EndEnumMediaTypes(pET, pMediaType)
      }
    }
    EndEnumPins(pEP, pPin)
  }

  /* Delete regex */
  while (! regex.empty())
  {
    delete regex.back();
    regex.pop_back();
  }

  return false;
}

int CDSConfig::GetAudioStreamCount()
{
/*  if (!m_pIAMStreamSelect)
    return 0; */

  return m_pAudioStreams.size();
}

int CDSConfig::GetAudioStream()
{
  if (m_pAudioStreams.size() == 0)
    return -1;
  //DWORD nStreams = 0, flags, group, prevgroup = -1;
  //LCID lcid;
  //WCHAR* wname = NULL;
  //IUnknown* pObj;
  //IUnknown* pUnk;
  //m_pIAMStreamSelect->Count(&nStreams);
  //flags = 0;
  //group = 0;
  //wname = NULL;
  //for(DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL)
  //{
  //  m_pIAMStreamSelect->Info(i, NULL, &flags, &lcid, &group, &wname, &pObj, &pUnk);
  //  if ( ((int)flags) == 1 )//AMSTREAMSELECTENABLE_ENABLE = 1
  //    return (int)i;
  //}
  int i = 0;
  for (std::map<long, IAMStreamSelectInfos *>::const_iterator it = m_pAudioStreams.begin();
    it != m_pAudioStreams.end(); ++it, i++)
  {
    if ( (*it).second->flags == AMSTREAMSELECTINFO_ENABLED)
      return i;
  }

  return -1;
}

void CDSConfig::GetAudioStreamName(int iStream, CStdString &strStreamName)
{
  if (m_pAudioStreams.size() == 0)
    return;

  int i = 0;
  for (std::map<long, IAMStreamSelectInfos *>::const_iterator it = m_pAudioStreams.begin();
    it != m_pAudioStreams.end(); ++it, i++)
  {
    if (i == iStream)
    {
      strStreamName = (*it).second->name;
    }
  }
}

void CDSConfig::SetAudioStream(int iStream)
{

  if (!m_pIAMStreamSelect)
    return;

  //DWORD nCount = m_pAudioStreams.size();
  int i =0; long lIndex = 0;
  for (std::map<long, IAMStreamSelectInfos *>::const_iterator it = m_pAudioStreams.begin();
    it != m_pAudioStreams.end(); ++it, i++)
  {
    /* Delete all streams */
    m_pIAMStreamSelect->Enable(it->first, 0);
    it->second->flags = 0;
    if (iStream == i)
      lIndex = it->first;
  }

  if (SUCCEEDED(m_pIAMStreamSelect->Enable(lIndex, AMSTREAMSELECTENABLE_ENABLE)))
  {
    m_pAudioStreams[lIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
    CLog::Log(LOGDEBUG, "%s Sucessfully selected audio stream", __FUNCTION__);
  }
}


int CDSConfig::GetSubtitleCount()
{
  return m_pEmbedSubtitles.size();
}

int CDSConfig::GetSubtitle()
{
  int i = 0;
  for (std::map<long, IAMStreamSelectInfos *>::const_iterator it = m_pEmbedSubtitles.begin();
    it != m_pEmbedSubtitles.end(); ++it, i++)
  {
    if ( (*it).second->flags == AMSTREAMSELECTINFO_ENABLED)
      return i;
  }

  return -1;
}

void CDSConfig::GetSubtitleName(int iStream, CStdString &strStreamName)
{
  if (m_pAudioStreams.size() == 0)
    return;

  int i = 0;
  for (std::map<long, IAMStreamSelectInfos *>::const_iterator it = m_pEmbedSubtitles.begin();
    it != m_pEmbedSubtitles.end(); ++it, i++)
  {
    if (i == iStream)
    {
      strStreamName = (*it).second->name;
    }
  }
}

void CDSConfig::SetSubtitle(int iStream)
{

}

bool CDSConfig::GetMpcVideoDec(IBaseFilter* pBF)
{
  if (m_pIMpcDecFilter)
    return false;
  pBF->QueryInterface(__uuidof(m_pIMpcDecFilter), (void **)&m_pIMpcDecFilter);
  if (!m_pIMpcDecFilter)
    return false;
  m_pStdDxva.Format("");
  if (g_guiSettings.GetBool("dsplayer.forcenondefaultrenderer"))
  {
    m_pStdDxva = CStdString("");
    CLog::Log(LOGERROR,"%s DXVA WILL NEVER WORK WITH NON DEFAULT RENDERER ON",__FUNCTION__);
  }
  else
  {
    if ( m_pIMpcDecFilter )
    {
      m_pStdDxva = DShowUtil::GetDXVAMode(m_pIMpcDecFilter->GetDXVADecoderGuid());
      CLog::Log(LOGDEBUG,"Got IMPCVideoDecFilter");
    }
  }

  return true;
}

bool CDSConfig::GetMpaDec(IBaseFilter* pBF)
{
  if (m_pIMpaDecFilter)
    return false;
  pBF->QueryInterface(__uuidof(m_pIMpaDecFilter), (void **)&m_pIMpaDecFilter);

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
      //m_pIMpaDecFilter->SaveSettings();
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
      //m_pIMpaDecFilter->SaveSettings();
    }
    
    CLog::Log(LOGNOTICE,"%s %s",__FUNCTION__,audiodowntostereo ? "dts and ac3 is now on stereo" : "dts and ac3 is now on 3 front, 2 rear");
    CLog::Log(LOGNOTICE,"%s %s",__FUNCTION__,audioisdigital ? "SPDIF" : "not SPDIF");
                                                
  }
  return true;
}