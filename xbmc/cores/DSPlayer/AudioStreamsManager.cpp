#include "pch.h"
#include "AudioStreamsManager.h"

CAudioStreamsManager *CAudioStreamsManager::m_pSingleton = NULL;

CAudioStreamsManager *CAudioStreamsManager::getSingleton()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CAudioStreamsManager());
}

CAudioStreamsManager::CAudioStreamsManager(void):
  m_pIAMStreamSelect(NULL),
  m_init(false),
  m_bChangingAudioStream(false)
{
}

CAudioStreamsManager::~CAudioStreamsManager(void)
{
  for (std::map<long, SAudioStreamInfos *>::iterator it = m_audioStreams.begin();
    it != m_audioStreams.end(); ++it)
    delete it->second;

  SAFE_RELEASE(m_pIAMStreamSelect);
}

std::map<long, SAudioStreamInfos *> CAudioStreamsManager::Get()
{
  return m_audioStreams;
}

int CAudioStreamsManager::GetAudioStreamCount()
{
  return m_audioStreams.size();
}

int CAudioStreamsManager::GetAudioStream()
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

void CAudioStreamsManager::GetAudioStreamName(int iStream, CStdString &strStreamName)
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

void CAudioStreamsManager::SetAudioStream(int iStream)
{
  if (! m_init)
    return;

  m_bChangingAudioStream = true;

  if (m_pIAMStreamSelect)
  {

    int i =0; long lIndex = 0;
    for (std::map<long, SAudioStreamInfos *>::iterator it = m_audioStreams.begin();
      it != m_audioStreams.end(); ++it, i++)
    {
      /* Disable all streams */
      m_pIAMStreamSelect->Enable(it->first, 0);
      it->second->flags = 0;
      if (iStream == i)
        lIndex = it->first;
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

    //LONGLONG currentPos;
    //m_pMediaSeeking->GetCurrentPosition(&currentPos);
    //m_pMediaControl->Stop();
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
    //m_pGraph->SeekInMilliSec(DShowUtil::MFTimeToMsec(currentPos));
    m_pGraph->Play();
    m_bChangingAudioStream = false;

    if (SUCCEEDED(hr))
      CLog::Log(LOGNOTICE, "%s Successfully changed audio stream", __FUNCTION__);
    else
      CLog::Log(LOGERROR, "%s Can't change audio stream", __FUNCTION__);
  }
}

void CAudioStreamsManager::LoadAudioStreams()
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

    DWORD nStreams = 0, flags = 0;
    WCHAR* wname = NULL;
    SAudioStreamInfos *infos = NULL;

    m_pIAMStreamSelect->Count(&nStreams);

    AM_MEDIA_TYPE * mediaType = NULL;
    for(unsigned int i = 0; i < nStreams; i++)
    {
      infos = new SAudioStreamInfos();
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
        m_audioStreams.insert( std::pair<long, SAudioStreamInfos *>(i, infos) );
        CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());

      }/* else if (mediaType->majortype == MEDIATYPE_Subtitle)
      {
        // Embed subtitles
        m_pEmbedSubtitles.insert( std::pair<long, IAMStreamSelectInfos *>(i, infos) );
        CLog::Log(LOGNOTICE, "%s Embed subtitle found : %s", __FUNCTION__, infos->name.c_str());
      }*/

      DeleteMediaType(mediaType);
    }
  } else {
    /* No */  
  
    // Enumerate output pins
    PIN_DIRECTION dir;
    int i = 0, j = 0;
    CStdStringW pinNameW;
    CStdString pinName;
    SAudioStreamInfos *infos = NULL;
    bool pinConnected = FALSE;
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

          if (pMediaType->majortype != MEDIATYPE_Audio/* &&
            pMediaType->majortype != MEDIATYPE_Subtitle*/)
            continue;

          infos = new SAudioStreamInfos();
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

          if (infos->name.Trim().Equals("Undetermined"))
            infos->name.Format("Audio %02d", i + 1);

          infos->pObj = pPin;

          if (pMediaType->majortype == MEDIATYPE_Audio)
          {
            /*if (S_OK == DShowUtil::IsPinConnected(pPin))
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
            }*/
            pPin->ConnectedTo((IPin **)&infos->pUnk);
            if (i == 0)
              infos->flags = AMSTREAMSELECTINFO_ENABLED;

            m_audioStreams.insert( std::pair<long, SAudioStreamInfos *>(i, infos) );
            CLog::Log(LOGNOTICE, "%s Audio stream found : %s", __FUNCTION__, infos->name.c_str());
            i++;
          }/* else {

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

          }*/
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

bool CAudioStreamsManager::InitManager(IBaseFilter *Splitter, IFilterGraph2 *graphBuilder, CDSGraph *DSGraph)
{
  m_pSplitter = Splitter;
  m_pGraphBuilder = graphBuilder;
  m_pGraph = DSGraph;
  m_init = true;

  return true;
}

void CAudioStreamsManager::Destroy()
{
  delete m_pSingleton;
  m_pSingleton = NULL;
}

bool CAudioStreamsManager::IsChangingAudioStream()
{
  return m_bChangingAudioStream;
}
