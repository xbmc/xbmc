#include "pch.h"
#include "StreamsManager.h"

CStreamsManager *CStreamsManager::m_pSingleton = NULL;

CStreamsManager *CStreamsManager::getSingleton()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CStreamsManager());
}

CStreamsManager::CStreamsManager(void):
  m_pIAMStreamSelect(NULL),
  m_init(false),
  m_bChangingAudioStream(false)
{
  m_videoStream.Clear();
}

CStreamsManager::~CStreamsManager(void)
{
  for (std::map<long, SAudioStreamInfos *>::iterator it = m_audioStreams.begin();
    it != m_audioStreams.end(); ++it)
    delete it->second;

  for (std::map<long, SSubtitleStreamInfos *>::iterator it = m_subtitleStreams.begin();
    it != m_subtitleStreams.end(); ++it)
    delete it->second;

}

std::map<long, SAudioStreamInfos *> CStreamsManager::GetAudios()
{
  return m_audioStreams;
}

std::map<long, SSubtitleStreamInfos *> CStreamsManager::GetSubtitles()
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
  for (std::map<long, SAudioStreamInfos *>::const_iterator it = m_audioStreams.begin();
    it != m_audioStreams.end(); ++it, i++)
  {
    if ( (*it).second->flags == AMSTREAMSELECTINFO_ENABLED)
      return i;
  }

  return -1;
}

void CStreamsManager::GetAudioStreamName(int iStream, CStdString &strStreamName)
{
  if (m_audioStreams.size() == 0)
    return;

  int i = 0;
  for (std::map<long, SAudioStreamInfos *>::const_iterator it = m_audioStreams.begin();
    it != m_audioStreams.end(); ++it, i++)
  {
    if (i == iStream)
    {
      strStreamName = (*it).second->name;
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
    for (std::map<long, SAudioStreamInfos *>::iterator it = m_audioStreams.begin();
      it != m_audioStreams.end(); ++it, i++)
    {
      if (iStream == i)
        lIndex = it->first;
      if (it->second->flags == AMSTREAMSELECTENABLE_ENABLE)
      {
        m_pIAMStreamSelect->Enable(it->first, 0);
        it->second->flags = 0;
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

    long disableIndex = 0, enableIndex = iStream;
    for (std::map<long, SAudioStreamInfos *>::const_iterator it = m_audioStreams.begin();
      it != m_audioStreams.end(); ++it)
    {
      if (it->second->flags == AMSTREAMSELECTINFO_ENABLED)
      {
        disableIndex = it->first;
        break;
      }
    }

    IPin *connectedToPin = NULL;
    HRESULT hr = S_OK;

    m_pGraph->Stop(); // Current position is kept by the graph

    /* Disable filter */
    IPin *oldAudioStreamPin = (IPin *)(m_audioStreams[disableIndex]->pObj);
    connectedToPin = (IPin *)(m_audioStreams[disableIndex]->pUnk);
    if (! connectedToPin)
      hr = oldAudioStreamPin->ConnectedTo(&connectedToPin);

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
    m_bChangingAudioStream = false;

    if (SUCCEEDED(hr))
      CLog::Log(LOGNOTICE, "%s Successfully changed audio stream", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Can't change audio stream", __FUNCTION__);
  }
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

        m_audioStreams.insert( std::pair<long, SAudioStreamInfos *>(i++, reinterpret_cast<SAudioStreamInfos *>(infos)) );
        CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());
      } else if (mediaType->majortype == MEDIATYPE_Subtitle)
      {
        m_subtitleStreams.insert( std::pair<long, SSubtitleStreamInfos *>(j++, reinterpret_cast<SSubtitleStreamInfos *>(infos)) );
        CLog::Log(LOGNOTICE, "%s Subtitle stream found : %s", __FUNCTION__, infos->name.c_str());
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
            if (infos->name.Trim().Equals("Undetermined"))
              infos->name.Format("Audio %02d", i + 1);

            if (i == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;         

            m_audioStreams.insert( std::pair<long, SAudioStreamInfos *>(i++, reinterpret_cast<SAudioStreamInfos *>(infos)) );
            CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());
            
          } else if (pMediaType->majortype == MEDIATYPE_Subtitle)
          {
            if (j == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;

            m_subtitleStreams.insert( std::pair<long, SSubtitleStreamInfos *>(j++, reinterpret_cast<SSubtitleStreamInfos *>(infos)) );
            CLog::Log(LOGNOTICE, "%s Subtitle stream found : %s", __FUNCTION__, infos->name.c_str());
          }

          break; // if the pin has multiple output type, get only the first one
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
}

bool CStreamsManager::InitManager(IFilterGraph2 *graphBuilder, CDSGraph *DSGraph)
{
  m_pSplitter = CFGLoader::GetSplitter();
  m_pGraphBuilder = graphBuilder;
  m_pGraph = DSGraph;
  m_bSubtitlesVisible = false;
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
  return m_bChangingAudioStream;
}

int CStreamsManager::GetChannels()
{
  int i = InternalGetAudioStream();
  return (i == -1) ? 0 : m_audioStreams[i]->channels;
}

int CStreamsManager::GetBitsPerSample()
{
  int i = InternalGetAudioStream();
  return (i == -1) ? 0 : m_audioStreams[i]->bitrate;
}

int CStreamsManager::GetSampleRate()
{
  int i = InternalGetAudioStream();
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

int CStreamsManager::InternalGetAudioStream()
{
  if (m_audioStreams.size() == 0)
    return -1;

  for (std::map<long, SAudioStreamInfos *>::const_iterator it = m_audioStreams.begin();
    it != m_audioStreams.end(); ++it)
  {
    if ( (*it).second->flags == AMSTREAMSELECTINFO_ENABLED)
      return it->first;
  }

  return -1;
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
  int i = InternalGetAudioStream();
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
  for (std::map<long, SSubtitleStreamInfos *>::const_iterator it = m_subtitleStreams.begin();
    it != m_subtitleStreams.end(); ++it, i++)
  {
    if ( (*it).second->flags == AMSTREAMSELECTINFO_ENABLED)
      return i;
  }

  return -1;
}

void CStreamsManager::GetSubtitleName( int iStream, CStdString &strStreamName )
{
  if (m_subtitleStreams.size() == 0)
    return;

  int i = 0;
  for (std::map<long, SSubtitleStreamInfos *>::const_iterator it = m_subtitleStreams.begin();
    it != m_subtitleStreams.end(); ++it, i++)
  {
    if (i == iStream)
    {
      strStreamName = (*it).second->name;
    }
  }
}

void CStreamsManager::SetSubtitle( int iStream )
{
  if (! m_init)
    return;

  m_bChangingAudioStream = true;

  if (m_pIAMStreamSelect)
  {

    int i = 0; long lIndex = -1;
    for (std::map<long, SSubtitleStreamInfos *>::iterator it = m_subtitleStreams.begin();
      it != m_subtitleStreams.end(); ++it, i++)
    {
      if (iStream == i)
        lIndex = it->first;
      if (it->second->flags == AMSTREAMSELECTENABLE_ENABLE)
      {
        m_pIAMStreamSelect->Enable(it->first, 0);
        it->second->flags = 0;
        if (lIndex != -1)
          break;
      }
    }

    if (SUCCEEDED(m_pIAMStreamSelect->Enable(lIndex, AMSTREAMSELECTENABLE_ENABLE)))
    {
      m_audioStreams[lIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      CLog::Log(LOGDEBUG, "%s Sucessfully selected subtitle stream", __FUNCTION__);
    }
  } else {

    long disableIndex = 0, enableIndex = iStream;
    for (std::map<long, SSubtitleStreamInfos *>::const_iterator it = m_subtitleStreams.begin();
      it != m_subtitleStreams.end(); ++it)
    {
      if (it->second->flags == AMSTREAMSELECTINFO_ENABLED)
      {
        disableIndex = it->first;
        break;
      }
    }

    IPin *connectedToPin = NULL;
    HRESULT hr = S_OK;

    m_pGraph->Stop(); // Current position is kept by the graph

    /* Disable filter */
    IPin *oldAudioStreamPin = (IPin *)(m_subtitleStreams[disableIndex]->pObj);
    connectedToPin = (IPin *)(m_subtitleStreams[disableIndex]->pUnk);
    if (! connectedToPin)
      hr = oldAudioStreamPin->ConnectedTo(&connectedToPin);

    m_subtitleStreams[enableIndex]->pUnk = connectedToPin;

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

    m_subtitleStreams[disableIndex]->flags = 0;

    /*if (! m_bSubtitlesVisible)
    {
      // If subtitles aren't visible, only disconnect the subtitle track,
      // and don't connect the new one. We change the flag for the xbmc gui
      m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;

      goto done;
    }*/

    /* Enable filter */
    IPin *newAudioStreamPin = (IPin *)(m_subtitleStreams[enableIndex]->pObj);
    hr = m_pGraphBuilder->ConnectDirect(newAudioStreamPin, connectedToPin, NULL);
    if (FAILED(hr))
    {
      /* Reconnect previous working pins */
      m_pGraphBuilder->ConnectDirect(oldAudioStreamPin, connectedToPin, NULL);
      m_subtitleStreams[disableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;
      goto done;
    }

    m_subtitleStreams[enableIndex]->flags = AMSTREAMSELECTINFO_ENABLED;

done:
    m_pGraph->Play();
    m_bChangingAudioStream = false;

    if (SUCCEEDED(hr))
      CLog::Log(LOGNOTICE, "%s Successfully changed subtitle stream", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Can't change subtitle stream", __FUNCTION__);
  }
}

void CStreamsManager::SetStreamInternal( int iStream, SStreamInfos *s )
{
  
}

bool CStreamsManager::GetSubtitleVisible()
{
  return m_bSubtitlesVisible;
}

void CStreamsManager::SetSubtitleVisible( bool bVisible )
{
  m_bSubtitlesVisible = bVisible;
 
  // SetSubtitle(GetSubtitle());
}
