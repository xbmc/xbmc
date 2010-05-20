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
#include "WindowingFactory.h"
#include "LangInfo.h"
#include "GUISettings.h"

CStreamsManager *CStreamsManager::m_pSingleton = NULL;

CStreamsManager *CStreamsManager::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CStreamsManager());
}

CStreamsManager::CStreamsManager(void):
  m_pIAMStreamSelect(NULL), m_init(false), m_bChangingStream(false),
  m_dvdStreamLoaded(false)
{
}

CStreamsManager::~CStreamsManager(void)
{

  while (! m_audioStreams.empty())
  {
    delete m_audioStreams.back();
    m_audioStreams.pop_back();
  }

  m_pSplitter = NULL;
  m_pGraphBuilder = NULL;

  CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);

}

std::vector<SAudioStreamInfos *>& CStreamsManager::GetAudios()
{
  return m_audioStreams;
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
      strStreamName = (*it)->displayname;
    }
  }
}

void CStreamsManager::SetAudioStream(int iStream)
{
  if (! m_init)
    return;

  if (CFGLoader::Filters.isDVD)
    return; // currently not implemented

  CSingleLock lock(m_lock);

  long disableIndex = GetAudioStream(), enableIndex = iStream;

  if (disableIndex == enableIndex
    || ! m_audioStreams[disableIndex]->connected
    || m_audioStreams[enableIndex]->connected)
    return;

  m_bChangingStream = true;

  if (m_pIAMStreamSelect)
  {

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

        // Update configuration
        g_dsconfig.ConfigureFilters();

        // Connections done, run the graph!
        goto done;
      }

    }
#endif

    /* Disable filter */
//standardway:

    g_dsGraph->Stop(); // Current position is kept by the graph

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
    g_dsGraph->Play();

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
  std::vector<boost::shared_ptr<CRegExp>> regex;

  boost::shared_ptr<CRegExp> reg(new CRegExp(true));
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

      g_charsetConverter.wToUTF8(wname, infos->displayname);
      CoTaskMemFree(wname);

      infos->flags = flags; infos->lcid = lcid; infos->group = group; infos->pObj = (IPin *)pObj; infos->pUnk = (IPin *)pUnk;
      if (flags == AMSTREAMSELECTINFO_ENABLED)
        infos->connected = true;

      /* Apply regex */
      for (std::vector<boost::shared_ptr<CRegExp>>::iterator it = regex.begin(); it != regex.end(); ++it)
      {
        if ( (*it)->RegFind(infos->displayname) > -1 )
        {
          infos->displayname = (*it)->GetMatch(1);
          break;
        }
      }

      GetStreamInfos(mediaType, infos);

      if (mediaType->majortype == MEDIATYPE_Audio)
      {
        /* Audio stream */
        if (infos->displayname.find("Undetermined") != std::string::npos )
          infos->displayname.Format("A: Audio %02d", i + 1);

        m_audioStreams.push_back(reinterpret_cast<SAudioStreamInfos *>(infos));
        CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->displayname.c_str());
      } else if (mediaType->majortype == MEDIATYPE_Subtitle)
      {
        SubtitleManager->GetSubtitles().push_back(reinterpret_cast<SSubtitleStreamInfos *>(infos));
        CLog::Log(LOGNOTICE, "%s Subtitle stream found : %s", __FUNCTION__, infos->displayname.c_str());
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
    bool audioPinAlreadyConnected = false;//, subtitlePinAlreadyConnected = FALSE;

    // If we're playing a DVD, the bottom method doesn't work
    if (CFGLoader::Filters.isDVD)
    { // The playback need to be started in order to get informations
      return;
    }

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
          if (infos->displayname.empty())
          {
            infos->displayname = pinName;
          }

          /* Apply regex */
          for (std::vector<boost::shared_ptr<CRegExp>>::iterator it = regex.begin(); it != regex.end(); ++it)
          {
            if ( (*it)->RegFind(infos->displayname) > -1 )
            {
              infos->displayname = (*it)->GetMatch(1);
              break;
            }
          }

          infos->pObj = pPin;
          pPin->ConnectedTo(&infos->pUnk);

          if (pMediaType->majortype == MEDIATYPE_Audio)
          {
            if (infos->displayname.find("Undetermined") != std::string::npos )
              infos->displayname.Format("Audio %02d", i + 1);

            if (i == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;
            if (infos->pUnk)
              infos->connected = true;

            m_audioStreams.push_back(reinterpret_cast<SAudioStreamInfos *>(infos));
            CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->displayname.c_str());
            i++;
            
          } else if (pMediaType->majortype == MEDIATYPE_Subtitle)
          {
            if (infos->displayname.find("Undetermined") != std::string::npos )
              infos->displayname.Format("Subtitle %02d", j + 1);

            if (j == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;
            if (infos->pUnk)
              infos->connected = true;

            SubtitleManager->GetSubtitles().push_back(reinterpret_cast<SSubtitleStreamInfos *>(infos));
            CLog::Log(LOGNOTICE, "%s Subtitle stream found : %s", __FUNCTION__, infos->displayname.c_str());
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
  regex.clear();

  SubtitleManager->Initialize();
  if (! SubtitleManager->Ready())
  {
    SubtitleManager->Unload();
    SubtitleManager->SetSubtitleVisible(g_settings.m_currentVideoSettings.m_SubtitleOn);

    return;
  } 
  
  /* We're done, internal audio & subtitles stream are loaded.
     We load external subtitle file */

  std::vector<std::string> subtitles;
  CDVDFactorySubtitle::GetSubtitles(subtitles, CDSPlayer::currentFileItem.m_strPath);

  for (std::vector<std::string>::const_iterator it = subtitles.begin(); it != subtitles.end(); ++it)
  {
    SubtitleManager->AddSubtitle(*it);
  }
  g_settings.m_currentVideoSettings.m_SubtitleCached = true;

  // TODO: Select subtitle based on user pref
  SubtitleManager->SetSubtitle(0);

  SubtitleManager->SetSubtitleVisible(g_settings.m_currentVideoSettings.m_SubtitleOn);
}

bool CStreamsManager::InitManager()
{
  if (! g_dsGraph)
    return false;

  m_pSplitter = CFGLoader::Filters.Splitter.pBF;
  m_pGraphBuilder = g_dsGraph->pFilterGraph;

  // Create subtitle manager
  SubtitleManager.reset(new CSubtitleManager(this));

  m_init = true;

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

void CStreamsManager::ExtractCodecInfos(SStreamInfos& s, CStdString& codecInfos)
{
  std::vector<CStdString> tokens;
  codecInfos.Tokenize("|", tokens);

  if (tokens.empty() || tokens.size() != 2)
  {
    s.codecname = s.codec = codecInfos;
    return;
  }

  s.codecname = tokens[0];
  s.codec = tokens[1];
}

void CStreamsManager::GetStreamInfos( AM_MEDIA_TYPE *pMediaType, SStreamInfos *s )
{
  if (pMediaType->majortype == MEDIATYPE_Audio)
  {
    SAudioStreamInfos* infos = reinterpret_cast<SAudioStreamInfos *>(s);
    if (pMediaType->formattype == FORMAT_WaveFormatEx)
    {
      if (pMediaType->cbFormat >= sizeof(WAVEFORMATEX))
      {
        WAVEFORMATEX *f = reinterpret_cast<WAVEFORMATEX *>(pMediaType->pbFormat);
        infos->channels = f->nChannels;
        infos->samplerate = f->nSamplesPerSec;
        infos->bitrate = f->nAvgBytesPerSec;
        ExtractCodecInfos(*infos, CMediaTypeEx::GetAudioCodecName(pMediaType->subtype, f->wFormatTag));
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
        ExtractCodecInfos(*infos, CMediaTypeEx::GetAudioCodecName(pMediaType->subtype, 0));
      }
    } else if (pMediaType->formattype == FORMAT_VorbisFormat)
    {
      if (pMediaType->cbFormat >= sizeof(VORBISFORMAT))
      {
        VORBISFORMAT *v = reinterpret_cast<VORBISFORMAT *>(pMediaType->pbFormat);
        infos->channels = v->nChannels;
        infos->samplerate = v->nSamplesPerSec;
        infos->bitrate = v->nAvgBitsPerSec;
        ExtractCodecInfos(*infos, CMediaTypeEx::GetAudioCodecName(pMediaType->subtype, 0));
      }
    }
  } 
  else if (pMediaType->majortype == MEDIATYPE_Video)
  {
    SVideoStreamInfos* infos = reinterpret_cast<SVideoStreamInfos* >(s);

    if (pMediaType->formattype == FORMAT_VideoInfo)
    {
      if (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER))
      {
        VIDEOINFOHEADER *v = reinterpret_cast<VIDEOINFOHEADER *>(pMediaType->pbFormat);
        infos->width = v->bmiHeader.biWidth;
        infos->height = v->bmiHeader.biHeight;
        ExtractCodecInfos(*infos, CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, v->bmiHeader.biCompression, &infos->fourcc));
      }
    } 
    else if (pMediaType->formattype == FORMAT_MPEG2Video)
    {
      if (pMediaType->cbFormat >= sizeof(MPEG2VIDEOINFO))
      {
        MPEG2VIDEOINFO *m = reinterpret_cast<MPEG2VIDEOINFO *>(pMediaType->pbFormat);
        infos->width = m->hdr.bmiHeader.biWidth;
        infos->height = m->hdr.bmiHeader.biHeight;
        ExtractCodecInfos(*infos, CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, m->hdr.bmiHeader.biCompression, &infos->fourcc));
        if (infos->fourcc == 0)
        {
          infos->fourcc = 'MPG2';
          infos->codecname = "MPEG2 Video";
          infos->codec = "mpeg2";
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
        ExtractCodecInfos(*infos, CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, v->bmiHeader.biCompression, &infos->fourcc));
      }
    } 
    else if (pMediaType->formattype == FORMAT_MPEGVideo)
    {
      if (pMediaType->cbFormat >= sizeof(MPEG1VIDEOINFO))
      {
        MPEG1VIDEOINFO *m = reinterpret_cast<MPEG1VIDEOINFO *>(pMediaType->pbFormat);
        infos->width = m->hdr.bmiHeader.biWidth;
        infos->height = m->hdr.bmiHeader.biHeight;
        ExtractCodecInfos(*infos, CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, m->hdr.bmiHeader.biCompression, &infos->fourcc));
      }
    }
  } 
  else if (pMediaType->majortype == MEDIATYPE_Subtitle)
  {
    SSubtitleStreamInfos* infos = reinterpret_cast<SSubtitleStreamInfos* >(s);

    if (pMediaType->formattype == FORMAT_SubtitleInfo
      && pMediaType->cbFormat >= sizeof(SUBTITLEINFO))
    {
      SUBTITLEINFO *i = reinterpret_cast<SUBTITLEINFO *>(pMediaType->pbFormat);
      infos->isolang = i->IsoLang;
      infos->offset = i->dwOffset;
      infos->trackname = i->TrackName;
      if (! infos->lcid)
        infos->lcid = DShowUtil::ISO6392ToLcid(i->IsoLang);
    }
    infos->subtype = pMediaType->subtype;
  }

  FormatStreamName(*s);
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
  return (i == -1) ? "" : m_audioStreams[i]->codec;
}

CStdString CStreamsManager::GetVideoCodecName()
{
  return m_videoStream.codec;
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

void CStreamsManager::FormatStreamName( SStreamInfos& s )
{
  // First, if lcid isn't 0, try GetLocalInfo
  if (s.lcid)
  {
    CStdString name; int len = 0;
    if (len = GetLocaleInfo(s.lcid, LOCALE_SLANGUAGE, name.GetBuffer(64), 64))
    {
      name.resize(len - 1); //get rid of last \0
      if (s.type == SUBTITLE && !((SSubtitleStreamInfos&)s).trackname.empty())
        s.displayname.Format("%s (%s)", name, ((SSubtitleStreamInfos&)s).trackname);
      else
        s.displayname = name;
    }
  }

  if (s.type == SUBTITLE && s.displayname.empty())
  {
    SSubtitleStreamInfos& c = ((SSubtitleStreamInfos&)s);
    CStdString name = ISOToLanguage(c.isolang);
    if (! c.trackname.empty())
      c.displayname.Format("%s (%s)", name, ((SSubtitleStreamInfos&)s).trackname);
    else
      c.displayname = name;
  }
}

CSubtitleManager::CSubtitleManager(CStreamsManager* pStreamManager):
  m_dll(), m_pStreamManager(pStreamManager), m_rtSubtitleDelay(0)
{
  m_dll.Load();

  memset(&m_subtitleMediaType, 0, sizeof(AM_MEDIA_TYPE));
  m_subtitleMediaType.majortype = MEDIATYPE_Subtitle;

  m_bSubtitlesVisible = g_settings.m_currentVideoSettings.m_SubtitleOn;
  SetSubtitleDelay(g_settings.m_currentVideoSettings.m_SubtitleDelay);
}

CSubtitleManager::~CSubtitleManager()
{
  Unload();
  m_dll.Unload();
}

#define FONT_STYLE_NORMAL       0
#define FONT_STYLE_BOLD         1
#define FONT_STYLE_ITALICS      2
#define FONT_STYLE_BOLD_ITALICS 3

static color_t color[8] = { 0x0000FFFF, 0x00FFFFFF, 0x00FF9900, 0x0000FF00, 0x00FFCC, 0x00FFFF00, 0x00E5E5E5, 0x00C0C0C0 };

void CSubtitleManager::Initialize()
{
  // Initialize subtitles
  // 1. Create Subtitle Manager

  SIZE s; s.cx = 0; s.cy = 0;
  ISubManager *pManager = NULL;

  // Log manager for the DLL
  m_Log.reset(new ILogImpl());

  m_dll.CreateSubtitleManager(g_Windowing.Get3DDevice(), s, m_Log.get(), &pManager);

  if (!pManager)
    return;

  m_pManager.reset(pManager, std::bind2nd(std::ptr_fun(DeleteSubtitleManager), m_dll));

  SSubStyle style;

  /*memset(style.colors, 0, sizeof(style.colors));
  memset(style.alpha, 0, sizeof(style.alpha));*/
  
  // Build style based on XBMC settings
  //subtitles.style subtitles.color subtitles.height subtitles.font subtitles.charset
  style.color = color[g_guiSettings.GetInt("subtitles.color")];
  style.alpha = g_guiSettings.GetInt("subtitles.alpha");

  g_graphicsContext.SetScalingResolution(RES_PAL_4x3, true);
  style.fontSize = (float) g_guiSettings.GetInt("subtitles.height") / g_graphicsContext.GetGUIScaleY();
  style.fontSize *= 26.6 / 72.0;

  int fontStyle = g_guiSettings.GetInt("subtitles.style");
  switch (fontStyle)
  {
  case FONT_STYLE_NORMAL:
  default:
    style.fItalic = false;
    style.fontWeight = FW_NORMAL;
    break;
  case FONT_STYLE_BOLD:
    style.fItalic = false;
    style.fontWeight = FW_BOLD;
    break;
  case FONT_STYLE_ITALICS:
    style.fontWeight = FW_NORMAL;
    style.fItalic = true;
    break;
  case FONT_STYLE_BOLD_ITALICS:
    style.fItalic = true;
    style.fontWeight = FW_BOLD;
    break;
  }

  style.charSet = g_charsetConverter.getCharsetIdByName(g_langInfo.GetSubtitleCharSet());

  style.borderStyle = g_guiSettings.GetInt("subtitles.border");
  style.shadowDepthX = style.shadowDepthY = g_guiSettings.GetInt("subtitles.shadow.depth");
  style.outlineWidthX = style.outlineWidthY = g_guiSettings.GetInt("subtitles.outline.width");
  
  CStdStringW fontName;
  g_charsetConverter.utf8ToW(g_guiSettings.GetString("subtitles.ds.font"), fontName);
  style.fontName = (wchar_t *) CoTaskMemAlloc(fontName.length() * sizeof(wchar_t) + 2);
  if (style.fontName)
    wcscpy_s(style.fontName, fontName.length() + 1, fontName.c_str());

  m_pManager->SetStyle(&style);

  if (FAILED(m_pManager->InsertPassThruFilter(g_dsGraph->pFilterGraph)))
  {
    // No internal subs
  } 
  m_pManager->SetEnable(true);
}

bool CSubtitleManager::Ready()
{
  return (!!m_pManager);
}

void CSubtitleManager::StopThread()
{
  if (!m_subtitleStreams.empty() && m_pManager)
    m_pManager->StopThread();
}

void CSubtitleManager::StartThread()
{
  if (!m_subtitleStreams.empty() && m_pManager)
    m_pManager->StartThread(g_Windowing.Get3DDevice());
}

void CSubtitleManager::Unload()
{
  if (m_pManager)
  {
    m_pManager->SetSubPicProvider(NULL);
    m_pManager->Free();
  }
  while (! m_subtitleStreams.empty())
  {
    if (m_subtitleStreams.back()->external)
      ((SExternalSubtitleInfos *) m_subtitleStreams.back())->substream.FullRelease();
    delete m_subtitleStreams.back();
    m_subtitleStreams.pop_back();
  }
  m_pManager.reset();
}

void CSubtitleManager::SetTimePerFrame( REFERENCE_TIME iTimePerFrame )
{
  if (m_pManager)
    m_pManager->SetTimePerFrame(iTimePerFrame);
}
void CSubtitleManager::SetTime(REFERENCE_TIME rtNow)
{
  if (m_pManager)
    m_pManager->SetTime(rtNow - m_rtSubtitleDelay);
}

HRESULT CSubtitleManager::GetTexture( Com::SmartPtr<IDirect3DTexture9>& pTexture, Com::SmartRect& pSrc, Com::SmartRect& pDest, Com::SmartRect& renderRect )
{
  if (m_pManager)
    return m_pManager->GetTexture(pTexture, pSrc, pDest, renderRect);
  return E_FAIL;
}

std::vector<SSubtitleStreamInfos *>& CSubtitleManager::GetSubtitles()
{
  return m_subtitleStreams;
}

int CSubtitleManager::GetSubtitleCount()
{
  return m_subtitleStreams.size();
}

int CSubtitleManager::GetSubtitle()
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

void CSubtitleManager::GetSubtitleName( int iStream, CStdString &strStreamName )
{
  if (m_subtitleStreams.size() == 0)
    return;

  int i = 0;
  for (std::vector<SSubtitleStreamInfos *>::const_iterator it = m_subtitleStreams.begin();
    it != m_subtitleStreams.end(); ++it, i++)
  {
    if (i == iStream)
    {
      strStreamName = (*it)->displayname;
    }
  }
}

void CSubtitleManager::SetSubtitle( int iStream )
{
  if (CFGLoader::Filters.isDVD)
    return; // currently not implemented

  CSingleLock lock(m_pStreamManager->m_lock);

  //If no subtitles just return
  if (GetSubtitleCount() <= 0)
    return;

  if (iStream > GetSubtitleCount())
    return;

  long disableIndex = GetSubtitle(), enableIndex = iStream;

  if (m_bSubtitlesVisible && m_subtitleStreams[enableIndex]->connected)
  {
    // The new subtitle stream is already connected, return
    return;
  }

  m_pStreamManager->m_bChangingStream = true;
  bool stopped = false;
  CStdString subtitlePath = "";
  Com::SmartPtr<IPin> newAudioStreamPin;

  if (m_subtitleStreams[enableIndex]->external)
  {
    /* External subtitle */
    DisconnectCurrentSubtitlePins();

    if (! m_bSubtitlesVisible)
    {
      m_pStreamManager->m_bChangingStream = false;
      return;
    }

    SExternalSubtitleInfos *s = reinterpret_cast<SExternalSubtitleInfos *>(m_subtitleStreams[enableIndex]);

    m_pManager->SetSubPicProvider(s->substream);
    
    s->flags = AMSTREAMSELECTINFO_ENABLED; // for gui
    s->connected = true;

    m_pStreamManager->m_bChangingStream = false;
    return;
  }

  if (m_pStreamManager->m_pIAMStreamSelect)
  {
    if (disableIndex >= 0 && m_subtitleStreams[disableIndex]->connected)
      DisconnectCurrentSubtitlePins();

    if (SUCCEEDED(m_pStreamManager->m_pIAMStreamSelect->Enable(m_subtitleStreams[enableIndex]->IAMStreamSelect_Index, AMSTREAMSELECTENABLE_ENABLE)))
    {
      m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_subtitleStreams[enableIndex]->connected = true;
      CLog::Log(LOGDEBUG, "%s Successfully selected subtitle stream", __FUNCTION__);
    }
  } 
  else 
  {
    
    Com::SmartPtr<IPin> connectedToPin = NULL;
    Com::SmartPtr<IPin> oldAudioStreamPin = NULL;
    HRESULT hr = S_OK;

    g_dsGraph->Stop(); // Current position is kept by the graph
    stopped = true;
    
    /* Disconnect pins */
    if (! m_subtitleStreams[disableIndex]->external)
    {
      
      oldAudioStreamPin = m_subtitleStreams[disableIndex]->pObj;
      hr = oldAudioStreamPin->ConnectedTo(&connectedToPin);

      if (connectedToPin)
        hr = m_pStreamManager->m_pGraphBuilder->Disconnect(connectedToPin);
      
      m_pStreamManager->m_pGraphBuilder->Disconnect(oldAudioStreamPin);
    }

    m_subtitleStreams[disableIndex]->flags = 0;
    m_subtitleStreams[disableIndex]->connected = false;

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

    hr = m_pStreamManager->m_pGraphBuilder->ConnectDirect(newAudioStreamPin, connectedToPin, NULL);
    if (FAILED(hr))
    {
      /* Reconnect previous working pins */
      if (! m_subtitleStreams[disableIndex]->external)
        m_pStreamManager->m_pGraphBuilder->ConnectDirect(oldAudioStreamPin, connectedToPin, NULL);

      m_subtitleStreams[disableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_subtitleStreams[disableIndex]->connected = true;

      goto done;
    }

    m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
    m_subtitleStreams[enableIndex]->connected = true;

    m_pManager->SetSubPicProviderToInternal();

done:
    if (stopped)
      g_dsGraph->Play();

    if (m_bSubtitlesVisible)
    {
      if (SUCCEEDED(hr))
        CLog::Log(LOGNOTICE, "%s Successfully changed subtitle stream", __FUNCTION__);
      else
        CLog::Log(LOGERROR, "%s Can't change subtitle stream", __FUNCTION__);
    }
  }

  m_pStreamManager->m_bChangingStream = false;
}

bool CSubtitleManager::GetSubtitleVisible()
{
  return m_bSubtitlesVisible;
}

void CSubtitleManager::SetSubtitleVisible( bool bVisible )
{
  g_settings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  m_bSubtitlesVisible = bVisible;
  if (m_pManager)
    m_pManager->SetEnable(bVisible);
}

int CSubtitleManager::AddSubtitle(const CStdString& subFilePath)
{
  if (! m_pManager)
    return -1;

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
    s->lcid = DShowUtil::ISO6392ToLcid(s->isolang);
    if (! s->lcid)
      s->lcid = DShowUtil::ISO6391ToLcid(s->isolang);

    m_pStreamManager->FormatStreamName(*s.get());
  } 

  if (s->displayname.empty())
    s->displayname = CUtil::GetFileName(s->path);
  else
    s->displayname += " [External]";

  // Load subtitle file
  Com::SmartPtr<ISubStream> pSubStream;
  
  CStdStringW unicodePath; g_charsetConverter.utf8ToW(s->path, unicodePath);

  CLog::Log(LOGNOTICE, "%s Loading subtitle file \"%s\"", __FUNCTION__, s->path.c_str());
  if (SUCCEEDED(m_pManager->LoadExternalSubtitle(unicodePath.c_str(), &pSubStream)))
  {
    s->substream = pSubStream;
    // if the sub is a vob, it has an internal title, grab it
    wchar_t* title = NULL;
    if (SUCCEEDED(m_pManager->GetStreamTitle(pSubStream, &title)))
    {
      g_charsetConverter.wToUTF8(CStdStringW(title), s->displayname);
      s->displayname += " [External]";
      CoTaskMemFree(title);
    }
    m_subtitleStreams.push_back(s.release());

    return m_subtitleStreams.size() - 1;
  }

  return -1;
}

void CSubtitleManager::DisconnectCurrentSubtitlePins( void )
{
  int i = GetSubtitle();
  if (i == -1)
    return;

  if (m_pStreamManager->m_pIAMStreamSelect)
  {
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

      g_dsGraph->Stop();
      m_pStreamManager->m_pGraphBuilder->Disconnect(m_subtitleStreams[i]->pObj); // Splitter's output pin
      m_pStreamManager->m_pGraphBuilder->Disconnect(pin); // Filter's input pin
      g_dsGraph->Play();
    }
    m_subtitleStreams[i]->connected = false;
    m_subtitleStreams[i]->flags = 0;
  }
  //m_bSubtitlesUnconnected = true;
}

void CSubtitleManager::SetSubtitleDelay( float fValue )
{
  m_rtSubtitleDelay = (int) (-fValue * 10000000i64); // second to directshow timebase
}

float CSubtitleManager::GetSubtitleDelay( void )
{
  return (float) m_rtSubtitleDelay / 10000000i64;
}

IPin *CSubtitleManager::GetFirstSubtitlePin( void )
{
  PIN_DIRECTION  pindir;

  BeginEnumFilters(g_dsGraph->pFilterGraph, pEF, pBF)
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

void CStreamsManager::LoadDVDStreams()
{
  Com::SmartPtr<IDvdInfo2> pDvdI = CFGLoader::Filters.DVD.dvdInfo;
  Com::SmartPtr<IDvdControl2> pDvdC = CFGLoader::Filters.DVD.dvdControl;

  DVD_VideoAttributes vid;
  // First, video
  HRESULT hr = pDvdI->GetCurrentVideoAttributes(&vid);
  if (SUCCEEDED(hr))
  {
    m_videoStream.width = vid.ulSourceResolutionX;
    m_videoStream.height = vid.ulSourceResolutionY;
    switch (vid.Compression)
    {
    case DVD_VideoCompression_MPEG1:
      m_videoStream.codecname = "MPEG1";
      break;
    case DVD_VideoCompression_MPEG2:
      m_videoStream.codecname = "MPEG2";
      break;
    default:
      m_videoStream.codecname = "Unknown";
    }
  }

  UpdateDVDStream();

}

void CStreamsManager::UpdateDVDStream()
{
  if (m_dvdStreamLoaded)
    return;

  // Second, audio
  unsigned long nbrStreams = 0, currentStream = 0;
  HRESULT hr = CFGLoader::Filters.DVD.dvdInfo->GetCurrentAudio(&nbrStreams, &currentStream);
  if (FAILED(hr))
    return;

  CSingleLock lock(m_lock);

  m_audioStreams.clear();
  for (unsigned int i = 0; i < nbrStreams; i++)
  {
    std::auto_ptr<SAudioStreamInfos> s(new SAudioStreamInfos());
    DVD_AudioAttributes audio;
    hr = CFGLoader::Filters.DVD.dvdInfo->GetAudioAttributes(i, &audio);

    s->channels = audio.bNumberOfChannels;
    s->samplerate = audio.dwFrequency;
    s->bitrate = 0;
    s->lcid = audio.Language;
    if (i == currentStream)
    {
      s->flags = AMSTREAMSELECTINFO_ENABLED;
      s->connected = true;
    }
    switch (audio.AudioFormat)
    {
    case DVD_AudioFormat_AC3:
      s->codecname = "Dolby AC3";
      break;
    case DVD_AudioFormat_MPEG1:
      s->codecname = "MPEG1";
      break;
    case DVD_AudioFormat_MPEG1_DRC:
      s->codecname = "MPEG1 (DCR)";
      break;
    case DVD_AudioFormat_MPEG2:
      s->codecname = "MPEG2";
      break;
    case DVD_AudioFormat_MPEG2_DRC:
      s->codecname = "MPEG2 (DCR)";
      break;
    case DVD_AudioFormat_LPCM:
      s->codecname = "LPCM";
      break;
    case DVD_AudioFormat_DTS:
      s->codecname = "DTS";
      break;
    case DVD_AudioFormat_SDDS:
      s->codecname = "SDDS";
      break;
    default:
      s->codecname = "Unknown";
      break;
    }
    FormatStreamName((*s));

    m_audioStreams.push_back(s.release());
  }

  BOOL isDisabled = false;
  hr = CFGLoader::Filters.DVD.dvdInfo->GetCurrentSubpicture(&nbrStreams, &currentStream, &isDisabled);
  if (FAILED(hr))
    return;

  for (unsigned int i = 0; i < nbrStreams; i++)
  {
    DVD_SubpictureAttributes subpic;
    hr = CFGLoader::Filters.DVD.dvdInfo->GetSubpictureAttributes(i, &subpic);
    if (subpic.Type != DVD_SPType_Language)
      continue;

    std::auto_ptr<SSubtitleStreamInfos> s(new SSubtitleStreamInfos());
    s->lcid = subpic.Language;
    FormatStreamName((*s));

    SubtitleManager->GetSubtitles().push_back(s.release());
  }

  m_dvdStreamLoaded = true;
}
SSubtitleStreamInfos * CSubtitleManager::GetSubtitleStreamInfos( unsigned int iIndex /*= 0*/ )
{
  if (iIndex > m_subtitleStreams.size())
    return NULL;

  if (m_subtitleStreams[iIndex]->external)
    return NULL;

  return m_subtitleStreams[iIndex];
}

SExternalSubtitleInfos* CSubtitleManager::GetExternalSubtitleStreamInfos( unsigned int iIndex /*= 0*/ )
{
  if (iIndex > m_subtitleStreams.size())
    return NULL;

  if (!m_subtitleStreams[iIndex]->external)
    return NULL;

  return reinterpret_cast<SExternalSubtitleInfos *>(m_subtitleStreams[iIndex]);
}

void CSubtitleManager::DeleteSubtitleManager( ISubManager * pManager, DllLibSubs dll )
{
  dll.DeleteSubtitleManager(pManager);
}