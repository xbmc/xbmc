/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "pch.h"
#include "StreamsManager.h"
#include "SpecialProtocol.h"
#include "DVDSubtitles\DVDFactorySubtitle.h"
#include "Settings.h"
#include "filtercorefactory\filtercorefactory.h"
#include "DSPlayer.h"
#include "FGFilter.h"
#include "DShowUtil/smartptr.h"
#include "boost/ptr_container/ptr_vector.hpp"

CStreamsManager *CStreamsManager::m_pSingleton = NULL;

CStreamsManager *CStreamsManager::getSingleton()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CStreamsManager());
}

CStreamsManager::CStreamsManager(void):
  m_pIAMStreamSelect(NULL),
  m_init(false),
  m_bChangingStream(false),
  m_bSubtitlesUnconnected(false),
  m_SubtitleInputPin(NULL)
{
  memset(&m_subtitleMediaType, 0, sizeof(AM_MEDIA_TYPE));
  m_subtitleMediaType.majortype = MEDIATYPE_Subtitle;
  m_videoStream.Clear();
}

CStreamsManager::~CStreamsManager(void)
{

  while (! m_audioStreams.empty())
  {
    delete m_audioStreams.back();
    m_audioStreams.pop_back();
  }

  while (! m_subtitleStreams.empty())
  {
    delete m_subtitleStreams.back();
    m_subtitleStreams.pop_back();
  }

  m_pSplitter = NULL;
  m_pGraphBuilder = NULL;

  CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);

}

std::vector<SAudioStreamInfos *> CStreamsManager::GetAudios()
{
  return m_audioStreams;
}

std::vector<SSubtitleStreamInfos *> CStreamsManager::GetSubtitles()
{
  return m_subtitleStreams;
}

int CStreamsManager::GetAudioStreamCount()
{
  return m_audioStreams.size();
}

int CStreamsManager::GetAudioStream()
{
  if (m_audioStreams.size() == 0)
    return -1;
  
  int i = 0;
  for (std::vector<SAudioStreamInfos *>::const_iterator it = m_audioStreams.begin();
    it != m_audioStreams.end(); ++it, i++)
  {
    if ( (*it)->flags == AMSTREAMSELECTINFO_ENABLED)
      return i;
  }

  return -1;
}

void CStreamsManager::GetAudioStreamName(int iStream, CStdString &strStreamName)
{
  if (m_audioStreams.size() == 0)
    return;

  int i = 0;
  for (std::vector<SAudioStreamInfos *>::const_iterator it = m_audioStreams.begin();
    it != m_audioStreams.end(); ++it, i++)
  {
    if (i == iStream)
    {
      strStreamName = (*it)->name;
    }
  }
}

void CStreamsManager::SetAudioStream(int iStream)
{
  if (! m_init)
    return;

  long disableIndex = GetAudioStream(), enableIndex = iStream;

  if (disableIndex == enableIndex
    || ! m_audioStreams[disableIndex]->connected
    || m_audioStreams[enableIndex]->connected)
    return;

  m_bChangingStream = true;

  if (m_pIAMStreamSelect)
  {

    m_pIAMStreamSelect->Enable(m_audioStreams[disableIndex]->IAMStreamSelect_Index, 0);
    m_audioStreams[disableIndex]->connected = false;
    m_audioStreams[disableIndex]->flags = 0;

    if (SUCCEEDED(m_pIAMStreamSelect->Enable(m_audioStreams[enableIndex]->IAMStreamSelect_Index, AMSTREAMSELECTENABLE_ENABLE)))
    {
      m_audioStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_audioStreams[enableIndex]->connected = true;
      CLog::Log(LOGDEBUG, "%s Successfully selected audio stream", __FUNCTION__);
    }
  } else {

    Com::SmartPtr<IPin> connectedToPin = NULL;
    HRESULT hr = S_OK;
#if 0
    CStdString filter = "";
    SVideoStreamIndexes indexes(0, iStream, GetSubtitle());

    if (SUCCEEDED(CFilterCoreFactory::GetAudioFilter(CDSPlayer::currentFileItem, filter,
      CFGLoader::IsUsingDXVADecoder(), &indexes)))
    {
      CFGFilterFile *f;
      if (! (f = CFilterCoreFactory::GetFilterFromName(filter, false)))
      {
        CLog::Log(LOGERROR, "%s The filter corresponding to the new rule doesn't exist. Using current audio decoder to decode audio.", __FUNCTION__);
        goto standardway;
      }
      else
      {
        if (CFGLoader::Filters.Audio.guid == f->GetCLSID())
          goto standardway; // Same filter

        // Where this audio decoder is connected ?
        Com::SmartPtr<IPin> audioDecoderOutputPin = DShowUtil::GetFirstPin(CFGLoader::Filters.Audio.pBF, PINDIR_OUTPUT); // first output pin of audio decoder
        Com::SmartPtr<IPin> audioDecoderConnectedToPin = NULL;
        hr = audioDecoderOutputPin->ConnectedTo(&audioDecoderConnectedToPin);

        if (! audioDecoderConnectedToPin)
          goto standardway;

        audioDecoderOutputPin = NULL;

        m_pGraph->Stop(); // Current position is kept by the graph

        // Remove the old decoder from the graph
        hr = m_pGraphBuilder->RemoveFilter(CFGLoader::Filters.Audio.pBF);
        if (SUCCEEDED(hr))
          CLog::Log(LOGDEBUG, "%s Removed old audio decoder from the graph", __FUNCTION__);

        // Delete filter
        m_audioStreams[disableIndex]->pUnk = NULL;
        m_audioStreams[enableIndex]->pUnk = NULL;
        CFGLoader::Filters.Audio.pBF.FullRelease();

        // Create the new audio decoder
        if (FAILED(f->Create(&CFGLoader::Filters.Audio.pBF)))
          goto standardway;

        // Change references
        CFGLoader::Filters.Audio.guid = f->GetCLSID();
        g_charsetConverter.wToUTF8(f->GetName(), CFGLoader::Filters.Audio.osdname);

        // Add the new one
        if (FAILED(hr = m_pGraphBuilder->AddFilter(CFGLoader::Filters.Audio.pBF, f->GetName())))
        {
          CLog::Log(LOGERROR, "%s Failed to add the new audio decoder. No more sound!", __FUNCTION__);
          goto done;
        }

        // Make connections
        m_audioStreams[enableIndex]->pUnk = DShowUtil::GetFirstPin(CFGLoader::Filters.Audio.pBF);
        hr = m_pGraphBuilder->ConnectDirect(m_audioStreams[enableIndex]->pObj, m_audioStreams[enableIndex]->pUnk, NULL);

        Com::SmartPtr<IPin> pin = NULL;
        pin = DShowUtil::GetFirstPin(CFGLoader::Filters.Audio.pBF, PINDIR_OUTPUT);
        hr = m_pGraphBuilder->ConnectDirect(pin, audioDecoderConnectedToPin, NULL);

        m_audioStreams[disableIndex]->flags = 0;
        m_audioStreams[disableIndex]->connected = false;

        m_audioStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
        m_audioStreams[enableIndex]->connected = true;

        CLog::Log(LOGINFO, "%s New audio decoder \"%s\" inserted because of rules configuration.", __FUNCTION__, CFGLoader::Filters.Audio.osdname.c_str());

        // Connections done, run the graph!
        goto done;
      }

    }
#endif

    /* Disable filter */
standardway:

    m_pGraph->Stop(); // Current position is kept by the graph

    IPin *newAudioStreamPin = m_audioStreams[enableIndex]->pObj; // Splitter pin
    IPin *oldAudioStreamPin = m_audioStreams[disableIndex]->pObj; // Splitter pin

    connectedToPin = m_audioStreams[disableIndex]->pUnk; // Audio dec pin
    if (! connectedToPin) // This case shouldn't happened
      hr = oldAudioStreamPin->ConnectedTo(&connectedToPin);

    if (FAILED(hr))
      goto done;

    m_audioStreams[enableIndex]->pUnk = connectedToPin;

    hr = m_pGraphBuilder->Disconnect(connectedToPin);
    if (FAILED(hr))
      goto done;
    hr = m_pGraphBuilder->Disconnect(oldAudioStreamPin);
    if (FAILED(hr))
    {
      /* Reconnect pin */
      m_pGraphBuilder->ConnectDirect(oldAudioStreamPin, connectedToPin, NULL);
      goto done;
    }

    m_audioStreams[disableIndex]->flags = 0;
    m_audioStreams[disableIndex]->connected = false;

    /* Enable filter */
    hr = m_pGraphBuilder->ConnectDirect(newAudioStreamPin, connectedToPin, NULL);
    if (FAILED(hr))
    {
      /* Reconnect previous working pins */
      m_pGraphBuilder->ConnectDirect(oldAudioStreamPin, connectedToPin, NULL);
      m_audioStreams[disableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_audioStreams[disableIndex]->connected = true;
      goto done;
    }

    m_audioStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
    m_audioStreams[enableIndex]->connected = true;

done:
    m_pGraph->Play();

    if (SUCCEEDED(hr))
      CLog::Log(LOGNOTICE, "%s Successfully changed audio stream", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Can't change audio stream", __FUNCTION__);
  }

  m_bChangingStream = false;
}

void CStreamsManager::LoadStreams()
{
  if (! m_init)
    return;

  CStdString splitterName;
  g_charsetConverter.wToUTF8(DShowUtil::GetFilterName(m_pSplitter), splitterName);

  CLog::Log(LOGDEBUG, "%s Looking for audio streams in %s splitter", __FUNCTION__, splitterName.c_str());

  /* Regex to rename audio stream */
  boost::ptr_vector<CRegExp> regex;
  IXMLDOMNodePtr

  std::auto_ptr<CRegExp> reg(new CRegExp(true));
  reg->RegComp("(.*?)(\\(audio.*?\\)|\\(subtitle.*?\\))"); // mkv source audio / subtitle
  regex.push_back(reg);

  reg.reset(new CRegExp(true));
  reg->RegComp(".* - (.*),.*\\(.*\\)"); // mpeg source audio / subtitle
  regex.push_back(reg);

  /* Does the splitter support IAMStreamSelect ?*/
  m_pIAMStreamSelect = NULL;
  HRESULT hr = m_pSplitter->QueryInterface(__uuidof(m_pIAMStreamSelect), (void **) &m_pIAMStreamSelect);
  if (SUCCEEDED(hr))
  {
    /* Yes */
    CLog::Log(LOGDEBUG, "%s Get IAMStreamSelect interface from %s", __FUNCTION__, splitterName.c_str());

    DWORD nStreams = 0, flags = 0, group = 0;
    WCHAR* wname = NULL;
    SStreamInfos* infos = NULL;
    LCID lcid;
    IUnknown *pObj = NULL, *pUnk = NULL;
    int j = 0;

    m_pIAMStreamSelect->Count(&nStreams);

    AM_MEDIA_TYPE * mediaType = NULL;
    for(unsigned int i = 0; i < nStreams; i++)
    {
      m_pIAMStreamSelect->Info(i, &mediaType, &flags, &lcid, &group, &wname, &pObj, &pUnk);

      if (mediaType->majortype == MEDIATYPE_Video)
        infos = &m_videoStream;
      else if (mediaType->majortype == MEDIATYPE_Audio)
        infos = new SAudioStreamInfos();
      else if (mediaType->majortype == MEDIATYPE_Subtitle)
        infos = new SSubtitleStreamInfos();
      else
        continue;

      infos->IAMStreamSelect_Index = i;

      g_charsetConverter.wToUTF8(wname, infos->name);
      CoTaskMemFree(wname);

      infos->flags = flags; infos->lcid = lcid; infos->group = group; infos->pObj = (IPin *)pObj; infos->pUnk = (IPin *)pUnk;

      /* Apply regex */
      for (boost::ptr_vector<CRegExp>::iterator it = regex.begin(); it != regex.end(); ++it)
      {
        if ( it->RegFind(infos->name) > -1 )
        {
          infos->name = it->GetMatch(1);
          break;
        }
      }

      GetStreamInfos(mediaType, infos);

      if (mediaType->majortype == MEDIATYPE_Audio)
      {
        /* Audio stream */
        if (infos->name.find("Undetermined") != std::string::npos )
          infos->name.Format("A: Audio %02d", i + 1);

        m_audioStreams.push_back(reinterpret_cast<SAudioStreamInfos *>(infos));
        CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());
      } else if (mediaType->majortype == MEDIATYPE_Subtitle)
      {
        m_subtitleStreams.push_back(reinterpret_cast<SSubtitleStreamInfos *>(infos));
        CLog::Log(LOGNOTICE, "%s Subtitle stream found : %s", __FUNCTION__, infos->name.c_str());
      }

      DeleteMediaType(mediaType);
    }
  } 
  else 
  {
    /* No */  
  
    // Enumerate output pins
    PIN_DIRECTION dir;
    int i = 0, j = 0;
    CStdStringW pinNameW;
    CStdString pinName;
    SStreamInfos *infos = NULL;
    bool audioPinAlreadyConnected = FALSE;//, subtitlePinAlreadyConnected = FALSE;

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

        BeginEnumMediaTypes(pPin, pET, pMediaType)
        {

          CLog::Log(LOGDEBUG, "%s \tOutput pin major type : %s", __FUNCTION__, GuidNames[pMediaType->majortype]);
          CLog::Log(LOGDEBUG, "%s \tOutput pin sub type : %s", __FUNCTION__, GuidNames[pMediaType->subtype]);
          CLog::Log(LOGDEBUG, "%s \tOutput pin format type : %s", __FUNCTION__, GuidNames[pMediaType->formattype]);

          if (pMediaType->majortype == MEDIATYPE_Video)
            infos = &m_videoStream;
          else if (pMediaType->majortype == MEDIATYPE_Audio)
            infos = new SAudioStreamInfos();
          else if (pMediaType->majortype == MEDIATYPE_Subtitle)
            infos = new SSubtitleStreamInfos();
          else
            continue;

          GetStreamInfos(pMediaType, infos);
          if (infos->name.empty())
          {
            infos->name = pinName;
          }

          /* Apply regex */
          for (boost::ptr_vector<CRegExp>::iterator it = regex.begin(); it != regex.end(); ++it)
          {
            if ( it->RegFind(infos->name) > -1 )
            {
              infos->name = it->GetMatch(1);
              break;
            }
          }

          infos->pObj = pPin;
          pPin->ConnectedTo(&infos->pUnk);

          if (pMediaType->majortype == MEDIATYPE_Audio)
          {
            if (infos->name.find("Undetermined") != std::string::npos )
              infos->name.Format("Audio %02d", i + 1);

            if (i == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;
            if (infos->pUnk)
              infos->connected = true;

            m_audioStreams.push_back(reinterpret_cast<SAudioStreamInfos *>(infos));
            CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());
            i++;
            
          } else if (pMediaType->majortype == MEDIATYPE_Subtitle)
          {
            if (infos->name.find("Undetermined") != std::string::npos )
              infos->name.Format("Subtitle %02d", j + 1);

            if (j == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;
            if (infos->pUnk)
              infos->connected = true;

            m_subtitleStreams.push_back(reinterpret_cast<SSubtitleStreamInfos *>(infos));
            CLog::Log(LOGNOTICE, "%s Subtitle stream found : %s", __FUNCTION__, infos->name.c_str());
            j++;
          }

          break; // if the pin has multiple output type, get only the first one
        }
        EndEnumMediaTypes(pMediaType)
      }
    }
    EndEnumPins
  }

  ///* Delete regex */
  //while (! regex.empty())
  //{
  //  delete regex.back();
  //  regex.pop_back();
  //}
  regex.clear();

  /* We're done, internal audio & subtitles stream are loaded.
     We load external subtitle file */

  std::vector<std::string> subtitles;
  CDVDFactorySubtitle::GetSubtitles(subtitles, CDSPlayer::currentFileItem.m_strPath);

  SExternalSubtitleInfos *s = NULL;
  
  for (std::vector<std::string>::const_iterator it = subtitles.begin(); it != subtitles.end(); ++it)
  {
    AddSubtitle(*it);
  }
  g_settings.m_currentVideoSettings.m_SubtitleCached = true;
}

bool CStreamsManager::InitManager(CDSGraph *DSGraph)
{
  if (! DSGraph)
    return false;

  m_pSplitter = CFGLoader::Filters.Splitter.pBF;
  m_pGraphBuilder = CDSGraph::m_pFilterGraph;
  m_pGraph = DSGraph;
  m_bSubtitlesVisible = g_settings.m_currentVideoSettings.m_SubtitleOn;
  m_init = true;

  SetSubtitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);
  g_settings.m_currentVideoSettings.m_SubtitleCached = true;

  return true;
}

void CStreamsManager::Destroy()
{
  delete m_pSingleton;
  m_pSingleton = NULL;
}

bool CStreamsManager::IsChangingStream()
{
  return m_bChangingStream;
}

int CStreamsManager::GetChannels()
{
  int i = GetAudioStream();
  return (i == -1) ? 0 : m_audioStreams[i]->channels;
}

int CStreamsManager::GetBitsPerSample()
{
  int i = GetAudioStream();
  return (i == -1) ? 0 : m_audioStreams[i]->bitrate;
}

int CStreamsManager::GetSampleRate()
{
  int i = GetAudioStream();
  return (i == -1) ? 0 : m_audioStreams[i]->samplerate;
}

void CStreamsManager::GetStreamInfos( AM_MEDIA_TYPE *pMediaType, SStreamInfos *s )
{
  if (pMediaType->majortype == MEDIATYPE_Audio)
  {
    SAudioStreamInfos *infos = reinterpret_cast<SAudioStreamInfos *>(s);
    if (pMediaType->formattype == FORMAT_WaveFormatEx)
    {
      if (pMediaType->cbFormat >= sizeof(WAVEFORMATEX))
      {
        WAVEFORMATEX *f = reinterpret_cast<WAVEFORMATEX *>(pMediaType->pbFormat);
        infos->channels = f->nChannels;
        infos->samplerate = f->nSamplesPerSec;
        infos->bitrate = f->nAvgBytesPerSec;
        infos->codecname = CMediaTypeEx::GetAudioCodecName(pMediaType->subtype, f->wFormatTag);
      }
    } 
    else if (pMediaType->formattype == FORMAT_VorbisFormat2)
    {
      if (pMediaType->cbFormat >= sizeof(VORBISFORMAT2))
      {
        VORBISFORMAT2 *v = reinterpret_cast<VORBISFORMAT2 *>(pMediaType->pbFormat);
        infos->channels = v->Channels;
        infos->samplerate = v->SamplesPerSec;
        infos->bitrate = v->BitsPerSample;
        infos->codecname = CMediaTypeEx::GetAudioCodecName(pMediaType->subtype, 0);
      }
    } else if (pMediaType->formattype == FORMAT_VorbisFormat)
    {
      if (pMediaType->cbFormat >= sizeof(VORBISFORMAT))
      {
        VORBISFORMAT *v = reinterpret_cast<VORBISFORMAT *>(pMediaType->pbFormat);
        infos->channels = v->nChannels;
        infos->samplerate = v->nSamplesPerSec;
        infos->bitrate = v->nAvgBitsPerSec;
        infos->codecname = CMediaTypeEx::GetAudioCodecName(pMediaType->subtype, 0);
      }
    }
  } 
  else if (pMediaType->majortype == MEDIATYPE_Video)
  {
    SVideoStreamInfos *infos = reinterpret_cast<SVideoStreamInfos *>(s);

    if (pMediaType->formattype == FORMAT_VideoInfo)
    {
      if (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER))
      {
        VIDEOINFOHEADER *v = reinterpret_cast<VIDEOINFOHEADER *>(pMediaType->pbFormat);
        infos->width = v->bmiHeader.biWidth;
        infos->height = v->bmiHeader.biHeight;
        infos->codecname = CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, v->bmiHeader.biCompression, &infos->fourcc);
      }
    } 
    else if (pMediaType->formattype == FORMAT_MPEG2Video)
    {
      if (pMediaType->cbFormat >= sizeof(MPEG2VIDEOINFO))
      {
        MPEG2VIDEOINFO *m = reinterpret_cast<MPEG2VIDEOINFO *>(pMediaType->pbFormat);
        infos->width = m->hdr.bmiHeader.biWidth;
        infos->height = m->hdr.bmiHeader.biHeight;
        infos->codecname = CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, m->hdr.bmiHeader.biCompression, &infos->fourcc);
        if (infos->fourcc == 0)
        {
          infos->fourcc = 'MPG2';
          infos->codecname = "MPEG2 Video";
        }
      }
    } 
    else if (pMediaType->formattype == FORMAT_VideoInfo2)
    {
      if (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2))
      {
        VIDEOINFOHEADER2 *v = reinterpret_cast<VIDEOINFOHEADER2 *>(pMediaType->pbFormat);
        infos->width = v->bmiHeader.biWidth;
        infos->height = v->bmiHeader.biHeight;
        infos->codecname = CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, v->bmiHeader.biCompression, &infos->fourcc);
      }
    } 
    else if (pMediaType->formattype == FORMAT_MPEGVideo)
    {
      if (pMediaType->cbFormat >= sizeof(MPEG1VIDEOINFO))
      {
        MPEG1VIDEOINFO *m = reinterpret_cast<MPEG1VIDEOINFO *>(pMediaType->pbFormat);
        infos->width = m->hdr.bmiHeader.biWidth;
        infos->height = m->hdr.bmiHeader.biHeight;
        infos->codecname = CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, m->hdr.bmiHeader.biCompression, &infos->fourcc);
      }
    }
  } 
  else if (pMediaType->majortype == MEDIATYPE_Subtitle)
  {
    SSubtitleStreamInfos *infos = reinterpret_cast<SSubtitleStreamInfos *>(s);

    if (pMediaType->formattype == FORMAT_SubtitleInfo
      && pMediaType->cbFormat >= sizeof(SUBTITLEINFO))
    {
      SUBTITLEINFO *i = reinterpret_cast<SUBTITLEINFO *>(pMediaType->pbFormat);
      infos->isolang = i->IsoLang;
      infos->offset = i->dwOffset;
      infos->name = i->TrackName;
      if (infos->name.empty())
        infos->name = ISOToLanguage(infos->isolang);
      else 
        infos->name.Format("%s [%s]", infos->name, ISOToLanguage(infos->isolang));
    }
    infos->subtype = pMediaType->subtype;
  }
}

int CStreamsManager::GetPictureWidth()
{
  return m_videoStream.width;
}

int CStreamsManager::GetPictureHeight()
{
  return m_videoStream.height;
}

CStdString CStreamsManager::GetAudioCodecName()
{
  int i = GetAudioStream();
  return (i == -1) ? "" : m_audioStreams[i]->codecname;
}

CStdString CStreamsManager::GetVideoCodecName()
{
  return m_videoStream.codecname;
}

int CStreamsManager::GetSubtitleCount()
{
  return m_subtitleStreams.size();
}

int CStreamsManager::GetSubtitle()
{
  if (m_subtitleStreams.size() == 0)
    return -1;

  int i = 0;
  for (std::vector<SSubtitleStreamInfos *>::const_iterator it = m_subtitleStreams.begin();
    it != m_subtitleStreams.end(); ++it, i++)
  {
    if ( (*it)->flags == AMSTREAMSELECTINFO_ENABLED)
      return i;
  }

  return -1;
}

void CStreamsManager::GetSubtitleName( int iStream, CStdString &strStreamName )
{
  if (m_subtitleStreams.size() == 0)
    return;

  int i = 0;
  for (std::vector<SSubtitleStreamInfos *>::const_iterator it = m_subtitleStreams.begin();
    it != m_subtitleStreams.end(); ++it, i++)
  {
    if (i == iStream)
    {
      strStreamName = (*it)->name;
    }
  }
}

void CStreamsManager::SetSubtitle( int iStream )
{
  if (! m_init || iStream > GetSubtitleCount())
    return;

  long disableIndex = GetSubtitle(), enableIndex = iStream;

  if (m_bSubtitlesVisible && m_subtitleStreams[enableIndex]->connected)
  {
    // The new subtitle stream is already connected, return
    return;
  }

  m_bChangingStream = true;
  bool stopped = false;
  CStdString subtitlePath = "";
  Com::SmartPtr<IPin> newAudioStreamPin;

  if (m_subtitleStreams[enableIndex]->external)
  {
    /* External subtitle */
    DisconnectCurrentSubtitlePins();

    if (! m_bSubtitlesVisible)
    {
      m_bChangingStream = false;
      return;
    }

    SExternalSubtitleInfos *s = reinterpret_cast<SExternalSubtitleInfos *>(m_subtitleStreams[enableIndex]);

    if (g_dsconfig.LoadffdshowSubtitles(s->path))
      CLog::Log(LOGNOTICE, "%s Using \"%s\" for external subtitle", __FUNCTION__, s->path.c_str());
    
    s->flags = AMSTREAMSELECTINFO_ENABLED; // for gui
    s->connected = true;

    m_bChangingStream = false;
    return;
  }

  if (m_pIAMStreamSelect)
  {

    /*int i = 0; long lIndex = -1;
    for (std::vector<SSubtitleStreamInfos *>::iterator it = m_subtitleStreams.begin();
      it != m_subtitleStreams.end(); ++it, i++)
    {
      if (iStream == i)
        lIndex = (*it)->IAMStreamSelect_Index;
      if ((*it)->flags == AMSTREAMSELECTINFO_ENABLED)
      {
        m_pIAMStreamSelect->Enable((*it)->IAMStreamSelect_Index, 0);
        (*it)->flags = 0;
        if (lIndex != -1)
          break;
      }
    }*/

    if (m_subtitleStreams[disableIndex]->connected)
      DisconnectCurrentSubtitlePins();

    if (! m_bSubtitlesVisible)
    {
      // If subtitles aren't visible, only disconnect the subtitle track,
      // and don't connect the new one. We change the flag for the xbmc gui
      m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_bChangingStream = false;
      return;
    }

    if (SUCCEEDED(m_pIAMStreamSelect->Enable(m_subtitleStreams[enableIndex]->IAMStreamSelect_Index, AMSTREAMSELECTENABLE_ENABLE)))
    {
      m_audioStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_audioStreams[enableIndex]->connected = true;
      CLog::Log(LOGDEBUG, "%s Successfully selected subtitle stream", __FUNCTION__);
    }
  } 
  else 
  {
    
    Com::SmartPtr<IPin> connectedToPin = NULL;
    Com::SmartPtr<IPin> oldAudioStreamPin = NULL;
    HRESULT hr = S_OK;

    m_pGraph->Stop(); // Current position is kept by the graph
    stopped = true;
    
    /* Disconnect pins */
    if (! m_subtitleStreams[disableIndex]->external)
    {
      
      oldAudioStreamPin = m_subtitleStreams[disableIndex]->pObj;
      hr = oldAudioStreamPin->ConnectedTo(&connectedToPin);

      if (connectedToPin)
        hr = m_pGraphBuilder->Disconnect(connectedToPin);
      
      m_pGraphBuilder->Disconnect(oldAudioStreamPin);
    } else {
      // Store subtitle path for error fallback
      subtitlePath = ((SExternalSubtitleInfos *)(m_subtitleStreams[disableIndex]))->path;
    }

    m_subtitleStreams[disableIndex]->flags = 0;
    m_subtitleStreams[disableIndex]->connected = false;

    if (! m_bSubtitlesVisible)
    {
      // If subtitles aren't visible, don't connect pins. Only change the flag
      // for the xbmc gui
      m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_subtitleStreams[enableIndex]->connected = false;
      goto done;
    }


    if (! connectedToPin)
    {
      m_subtitleMediaType.subtype = m_subtitleStreams[enableIndex]->subtype;
      connectedToPin = GetFirstSubtitlePin(); // Find where connect the subtitle
    }

    if (! connectedToPin)
    {
      /* Nothing support subtitles! */
      CLog::Log(LOGERROR, "%s You're trying to use subtitle, but no filters in the graph seems to accept subtitles!", __FUNCTION__);
      hr = E_FAIL;
      goto done;
    }

    /* Enable filter */
    newAudioStreamPin = m_subtitleStreams[enableIndex]->pObj;

    hr = m_pGraphBuilder->ConnectDirect(newAudioStreamPin, connectedToPin, NULL);
    if (FAILED(hr))
    {
      /* Reconnect previous working pins */
      if (! m_subtitleStreams[disableIndex]->external)
        m_pGraphBuilder->ConnectDirect(oldAudioStreamPin, connectedToPin, NULL);
      else
        g_dsconfig.LoadffdshowSubtitles(subtitlePath.c_str());

      m_subtitleStreams[disableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_subtitleStreams[disableIndex]->connected = true;

      goto done;
    }

    m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
    m_subtitleStreams[enableIndex]->connected = true;

done:
    if (stopped)
      m_pGraph->Play();

    if (m_bSubtitlesVisible)
    {
      if (SUCCEEDED(hr))
        CLog::Log(LOGNOTICE, "%s Successfully changed subtitle stream", __FUNCTION__);
      else
        CLog::Log(LOGERROR, "%s Can't change subtitle stream", __FUNCTION__);
    }
  }

  m_bChangingStream = false;
}

bool CStreamsManager::GetSubtitleVisible()
{
  return m_bSubtitlesVisible;
}

void CStreamsManager::SetSubtitleVisible( bool bVisible )
{
  g_settings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  if (g_dsconfig.pIffdshowDecoder)
  {
    g_dsconfig.pIffdshowDecoder->compat_putParam(IDFF_isSubtitles, (int)bVisible); // Show or not subtitles
  }
  m_bSubtitlesVisible = bVisible;

  // Needed to unsure that pins are disconnected
  int i = GetSubtitle();
  if (i >= 0)
    SetSubtitle(i);
}

int CStreamsManager::AddSubtitle(const CStdString& subFilePath)
{
  if (g_dsconfig.pIffdshowDecoder) // We're currently using ffdshow for subtitles
  {    
    std::auto_ptr<SExternalSubtitleInfos> s(new SExternalSubtitleInfos());

    if (m_subtitleStreams.empty())
      s->flags = AMSTREAMSELECTINFO_ENABLED;

    s->external = true; 
    s->path = CSpecialProtocol::TranslatePath(subFilePath);
    if (! XFILE::CFile::Exists(s->path))
      return -1;

    // Try to detect isolang of subtitle
    CRegExp regex(true);
    regex.RegComp("^.*\\.(.*)\\.[^\\.]*.*$");
    if (regex.RegFind(s->path) > -1)
      s->isolang = regex.GetMatch(1);

    if (! s->isolang.empty())
    {
      s->name = ISOToLanguage(s->isolang);
      if (s->name.empty())
        s->name = CUtil::GetFileName(s->path);
      else
        s->name += " [External]";
    } 
    else
      s->name = CUtil::GetFileName(s->path);

    m_subtitleStreams.push_back(s.release());

    return m_subtitleStreams.size() - 1;
  }

  return -1;
}

void CStreamsManager::DisconnectCurrentSubtitlePins( void )
{
  int i = GetSubtitle();
  if (i == -1)
    return;

  if (m_pIAMStreamSelect)
  {
    m_pIAMStreamSelect->Enable(m_subtitleStreams[i]->IAMStreamSelect_Index, 0);
    m_subtitleStreams[i]->connected = false;
    m_subtitleStreams[i]->flags = 0;
  } 
  else 
  {
    if (m_subtitleStreams[i]->connected && !m_subtitleStreams[i]->external)
    {
      Com::SmartPtr<IPin> pin = NULL;

      if (VFW_E_NOT_CONNECTED == m_subtitleStreams[i]->pObj->ConnectedTo(&pin))
      {
        // Shouldn't happened!
        m_subtitleStreams[i]->connected = false;
        m_subtitleStreams[i]->flags = 0;
        return;
      }

      m_pGraph->Stop();
      m_pGraphBuilder->Disconnect(m_subtitleStreams[i]->pObj); // Splitter's output pin
      m_pGraphBuilder->Disconnect(pin); // Filter's input pin
      m_pGraph->Play();
    }
    m_subtitleStreams[i]->connected = false;
    m_subtitleStreams[i]->flags = 0;
  }
  //m_bSubtitlesUnconnected = true;
}

void CStreamsManager::SetSubtitleDelay( float fValue )
{
  int delaysub = (int) -fValue * 1000; //1000 is a millisec
  if (g_dsconfig.pIffdshowDecoder)
    g_dsconfig.pIffdshowDecoder->compat_putParam(IDFF_subDelay, delaysub);
}

float CStreamsManager::GetSubtitleDelay( void )
{
  if (g_dsconfig.pIffdshowDecoder)
  {
    int delaySub = 0;
    g_dsconfig.pIffdshowDecoder->compat_getParam(IDFF_subDelay, &delaySub);
    return (float) delaySub;
  } else
    return 0.0f;
}

IPin *CStreamsManager::GetFirstSubtitlePin( void )
{
  PIN_DIRECTION  pindir;
  if (g_dsconfig.pIffdshowDecoder)
    g_dsconfig.pIffdshowDecoder->compat_putParam(IDFF_subTextpin, 1); // ffdshow accept internal subtitle

  BeginEnumFilters(CDSGraph::m_pFilterGraph, pEF, pBF)
  {
    BeginEnumPins(pBF, pEP, pPin)
    {
      Com::SmartPtr<IPin> pFellow;

      if (SUCCEEDED (pPin->QueryDirection(&pindir)) &&
        pindir == PINDIR_INPUT && pPin->ConnectedTo(&pFellow) == VFW_E_NOT_CONNECTED)
      {
        if (pPin->QueryAccept(&m_subtitleMediaType) == S_OK)
          return (pPin);
      }
    }
    EndEnumPins
  }
  EndEnumFilters

  return NULL;
}

CStdString CStreamsManager::ISOToLanguage( CStdString code )
{
  if (code.length() == 2)
    return DShowUtil::ISO6391ToLanguage(code);
  else
    return DShowUtil::ISO6392ToLanguage(code);
}

SVideoStreamInfos * CStreamsManager::GetVideoStreamInfos( unsigned int iIndex /*= 0*/ )
{
  /* We currently supports only one video stream */
  if (iIndex != 0)
    return NULL;

  return &m_videoStream;
}

SAudioStreamInfos * CStreamsManager::GetAudioStreamInfos( unsigned int iIndex /*= 0*/ )
{
  if (iIndex > m_audioStreams.size())
    return NULL;

  return m_audioStreams[iIndex];
}

SSubtitleStreamInfos * CStreamsManager::GetSubtitleStreamInfos( unsigned int iIndex /*= 0*/ )
{
  if (iIndex > m_audioStreams.size())
    return NULL;

  if (m_subtitleStreams[iIndex]->external)
    return NULL;

  return m_subtitleStreams[iIndex];
}

SExternalSubtitleInfos* CStreamsManager::GetExternalSubtitleStreamInfos( unsigned int iIndex /*= 0*/ )
{
  if (iIndex > m_audioStreams.size())
    return NULL;

  if (!m_subtitleStreams[iIndex]->external)
    return NULL;

  return reinterpret_cast<SExternalSubtitleInfos *>(m_subtitleStreams[iIndex]);
}