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
#include "DVDPlayer.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "DVDInputStreams/DVDInputStreamTV.h"

#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDDemuxVobsub.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDDemuxers/DVDDemuxFFmpeg.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDCodecs/DVDFactoryCodec.h"

#include "Util.h"
#include "utils/GUIInfoManager.h"
#include "Application.h"
#include "DVDPerformanceCounter.h"
#include "../../FileSystem/cdioSupport.h"
#include "../../Picture.h"
#include "../ffmpeg/DllSwScale.h"
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#ifdef HAS_PERFORMANCE_SAMPLE
#include "../../xbmc/utils/PerformanceSample.h"
#else
#define MEASURE_FUNCTION
#endif
#include "Settings.h"
#include "FileItem.h"

using namespace std;

#ifdef HAVE_LIBVDPAU
#include "cores/dvdplayer/DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#endif

void CSelectionStreams::Clear(StreamType type, StreamSource source)
{
  CSingleLock lock(m_section);
  for(int i=m_Streams.size()-1;i>=0;i--)
  {
    if(type && m_Streams[i].type != type)
      continue;

    if(source && m_Streams[i].source != source)
      continue;

    m_Streams.erase(m_Streams.begin() + i);
  }
}

SelectionStream& CSelectionStreams::Get(StreamType type, int index)
{
  CSingleLock lock(m_section);
  int count = -1;
  for(int i=0;i<(int)m_Streams.size();i++)
  {
    if(m_Streams[i].type != type)
      continue;
    count++;
    if(count == index)
      return m_Streams[i];    
  }
  CLog::Log(LOGERROR, "%s - failed to get stream", __FUNCTION__);
  return m_invalid;
}

int CSelectionStreams::IndexOf(StreamType type, int source, int id)
{
  CSingleLock lock(m_section);
  int count = -1;
  for(int i=0;i<(int)m_Streams.size();i++)
  {
    if(type && m_Streams[i].type != type)
      continue;
    count++;
    if(source && m_Streams[i].source != source)
      continue;
    if(id < 0)
      continue;
    if(m_Streams[i].id == id)
      return count;
  }
  if(id < 0)
    return count;
  else
    return -1;
}

int CSelectionStreams::IndexOf(StreamType type, CDVDPlayer& p)
{
  if (p.m_pInputStream && p.m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    int id = -1;
    if(type == STREAM_AUDIO)
      id = ((CDVDInputStreamNavigator*)p.m_pInputStream)->GetActiveAudioStream();
    else if(type == STREAM_VIDEO)
      id = p.m_CurrentVideo.id;
    else if(type == STREAM_SUBTITLE)
      id = ((CDVDInputStreamNavigator*)p.m_pInputStream)->GetActiveSubtitleStream();

    return IndexOf(type, STREAM_SOURCE_NAV, id);
  }

  if(type == STREAM_AUDIO)
    return IndexOf(type, p.m_CurrentAudio.source, p.m_CurrentAudio.id);
  else if(type == STREAM_VIDEO)
    return IndexOf(type, p.m_CurrentVideo.source, p.m_CurrentVideo.id);
  else if(type == STREAM_SUBTITLE)
    return IndexOf(type, p.m_CurrentSubtitle.source, p.m_CurrentSubtitle.id);

  return -1;
}

int CSelectionStreams::Source(StreamSource source, std::string filename)
{
  CSingleLock lock(m_section);
  int index = source - 1;
  for(int i=0;i<(int)m_Streams.size();i++)
  {
    SelectionStream &s = m_Streams[i];
    if(STREAM_SOURCE_MASK(s.source) != source)
      continue;
    // if it already exists, return same
    if(s.filename == filename)
      return s.source;
    if(index < s.source)
      index = s.source;
  }
  // return next index
  return index + 1;
}

void CSelectionStreams::Update(SelectionStream& s)
{
  CSingleLock lock(m_section);
  int index = IndexOf(s.type, s.source, s.id);
  if(index >= 0)
    Get(s.type, index) = s;
  else
    m_Streams.push_back(s);
}

void CSelectionStreams::Update(CDVDInputStream* input, CDVDDemux* demuxer)
{
  if(input && input->IsStreamType(DVDSTREAM_TYPE_DVD))
  {    
    CDVDInputStreamNavigator* nav = (CDVDInputStreamNavigator*)input;
    string filename = nav->GetFileName();
    int source = Source(STREAM_SOURCE_NAV, filename);

    int count;
    count = nav->GetAudioStreamCount();
    for(int i=0;i<count;i++)
    {
      SelectionStream s;
      s.source   = source;
      s.type     = STREAM_AUDIO;
      s.id       = i;
      s.name     = nav->GetAudioStreamLanguage(i);
      s.filename = filename;
      Update(s);
    }

    count = nav->GetSubTitleStreamCount();
    for(int i=0;i<count;i++)
    {
      SelectionStream s;
      s.source   = source;
      s.type     = STREAM_SUBTITLE;
      s.id       = i;
      s.name     = nav->GetSubtitleStreamLanguage(i);
      s.filename = filename;
      Update(s);
    }
  }
  else if(demuxer)
  {
    string filename = demuxer->GetFileName();
    int count = demuxer->GetNrOfStreams();
    int source;
    if(input) /* hack to know this is sub decoder */
      source = Source(STREAM_SOURCE_DEMUX, filename);
    else
      source = Source(STREAM_SOURCE_DEMUX_SUB, filename);


    for(int i=0;i<count;i++)
    {
      CDemuxStream* stream = demuxer->GetStream(i);
      /* make sure stream is marked with right source */
      stream->source = source;

      SelectionStream s;
      s.source   = source;
      s.type     = stream->type;
      s.id       = stream->iId;
      s.language = stream->language;
      s.filename = demuxer->GetFileName();
      stream->GetStreamName(s.name);
      if(stream->type == STREAM_AUDIO)
      {
        std::string type;
        ((CDemuxStreamAudio*)stream)->GetStreamType(type);
        if(type.length() > 0)
        {
          if(s.name.length() > 0)
            s.name += " - ";
          s.name += type;
        }
      }
      Update(s);
    }
  }
}

CDVDPlayer::CDVDPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread(),
      m_CurrentAudio(STREAM_AUDIO),
      m_CurrentVideo(STREAM_VIDEO),
      m_CurrentSubtitle(STREAM_SUBTITLE),
      m_dvdPlayerVideo(&m_clock, &m_overlayContainer),
      m_dvdPlayerAudio(&m_clock),
      m_dvdPlayerSubtitle(&m_overlayContainer),
      m_messenger("player")
{
  CLog::Log(LOGNOTICE, "CDVDPlayer");
  m_pDemuxer = NULL;
  m_pSubtitleDemuxer = NULL;
  m_pInputStream = NULL;

  m_tmLastSeek = time(NULL);

  m_hReadyEvent = CreateEvent(NULL, true, false, NULL);

  InitializeCriticalSection(&m_critStreamSection);

  m_dvd.Clear();
  m_State.Clear();
  m_UpdateApplication = 0;

  m_bAbortRequest = false;
  m_errorCount = 0;
  m_playSpeed = DVD_PLAYSPEED_NORMAL;
  m_caching = false;
  m_seeking = false;

  m_pDlgCache = NULL;

#ifdef DVDDEBUG_MESSAGE_TRACKER
  g_dvdMessageTracker.Init();
#endif
}

CDVDPlayer::~CDVDPlayer()
{
  CloseFile();
  CLog::Log(LOGNOTICE, "~CDVDPlayer");
  CloseHandle(m_hReadyEvent);
  DeleteCriticalSection(&m_critStreamSection);
#ifdef DVDDEBUG_MESSAGE_TRACKER
  g_dvdMessageTracker.DeInit();
#endif
}

bool CDVDPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  CLog::Log(LOGNOTICE, "OpenFile");
  try
  {
    if (m_pDlgCache)
      m_pDlgCache->Close();

    CStdString strHeader;
    if (file.IsInternetStream())
      strHeader = g_localizeStrings.Get(10214);

    if(file.IsInternetStream())
      m_pDlgCache = new CDlgCache(0, strHeader, file.GetLabel());
    else if(!file.IsDVDFile(false, true) && !file.IsDVDImage() && !file.IsDVD())
      m_pDlgCache = new CDlgCache(3000, strHeader, file.GetLabel());

    CLog::Log(LOGNOTICE, "DVDPlayer: Opening: %s", file.m_strPath.c_str());

    // if playing a file close it first
    // this has to be changed so we won't have to close it.
    if(ThreadHandle())
      CloseFile();

    m_bAbortRequest = false;
    m_seeking = false;
    SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

    m_State.Clear();
    m_UpdateApplication = 0;

    m_PlayerOptions = options;
    m_item     = file;
    m_content  = file.GetContentType();
    m_filename = file.m_strPath;

    ResetEvent(m_hReadyEvent);
    Create();
    WaitForSingleObject(m_hReadyEvent, INFINITE);

    // Playback might have been stopped due to some error
    if (m_bStop || m_bAbortRequest) 
    {
      if (m_pDlgCache)
      {
        m_pDlgCache->Close();
        m_pDlgCache = NULL;
      }
      return false;
    }

   
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown on open", __FUNCTION__);
    if (m_pDlgCache)
    {
      m_pDlgCache->Close();
      m_pDlgCache = NULL;
    }

    return false;
  }
}

bool CDVDPlayer::CloseFile()
{
  CLog::Log(LOGNOTICE, "CDVDPlayer::CloseFile()");

  // unpause the player
  SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

  // set the abort request so that other threads can finish up
  m_bAbortRequest = true;

  // tell demuxer to abort
  if(m_pDemuxer)
    m_pDemuxer->Abort();

  if(m_pSubtitleDemuxer)
    m_pSubtitleDemuxer->Abort();

  CLog::Log(LOGNOTICE, "DVDPlayer: waiting for threads to exit");

  // wait for the main thread to finish up
  // since this main thread cleans up all other resources and threads
  // we are done after the StopThread call
  StopThread();

  m_Edl.Reset();

  CLog::Log(LOGNOTICE, "DVDPlayer: finished waiting");
#if defined(_LINUX) && defined(HAS_VIDEO_PLAYBACK)
  g_renderManager.OnClose();
#endif
  return true;
}

bool CDVDPlayer::IsPlaying() const
{
  return !m_bStop;
}

void CDVDPlayer::OnStartup()
{
  CThread::SetName("CDVDPlayer");
  m_CurrentVideo.Clear();
  m_CurrentAudio.Clear();
  m_CurrentSubtitle.Clear();

  m_messenger.Init();

  g_dvdPerformanceCounter.EnableMainPerformance(ThreadHandle());
}

bool CDVDPlayer::OpenInputStream()
{
  if(m_pInputStream)
    SAFE_DELETE(m_pInputStream);

  CLog::Log(LOGNOTICE, "Creating InputStream");

  // correct the filename if needed
  CStdString filename(m_filename);
  if (filename.Find("dvd://") == 0 
  ||  filename.CompareNoCase("d:\\video_ts\\video_ts.ifo") == 0
  ||  filename.CompareNoCase("iso9660://video_ts/video_ts.ifo") == 0)
  {
#ifdef _WIN32PC
    m_filename = MEDIA_DETECT::CLibcdio::GetInstance()->GetDeviceFileName()+4;
#else
    m_filename = MEDIA_DETECT::CLibcdio::GetInstance()->GetDeviceFileName();
#endif
  }
  
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(this, m_filename, m_content);
  if(m_pInputStream == NULL)
  {
    CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - unable to create input stream for [%s]", m_filename.c_str());
    return false;
  }
  else
    m_pInputStream->SetFileItem(m_item);

  if (!m_pInputStream->Open(m_filename.c_str(), m_content))
  {
    CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - error opening [%s]", m_filename.c_str());
    return false;
  }

  // find any available external subtitles for non dvd files
  if (!m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) 
  &&  !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV)
  &&  !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_HTSP))
  {
    if(g_stSettings.m_currentVideoSettings.m_SubtitleOn)
    {
      // find any available external subtitles
      std::vector<std::string> filenames;
      CDVDFactorySubtitle::GetSubtitles(filenames, m_filename);
      for(unsigned int i=0;i<filenames.size();i++)
        AddSubtitleFile(filenames[i]);

      g_stSettings.m_currentVideoSettings.m_SubtitleCached = true;
    }

    // look for any edl files
    m_Edl.Reset();
    if (g_guiSettings.GetBool("videoplayer.editdecision") && !m_item.IsInternetStream())
      m_Edl.ReadnCacheAny(m_filename);
  }

  SetAVDelay(g_stSettings.m_currentVideoSettings.m_AudioDelay);
  m_clock.Discontinuity(CLOCK_DISC_FULL);
  m_dvd.Clear();
  m_errorCount = 0;

  return true;
}

bool CDVDPlayer::OpenDemuxStream()
{
  if(m_pDemuxer)
    SAFE_DELETE(m_pDemuxer);

  CLog::Log(LOGNOTICE, "Creating Demuxer");

  try
  {
    int attempts = 10;
    while(!m_bStop && attempts-- > 0)
    {
      m_pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(m_pInputStream);
      if(!m_pDemuxer && m_pInputStream->NextStream())
      {
        CLog::Log(LOGDEBUG, "%s - New stream available from input, retry open", __FUNCTION__);
        continue;
      }
      break;
    }

    if(!m_pDemuxer)
    {
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return false;
    }

  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opeing demuxer", __FUNCTION__);
    return false;
  }

  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
  m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NAV);
  m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);

  return true;
}

void CDVDPlayer::OpenDefaultStreams()
{
  int  count;
  bool valid;
  // open video stream
  count = m_SelectionStreams.Count(STREAM_VIDEO);
  valid = false;
  for(int i = 0;i<count && !valid;i++)
  {
    SelectionStream& s = m_SelectionStreams.Get(STREAM_VIDEO, i);
    if(OpenVideoStream(s.id, s.source))
      valid = true;
  }
  if(!valid)
    CloseVideoStream(true);

  if(!m_PlayerOptions.video_only)
  {
    // open audio stream
    count = m_SelectionStreams.Count(STREAM_AUDIO);
    valid = false;
    if(g_stSettings.m_currentVideoSettings.m_AudioStream >= 0 
    && g_stSettings.m_currentVideoSettings.m_AudioStream < count)
    {
      SelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, g_stSettings.m_currentVideoSettings.m_AudioStream);
      if(OpenAudioStream(s.id, s.source))
        valid = true;
      else
        CLog::Log(LOGWARNING, "%s - failed to restore selected audio stream (%d)", __FUNCTION__, g_stSettings.m_currentVideoSettings.m_AudioStream);
    }

    for(int i = 0; i<count && !valid; i++)
    {
      SelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, i);
      if(OpenAudioStream(s.id, s.source))
        valid = true;
    }
    if(!valid)
      CloseAudioStream(true);
  }

  // open subtitle stream
  if(g_stSettings.m_currentVideoSettings.m_SubtitleOn && !m_PlayerOptions.video_only)
  {
    m_dvdPlayerVideo.EnableSubtitle(true);
    count = m_SelectionStreams.Count(STREAM_SUBTITLE);
    valid = false;
    if(g_stSettings.m_currentVideoSettings.m_SubtitleStream >= 0 
    && g_stSettings.m_currentVideoSettings.m_SubtitleStream < count)
    {
      SelectionStream& s = m_SelectionStreams.Get(STREAM_SUBTITLE, g_stSettings.m_currentVideoSettings.m_SubtitleStream);
      if(OpenSubtitleStream(s.id, s.source))
        valid = true;
      else
        CLog::Log(LOGWARNING, "%s - failed to restore selected subtitle stream (%d)", __FUNCTION__, g_stSettings.m_currentVideoSettings.m_SubtitleStream);
    }

    for(int i = 0;i<count && !valid; i++)
    {
      SelectionStream& s = m_SelectionStreams.Get(STREAM_SUBTITLE, i);
      if(OpenSubtitleStream(s.id, s.source))
        valid = true;
    }
    if(!valid)
      CloseSubtitleStream(false);
  }
  else
    m_dvdPlayerVideo.EnableSubtitle(false);

}

bool CDVDPlayer::ReadPacket(DemuxPacket*& packet, CDemuxStream*& stream)
{

  // check if we should read from subtitle demuxer
  if(m_dvdPlayerSubtitle.AcceptsData() && m_pSubtitleDemuxer )
  {
    if(m_pSubtitleDemuxer)
      packet = m_pSubtitleDemuxer->Read();

    if(packet)
    {
      if(packet->iStreamId < 0)
        return true;

      stream = m_pSubtitleDemuxer->GetStream(packet->iStreamId);
      if (!stream)
      {
        CLog::Log(LOGERROR, "%s - Error demux packet doesn't belong to a valid stream", __FUNCTION__);
        return false;
      }
      if(stream->source == STREAM_SOURCE_NONE)
      {
        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX_SUB);
        m_SelectionStreams.Update(NULL, m_pSubtitleDemuxer);
      }
      return true;
    }
  }

  // read a data frame from stream.
  if(m_pDemuxer)
    packet = m_pDemuxer->Read();

  if(packet)
  {

    // correct for timestamp errors, maybe should be inside demuxer
    if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      CDVDInputStreamNavigator *pInput = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);

      if (packet->dts != DVD_NOPTS_VALUE)
        packet->dts -= pInput->GetTimeStampCorrection();
      if (packet->pts != DVD_NOPTS_VALUE)
        packet->pts -= pInput->GetTimeStampCorrection();
    }

    // this groupId stuff is getting a bit messy, need to find a better way
    // currently it is used to determine if a menu overlay is associated with a picture
    // for dvd's we use as a group id, the current cell and the current title
    // to be a bit more precise we alse count the number of disc's in case of a pts wrap back in the same cell / title
    packet->iGroupId = m_pInputStream->GetCurrentGroupId();

    if(packet->iStreamId < 0)
      return true;

    stream = m_pDemuxer->GetStream(packet->iStreamId);
    if (!stream) 
    {
      CLog::Log(LOGERROR, "%s - Error demux packet doesn't belong to a valid stream", __FUNCTION__);
      return false;
    }
    if(stream->source == STREAM_SOURCE_NONE)
    {
      m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_DEMUX);
      m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
    }
    return true;
  }
  return false;
}

bool CDVDPlayer::IsValidStream(CCurrentStream& stream)
{
  if(stream.id<0)
    return true; // we consider non selected as valid

  int source = STREAM_SOURCE_MASK(stream.source);
  if(source == STREAM_SOURCE_TEXT)
    return true;
  if(source == STREAM_SOURCE_DEMUX_SUB)
  {
    CDemuxStream* st = m_pSubtitleDemuxer->GetStream(stream.id);
    if(st == NULL || st->disabled)
      return false;
    if(st->type != stream.type)
      return false;
    return true;
  }
  if(source == STREAM_SOURCE_DEMUX)
  {
    CDemuxStream* st = m_pDemuxer->GetStream(stream.id);
    if(st == NULL || st->disabled)
      return false;
    if(st->type != stream.type)
      return false;

    if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      if(stream.type == STREAM_AUDIO    && st->iPhysicalId != m_dvd.iSelectedAudioStream)
        return false;
      if(stream.type == STREAM_SUBTITLE && st->iPhysicalId != m_dvd.iSelectedSPUStream)
        return false;
    }

    return true;
  }

  return false;
}

bool CDVDPlayer::IsBetterStream(CCurrentStream& current, CDemuxStream* stream)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    int source_type;

    source_type = STREAM_SOURCE_MASK(current.source);
    if(source_type != STREAM_SOURCE_DEMUX 
    && source_type != STREAM_SOURCE_NONE)
      return false;

    source_type = STREAM_SOURCE_MASK(stream->source);
    if(source_type  != STREAM_SOURCE_DEMUX
    || stream->type != current.type
    || stream->iId  == current.id)
      return false;

    if(current.type == STREAM_AUDIO    && stream->iPhysicalId == m_dvd.iSelectedAudioStream)
      return true;
    if(current.type == STREAM_SUBTITLE && stream->iPhysicalId == m_dvd.iSelectedSPUStream)
      return true;
    if(current.type == STREAM_VIDEO    && current.id < 0)
      return true;
  }
  else
  {
    if(stream->source == current.source 
    && stream->iId    == current.id)
      return false;

    if(stream->disabled)
      return false;

    if(stream->type != current.type)
      return false;

    if(current.type == STREAM_SUBTITLE)
      return false;

    if(current.id < 0)
      return true;
  }
  return false;
}

void CDVDPlayer::Process()
{
  if (m_pDlgCache && m_pDlgCache->IsCanceled())
    return;
 
  if (!OpenInputStream())
  {
    m_bAbortRequest = true;
    return;
  }

  if (m_pDlgCache && m_pDlgCache->IsCanceled())
    return;

  if(m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CLog::Log(LOGNOTICE, "DVDPlayer: playing a dvd with menu's");
    m_PlayerOptions.starttime = 0;


    if(m_PlayerOptions.state.size() > 0)
      ((CDVDInputStreamNavigator*)m_pInputStream)->SetNavigatorState(m_PlayerOptions.state);
    ((CDVDInputStreamNavigator*)m_pInputStream)->EnableSubtitleStream(g_stSettings.m_currentVideoSettings.m_SubtitleOn);

    g_stSettings.m_currentVideoSettings.m_SubtitleCached = true;
  }

  if(!OpenDemuxStream())
  {
    m_bAbortRequest = true;
    return;
  }

  if (m_pDlgCache && m_pDlgCache->IsCanceled())
    return;

  OpenDefaultStreams();

  if( m_PlayerOptions.starttime > 0 )
  {
    double startpts = DVD_NOPTS_VALUE;
    if(m_pDemuxer)
    {
      if (m_pDemuxer->SeekTime(m_PlayerOptions.starttime * 1000, false, &startpts))
        CLog::Log(LOGDEBUG, "%s - starting demuxer from: %f", __FUNCTION__, m_PlayerOptions.starttime);
      else
        CLog::Log(LOGDEBUG, "%s - failed to start demuxing from %f", __FUNCTION__,  m_PlayerOptions.starttime);
    }

    if(m_pSubtitleDemuxer)
    {
      if(m_pSubtitleDemuxer->SeekTime(m_PlayerOptions.starttime * 1000, false, &startpts))
        CLog::Log(LOGDEBUG, "%s - starting subtitle demuxer from: %f", __FUNCTION__, m_PlayerOptions.starttime);
      else
        CLog::Log(LOGDEBUG, "%s - failed to start subtitle demuxing from: %f", __FUNCTION__, m_PlayerOptions.starttime);
    }
  }

  // allow renderer to switch to fullscreen if requested
  m_dvdPlayerVideo.EnableFullscreen(m_PlayerOptions.fullscreen);

  // make sure application know our info
  UpdateApplication(0);
  UpdatePlayState(0);

  // we are done initializing now, set the readyevent
  SetEvent(m_hReadyEvent);

  if(m_PlayerOptions.identify == false)
    m_callback.OnPlayBackStarted();

  if (m_pDlgCache && m_pDlgCache->IsCanceled())
    return;

  if (m_pDlgCache)
    m_pDlgCache->SetMessage(g_localizeStrings.Get(10213));

  if(!m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) 
  && !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV)
  && !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_HTSP))
    SetCaching(true);

  while (!m_bAbortRequest)
  {
    // handle messages send to this thread, like seek or demuxer reset requests
    HandleMessages();

    if(m_bAbortRequest)
      break;

    // should we open a new input stream?
    if(!m_pInputStream)
    {
      if (OpenInputStream() == false)
        break;
    }

    // should we open a new demuxer?
    if(!m_pDemuxer)
    {
      if (m_pInputStream->NextStream() == false)
        break;

      if (m_pInputStream->IsEOF())
        break;

      if (OpenDemuxStream() == false)
        break;

      OpenDefaultStreams();
      UpdateApplication(0);
      UpdatePlayState(0);
    }

    // handle eventual seeks due tp playspeed
    HandlePlaySpeed();

    // update player state
    UpdatePlayState(200);

    // update application with our state
    UpdateApplication(1000);

    // if the queues are full, no need to read more
    if ((!m_dvdPlayerAudio.AcceptsData() && m_CurrentAudio.id >= 0)
    ||  (!m_dvdPlayerVideo.AcceptsData() && m_CurrentVideo.id >= 0))
    {
      Sleep(10);
      if (m_caching)
        SetCaching(false);
      continue;
    }

    DemuxPacket* pPacket = NULL;
    CDemuxStream *pStream = NULL;
    ReadPacket(pPacket, pStream);
    if (pPacket && !pStream)
    {
      /* probably a empty packet, just free it and move on */
      CDVDDemuxUtils::FreeDemuxPacket(pPacket); 
      continue;
    }

    if (!pPacket)
    {
      // when paused, demuxer could be be returning empty
      if (m_playSpeed == DVD_PLAYSPEED_PAUSE)
        continue;

      // if there is another stream available, let 
      // player reopen demuxer
      if(m_pInputStream->NextStream())
      {
        SAFE_DELETE(m_pDemuxer);
        continue;
      }

      if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        CDVDInputStreamNavigator* pStream = static_cast<CDVDInputStreamNavigator*>(m_pInputStream);

        // stream is holding back data until demuxer has flushed
        if(pStream->IsHeld())
        {
          pStream->SkipHold();
          continue;
        }

        // stills will be skipped
        if(m_dvd.state == DVDSTATE_STILL)
        {
          if (m_dvd.iDVDStillTime > 0)
          {
            if (GetTickCount() >= (m_dvd.iDVDStillStartTime + m_dvd.iDVDStillTime))
            {
              m_dvd.iDVDStillTime = 0;
              m_dvd.iDVDStillStartTime = 0;
              m_dvd.state = DVDSTATE_NORMAL;
              pStream->SkipStill();
              continue;
            }
          }
        }

        // always continue on dvd's
        Sleep(100);
        continue;
      }

      // make sure we tell all players to finish it's data
      if(m_CurrentAudio.inited)
        m_dvdPlayerAudio.SendMessage   (new CDVDMsg(CDVDMsg::GENERAL_EOF));
      if(m_CurrentVideo.inited) 
        m_dvdPlayerVideo.SendMessage   (new CDVDMsg(CDVDMsg::GENERAL_EOF));
      if(m_CurrentSubtitle.inited) 
        m_dvdPlayerSubtitle.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_EOF));
      m_CurrentAudio.inited    = false;
      m_CurrentVideo.inited    = false;
      m_CurrentSubtitle.inited = false;

      // if we are caching, start playing it agian
      if (m_caching && !m_bAbortRequest)
        SetCaching(false);

      // while players are still playing, keep going to allow seekbacks
      if(m_dvdPlayerAudio.m_messageQueue.GetDataSize() > 0 
      || m_dvdPlayerVideo.m_messageQueue.GetDataSize() > 0)
      {
        Sleep(100);
        continue;
      }

      if (!m_pInputStream->IsEOF()) 
        CLog::Log(LOGINFO, "%s - eof reading from demuxer", __FUNCTION__);

      break;
    }

    // it's a valid data packet, reset error counter
    m_errorCount = 0;

    try
    {
      if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      {
        // check so dvdnavigator didn't want us to close stream,
        // we allow lingering invalid audio/subtitle streams here to let player pass vts/cell borders more cleanly
        if (m_dvd.iSelectedAudioStream < 0 && m_CurrentAudio.id >= 0) CloseAudioStream( true );
        if (m_dvd.iSelectedSPUStream < 0 && m_CurrentVideo.id >= 0)   CloseSubtitleStream( true );
      }

      // check so that none of our streams has become invalid
      if (!IsValidStream(m_CurrentAudio))    CloseAudioStream(true);
      if (!IsValidStream(m_CurrentVideo))    CloseVideoStream(true);
      if (!IsValidStream(m_CurrentSubtitle)) CloseSubtitleStream(true);

      // check if there is any better stream to use (normally for dvd's)
      if ( !m_PlayerOptions.video_only )
      {
        // Do not reopen non-video streams if we're in video-only mode
        if (IsBetterStream(m_CurrentAudio,    pStream)) OpenAudioStream(pStream->iId, pStream->source);
        if (IsBetterStream(m_CurrentSubtitle, pStream)) OpenSubtitleStream(pStream->iId, pStream->source);
      }

      if (IsBetterStream(m_CurrentVideo,    pStream)) OpenVideoStream(pStream->iId, pStream->source);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when attempting to open stream", __FUNCTION__);
      break;
    }
    CSingleLock lock(g_VDPAUSection);
    if (g_VDPAU  && g_VDPAU->RefNotify)
    {
      m_SpeedState.lastRef = GetTime();
      g_VDPAU->RefNotify = false;
    }
    if (g_VDPAU && g_VDPAU->VDPAURecovered)
    {
      CLog::Log(LOGDEBUG, "CDVDPlayer::Process - caught preemption");
      if (m_SpeedState.lastRef)
      {
        if (m_SpeedState.lastRef < 100) m_SpeedState.lastRef = 100;
        __int64 iTime = m_SpeedState.lastRef - 100;
        m_messenger.Put(new CDVDMsgPlayerSeek(iTime, (GetPlaySpeed() < 0), true, false));
      }
      else
      {
        if (m_SpeedState.lasttime < 100) m_SpeedState.lasttime = 100;
        __int64 iTime = (__int64)(m_SpeedState.lasttime - 100);
        m_messenger.Put(new CDVDMsgPlayerSeek(iTime, (GetPlaySpeed() < 0), true, false));
      }
      g_VDPAU->VDPAURecovered = false;
    }
    // process the packet
    ProcessPacket(pStream, pPacket);

    // present the cache dialog until playback actually started
    if (m_pDlgCache)
    {
      if (m_pDlgCache->IsCanceled())
      {
        m_bAbortRequest = true;
        break;
      }

      if (m_caching)
      {
        m_pDlgCache->ShowProgressBar(true);
        m_pDlgCache->SetPercentage(GetCacheLevel());
      }
      else if (GetTime() > 500) // movie started to play
      {
        m_pDlgCache->Close();
        m_pDlgCache = NULL;
      }
    }

  }

  // playback ended, make sure anything buffered is displayed
  if (m_pDlgCache)
  {
    m_pDlgCache->Close();
    m_pDlgCache = NULL;
  }
}

void CDVDPlayer::ProcessPacket(CDemuxStream* pStream, DemuxPacket* pPacket)
{
    /* process packet if it belongs to selected stream. for dvd's down't allow automatic opening of streams*/
    LockStreams();

    try
    {
      if (pPacket->iStreamId == m_CurrentAudio.id && pStream->source == m_CurrentAudio.source && pStream->type == STREAM_AUDIO)
        ProcessAudioData(pStream, pPacket);
      else if (pPacket->iStreamId == m_CurrentVideo.id && pStream->source == m_CurrentVideo.source && pStream->type == STREAM_VIDEO)
        ProcessVideoData(pStream, pPacket);
      else if (pPacket->iStreamId == m_CurrentSubtitle.id && pStream->source == m_CurrentSubtitle.source && pStream->type == STREAM_SUBTITLE)
        ProcessSubData(pStream, pPacket);
      else
      {
        pStream->SetDiscard(AVDISCARD_ALL);
        CDVDDemuxUtils::FreeDemuxPacket(pPacket); // free it since we won't do anything with it
      }
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when processing demux packet", __FUNCTION__);
    }

    UnlockStreams();
}

void CDVDPlayer::ProcessAudioData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  if (m_CurrentAudio.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have, reopen stream */

    if (m_CurrentAudio.hint != CDVDStreamInfo(*pStream, true))
    { 
      // we don't actually have to close audiostream here first, as 
      // we could send it as a stream message. only problem 
      // is how to notify player if a stream change failed.
      CloseAudioStream( true );
      OpenAudioStream( pPacket->iStreamId, pStream->source );
    }

    m_CurrentAudio.stream = (void*)pStream;
  }

  // check if we are too slow and need to recache
  if(CheckStartCaching(m_CurrentAudio) && m_dvdPlayerAudio.IsStalled())
    SetCaching(true);

  CheckContinuity(m_CurrentAudio, pPacket);
  if(pPacket->dts != DVD_NOPTS_VALUE)
    m_CurrentAudio.dts = pPacket->dts;
  else if(pPacket->pts != DVD_NOPTS_VALUE)
    m_CurrentAudio.dts = pPacket->pts;

  bool drop = false;
  if (CheckPlayerInit(m_CurrentAudio, DVDPLAYER_AUDIO))
    drop = true;

  if (CheckSceneSkip(m_CurrentAudio))
    drop = true;

  m_dvdPlayerAudio.SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
}

void CDVDPlayer::ProcessVideoData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  if (m_CurrentVideo.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have reopen stream */

    if (m_CurrentVideo.hint != CDVDStreamInfo(*pStream, true))
    {
      CloseVideoStream(true);
      OpenVideoStream(pPacket->iStreamId, pStream->source);
    }

    m_CurrentVideo.stream = (void*)pStream;
  }

  // check if we are too slow and need to recache
  if(CheckStartCaching(m_CurrentVideo) && m_dvdPlayerVideo.IsStalled())
    SetCaching(true);

  if( pPacket->iSize != 4) //don't check the EOF_SEQUENCE of stillframes
  {
    CheckContinuity(m_CurrentVideo, pPacket);
    if(pPacket->dts != DVD_NOPTS_VALUE)
      m_CurrentVideo.dts = pPacket->dts;
    else if(pPacket->pts != DVD_NOPTS_VALUE)
      m_CurrentVideo.dts = pPacket->pts;
  }

  bool drop = false;
  if (CheckPlayerInit(m_CurrentVideo, DVDPLAYER_VIDEO))
    drop = true;

  if (CheckSceneSkip(m_CurrentAudio))
    drop = true;

  m_dvdPlayerVideo.SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));
}

void CDVDPlayer::ProcessSubData(CDemuxStream* pStream, DemuxPacket* pPacket)
{
  if (m_CurrentSubtitle.stream != (void*)pStream)
  {
    /* check so that dmuxer hints or extra data hasn't changed */
    /* if they have reopen stream */

    if (m_CurrentSubtitle.hint != CDVDStreamInfo(*pStream, true))
    {
        CloseSubtitleStream(true);
        OpenSubtitleStream(pPacket->iStreamId, pStream->source);
    }

    m_CurrentSubtitle.stream = (void*)pStream;
  }
  if(pPacket->dts != DVD_NOPTS_VALUE)
    m_CurrentSubtitle.dts = pPacket->dts;
  else if(pPacket->pts != DVD_NOPTS_VALUE)
    m_CurrentSubtitle.dts = pPacket->pts;

  bool drop = false;
  if (CheckPlayerInit(m_CurrentSubtitle, DVDPLAYER_SUBTITLE))
    drop = true;

  if (CheckSceneSkip(m_CurrentAudio))
    drop = true;

  m_dvdPlayerSubtitle.SendMessage(new CDVDMsgDemuxerPacket(pPacket, drop));

  if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))    
    m_dvdPlayerSubtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_NORMAL);
}

void CDVDPlayer::HandlePlaySpeed()
{
  if(GetPlaySpeed() != DVD_PLAYSPEED_NORMAL && GetPlaySpeed() != DVD_PLAYSPEED_PAUSE)
  {
    if (IsInMenu())
    {
      // this can't be done in menu
      SetPlaySpeed(DVD_PLAYSPEED_NORMAL);

    }
    else if (m_CurrentVideo.id >= 0 
          &&  m_CurrentVideo.inited == true
          &&  m_SpeedState.lastpts  != m_dvdPlayerVideo.GetCurrentPts()
          &&  m_SpeedState.lasttime != GetTime())
    {
      m_SpeedState.lastpts  = m_dvdPlayerVideo.GetCurrentPts();
      m_SpeedState.lasttime = GetTime();
      // check how much off clock video is when ff/rw:ing
      // a problem here is that seeking isn't very accurate
      // and since the clock will be resynced after seek
      // we might actually not really be playing at the wanted
      // speed. we'd need to have some way to not resync the clock
      // after a seek to remember timing. still need to handle
      // discontinuities somehow

      // when seeking, give the player a headstart to make sure 
      // the time it takes to seek doesn't make a difference.
      double error;
      error  = m_clock.GetClock() - m_SpeedState.lastpts;
      error *= m_playSpeed / abs(m_playSpeed);

      if(error > DVD_MSEC_TO_TIME(1000))
      {
        CLog::Log(LOGDEBUG, "CDVDPlayer::Process - Seeking to catch up");
        __int64 iTime = (__int64)(m_SpeedState.lasttime + 500.0 * m_playSpeed / DVD_PLAYSPEED_NORMAL);
        m_messenger.Put(new CDVDMsgPlayerSeek(iTime, (GetPlaySpeed() < 0), true, false));
      }
    }
  }
}

bool CDVDPlayer::CheckStartCaching(CCurrentStream& current)
{
  return !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) 
      && !m_caching && m_playSpeed == DVD_PLAYSPEED_NORMAL
      && current.inited;
}

bool CDVDPlayer::CheckPlayerInit(CCurrentStream& current, unsigned int source)
{
  if(current.startsync)
  {
    if ((current.startpts < current.dts && current.dts != DVD_NOPTS_VALUE)
    ||  (current.startpts == DVD_NOPTS_VALUE))
    {
      SendPlayerMessage(current.startsync, source);

      current.startpts = DVD_NOPTS_VALUE;
      current.startsync = NULL;
    }
    else if((current.startpts - current.dts) > DVD_SEC_TO_TIME(20)
         &&  current.dts != DVD_NOPTS_VALUE)
    {
      CLog::Log(LOGDEBUG, "%s - too far to decode before finishing seek", __FUNCTION__);
      if(m_CurrentAudio.startpts != DVD_NOPTS_VALUE)
        m_CurrentAudio.startpts = current.dts;
      if(m_CurrentVideo.startpts != DVD_NOPTS_VALUE)
        m_CurrentVideo.startpts = current.dts;
      if(m_CurrentSubtitle.startpts != DVD_NOPTS_VALUE)
        m_CurrentSubtitle.startpts = current.dts;
    }
  }

  // await start sync to be finished
  if(current.startsync)
  {
    CLog::Log(LOGDEBUG, "%s - dropping packet type:%d dts:%f to get to start point at %f", __FUNCTION__, source,  current.dts, current.startpts);
    return true;
  }


  //If this is the first packet after a discontinuity, send it as a resync
  if (current.inited == false && current.dts != DVD_NOPTS_VALUE)
  {
    current.inited   = true;
    current.startpts = current.dts;

    bool setclock = false;
    if(m_playSpeed == DVD_PLAYSPEED_NORMAL)
    {
      if(     source == DVDPLAYER_AUDIO)
        setclock = !m_CurrentVideo.inited;
      else if(source == DVDPLAYER_VIDEO)
        setclock = !m_CurrentAudio.inited;
    }
    else
    {
      if(source == DVDPLAYER_VIDEO)
        setclock = true;
    }

    double starttime = current.startpts;
    if(m_CurrentAudio.inited 
    && m_CurrentAudio.startpts != DVD_NOPTS_VALUE 
    && m_CurrentAudio.startpts < starttime)
      starttime = m_CurrentAudio.startpts;
    if(m_CurrentVideo.inited 
    && m_CurrentVideo.startpts != DVD_NOPTS_VALUE
    && m_CurrentVideo.startpts < starttime)
      starttime = m_CurrentVideo.startpts;

    starttime = current.startpts - starttime;
    if(starttime > 0)
    {
      if(starttime > DVD_SEC_TO_TIME(2))
        CLog::Log(LOGWARNING, "CDVDPlayer::CheckPlayerInit(%d) - Ignoring too large delay of %f", source, starttime);
      else
        SendPlayerMessage(new CDVDMsgDouble(CDVDMsg::GENERAL_DELAY, starttime), source);
    }

    SendPlayerMessage(new CDVDMsgGeneralResync(current.dts, setclock), source);
  }
  return false;
}

void CDVDPlayer::CheckContinuity(CCurrentStream& current, DemuxPacket* pPacket)
{
  if (m_playSpeed < DVD_PLAYSPEED_PAUSE)
    return;

  if( pPacket->dts == DVD_NOPTS_VALUE )
    return;

  if (current.type == STREAM_VIDEO
  && m_CurrentAudio.dts != DVD_NOPTS_VALUE 
  && m_CurrentVideo.dts != DVD_NOPTS_VALUE)
  {
    /* check for looping stillframes on non dvd's, dvd's will be detected by long duration check later */
    if( m_pInputStream && !m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      /* special case for looping stillframes THX test discs*/
      /* only affect playback when not from dvd */
      if( (m_CurrentAudio.dts > m_CurrentVideo.dts + DVD_MSEC_TO_TIME(200)) 
      && (m_CurrentVideo.dts == pPacket->dts) )
      {
        CLog::Log(LOGDEBUG, "CDVDPlayer::CheckContinuity - Detected looping stillframe");
        SyncronizePlayers(SYNCSOURCE_VIDEO);
        return;
      }
    }

    /* if we haven't received video for a while, but we have been */
    /* getting audio much more recently, make sure video wait's  */
    /* this occurs especially on thx test disc */
    if( (pPacket->dts > m_CurrentVideo.dts + DVD_MSEC_TO_TIME(200)) 
     && (pPacket->dts < m_CurrentAudio.dts + DVD_MSEC_TO_TIME(50)) )
    {
      CLog::Log(LOGDEBUG, "CDVDPlayer::CheckContinuity - Potential long duration frame");
      SyncronizePlayers(SYNCSOURCE_VIDEO);
      return;
    }
  }

  double mindts, maxdts;
  if(m_CurrentAudio.dts == DVD_NOPTS_VALUE)
    maxdts = mindts = m_CurrentVideo.dts;
  else if(m_CurrentVideo.dts == DVD_NOPTS_VALUE)
    maxdts = mindts = m_CurrentAudio.dts;
  else
  {
    maxdts = max(m_CurrentAudio.dts, m_CurrentVideo.dts);
    mindts = min(m_CurrentAudio.dts, m_CurrentVideo.dts);
  }

  /* if we don't have max and min, we can't do anything more */
  if( mindts == DVD_NOPTS_VALUE || maxdts == DVD_NOPTS_VALUE )
    return;

  /* warn if dts is moving backwords */
  if(current.dts != DVD_NOPTS_VALUE && pPacket->dts < current.dts)
    CLog::Log(LOGWARNING, "CDVDPlayer::CheckContinuity - wrapback of stream:%d, prev:%f, curr:%f, diff:%f"
                        , current.type, current.dts, pPacket->dts, pPacket->dts - current.dts);

  /* if video player is rendering a stillframe, we need to make sure */
  /* audio has finished processing it's data otherwise it will be */
  /* displayed too early */

  if( pPacket->dts < mindts - DVD_MSEC_TO_TIME(100) && current.inited)
  {
    CLog::Log(LOGWARNING, "CDVDPlayer::CheckContinuity - resyncing due to stream wrapback (%d)"
                        , current.type);
    if (m_dvdPlayerVideo.IsStalled() && m_CurrentVideo.dts != DVD_NOPTS_VALUE)
      SyncronizePlayers(SYNCSOURCE_VIDEO);
    else if (m_dvdPlayerAudio.IsStalled() && m_CurrentAudio.dts != DVD_NOPTS_VALUE)
      SyncronizePlayers(SYNCSOURCE_AUDIO);

    m_CurrentAudio.inited = false;
    m_CurrentVideo.inited = false;
    m_CurrentSubtitle.inited = false;
  }

  /* stream jump forward */
  if( pPacket->dts > maxdts + DVD_MSEC_TO_TIME(1000) && current.inited)
  {
    CLog::Log(LOGWARNING, "CDVDPlayer::CheckContinuity - stream forward jump detected (%d)"
                        , current.type);
    /* normally don't need to sync players since video player will keep playing at normal fps */
    /* after a discontinuity */
    //SyncronizePlayers(dts, pts, MSGWAIT_ALL);
    m_CurrentAudio.inited = false;
    m_CurrentVideo.inited = false;
    m_CurrentSubtitle.inited = false;
  }

}

bool CDVDPlayer::CheckSceneSkip(CCurrentStream& current)
{
  CEdl::Cut cut;

  if(!m_Edl.HaveCutpoints())
    return false;

  if(current.dts == DVD_NOPTS_VALUE)
    return false;

  if (m_Edl.InCutpoint(DVD_TIME_TO_MSEC(current.dts), &cut) && cut.CutAction == CEdl::CUT)
  {
    // check if both streams are in cut position, if they are do the seek
    if (m_CurrentAudio.id >= 0 && m_CurrentAudio.dts != DVD_NOPTS_VALUE && m_CurrentAudio.dts > DVD_MSEC_TO_TIME(cut.CutStart)
    &&  m_CurrentVideo.id >= 0 && m_CurrentVideo.dts != DVD_NOPTS_VALUE && m_CurrentVideo.dts > DVD_MSEC_TO_TIME(cut.CutStart))
      m_messenger.Put(new CDVDMsgPlayerSeek(cut.CutEnd+1, false, false, true)); // seek past cutpoint

    return true;
  }
  return false;
}


void CDVDPlayer::SyncronizeDemuxer(DWORD timeout)
{
  if(CThread::ThreadId() == GetCurrentThreadId())
    return;
  if(!m_messenger.IsInited())
    return;

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout, 0);
  message->Acquire();  
  m_messenger.Put(message);
  message->Wait(&m_bStop, 0);
  message->Release();
}

void CDVDPlayer::SyncronizePlayers(DWORD sources, double pts)
{
  /* if we are awaiting a start sync, we can't sync here or we could deadlock */
  if(m_CurrentAudio.startsync 
  || m_CurrentVideo.startsync
  || m_CurrentSubtitle.startsync)
  {
    CLog::Log(LOGDEBUG, "%s - can't sync since we are already awaiting a sync", __FUNCTION__);
    return;
  }

  /* we need a big timeout as audio queue is about 8seconds for 2ch ac3 */
  const int timeout = 10*1000; // in milliseconds

  CDVDMsgGeneralSynchronize* message = new CDVDMsgGeneralSynchronize(timeout, sources);
  if (m_CurrentAudio.id >= 0)
  {
    message->Acquire();
    m_CurrentAudio.dts = DVD_NOPTS_VALUE;
    m_CurrentAudio.startpts  = pts;
    m_CurrentAudio.startsync = message;
  }
  if (m_CurrentVideo.id >= 0)
  {
    message->Acquire();
    m_CurrentVideo.dts = DVD_NOPTS_VALUE;
    m_CurrentVideo.startpts  = pts;
    m_CurrentVideo.startsync = message;
  }
/* TODO - we have to rewrite the sync class, to not require
          all other players waiting for subtitle, should only
          be the oposite way
  if (m_CurrentSubtitle.id >= 0)
  {
    message->Acquire();
    m_CurrentSubtitle.dts = DVD_NOPTS_VALUE;
    m_CurrentSubtitle.startpts  = pts;
    m_CurrentSubtitle.startsync = message;
  }
*/
  message->Release();
}

void CDVDPlayer::SendPlayerMessage(CDVDMsg* pMsg, unsigned int target)
{
  if(target == DVDPLAYER_AUDIO)
    m_dvdPlayerAudio.SendMessage(pMsg);
  if(target == DVDPLAYER_VIDEO)
    m_dvdPlayerVideo.SendMessage(pMsg);
  if(target == DVDPLAYER_SUBTITLE)
    m_dvdPlayerSubtitle.SendMessage(pMsg);
}

void CDVDPlayer::OnExit()
{
  g_dvdPerformanceCounter.DisableMainPerformance();

  if (m_pDlgCache)
    m_pDlgCache->SetMessage(g_localizeStrings.Get(10212));

  try
  {
    CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit()");

    // set event to inform openfile something went wrong in case openfile is still waiting for this event
    SetEvent(m_hReadyEvent);
    SetCaching(false);

    // close each stream
    if (!m_bAbortRequest) CLog::Log(LOGNOTICE, "DVDPlayer: eof, waiting for queues to empty");
    if (m_CurrentAudio.id >= 0)
    {
      CLog::Log(LOGNOTICE, "DVDPlayer: closing audio stream");
      CloseAudioStream(!m_bAbortRequest);
    }
    if (m_CurrentVideo.id >= 0)
    {
      CLog::Log(LOGNOTICE, "DVDPlayer: closing video stream");
      CloseVideoStream(!m_bAbortRequest);
    }
    if (m_CurrentSubtitle.id >= 0)
    {
      CLog::Log(LOGNOTICE, "DVDPlayer: closing video stream");
      CloseSubtitleStream(!m_bAbortRequest);
    }
    // destroy the demuxer
    if (m_pDemuxer)
    {
      CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deleting demuxer");
      delete m_pDemuxer;
    }
    m_pDemuxer = NULL;

    if (m_pSubtitleDemuxer)
    {
      CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deleting subtitle demuxer");
      delete m_pSubtitleDemuxer;
    }
    m_pSubtitleDemuxer = NULL;

    // destroy the inputstream
    if (m_pInputStream)
    {
      CLog::Log(LOGNOTICE, "CDVDPlayer::OnExit() deleting input stream");
      delete m_pInputStream;
    }
    m_pInputStream = NULL;

    // clean up all selection streams
    m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NONE);

    m_messenger.End();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when trying to close down player, memory leak will follow", __FUNCTION__);
    m_pInputStream = NULL;
    m_pDemuxer = NULL;   
  }
  // set event to inform openfile something went wrong in case openfile is still waiting for this event
  SetEvent(m_hReadyEvent);
  if (m_pDlgCache)
  {
    m_pDlgCache->Close();
    m_pDlgCache = NULL;
  }

  m_bStop = true;
  // if we didn't stop playing, advance to the next item in xbmc's playlist
  if(m_PlayerOptions.identify == false)
  {
    if (m_bAbortRequest)
      m_callback.OnPlayBackStopped();
    else
      m_callback.OnPlayBackEnded();
  }
}

void CDVDPlayer::HandleMessages()
{
  CDVDMsg* pMsg;

  MsgQueueReturnCode ret = m_messenger.Get(&pMsg, 0);

  while (ret == MSGQ_OK)
  {
    LockStreams();

    try
    {
      if (pMsg->IsType(CDVDMsg::PLAYER_SEEK))
      {
        CPlayerSeek m_pause(this);

        CDVDMsgPlayerSeek &msg(*((CDVDMsgPlayerSeek*)pMsg));
        double start = DVD_NOPTS_VALUE;

        CLog::Log(LOGDEBUG, "demuxer seek to: %d", msg.GetTime());
        if (m_pDemuxer && m_pDemuxer->SeekTime(msg.GetTime(), msg.GetBackward(), &start))
        {
          CLog::Log(LOGDEBUG, "demuxer seek to: %d, success", msg.GetTime());
          if(m_pSubtitleDemuxer)
          {
            if(!m_pSubtitleDemuxer->SeekTime(msg.GetTime(), msg.GetBackward()))
              CLog::Log(LOGDEBUG, "failed to seek subtitle demuxer: %d, success", msg.GetTime());
          }
          FlushBuffers(!msg.GetFlush());
          if(msg.GetAccurate())
            SyncronizePlayers(SYNCSOURCE_ALL, start);
          else
            SyncronizePlayers(SYNCSOURCE_ALL, DVD_NOPTS_VALUE);
        }
        else
          CLog::Log(LOGWARNING, "error while seeking");

        // set flag to indicate we have finished a seeking request
        g_infoManager.m_performingSeek = false;
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SEEK_CHAPTER))
      {
        CPlayerSeek m_pause(this);

        CDVDMsgPlayerSeekChapter &msg(*((CDVDMsgPlayerSeekChapter*)pMsg));
        double start = DVD_NOPTS_VALUE;
        
        // This should always be the case. 
        if(m_pDemuxer && m_pDemuxer->SeekChapter(msg.GetChapter(), &start))
        {
          FlushBuffers(false);
          SyncronizePlayers(SYNCSOURCE_ALL, start);
        }
      }
      else if (pMsg->IsType(CDVDMsg::DEMUXER_RESET))
      {
          m_CurrentAudio.stream = NULL;
          m_CurrentVideo.stream = NULL;
          m_CurrentSubtitle.stream = NULL;

          // we need to reset the demuxer, probably because the streams have changed
          if(m_pDemuxer)
            m_pDemuxer->Reset();
          if(m_pSubtitleDemuxer)
            m_pSubtitleDemuxer->Reset();
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_AUDIOSTREAM))
      {
        CDVDMsgPlayerSetAudioStream* pMsg2 = (CDVDMsgPlayerSetAudioStream*)pMsg;        

        SelectionStream& st = m_SelectionStreams.Get(STREAM_AUDIO, pMsg2->GetStreamId());
        if(st.source != STREAM_SOURCE_NONE)
        {
          if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          {
            CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
            if(pStream->SetActiveAudioStream(st.id))
            {
              m_dvd.iSelectedAudioStream = -1;
              CloseAudioStream(false);
            }
          }
          else
          {
            CloseAudioStream(false);
            OpenAudioStream(st.id, st.source);
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM))
      {
        CDVDMsgPlayerSetSubtitleStream* pMsg2 = (CDVDMsgPlayerSetSubtitleStream*)pMsg;

        SelectionStream& st = m_SelectionStreams.Get(STREAM_SUBTITLE, pMsg2->GetStreamId());
        if(st.source != STREAM_SOURCE_NONE)
        {
          if(st.source == STREAM_SOURCE_NAV && m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          {
            CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
            if(pStream->SetActiveSubtitleStream(st.id))
            {
              m_dvd.iSelectedSPUStream = -1;
              CloseSubtitleStream(false);
            }
          }
          else
          {
            CloseSubtitleStream(false);
            OpenSubtitleStream(st.id, st.source);
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE))
      {
        CDVDMsgBool* pValue = (CDVDMsgBool*)pMsg;

        m_dvdPlayerVideo.EnableSubtitle(pValue->m_value);

        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
          static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->EnableSubtitleStream(pValue->m_value);
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_STATE))
      {
        CPlayerSeek m_pause(this);

        CDVDMsgPlayerSetState* pMsgPlayerSetState = (CDVDMsgPlayerSetState*)pMsg;

        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          std::string s = pMsgPlayerSetState->GetState();
          ((CDVDInputStreamNavigator*)m_pInputStream)->SetNavigatorState(s);
          m_dvd.state = DVDSTATE_NORMAL;
          m_dvd.iDVDStillStartTime = 0;
          m_dvd.iDVDStillTime = 0;
        }
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SET_RECORD))
      {
        if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV))
          static_cast<CDVDInputStreamTV*>(m_pInputStream)->Record(*(CDVDMsgBool*)pMsg);
      }
      else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH))
      {
        FlushBuffers(false);
      }
      else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
      {
        int speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
        
        // correct our current clock, as it would start going wrong otherwise
        if(m_State.timestamp > 0)
        {
          double offset;
          offset  = CDVDClock::GetAbsoluteClock() - m_State.timestamp;
          offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
          if(offset >  1000) offset =  1000;
          if(offset < -1000) offset = -1000;
          m_State.time     += DVD_TIME_TO_MSEC(offset);
          m_State.timestamp =  CDVDClock::GetAbsoluteClock();
        }

        // if playspeed is different then DVD_PLAYSPEED_NORMAL or DVD_PLAYSPEED_PAUSE
        // audioplayer, stops outputing audio to audiorendere, but still tries to
        // sleep an correct amount for each packet
        // videoplayer just plays faster after the clock speed has been increased
        // 1. disable audio
        // 2. skip frames and adjust their pts or the clock
        m_playSpeed = speed;
        m_caching = false;
        m_clock.SetSpeed(speed);
        m_dvdPlayerAudio.SetSpeed(speed);
        m_dvdPlayerVideo.SetSpeed(speed);

        // TODO - we really shouldn't pause demuxer 
        //        until our buffers are somewhat filled
        if(m_pDemuxer)
          m_pDemuxer->SetSpeed(speed);
      } 
      else if (pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_NEXT) || pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_PREV))
      {
        CPlayerSeek m_pause(this);

        CDVDInputStream::IChannel* input = dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream);
        if(input)
        {
          bool result;
          if(pMsg->IsType(CDVDMsg::PLAYER_CHANNEL_NEXT))
            result = input->NextChannel();
          else
            result = input->PrevChannel();

          if(result)
          {
            FlushBuffers(false);
            CloseVideoStream(false);
            CloseAudioStream(false);
            CloseSubtitleStream(false);
            SAFE_DELETE(m_pDemuxer);
          }
        }
      }
      else if (pMsg->IsType(CDVDMsg::GENERAL_GUI_ACTION))
        OnAction(((CDVDMsgType<CAction>*)pMsg)->m_value);
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - Exception thrown when handling message", __FUNCTION__);
    }
    
    UnlockStreams();
    
    pMsg->Release();
    ret = m_messenger.Get(&pMsg, 0);
  }
}

void CDVDPlayer::SetCaching(bool enabled)
{
  if(m_caching == enabled)
    return;

  if(enabled)
  {
    m_clock.SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_dvdPlayerAudio.SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_dvdPlayerVideo.SetSpeed(DVD_PLAYSPEED_PAUSE);
    m_caching = true;
  }
  else
  {
    m_clock.SetSpeed(m_playSpeed);
    m_dvdPlayerAudio.SetSpeed(m_playSpeed);
    m_dvdPlayerVideo.SetSpeed(m_playSpeed);
    m_caching = false;

    if (m_pDlgCache)
    {
      m_pDlgCache->Close();
      m_pDlgCache = NULL;
    }
  }
}

void CDVDPlayer::SetPlaySpeed(int speed)
{
  m_messenger.Put(new CDVDMsgInt(CDVDMsg::PLAYER_SETSPEED, speed));
  m_dvdPlayerAudio.SetSpeed(speed);
  m_dvdPlayerVideo.SetSpeed(speed);
  SyncronizeDemuxer(100);
}

void CDVDPlayer::Pause()
{
  if(m_playSpeed != DVD_PLAYSPEED_PAUSE && m_caching)
  {
    SetCaching(false);
    return;
  }

  // return to normal speed if it was paused before, pause otherwise
  if (m_playSpeed == DVD_PLAYSPEED_PAUSE) 
    SetPlaySpeed(DVD_PLAYSPEED_NORMAL);
  else 
    SetPlaySpeed(DVD_PLAYSPEED_PAUSE);
}

bool CDVDPlayer::IsPaused() const
{
  return (m_playSpeed == DVD_PLAYSPEED_PAUSE) || m_caching;
}

bool CDVDPlayer::HasVideo() const
{
  if (m_pInputStream)
  {
    if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || m_CurrentVideo.id >= 0) return true;
  }
  return false;
}

bool CDVDPlayer::HasAudio() const
{
  return (m_CurrentAudio.id >= 0);
}

bool CDVDPlayer::CanSeek()
{
  return GetTotalTime() > 0;
}

void CDVDPlayer::Seek(bool bPlus, bool bLargeStep)
{
#if 0
  // sadly this doesn't work for now, audio player must
  // drop packets at the same rate as we play frames
  if( m_playSpeed == DVD_PLAYSPEED_PAUSE && bPlus && !bLargeStep)
  {
    m_dvdPlayerVideo.StepFrame();
    return;
  }
#endif
  if (bLargeStep && GetChapterCount() > 1)
  {
    if(bPlus)
      SeekChapter(GetChapter() + 1);
    else
      SeekChapter(GetChapter() - 1);
    return;
  }

  __int64 seek;
  if (g_advancedSettings.m_videoUseTimeSeeking && GetTotalTime() > 2*g_advancedSettings.m_videoTimeSeekForwardBig)
  {
    if (bLargeStep)
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForwardBig : g_advancedSettings.m_videoTimeSeekBackwardBig;
    else
      seek = bPlus ? g_advancedSettings.m_videoTimeSeekForward : g_advancedSettings.m_videoTimeSeekBackward;
    seek *= 1000;
    seek += GetTime();
  }
  else
  {
    float percent;
    if (bLargeStep)
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForwardBig : g_advancedSettings.m_videoPercentSeekBackwardBig;
    else
      percent = bPlus ? g_advancedSettings.m_videoPercentSeekForward : g_advancedSettings.m_videoPercentSeekBackward;
    seek = (__int64)(GetTotalTimeInMsec()*(GetPercentage()+percent)/100);
  }

  m_messenger.Put(new CDVDMsgPlayerSeek((int)seek, !bPlus, true, false));
  SyncronizeDemuxer(100);
  m_tmLastSeek = time(NULL);
}

bool CDVDPlayer::SeekScene(bool bPlus)
{
  __int64 iScenemarker;
  if( m_Edl.HaveScenes() && m_Edl.SeekScene(bPlus,&iScenemarker) )
  {
    m_messenger.Put(new CDVDMsgPlayerSeek((int)iScenemarker, false, false, true)); 
    SyncronizeDemuxer(100);
    return true;
  }
  return false;
}

void CDVDPlayer::ToggleFrameDrop()
{
  m_dvdPlayerVideo.EnableFrameDrop( !m_dvdPlayerVideo.IsFrameDropEnabled() );
}

void CDVDPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  CSingleLock lock(m_StateSection);
  strAudioInfo.Format("D( %s ), P( %s )", m_State.demux_audio.c_str()
                                        , m_dvdPlayerAudio.GetPlayerInfo().c_str());
}

void CDVDPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  CSingleLock lock(m_StateSection);
  strVideoInfo.Format("D( %s ), P( %s )", m_State.demux_video.c_str()
                                        , m_dvdPlayerVideo.GetPlayerInfo().c_str());
}

void CDVDPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  if (!m_bStop)
  {
    double dDelay = (double)m_dvdPlayerVideo.GetDelay() / DVD_TIME_BASE;

    double apts = m_dvdPlayerAudio.GetCurrentPts();
    double vpts = m_dvdPlayerVideo.GetCurrentPts();
    double dDiff = 0;
    char cEdlStatus;

    if( apts != DVD_NOPTS_VALUE && vpts != DVD_NOPTS_VALUE )
      dDiff = (apts - vpts) / DVD_TIME_BASE;

    int iFramesDropped = m_dvdPlayerVideo.GetNrOfDroppedFrames();
    cEdlStatus = m_Edl.GetEdlStatus();
    CSingleLock lock(g_VDPAUSection);
    if (g_VDPAU)
      strGeneralInfo.Format("DVD Player %s ad:%6.3f, a/v:%6.3f, dropped:%d, cpu: %i%%. edl: %c source bitrate: %4.2f MBit/s", (g_VDPAU->usingVDPAU) ? "(VDPAU)" : "", dDelay, dDiff, iFramesDropped, (int)(CThread::GetRelativeUsage()*100), cEdlStatus, (double)GetSourceBitrate() / (1024.0*1024.0));
    else
      strGeneralInfo.Format("DVD Player ad:%6.3f, a/v:%6.3f, dropped:%d, cpu: %i%%. edl: %c source bitrate: %4.2f MBit/s", dDelay, dDiff, iFramesDropped, (int)(CThread::GetRelativeUsage()*100), cEdlStatus, (double)GetSourceBitrate() / (1024.0*1024.0));

  }
}

void CDVDPlayer::SeekPercentage(float iPercent)
{
  __int64 iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return;

  SeekTime((__int64)(iTotalTime * iPercent / 100));
  m_tmLastSeek = time(NULL);
}

float CDVDPlayer::GetPercentage()
{
  __int64 iTotalTime = GetTotalTimeInMsec();

  if (!iTotalTime)
    return 0.0f;

  return GetTime() * 100 / (float)iTotalTime;
}

void CDVDPlayer::SetAVDelay(float fValue)
{
  m_dvdPlayerVideo.SetDelay( (fValue * DVD_TIME_BASE) ) ;
}

float CDVDPlayer::GetAVDelay()
{
  return m_dvdPlayerVideo.GetDelay() / (float)DVD_TIME_BASE;
}

void CDVDPlayer::SetSubTitleDelay(float fValue)
{
  m_dvdPlayerVideo.SetSubtitleDelay(-fValue * DVD_TIME_BASE);
}

float CDVDPlayer::GetSubTitleDelay()
{
  return -m_dvdPlayerVideo.GetSubtitleDelay() / DVD_TIME_BASE;
}

// priority: 1: libdvdnav, 2: external subtitles, 3: muxed subtitles
int CDVDPlayer::GetSubtitleCount()
{
  LockStreams();
  m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
  UnlockStreams();
  return m_SelectionStreams.Count(STREAM_SUBTITLE);
}

int CDVDPlayer::GetSubtitle()
{
  return m_SelectionStreams.IndexOf(STREAM_SUBTITLE, *this);
}

void CDVDPlayer::GetSubtitleName(int iStream, CStdString &strStreamName)
{
  strStreamName = "";
  SelectionStream& s = m_SelectionStreams.Get(STREAM_SUBTITLE, iStream);
  if(s.name.length() > 0)
    strStreamName = s.name;
  else
    strStreamName = "Unknown";

  if(s.type == STREAM_NONE)
    strStreamName += "(Invalid)";
}

void CDVDPlayer::SetSubtitle(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetSubtitleStream(iStream));
}

bool CDVDPlayer::GetSubtitleVisible()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    if(pStream->IsInMenu())
      return g_stSettings.m_currentVideoSettings.m_SubtitleOn;
    else
      return pStream->IsSubtitleStreamEnabled();
  }

  return m_dvdPlayerVideo.IsSubtitleEnabled();
}

void CDVDPlayer::SetSubtitleVisible(bool bVisible)
{
  g_stSettings.m_currentVideoSettings.m_SubtitleOn = bVisible;
  m_messenger.Put(new CDVDMsgBool(CDVDMsg::PLAYER_SET_SUBTITLESTREAM_VISIBLE, bVisible));
}

int CDVDPlayer::GetAudioStreamCount()
{
  LockStreams();
  m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);
  UnlockStreams();
  return m_SelectionStreams.Count(STREAM_AUDIO);
}

int CDVDPlayer::GetAudioStream()
{
  return m_SelectionStreams.IndexOf(STREAM_AUDIO, *this);
}

void CDVDPlayer::GetAudioStreamName(int iStream, CStdString& strStreamName)
{
  strStreamName = "";
  SelectionStream& s = m_SelectionStreams.Get(STREAM_AUDIO, iStream);
  if(s.name.length() > 0)
    strStreamName += s.name;
  else
    strStreamName += "Unknown";

  if(s.type == STREAM_NONE)
    strStreamName += " (Invalid)";
}

void CDVDPlayer::SetAudioStream(int iStream)
{
  m_messenger.Put(new CDVDMsgPlayerSetAudioStream(iStream));
  SyncronizeDemuxer(100);
}

void CDVDPlayer::SeekTime(__int64 iTime)
{
  m_messenger.Put(new CDVDMsgPlayerSeek((int)iTime, true, true, true));
  SyncronizeDemuxer(100);
  m_tmLastSeek = time(NULL);
}

// return the time in milliseconds
__int64 CDVDPlayer::GetTime()
{
  CSingleLock lock(m_StateSection);
  double offset = 0;
  if(m_State.timestamp > 0)
  {
    offset  = CDVDClock::GetAbsoluteClock() - m_State.timestamp;
    offset *= m_playSpeed / DVD_PLAYSPEED_NORMAL;
    if(offset >  1000) offset =  1000;
    if(offset < -1000) offset = -1000;
  }
  return m_State.time + DVD_TIME_TO_MSEC(offset);
}

// return length in msec
__int64 CDVDPlayer::GetTotalTimeInMsec()
{
  CSingleLock lock(m_StateSection);
  return m_State.time_total;
}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int CDVDPlayer::GetTotalTime()
{
  return (int)(GetTotalTimeInMsec() / 1000);
}

void CDVDPlayer::ToFFRW(int iSpeed)
{
  // can't rewind in menu as seeking isn't possible
  // forward is fine
  if (iSpeed < 0 && IsInMenu()) return;
  SetPlaySpeed(iSpeed * DVD_PLAYSPEED_NORMAL);
}

bool CDVDPlayer::OpenAudioStream(int iStream, int source)
{
  CLog::Log(LOGNOTICE, "Opening audio stream: %i source: %i", iStream, source);

  if (!m_pDemuxer)
    return false;
  
  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if (!pStream || pStream->disabled)
    return false;

  if( m_CurrentAudio.id < 0 &&  m_CurrentVideo.id >= 0 )
  {
    // up until now we wheren't playing audio, but we did play video
    // this will change what is used to sync the dvdclock.
    // since the new audio data doesn't have to have any relation
    // to the current video data in the packet que, we have to
    // wait for it to empty

    // this happens if a new cell has audio data, but previous didn't
    // and both have video data

    SyncronizePlayers(SYNCSOURCE_AUDIO);
  }

  CDVDStreamInfo hint(*pStream, true);

  if(m_CurrentAudio.id    < 0 
  || m_CurrentAudio.hint != hint)
  {
    if(m_CurrentAudio.id >= 0)
    {
      CLog::Log(LOGDEBUG, " - codecs hints have changed, must close previous stream");
      CloseAudioStream(true);
    }

    if (!m_dvdPlayerAudio.OpenStream( hint ))
    {
      /* mark stream as disabled, to disallaw further attempts*/
      CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
      pStream->disabled = true;
      pStream->SetDiscard(AVDISCARD_ALL);
      return false;
    }
  }
  else
    m_dvdPlayerAudio.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));

  /* store information about stream */
  m_CurrentAudio.id = iStream;
  m_CurrentAudio.source = source;
  m_CurrentAudio.hint = hint;
  m_CurrentAudio.stream = (void*)pStream;

  /* audio normally won't consume full cpu, so let it have prio */
  m_dvdPlayerAudio.SetPriority(GetThreadPriority(*this)+1);

  return true;
}

bool CDVDPlayer::OpenVideoStream(int iStream, int source)
{
  CLog::Log(LOGNOTICE, "Opening video stream: %i source: %i", iStream, source);

  if (!m_pDemuxer)
    return false;
  
  CDemuxStream* pStream = m_pDemuxer->GetStream(iStream);
  if(!pStream && pStream->disabled)
    return false;
  pStream->SetDiscard(AVDISCARD_NONE);

  CDVDStreamInfo hint(*pStream, true);

  if( m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) )
  {
    /* set aspect ratio as requested by navigator for dvd's */
    float aspect = static_cast<CDVDInputStreamNavigator*>(m_pInputStream)->GetVideoAspectRatio();
    if(aspect != 0.0)
      hint.aspect = aspect;
  }

  if(m_CurrentVideo.id    < 0 
  || m_CurrentVideo.hint != hint)
  {
    if(m_CurrentVideo.id >= 0)
    {
      CLog::Log(LOGDEBUG, " - codecs hints have changed, must close previous stream");
      CloseVideoStream(true);
    }

    if (!m_dvdPlayerVideo.OpenStream(hint))
    {
      /* mark stream as disabled, to disallaw further attempts */
      CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
      pStream->disabled = true;
      pStream->SetDiscard(AVDISCARD_ALL);
      return false;
    }
  }
  else
    m_dvdPlayerVideo.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));

  /* store information about stream */
  m_CurrentVideo.id = iStream;
  m_CurrentVideo.source = source;
  m_CurrentVideo.hint = hint;
  m_CurrentVideo.stream = (void*)pStream;

  /* use same priority for video thread as demuxing thread, as */
  /* otherwise demuxer will starve if video consumes the full cpu */
  m_dvdPlayerVideo.SetPriority(GetThreadPriority(*this));
  return true;

}

bool CDVDPlayer::OpenSubtitleStream(int iStream, int source)
{
  CLog::Log(LOGNOTICE, "Opening Subtitle stream: %i source: %i", iStream, source);

  CDemuxStream* pStream = NULL;
  std::string filename;
  CDVDStreamInfo hint;

  if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_DEMUX_SUB)
  {
    int index = m_SelectionStreams.IndexOf(STREAM_SUBTITLE, source, iStream);
    if(index < 0)
      return false;
    SelectionStream st = m_SelectionStreams.Get(STREAM_SUBTITLE, index);

    if(!m_pSubtitleDemuxer || m_pSubtitleDemuxer->GetFileName() != st.filename)
    {
      CLog::Log(LOGNOTICE, "Opening Subtitle file: %s", st.filename.c_str());
      auto_ptr<CDVDDemuxVobsub> demux(new CDVDDemuxVobsub());
      if(!demux->Open(st.filename))      
        return false;
      m_pSubtitleDemuxer = demux.release();
    }

    pStream = m_pSubtitleDemuxer->GetStream(iStream);
    if(!pStream || pStream->disabled)
      return false;
    pStream->SetDiscard(AVDISCARD_NONE);
    double pts = m_dvdPlayerVideo.GetCurrentPts();
    if(pts == DVD_NOPTS_VALUE)
      pts = m_CurrentVideo.dts;
    if(pts != DVD_NOPTS_VALUE)
      m_pSubtitleDemuxer->SeekTime((int)(1000.0 * pts / (double)DVD_TIME_BASE));

    hint.Assign(*pStream, true);
  }
  else if(STREAM_SOURCE_MASK(source) == STREAM_SOURCE_TEXT)
  {
    int index = m_SelectionStreams.IndexOf(STREAM_SUBTITLE, source, iStream);
    if(index < 0)
      return false;
    filename = m_SelectionStreams.Get(STREAM_SUBTITLE, index).filename;

    hint.Clear();
    hint.fpsscale = m_CurrentVideo.hint.fpsscale;
    hint.fpsrate  = m_CurrentVideo.hint.fpsrate;
  }
  else
  {
    if(!m_pDemuxer)
      return false;
    pStream = m_pDemuxer->GetStream(iStream);
    if(!pStream || pStream->disabled)
      return false;
    pStream->SetDiscard(AVDISCARD_NONE);

    hint.Assign(*pStream, true);

    if(m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
      filename = "dvd";
  }

  if(m_CurrentSubtitle.id    < 0 
  || m_CurrentSubtitle.hint != hint) 
  {
    if(m_CurrentSubtitle.id >= 0)
    {
      CLog::Log(LOGDEBUG, " - codecs hints have changed, must close previous stream");
      CloseSubtitleStream(false);
    }

    if(!m_dvdPlayerSubtitle.OpenStream(hint, filename))
    {
      CLog::Log(LOGWARNING, "%s - Unsupported stream %d. Stream disabled.", __FUNCTION__, iStream);
      if(pStream)
      {
        pStream->disabled = true;
        pStream->SetDiscard(AVDISCARD_ALL);
      }
      return false;
    }
  }
  else
    m_dvdPlayerSubtitle.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));

  m_CurrentSubtitle.id     = iStream;
  m_CurrentSubtitle.source = source;
  m_CurrentSubtitle.hint   = hint;
  m_CurrentSubtitle.stream = (void*)pStream;

  return true;
}

bool CDVDPlayer::CloseAudioStream(bool bWaitForBuffers)
{
  if (m_CurrentAudio.id < 0)
    return false;

  CLog::Log(LOGNOTICE, "Closing audio stream");

  m_dvdPlayerAudio.CloseStream(bWaitForBuffers);

  m_CurrentAudio.Clear();
  return true;
}

bool CDVDPlayer::CloseVideoStream(bool bWaitForBuffers)
{
  if (m_CurrentVideo.id < 0) 
    return false;

  CLog::Log(LOGNOTICE, "Closing video stream");  

  m_dvdPlayerVideo.CloseStream(bWaitForBuffers);

  m_CurrentVideo.Clear();
  return true;
}

bool CDVDPlayer::CloseSubtitleStream(bool bKeepOverlays)
{
  if (m_CurrentSubtitle.id < 0) 
    return false;

  CLog::Log(LOGNOTICE, "Closing subtitle stream");

  m_dvdPlayerSubtitle.CloseStream(!bKeepOverlays);

  m_CurrentSubtitle.Clear();
  return true;
}

void CDVDPlayer::FlushBuffers(bool queued)
{
  if(queued) 
  {
    m_dvdPlayerAudio.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
    m_dvdPlayerVideo.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
    m_dvdPlayerVideo.SendMessage(new CDVDMsg(CDVDMsg::VIDEO_NOSKIP));
    m_dvdPlayerSubtitle.SendMessage(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
    SyncronizePlayers(SYNCSOURCE_ALL);
  } 
  else
  {
    m_dvdPlayerAudio.Flush();
    m_dvdPlayerVideo.Flush();
    m_dvdPlayerSubtitle.Flush();

    // clear subtitle and menu overlays
    m_overlayContainer.Clear();
  }
  m_CurrentAudio.inited = false;
  m_CurrentVideo.inited = false;
  m_CurrentSubtitle.inited = false;  
}

// since we call ffmpeg functions to decode, this is being called in the same thread as ::Process() is
int CDVDPlayer::OnDVDNavResult(void* pData, int iMessage)
{
  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    switch (iMessage)
    {
    case DVDNAV_STILL_FRAME:
      {
        //CLog::Log(LOGDEBUG, "DVDNAV_STILL_FRAME");

        dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)pData;
        // should wait the specified time here while we let the player running
        // after that call dvdnav_still_skip(m_dvdnav);

        if (m_dvd.state != DVDSTATE_STILL)
        {
          // else notify the player we have received a still frame

          if(still_event->length < 0xff)
            m_dvd.iDVDStillTime = still_event->length * 1000;
          else
            m_dvd.iDVDStillTime = 0;

          m_dvd.iDVDStillStartTime = GetTickCount();

          /* adjust for the output delay in the video queue */
          DWORD time = 0;
          if( m_CurrentVideo.stream && m_dvd.iDVDStillTime > 0 )
          {
            time = (DWORD)(m_dvdPlayerVideo.GetOutputDelay() / ( DVD_TIME_BASE / 1000 ));
            if( time < 10000 && time > 0 ) 
              m_dvd.iDVDStillTime += time;
          }
          m_dvd.state = DVDSTATE_STILL;
          CLog::Log(LOGDEBUG,
                    "DVDNAV_STILL_FRAME - waiting %i sec, with delay of %d sec",
                    still_event->length, time / 1000);
        }
        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_SPU_CLUT_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_SPU_CLUT_CHANGE");
        m_dvdPlayerSubtitle.SendMessage(new CDVDMsgSubtitleClutChange((BYTE*)pData));
      }
      break;
    case DVDNAV_SPU_STREAM_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_SPU_STREAM_CHANGE");

        dvdnav_spu_stream_change_event_t* event = (dvdnav_spu_stream_change_event_t*)pData;

        int iStream = event->physical_wide;
        bool visible = !(iStream & 0x80);

        m_dvdPlayerVideo.EnableSubtitle(visible);

        if (iStream >= 0)
          m_dvd.iSelectedSPUStream = (iStream & ~0x80);
        else
          m_dvd.iSelectedSPUStream = -1;

        m_CurrentSubtitle.stream = NULL;
      }
      break;
    case DVDNAV_AUDIO_STREAM_CHANGE:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_AUDIO_STREAM_CHANGE");

        // This should be the correct way i think, however we don't have any streams right now
        // since the demuxer hasn't started so it doesn't change. not sure how to do this.
        dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)pData;

        // Tell system what audiostream should be opened by default
        if (event->logical >= 0)
          m_dvd.iSelectedAudioStream = event->physical;
        else
          m_dvd.iSelectedAudioStream = -1;

        m_CurrentAudio.stream = NULL;
      }
      break;
    case DVDNAV_HIGHLIGHT:
      {
        //dvdnav_highlight_event_t* pInfo = (dvdnav_highlight_event_t*)pData;
        int iButton = pStream->GetCurrentButton();
        CLog::Log(LOGDEBUG, "DVDNAV_HIGHLIGHT: Highlight button %d\n", iButton);        
        m_dvdPlayerSubtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_NORMAL);
      }
      break;
    case DVDNAV_VTS_CHANGE:
      {
        //dvdnav_vts_change_event_t* vts_change_event = (dvdnav_vts_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_VTS_CHANGE");

        //Make sure we clear all the old overlays here, or else old forced items are left.
        m_overlayContainer.Clear();

        // reset the demuxer, this also imples closing the video and the audio system
        // this is a bit tricky cause it's the demuxer that's is making this call in the end
        // so we send a message to indicate the main loop that the demuxer needs a reset
        // this also means the libdvdnav may not return any data packets after this command
        m_messenger.Put(new CDVDMsgDemuxerReset());

        //Force an aspect ratio that is set in the dvdheaders if available
        m_CurrentVideo.hint.aspect = pStream->GetVideoAspectRatio();
        if( m_dvdPlayerAudio.m_messageQueue.IsInited() )
          m_dvdPlayerVideo.SendMessage(new CDVDMsgDouble(CDVDMsg::VIDEO_SET_ASPECT, m_CurrentVideo.hint.aspect));

        m_SelectionStreams.Clear(STREAM_NONE, STREAM_SOURCE_NAV);
        m_SelectionStreams.Update(m_pInputStream, m_pDemuxer);

        // we must hold here once more, otherwise the demuxer message
        // will be executed after demuxer have filled with data
        return NAVRESULT_HOLD;
      }
      break;
    case DVDNAV_CELL_CHANGE:
      {
        //dvdnav_cell_change_event_t* cell_change_event = (dvdnav_cell_change_event_t*)pData;
        CLog::Log(LOGDEBUG, "DVDNAV_CELL_CHANGE");

        m_dvd.state = DVDSTATE_NORMAL;        
        
        if( m_dvdPlayerVideo.m_messageQueue.IsInited() )
          m_dvdPlayerVideo.SendMessage(new CDVDMsg(CDVDMsg::VIDEO_NOSKIP));
      }
      break;
    case DVDNAV_NAV_PACKET:
      {
          //pci_t* pci = (pci_t*)pData;

          // this should be possible to use to make sure we get
          // seamless transitions over these boundaries
          // if we remember the old vobunits boundaries
          // when a packet comes out of demuxer that has
          // pts values outside that boundary, it belongs
          // to the new vobunit, wich has new timestamps
      }
      break;
    case DVDNAV_HOP_CHANNEL:
      {
        // This event is issued whenever a non-seamless operation has been executed.
        // Applications with fifos should drop the fifos content to speed up responsiveness.
        CLog::Log(LOGDEBUG, "DVDNAV_HOP_CHANNEL");
        m_messenger.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
        return NAVRESULT_ERROR;
      }
      break;
    case DVDNAV_STOP:
      {
        CLog::Log(LOGDEBUG, "DVDNAV_STOP");
        m_dvd.state = DVDSTATE_NORMAL;
      }
      break;
    default:
    {}
      break;
    }
  }
  return NAVRESULT_NOP;
}

bool CDVDPlayer::OnAction(const CAction &action)
{
#define THREAD_ACTION(action) \
  do { \
    if(GetCurrentThreadId() != CThread::ThreadId()) { \
      m_messenger.Put(new CDVDMsgType<CAction>(CDVDMsg::GENERAL_GUI_ACTION, action)); \
      return true; \
    } \
  } while(false)

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;


    if( m_dvd.state == DVDSTATE_STILL && m_dvd.iDVDStillTime != 0 && pStream->GetTotalButtons() == 0 )
    {
      switch(action.wID)
      {
        case ACTION_NEXT_ITEM:
        case ACTION_MOVE_RIGHT:
        case ACTION_MOVE_UP:
        case ACTION_SELECT_ITEM:
          {
            THREAD_ACTION(action);
            /* this will force us out of the stillframe */
            CLog::Log(LOGDEBUG, "%s - User asked to exit stillframe", __FUNCTION__);
            m_dvd.iDVDStillStartTime = 0;
            m_dvd.iDVDStillTime = 1;            
          }
          return true;
      }
    }


    switch (action.wID)
    {
    case ACTION_PREV_ITEM:  // SKIP-:
      {
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - pushed prev");
        pStream->OnPrevious();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      break;
    case ACTION_NEXT_ITEM:  // SKIP+:
      {
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - pushed next");
        pStream->OnNext();
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      break;
    case ACTION_SHOW_VIDEOMENU:   // start button
      {
        THREAD_ACTION(action);
        CLog::Log(LOGDEBUG, " - go to menu");
        pStream->OnMenu();
        // send a message to everyone that we've gone to the menu
        CGUIMessage msg(GUI_MSG_VIDEO_MENU_STARTED, 0, 0);
        g_graphicsContext.SendMessage(msg);
        return true;
      }
      break;
    }

    if (pStream->IsInMenu())
    {
      switch (action.wID)
      {
      case ACTION_PREVIOUS_MENU:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - menu back");
          pStream->OnBack();
        }
        break;
      case ACTION_MOVE_LEFT:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - move left");
          pStream->OnLeft();
        }
        break;
      case ACTION_MOVE_RIGHT:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - move right");
          pStream->OnRight();
        }
        break;
      case ACTION_MOVE_UP:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - move up");
          pStream->OnUp();
        }
        break;
      case ACTION_MOVE_DOWN:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - move down");
          pStream->OnDown();
        }
        break;

      case ACTION_MOUSE:
        {
          // check the action
          CAction action2 = action;
          action2.m_dwButtonCode = g_Mouse.bClick[MOUSE_LEFT_BUTTON] ? 1 : 0;
          action2.fAmount1 = g_Mouse.GetLocation().x;
          action2.fAmount2 = g_Mouse.GetLocation().y;

          RECT rs, rd;
          GetVideoRect(rs, rd);
          if (action2.fAmount1 < rd.left || action2.fAmount1 > rd.right ||
              action2.fAmount2 < rd.top || action2.fAmount2 > rd.bottom)
            return false; // out of bounds
          THREAD_ACTION(action2);
          // convert to video coords...
          CPoint pt(action2.fAmount1, action2.fAmount2);
          pt -= CPoint(rd.left, rd.top);
          pt.x *= (float)(rs.right - rs.left) / (rd.right - rd.left);
          pt.y *= (float)(rs.bottom - rs.top) / (rd.bottom - rd.top);
          pt += CPoint(rs.left, rs.top);
          if (action2.m_dwButtonCode)
            return pStream->OnMouseClick(pt);
          return pStream->OnMouseMove(pt);
        }
        break;
      case ACTION_SHOW_OSD:
      case ACTION_SELECT_ITEM:
        {
          THREAD_ACTION(action);
          CLog::Log(LOGDEBUG, " - button select");
          // show button pushed overlay
          m_dvdPlayerSubtitle.UpdateOverlayInfo((CDVDInputStreamNavigator*)m_pInputStream, LIBDVDNAV_BUTTON_CLICKED);

          pStream->ActivateButton();
        }
        break;
      case REMOTE_0:
      case REMOTE_1:
      case REMOTE_2:
      case REMOTE_3:
      case REMOTE_4:
      case REMOTE_5:
      case REMOTE_6:
      case REMOTE_7:
      case REMOTE_8:
      case REMOTE_9:
        {
          THREAD_ACTION(action);
          // Offset from key codes back to button number
          int button = action.wID - REMOTE_0;
          CLog::Log(LOGDEBUG, " - button pressed %d", button);
          pStream->SelectButton(button);
        }
       break;
      default:
        return false;
        break;
      }
      return true; // message is handled
    }
  }

  if (dynamic_cast<CDVDInputStream::IChannel*>(m_pInputStream))
  {
    switch (action.wID)
    {
      case ACTION_NEXT_ITEM:
      case ACTION_PAGE_UP:
        m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_CHANNEL_NEXT));
        g_infoManager.SetDisplayAfterSeek();
        return true;
      break;

      case ACTION_PREV_ITEM:
      case ACTION_PAGE_DOWN:
        m_messenger.Put(new CDVDMsg(CDVDMsg::PLAYER_CHANNEL_PREV));
        g_infoManager.SetDisplayAfterSeek();
        return true;
      break;
    }
  }

  switch (action.wID)
  {
    case ACTION_NEXT_ITEM:
    case ACTION_PAGE_UP:
      if(GetChapterCount() > 0) 
      {
        m_messenger.Put(new CDVDMsgPlayerSeekChapter(GetChapter()+1));
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
    case ACTION_PREV_ITEM:
    case ACTION_PAGE_DOWN:
      if(GetChapterCount() > 0) 
      {
        m_messenger.Put(new CDVDMsgPlayerSeekChapter(GetChapter()-1));
        g_infoManager.SetDisplayAfterSeek();
        return true;
      }
      else
        break;
  }

  // return false to inform the caller we didn't handle the message
  return false;
}

bool CDVDPlayer::IsInMenu() const
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;
    if( m_dvd.state == DVDSTATE_STILL )
      return true;
    else
      return pStream->IsInMenu();
  }
  return false;
}

bool CDVDPlayer::HasMenu()
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    return true;
  else
    return false;
}

bool CDVDPlayer::GetCurrentSubtitle(CStdString& strSubtitle)
{
  double pts = m_clock.GetClock();

  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    return false;

  return m_dvdPlayerSubtitle.GetCurrentSubtitle(strSubtitle, pts - m_dvdPlayerVideo.GetSubtitleDelay());
}

CStdString CDVDPlayer::GetPlayerState()
{
  if (!m_pInputStream) return "";

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    CDVDInputStreamNavigator* pStream = (CDVDInputStreamNavigator*)m_pInputStream;

    std::string buffer;
    if( pStream->GetNavigatorState(buffer) ) return buffer;
  }

  return "";
}

bool CDVDPlayer::SetPlayerState(CStdString state)
{
  m_messenger.Put(new CDVDMsgPlayerSetState(state));
  return true;
}

int CDVDPlayer::GetChapterCount()
{
  CSingleLock lock(m_StateSection);
  return m_State.chapter_count;
}

int CDVDPlayer::GetChapter()
{
  CSingleLock lock(m_StateSection);
  return m_State.chapter;
}

void CDVDPlayer::GetChapterName(CStdString& strChapterName)
{
  CSingleLock lock(m_StateSection);
  strChapterName = m_State.chapter_name;
}

int CDVDPlayer::SeekChapter(int iChapter)
{
  if (GetChapterCount() > 0)
  {
    if (iChapter < 0)
      iChapter = 0;
    if (iChapter > GetChapterCount())
      return 0;
      
    // Seek to the chapter.
    m_messenger.Put(new CDVDMsgPlayerSeekChapter(iChapter));
    SyncronizeDemuxer(100);
    m_tmLastSeek = time(NULL);
  }
  else
  {
    // Do a regular big jump.
    if (iChapter > GetChapter())
      Seek(true, true);
    else
      Seek(false, true);
  }
  return 0;
}

bool CDVDPlayer::AddSubtitle(const CStdString& strSubPath)
{
  return AddSubtitleFile(strSubPath);
}

int CDVDPlayer::GetCacheLevel() const
{
  int a = min(100,100 * m_dvdPlayerAudio.m_messageQueue.GetDataSize() / m_dvdPlayerAudio.m_messageQueue.GetMaxDataSize());
  int v = min(100,100 * m_dvdPlayerVideo.m_messageQueue.GetDataSize() / m_dvdPlayerVideo.m_messageQueue.GetMaxDataSize());
  return max(a, v);
}

int CDVDPlayer::GetAudioBitrate()
{
  return m_dvdPlayerAudio.GetAudioBitrate();
}

int CDVDPlayer::GetVideoBitrate()
{
  return m_dvdPlayerVideo.GetVideoBitrate();
}

int CDVDPlayer::GetSourceBitrate()
{
  if (m_pInputStream)
    return (int)m_pInputStream->GetBitstreamStats().GetBitrate();

  return 0;
}


bool CDVDPlayer::AddSubtitleFile(const std::string& filename)
{
  std::string ext = CUtil::GetExtension(filename);
  if(ext == ".idx")
  {
    CDVDDemuxVobsub v;
    if(!v.Open(filename))
      return false;

    m_SelectionStreams.Update(NULL, &v);
    return true;
  }
  else if(ext == ".sub")
  {
    return false;
  }
  else
  {
    SelectionStream s;
    s.source   = m_SelectionStreams.Source(STREAM_SOURCE_TEXT, filename);
    s.type     = STREAM_SUBTITLE;
    s.id       = 0;
    s.filename = filename;
    s.name     = CUtil::GetFileName(filename);
    m_SelectionStreams.Update(s);
    return true;
  }
  
  return 0;
}

void CDVDPlayer::UpdatePlayState(double timeout)
{
  CSingleLock lock(m_StateSection);

  if(m_State.timestamp != 0 
  && m_State.timestamp + DVD_MSEC_TO_TIME(timeout) > CDVDClock::GetAbsoluteClock())
    return;

  if(m_pDemuxer)
  {
    m_State.chapter       = m_pDemuxer->GetChapter();
    m_State.chapter_count = m_pDemuxer->GetChapterCount();
    m_pDemuxer->GetChapterName(m_State.chapter_name);

    m_State.time       = DVD_TIME_TO_MSEC(m_clock.GetClock());
    m_State.time_total = m_pDemuxer->GetStreamLength();
  }

  if(m_pInputStream)
  {
    // override from input stream if needed

    if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV))
    {
      m_State.canrecord = static_cast<CDVDInputStreamTV*>(m_pInputStream)->CanRecord();
      m_State.recording = static_cast<CDVDInputStreamTV*>(m_pInputStream)->IsRecording();
    }

    if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      if(m_dvd.state == DVDSTATE_STILL) 
      {
        m_State.time       = GetTickCount() - m_dvd.iDVDStillStartTime;
        m_State.time_total = m_dvd.iDVDStillTime;
      }
      else
      {
        m_State.time       = ((CDVDInputStreamNavigator*)m_pInputStream)->GetTime();
        m_State.time_total = ((CDVDInputStreamNavigator*)m_pInputStream)->GetTotalTime();
      }
    }

    else if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV))
    {
      if(((CDVDInputStreamTV*)m_pInputStream)->GetTotalTime() > 0)
      {
        m_State.time      -= ((CDVDInputStreamTV*)m_pInputStream)->GetStartTime();
        m_State.time_total = ((CDVDInputStreamTV*)m_pInputStream)->GetTotalTime();
      }
    }
  }

  if (m_Edl.HaveCutpoints())
  {
    m_State.time =  m_Edl.RemoveCutTime(m_State.time);
    m_State.time -= m_Edl.TotalCutTime();
  }

  if (m_CurrentAudio.id >= 0 && m_pDemuxer)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_CurrentAudio.id);
    if (pStream && pStream->type == STREAM_AUDIO)
      ((CDemuxStreamAudio*)pStream)->GetStreamInfo(m_State.demux_audio);
  }
  else
    m_State.demux_audio = "";

  if (m_CurrentVideo.id >= 0 && m_pDemuxer)
  {
    CDemuxStream* pStream = m_pDemuxer->GetStream(m_CurrentVideo.id);
    if (pStream && pStream->type == STREAM_VIDEO)
      ((CDemuxStreamVideo*)pStream)->GetStreamInfo(m_State.demux_video);
  }
  else
    m_State.demux_video = "";

  m_State.timestamp = CDVDClock::GetAbsoluteClock();
}

void CDVDPlayer::UpdateApplication(double timeout)
{
  if(m_UpdateApplication != 0 
  || m_UpdateApplication + DVD_MSEC_TO_TIME(timeout) > CDVDClock::GetAbsoluteClock())
    return;

  if (m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV))
  {
    CDVDInputStreamTV* pStream = static_cast<CDVDInputStreamTV*>(m_pInputStream);

    CFileItem item(g_application.CurrentFileItem());
    if(pStream->UpdateItem(item))
    {
      g_application.CurrentFileItem() = item;
      g_infoManager.SetCurrentItem(item);
    }
  }
  m_UpdateApplication = CDVDClock::GetAbsoluteClock();
}

bool CDVDPlayer::CanRecord()
{
  CSingleLock lock(m_StateSection);
  return m_State.canrecord;
}

bool CDVDPlayer::IsRecording()
{
  CSingleLock lock(m_StateSection);
  return m_State.recording;
}

bool CDVDPlayer::Record(bool bOnOff)
{
  if (m_pInputStream && m_pInputStream->IsStreamType(DVDSTREAM_TYPE_TV))
  {
    m_messenger.Put(new CDVDMsgBool(CDVDMsg::PLAYER_SET_RECORD, bOnOff));
    return true;
  }
  return false;
}

CDVDPlayer::CPlayerSeek::CPlayerSeek(CDVDPlayer* player)
      : m_player(*player)
{
  if(m_player.m_playSpeed != DVD_PLAYSPEED_NORMAL)
    return;

  if(m_player.m_caching)
    return;

  g_infoManager.SetDisplayAfterSeek(100000);
  m_player.m_seeking = true;
  m_player.m_playSpeed = DVD_PLAYSPEED_PAUSE;
  m_player.m_clock.SetSpeed(DVD_PLAYSPEED_PAUSE);
  m_player.m_dvdPlayerAudio.SetSpeed(DVD_PLAYSPEED_PAUSE);
  m_player.m_dvdPlayerVideo.SetSpeed(DVD_PLAYSPEED_PAUSE);
}

CDVDPlayer::CPlayerSeek::~CPlayerSeek()
{
  if(m_player.m_playSpeed != DVD_PLAYSPEED_PAUSE)
    return;

  if(m_player.m_caching)
    return;
  
  g_infoManager.SetDisplayAfterSeek();
  m_player.m_playSpeed = DVD_PLAYSPEED_NORMAL;
  m_player.m_dvdPlayerAudio.SetSpeed(DVD_PLAYSPEED_NORMAL);
  m_player.m_dvdPlayerVideo.SetSpeed(DVD_PLAYSPEED_NORMAL);
  m_player.m_clock.SetSpeed(DVD_PLAYSPEED_NORMAL);
  m_player.m_seeking = false;
}
