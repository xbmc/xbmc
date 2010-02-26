#include "pch.h"
#include "StreamsManager.h"
#include "SpecialProtocol.h"
#include "DVDSubtitles\DVDFactorySubtitle.h"
#include "Settings.h"

CStreamsManager *CStreamsManager::m_pSingleton = NULL;

CStreamsManager *CStreamsManager::getSingleton()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CStreamsManager());
}

CStreamsManager::CStreamsManager(void):
  m_pIAMStreamSelect(NULL),
  m_init(false),
  m_bChangingAudioStream(false),
  m_bSubtitlesUnconnected(false),
  m_SubtitleInputPin(NULL)
{
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

  m_bChangingAudioStream = true;

  if (m_pIAMStreamSelect)
  {

    int i = 0; long lIndex = -1;
    for (std::vector<SAudioStreamInfos *>::iterator it = m_audioStreams.begin();
      it != m_audioStreams.end(); ++it, i++)
    {
      if (iStream == i)
        lIndex = (*it)->IAMStreamSelect_Index;
      if ((*it)->flags == AMSTREAMSELECTENABLE_ENABLE)
      {
        m_pIAMStreamSelect->Enable((*it)->IAMStreamSelect_Index, 0);
        (*it)->flags = 0;
        if (lIndex != -1)
          break;
      }
    }

    if (SUCCEEDED(m_pIAMStreamSelect->Enable(lIndex, AMSTREAMSELECTENABLE_ENABLE)))
    {
      m_audioStreams[lIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      CLog::Log(LOGDEBUG, "%s Sucessfully selected audio stream", __FUNCTION__);
    }
  } else {

    long disableIndex = GetAudioStream(), enableIndex = iStream;

    IPin *connectedToPin = NULL;
    HRESULT hr = S_OK;

    m_pGraph->Stop(); // Current position is kept by the graph

    /* Disable filter */
    IPin *oldAudioStreamPin = (IPin *)(m_audioStreams[disableIndex]->pObj);
    connectedToPin = (IPin *)(m_audioStreams[disableIndex]->pUnk);
    if (! connectedToPin)
      hr = oldAudioStreamPin->ConnectedTo(&connectedToPin);

    m_audioStreams[enableIndex]->pUnk = connectedToPin;

    if (FAILED(hr))
      goto done;

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

    /* Enable filter */
    IPin *newAudioStreamPin = (IPin *)(m_audioStreams[enableIndex]->pObj);
    hr = m_pGraphBuilder->ConnectDirect(newAudioStreamPin, connectedToPin, NULL);
    if (FAILED(hr))
    {
      /* Reconnect previous working pins */
      m_pGraphBuilder->ConnectDirect(oldAudioStreamPin, connectedToPin, NULL);
      m_audioStreams[disableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      goto done;
    }

    m_audioStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;

done:
    m_pGraph->Play();

    if (SUCCEEDED(hr))
      CLog::Log(LOGNOTICE, "%s Successfully changed audio stream", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Can't change audio stream", __FUNCTION__);
  }

  m_bChangingAudioStream = false;
}

void CStreamsManager::LoadStreams()
{
  if (! m_init)
    return;

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
  m_pIAMStreamSelect = NULL;
  HRESULT hr = m_pSplitter->QueryInterface(__uuidof(m_pIAMStreamSelect), (void **) &m_pIAMStreamSelect);
  if (SUCCEEDED(hr))
  {
    /* Yes */
    CLog::Log(LOGDEBUG, "%s Get IAMStreamSelect interface from %s", __FUNCTION__, splitterName.c_str());

    DWORD nStreams = 0, flags = 0, group = 0;
    WCHAR* wname = NULL;
    SStreamInfos *infos = NULL;
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

      infos->Clear();
      infos->IAMStreamSelect_Index = i;

      g_charsetConverter.wToUTF8(wname, infos->name);
      CoTaskMemFree(wname);

      infos->flags = flags; infos->lcid = lcid; infos->group = group; infos->pObj = pObj; infos->pUnk = pUnk;

      /* Apply regex */
      for (std::vector<CRegExp *>::const_iterator it = regex.begin(); it != regex.end(); ++it)
      {
        if ( (*it)->RegFind(infos->name) > -1 )
        {
          infos->name = (*it)->GetMatch(1);
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

          infos->Clear();

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

          GetStreamInfos(pMediaType, infos);

          infos->pObj = pPin;
          pPin->ConnectedTo((IPin **)&infos->pUnk);

          if (pMediaType->majortype == MEDIATYPE_Audio)
          {
            if (infos->name.find("Undetermined") != std::string::npos )
              infos->name.Format("Audio %02d", i + 1);

            if (i == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;         

            m_audioStreams.push_back(reinterpret_cast<SAudioStreamInfos *>(infos));
            CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());
            i++;
            
          } else if (pMediaType->majortype == MEDIATYPE_Subtitle)
          {
            if (infos->name.find("Undetermined") != std::string::npos )
              infos->name.Format("Subtitle %02d", j + 1);

            if (j == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;

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

  /* Delete regex */
  while (! regex.empty())
  {
    delete regex.back();
    regex.pop_back();
  }

  /* We're done, internal audio & subtitles stream are loaded.
     We load external subtitle file */

  std::vector<std::string> subtitles;
  CDVDFactorySubtitle::GetSubtitles(subtitles, m_pGraph->GetCurrentFile());

  SExternalSubtitleInfos *s = NULL;
  
  int i = 1;
  for (std::vector<std::string>::const_iterator it = subtitles.begin(); it != subtitles.end(); it++)
  {
    s = new SExternalSubtitleInfos(); s->Clear();

    s->external = true; 
    s->path = CSpecialProtocol::TranslatePath((*it));

    s->name.Format("External sub %02d", i);
    i++;
    m_subtitleStreams.push_back(s);
  }
}

bool CStreamsManager::InitManager(IFilterGraph2 *graphBuilder, CDSGraph *DSGraph)
{
  m_pSplitter = CFGLoader::Filters.Splitter.pBF;
  m_pGraphBuilder = graphBuilder;
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
  return m_bChangingAudioStream;
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
    } else if (pMediaType->formattype == FORMAT_VorbisFormat2)
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
  } else if (pMediaType->majortype == MEDIATYPE_Video)
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
    } else if (pMediaType->formattype == FORMAT_MPEG2Video)
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
    } else if (pMediaType->formattype == FORMAT_VideoInfo2)
    {
      if (pMediaType->cbFormat >= sizeof(VIDEOINFOHEADER2))
      {
        VIDEOINFOHEADER2 *v = reinterpret_cast<VIDEOINFOHEADER2 *>(pMediaType->pbFormat);
        infos->width = v->bmiHeader.biWidth;
        infos->height = v->bmiHeader.biHeight;
        infos->codecname = CMediaTypeEx::GetVideoCodecName(pMediaType->subtype, v->bmiHeader.biCompression, &infos->fourcc);
      }
    } else if (pMediaType->formattype == FORMAT_MPEGVideo)
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

  m_bChangingAudioStream = true;
  bool stopped = false;
  CStdString subtitlePath = "";

  if (m_subtitleStreams[iStream]->external)
  {
    /* External subtitle */
    if (! m_bSubtitlesUnconnected)
      UnconnectSubtitlePins();

    SExternalSubtitleInfos *s = reinterpret_cast<SExternalSubtitleInfos *>(m_subtitleStreams[iStream]);

    if (g_dsconfig.LoadffdshowSubtitles(s->path))
      CLog::Log(LOGNOTICE,"%s using this file for subtitle %s", __FUNCTION__, s->path.c_str());

    s->flags = AMSTREAMSELECTINFO_ENABLED;

    m_bChangingAudioStream = false;
    return;
  }

  if (m_pIAMStreamSelect)
  {

    int i = 0; long lIndex = -1;
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
    }

    if (! m_bSubtitlesVisible)
    {
      // If subtitles aren't visible, only disconnect the subtitle track,
      // and don't connect the new one. We change the flag for the xbmc gui
      m_subtitleStreams[iStream]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_bChangingAudioStream = false;
      return;
    }

    if (SUCCEEDED(m_pIAMStreamSelect->Enable(lIndex, AMSTREAMSELECTENABLE_ENABLE)))
    {
      m_audioStreams[iStream]->flags = AMSTREAMSELECTINFO_ENABLED;
      m_bSubtitlesUnconnected = false;
      CLog::Log(LOGDEBUG, "%s Successfully selected subtitle stream", __FUNCTION__);
    }
  } else {

    long disableIndex = GetSubtitle(), enableIndex = iStream;
    
    IPin *connectedToPin = NULL;
    HRESULT hr = S_OK;

    m_pGraph->Stop(); // Current position is kept by the graph
    stopped = true;

    IPin *oldAudioStreamPin = NULL;
    
    /* Disconnect pins */
    if (! m_subtitleStreams[disableIndex]->external)
    {
      // Embed subtitle

      oldAudioStreamPin = (IPin *)(m_subtitleStreams[disableIndex]->pObj);
      connectedToPin = (IPin *)(m_subtitleStreams[disableIndex]->pUnk);
      if (! connectedToPin)
        hr = oldAudioStreamPin->ConnectedTo(&connectedToPin);

      m_subtitleStreams[enableIndex]->pUnk = connectedToPin;
      /* We need to store the pin the current subtitle is connected on.
      If we switch from an external subtitle to an internal subtitle,
      pUnk is set to NULL, so, we don't know where connect the pin! */
      m_SubtitleInputPin = connectedToPin;

      if (FAILED(hr))
        goto done;

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

    } else {
      /* The subtitle to disable is an external subtitle */
      SExternalSubtitleInfos *s = reinterpret_cast<SExternalSubtitleInfos*>(m_subtitleStreams[disableIndex]);
      subtitlePath = s->path;

      if (g_dsconfig.pIffdshowBase)
        g_dsconfig.pIffdshowBase->putParamStr(IDFF_subFilename, NULL);
    }

    m_subtitleStreams[disableIndex]->flags = 0;

    if (! m_bSubtitlesVisible)
    {
      // If subtitles aren't visible, don't connect pins. Only change the flag
      // for the xbmc gui
      m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;

      goto done;
    }

    /* Enable filter */
    IPin *newAudioStreamPin = (IPin *)(m_subtitleStreams[enableIndex]->pObj);    
    if (connectedToPin == NULL)
    {
      /* connectedToPin is NULL if we switch from an external to an internal
      subtitle. Use the internal stored value. */
      connectedToPin = m_SubtitleInputPin;
      m_subtitleStreams[enableIndex]->pUnk = connectedToPin;
    }

    
    if (connectedToPin == NULL)
    {
      connectedToPin = m_SubtitleInputPin;
      m_subtitleStreams[enableIndex]->pUnk = connectedToPin;
    }

    hr = m_pGraphBuilder->ConnectDirect(newAudioStreamPin, connectedToPin, NULL);
    if (FAILED(hr))
    {
      /* Reconnect previous working pins */
      if (oldAudioStreamPin)
        m_pGraphBuilder->ConnectDirect(oldAudioStreamPin, connectedToPin, NULL);
      else
      {
        if (g_dsconfig.pIffdshowBase)
          g_dsconfig.pIffdshowBase->putParamStr(IDFF_subFilename, subtitlePath.c_str());
      }

      m_subtitleStreams[disableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      goto done;
    }

    m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
    m_bSubtitlesUnconnected = false;

done:
    if (stopped)
      m_pGraph->Play();

    if (SUCCEEDED(hr))
      CLog::Log(LOGNOTICE, "%s Successfully changed subtitle stream", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Can't change subtitle stream", __FUNCTION__);
  }

  m_bChangingAudioStream = false;
}

bool CStreamsManager::GetSubtitleVisible()
{
  return m_bSubtitlesVisible;
}

void CStreamsManager::SetSubtitleVisible( bool bVisible )
{
  //int i = GetSubtitle();
  if (g_dsconfig.pIffdshowDecoder)
  {
    g_dsconfig.pIffdshowDecoder->compat_putParam(IDFF_isSubtitles, bVisible); // Show or not subtitles
  }
  m_bSubtitlesVisible = bVisible;

  int i = GetSubtitle();
  if (i >= 0)
    SetSubtitle(i);
}

bool CStreamsManager::AddSubtitle(const CStdString& subFilePath)
{
  if (g_dsconfig.pIffdshowBase) // We're currently using ffdshow for subtitles
  {    
    CStdString newPath = CSpecialProtocol::TranslatePath(subFilePath);
    SExternalSubtitleInfos *s = new SExternalSubtitleInfos();
    s->Clear();

    s->name = newPath.substr(newPath.find_last_of( '\\' ) + 1);
    s->external = true;
    s->path = newPath;

    m_subtitleStreams.push_back(s);

    return true;
  }

  return false;
}

void CStreamsManager::UnconnectSubtitlePins( void )
{
  int i = GetSubtitle();
  if (i == -1)
    return;

  if (m_pIAMStreamSelect)
  {
    m_pIAMStreamSelect->Enable(m_subtitleStreams[i]->IAMStreamSelect_Index, 0);
    m_subtitleStreams[i]->flags = 0;
  } else {
    m_pGraph->Stop();

    m_pGraphBuilder->Disconnect((IPin *)m_subtitleStreams[i]->pObj); // Splitter's output pin
    m_pGraphBuilder->Disconnect((IPin *)m_subtitleStreams[i]->pUnk); // Filter's input pin
    m_subtitleStreams[i]->flags = 0;

    m_pGraph->Play();
  }
  m_bSubtitlesUnconnected = true;
}

void CStreamsManager::SetSubtitleDelay( float fValue )
{
  int delaysub = -fValue * 1000; //1000 is a millisec
  if (g_dsconfig.pIffdshowDecoder)
    g_dsconfig.pIffdshowDecoder->compat_putParam(IDFF_subDelay,delaysub);
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
