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

#include "threads/SystemClock.h"
#include "system.h"
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifdef _LINUX
#include "stdint.h"
#endif
#include "DVDDemuxFFmpeg.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DVDInputStreams/DVDInputStreamNavigator.h"
#include "DVDInputStreams/DVDInputStreamBluray.h"
#include "DVDInputStreams/DVDInputStreamPVRManager.h"
#include "DVDDemuxUtils.h"
#include "DVDClock.h" // for DVD_TIME_BASE
#include "utils/Win32Exception.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "utils/log.h"
#include "threads/Thread.h"
#include "utils/TimeUtils.h"

void CDemuxStreamAudioFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!m_stream) return;
  char temp[128];
  m_parent->m_dllAvCodec.avcodec_string(temp, 128, m_stream->codec, 0);
  strInfo = temp;
}

void CDemuxStreamAudioFFmpeg::GetStreamName(std::string& strInfo)
{
  if(!m_stream) return;
  if(!m_description.empty())
    strInfo = m_description;
  else
    CDemuxStream::GetStreamName(strInfo);
}

void CDemuxStreamSubtitleFFmpeg::GetStreamName(std::string& strInfo)
{
  if(!m_stream) return;
  if(!m_description.empty())
    strInfo = m_description;
  else
    CDemuxStream::GetStreamName(strInfo);
}

void CDemuxStreamVideoFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!m_stream) return;
  char temp[128];
  m_parent->m_dllAvCodec.avcodec_string(temp, 128, m_stream->codec, 0);
  strInfo = temp;
}

void CDemuxStreamSubtitleFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!m_stream) return;
  char temp[128];
  m_parent->m_dllAvCodec.avcodec_string(temp, 128, m_stream->codec, 0);
  strInfo = temp;
}

// these need to be put somewhere that are compiled, we should have some better place for it

CCriticalSection DllAvCodec::m_critSection;
std::map<uintptr_t, CStdString> g_logbuffer;

void ff_avutil_log(void* ptr, int level, const char* format, va_list va)
{
  CSingleLock lock(DllAvCodec::m_critSection);
  uintptr_t threadId = (uintptr_t)CThread::GetCurrentThreadId();
  CStdString &buffer = g_logbuffer[threadId];

  AVClass* avc= ptr ? *(AVClass**)ptr : NULL;

  if(level >= AV_LOG_DEBUG && g_advancedSettings.m_logLevel <= LOG_LEVEL_DEBUG_SAMBA)
    return;
  else if(g_advancedSettings.m_logLevel <= LOG_LEVEL_NORMAL)
    return;

  int type;
  switch(level)
  {
    case AV_LOG_INFO   : type = LOGINFO;    break;
    case AV_LOG_ERROR  : type = LOGERROR;   break;
    case AV_LOG_DEBUG  :
    default            : type = LOGDEBUG;   break;
  }

  CStdString message, prefix;
  message.FormatV(format, va);

  prefix.Format("ffmpeg[%X]: ", threadId);
  if(avc)
  {
    if(avc->item_name)
      prefix += CStdString("[") + avc->item_name(ptr) + "] ";
    else if(avc->class_name)
      prefix += CStdString("[") + avc->class_name + "] ";
  }

  buffer += message;
  int pos, start = 0;
  while( (pos = buffer.find_first_of('\n', start)) >= 0 )
  {
    if(pos>start)
      CLog::Log(type, "%s%s", prefix.c_str(), buffer.substr(start, pos-start).c_str());
    start = pos+1;
  }
  buffer.erase(0, start);
}

static void ff_flush_avutil_log_buffers(void)
{
  CSingleLock lock(DllAvCodec::m_critSection);

  /* Loop through the logbuffer list and remove any blank buffers
     If the thread using the buffer is still active, it will just
     add a new buffer next time it writes to the log */
  std::map<uintptr_t, CStdString>::iterator it;
  for (it = g_logbuffer.begin(); it != g_logbuffer.end(); )
    if ((*it).second.IsEmpty())
      g_logbuffer.erase(it++);
    else
      ++it;
}

#ifdef _MSC_VER
static __declspec(thread) CDVDDemuxFFmpeg* g_demuxer = 0;
#else
static TLS g_tls;
#define g_demuxer (*((CDVDDemuxFFmpeg**)g_tls.Get()))
#endif

static int interrupt_cb(void)
{
  if(g_demuxer && g_demuxer->Aborted())
    return 1;
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
/*
static int dvd_file_open(URLContext *h, const char *filename, int flags)
{
  return -1;
}
*/

static int dvd_file_read(void *h, uint8_t* buf, int size)
{
  if(interrupt_cb())
    return -1;

  CDVDInputStream* pInputStream = (CDVDInputStream*)h;
  return pInputStream->Read(buf, size);
}
/*
static int dvd_file_write(URLContext *h, BYTE* buf, int size)
{
  return -1;
}
*/
static offset_t dvd_file_seek(void *h, offset_t pos, int whence)
{
  if(interrupt_cb())
    return -1;

  CDVDInputStream* pInputStream = (CDVDInputStream*)h;
  if(whence == AVSEEK_SIZE)
    return pInputStream->GetLength();
  else
    return pInputStream->Seek(pos, whence & ~AVSEEK_FORCE);
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

CDVDDemuxFFmpeg::CDVDDemuxFFmpeg() : CDVDDemux()
{
  m_pFormatContext = NULL;
  m_pInput = NULL;
  m_ioContext = NULL;
  for (int i = 0; i < MAX_STREAMS; i++) m_streams[i] = NULL;
  m_iCurrentPts = DVD_NOPTS_VALUE;
}

CDVDDemuxFFmpeg::~CDVDDemuxFFmpeg()
{
  Dispose();
  ff_flush_avutil_log_buffers();
}

bool CDVDDemuxFFmpeg::Aborted()
{
  if(m_timeout.IsTimePast())
    return true;

  return false;
}

bool CDVDDemuxFFmpeg::Open(CDVDInputStream* pInput)
{
  AVInputFormat* iformat = NULL;
  std::string strFile;
  m_iCurrentPts = DVD_NOPTS_VALUE;
  m_speed = DVD_PLAYSPEED_NORMAL;
  g_demuxer = this;
  m_program = UINT_MAX;

  if (!pInput) return false;

  if (!m_dllAvUtil.Load() || !m_dllAvCodec.Load() || !m_dllAvFormat.Load())  {
    CLog::Log(LOGERROR,"CDVDDemuxFFmpeg::Open - failed to load ffmpeg libraries");
    return false;
  }

  // register codecs
  m_dllAvFormat.av_register_all();
  m_dllAvFormat.url_set_interrupt_cb(interrupt_cb);

  // could be used for interupting ffmpeg while opening a file (eg internet streams)
  // url_set_interrupt_cb(NULL);

  m_pInput = pInput;
  strFile = m_pInput->GetFileName();

  bool streaminfo = true; /* set to true if we want to look for streams before playback*/

  if( m_pInput->GetContent().length() > 0 )
  {
    std::string content = m_pInput->GetContent();

    /* check if we can get a hint from content */
    if     ( content.compare("video/x-vobsub") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mpeg");
    else if( content.compare("video/x-dvd-mpeg") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mpeg");
    else if( content.compare("video/x-mpegts") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mpegts");
    else if( content.compare("multipart/x-mixed-replace") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mjpeg");
  }

  // try to abort after 30 seconds
  m_timeout.Set(30000);

  if( m_pInput->IsStreamType(DVDSTREAM_TYPE_FFMPEG) )
  {
    // special stream type that makes avformat handle file opening
    // allows internal ffmpeg protocols to be used
    int result=-1;
    if (strFile.substr(0,6) == "mms://")
    {
      // try mmsh, then mmst
      CStdString strFile2;
      strFile2.Format("mmsh://%s",strFile.substr(6,strFile.size()-6).c_str());
      result = m_dllAvFormat.av_open_input_file(&m_pFormatContext, strFile2.c_str(), iformat, FFMPEG_FILE_BUFFER_SIZE, NULL);
      if (result < 0)
      {
        strFile = "mmst://";
        strFile += strFile2.Mid(7).c_str();
      } 
    }
    if (result < 0 && m_dllAvFormat.av_open_input_file(&m_pFormatContext, strFile.c_str(), iformat, FFMPEG_FILE_BUFFER_SIZE, NULL) < 0 )
    {
      CLog::Log(LOGDEBUG, "Error, could not open file %s", strFile.c_str());
      Dispose();
      return false;
    }
  }
  else
  {
    unsigned char* buffer = (unsigned char*)m_dllAvUtil.av_malloc(FFMPEG_FILE_BUFFER_SIZE);
    m_ioContext = m_dllAvFormat.av_alloc_put_byte(buffer, FFMPEG_FILE_BUFFER_SIZE, 0, m_pInput, dvd_file_read, NULL, dvd_file_seek);
    m_ioContext->max_packet_size = m_pInput->GetBlockSize();
    if(m_ioContext->max_packet_size)
      m_ioContext->max_packet_size *= FFMPEG_FILE_BUFFER_SIZE / m_ioContext->max_packet_size;

    if(m_pInput->Seek(0, SEEK_POSSIBLE) == 0)
      m_ioContext->is_streamed = 1;

    if( iformat == NULL )
    {
#if defined(USE_EXTERNAL_FFMPEG) && LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52,98,0)
      // API added on: 2011-02-09
      // Old versions of ffmpeg do not have av_probe_input_format, so we need
      // to always probe using the lower-level functions as well.
      const bool legacyProbing = true;
#else
      const bool legacyProbing = false;
#endif
      // let ffmpeg decide which demuxer we have to open

      bool trySPDIFonly = (m_pInput->GetContent() == "audio/x-spdif-compressed");

      if (!trySPDIFonly)
        m_dllAvFormat.av_probe_input_buffer(m_ioContext, &iformat, strFile.c_str(), NULL, 0, 0);

      // Use the more low-level code in case we have been built against an old
      // FFmpeg without the above av_probe_input_buffer(), or in case we only
      // want to probe for spdif (DTS or IEC 61937) compressed audio
      // specifically, or in case the file is a wav which may contain DTS or
      // IEC 61937 (e.g. ac3-in-wav) and we want to check for those formats.
      if (legacyProbing || trySPDIFonly || (iformat && strcmp(iformat->name, "wav") == 0))
      {
        AVProbeData pd;
        BYTE probe_buffer[FFMPEG_FILE_BUFFER_SIZE + AVPROBE_PADDING_SIZE];

        // init probe data
        pd.buf = probe_buffer;
        pd.filename = strFile.c_str();

        // read data using avformat's buffers
        pd.buf_size = m_dllAvFormat.get_buffer(m_ioContext, pd.buf, m_ioContext->max_packet_size ? m_ioContext->max_packet_size : m_ioContext->buffer_size);
        if (pd.buf_size <= 0)
        {
          CLog::Log(LOGERROR, "%s - error reading from input stream, %s", __FUNCTION__, strFile.c_str());
          return false;
        }
        memset(pd.buf+pd.buf_size, 0, AVPROBE_PADDING_SIZE);

        // restore position again
        m_dllAvFormat.url_fseek(m_ioContext , 0, SEEK_SET);

        if (legacyProbing && !trySPDIFonly)
          iformat = m_dllAvFormat.av_probe_input_format(&pd, 1);

        // the advancedsetting is for allowing the user to force outputting the
        // 44.1 kHz DTS wav file as PCM, so that an A/V receiver can decode
        // it (this is temporary until we handle 44.1 kHz passthrough properly)
        if (trySPDIFonly || (iformat && strcmp(iformat->name, "wav") == 0 && !g_advancedSettings.m_dvdplayerIgnoreDTSinWAV))
        {
          // check for spdif and dts
          // This is used with wav files and audio CDs that may contain
          // a DTS or AC3 track padded for S/PDIF playback. If neither of those
          // is present, we assume it is PCM audio.
          // AC3 is always wrapped in iec61937 (ffmpeg "spdif"), while DTS
          // may be just padded.
          AVInputFormat *iformat2;
          iformat2 = m_dllAvFormat.av_find_input_format("spdif");

          if (iformat2 && iformat2->read_probe(&pd) > AVPROBE_SCORE_MAX / 4)
          {
            iformat = iformat2;
          }
          else
          {
            // not spdif or no spdif demuxer, try dts
            iformat2 = m_dllAvFormat.av_find_input_format("dts");

            if (iformat2 && iformat2->read_probe(&pd) > AVPROBE_SCORE_MAX / 4)
            {
              iformat = iformat2;
            }
            else if (trySPDIFonly)
            {
              // not dts either, return false in case we were explicitely
              // requested to only check for S/PDIF padded compressed audio
              CLog::Log(LOGDEBUG, "%s - not spdif or dts file, fallbacking", __FUNCTION__);
              return false;
            }
          }
        }
      }

      if(!iformat)
      {
        std::string content = m_pInput->GetContent();

        /* check if we can get a hint from content */
        if( content.compare("audio/aacp") == 0 )
          iformat = m_dllAvFormat.av_find_input_format("aac");
        else if( content.compare("audio/aac") == 0 )
          iformat = m_dllAvFormat.av_find_input_format("aac");
        else if( content.compare("video/flv") == 0 )
          iformat = m_dllAvFormat.av_find_input_format("flv");
        else if( content.compare("video/x-flv") == 0 )
          iformat = m_dllAvFormat.av_find_input_format("flv");
      }

      if (!iformat)
      {
        CLog::Log(LOGERROR, "%s - error probing input format, %s", __FUNCTION__, strFile.c_str());
        return false;
      }
      else
      {
        if (iformat->name)
          CLog::Log(LOGDEBUG, "%s - probing detected format [%s]", __FUNCTION__, iformat->name);
        else
          CLog::Log(LOGDEBUG, "%s - probing detected unnamed format", __FUNCTION__);
      }
    }


    // open the demuxer
    if (m_dllAvFormat.av_open_input_stream(&m_pFormatContext, m_ioContext, strFile.c_str(), iformat, NULL) < 0)
    {
      CLog::Log(LOGERROR, "%s - Error, could not open file %s", __FUNCTION__, strFile.c_str());
      Dispose();
      return false;
    }
  }

  // analyse very short to speed up mjpeg playback start
  if (iformat && (strcmp(iformat->name, "mjpeg") == 0) && m_ioContext->is_streamed)
    m_pFormatContext->max_analyze_duration = 500000;

  // we need to know if this is matroska or avi later
  m_bMatroska = strncmp(m_pFormatContext->iformat->name, "matroska", 8) == 0;	// for "matroska.webm"
  m_bAVI = strcmp(m_pFormatContext->iformat->name, "avi") == 0;

  if (streaminfo)
  {
    /* too speed up dvd switches, only analyse very short */
    if(m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
      m_pFormatContext->max_analyze_duration = 500000;


    CLog::Log(LOGDEBUG, "%s - av_find_stream_info starting", __FUNCTION__);
    int iErr = m_dllAvFormat.av_find_stream_info(m_pFormatContext);
    if (iErr < 0)
    {
      CLog::Log(LOGWARNING,"could not find codec parameters for %s", strFile.c_str());
      if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) || (m_pFormatContext->nb_streams == 1 && m_pFormatContext->streams[0]->codec->codec_id == CODEC_ID_AC3))
      {
        // special case, our codecs can still handle it.
      }
      else
      {
        Dispose();
        return false;
      }
    }
    CLog::Log(LOGDEBUG, "%s - av_find_stream_info finished", __FUNCTION__);
  }
  // reset any timeout
  m_timeout.SetInfinite();

  // if format can be nonblocking, let's use that
  m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;

  // print some extra information
  m_dllAvFormat.dump_format(m_pFormatContext, 0, strFile.c_str(), 0);

  UpdateCurrentPTS();

  // add the ffmpeg streams to our own stream array
  if (m_pFormatContext->nb_programs)
  {
    // look for first non empty stream and discard nonselected programs
    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; i++)
    {
      if(m_program == UINT_MAX && m_pFormatContext->programs[i]->nb_stream_indexes > 0)
        m_program = i;

      if(i != m_program)
        m_pFormatContext->programs[i]->discard = AVDISCARD_ALL;
    }
    if(m_program != UINT_MAX)
    {
      // add streams from selected program
      for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
        AddStream(m_pFormatContext->programs[m_program]->stream_index[i]);
    }
  }
  // if there were no programs or they were all empty, add all streams
  if (m_program == UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
      AddStream(i);
  }

  return true;
}

void CDVDDemuxFFmpeg::Dispose()
{
  g_demuxer = this;

  if (m_pFormatContext)
  {
    if (m_ioContext)
    {
      if(m_pFormatContext->pb && m_pFormatContext->pb != m_ioContext)
      {
        CLog::Log(LOGWARNING, "CDVDDemuxFFmpeg::Dispose - demuxer changed our byte context behind our back, possible memleak");
        m_ioContext = m_pFormatContext->pb;
      }
      m_dllAvFormat.av_close_input_stream(m_pFormatContext);
      if (m_ioContext->buffer)
        m_dllAvUtil.av_free(m_ioContext->buffer);
      m_dllAvUtil.av_free(m_ioContext);
    }
    else
      m_dllAvFormat.av_close_input_file(m_pFormatContext);
  }
  m_ioContext = NULL;
  m_pFormatContext = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  for (int i = 0; i < MAX_STREAMS; i++)
  {
    if (m_streams[i])
    {
      if (m_streams[i]->ExtraData)
        delete[] (BYTE*)(m_streams[i]->ExtraData);
      delete m_streams[i];
    }
    m_streams[i] = NULL;
  }
  m_pInput = NULL;

  m_dllAvFormat.Unload();
  m_dllAvCodec.Unload();
  m_dllAvUtil.Unload();
}

void CDVDDemuxFFmpeg::Reset()
{
  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream);
}

void CDVDDemuxFFmpeg::Flush()
{
  g_demuxer = this;

  // naughty usage of an internal ffmpeg function
  if (m_pFormatContext)
    m_dllAvFormat.av_read_frame_flush(m_pFormatContext);

  m_iCurrentPts = DVD_NOPTS_VALUE;
}

void CDVDDemuxFFmpeg::Abort()
{
  m_timeout.SetExpired();
}

void CDVDDemuxFFmpeg::SetSpeed(int iSpeed)
{
  g_demuxer = this;

  if(!m_pFormatContext)
    return;

  if(m_speed != DVD_PLAYSPEED_PAUSE && iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    m_pInput->Pause((double)m_iCurrentPts);
    m_dllAvFormat.av_read_pause(m_pFormatContext);
  }
  else if(m_speed == DVD_PLAYSPEED_PAUSE && iSpeed != DVD_PLAYSPEED_PAUSE)
  {
    m_pInput->Pause((double)m_iCurrentPts);
    m_dllAvFormat.av_read_play(m_pFormatContext);
  }
  m_speed = iSpeed;

  AVDiscard discard = AVDISCARD_NONE;
  if(m_speed > 4*DVD_PLAYSPEED_NORMAL)
    discard = AVDISCARD_NONKEY;
  else if(m_speed > 2*DVD_PLAYSPEED_NORMAL)
    discard = AVDISCARD_BIDIR;
  else if(m_speed < DVD_PLAYSPEED_PAUSE)
    discard = AVDISCARD_NONKEY;


  for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    if(m_pFormatContext->streams[i])
    {
      if(m_pFormatContext->streams[i]->discard != AVDISCARD_ALL)
        m_pFormatContext->streams[i]->discard = discard;
    }
  }
}

double CDVDDemuxFFmpeg::ConvertTimestamp(int64_t pts, int den, int num)
{
  if (pts == (int64_t)AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completly exact timestamp anyway
  double timestamp = (double)pts * num  / den;
  double starttime = 0.0f;

  // for dvd's we need the original time
  if(m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
    starttime = static_cast<CDVDInputStreamNavigator*>(m_pInput)->GetTimeStampCorrection() / DVD_TIME_BASE;
  else if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = (double)m_pFormatContext->start_time / AV_TIME_BASE;

  if(timestamp > starttime)
    timestamp -= starttime;
  else if( timestamp + 0.1f > starttime )
    timestamp = 0;

  return timestamp*DVD_TIME_BASE;
}

DemuxPacket* CDVDDemuxFFmpeg::Read()
{
  g_demuxer = this;

  AVPacket pkt;
  DemuxPacket* pPacket = NULL;
  // on some cases where the received packet is invalid we will need to return an empty packet (0 length) otherwise the main loop (in CDVDPlayer)
  // would consider this the end of stream and stop.
  bool bReturnEmpty = false;
  { CSingleLock lock(m_critSection); // open lock scope
  if (m_pFormatContext)
  {
    // assume we are not eof
    if(m_pFormatContext->pb)
      m_pFormatContext->pb->eof_reached = 0;

    // keep track if ffmpeg doesn't always set these
    pkt.size = 0;
    pkt.data = NULL;
    pkt.stream_index = MAX_STREAMS;

    // timeout reads after 100ms
    m_timeout.Set(20000);
    int result = 0;
    try
    {
      result = m_dllAvFormat.av_read_frame(m_pFormatContext, &pkt);
    }
    catch(const win32_exception &e)
    {
      e.writelog(__FUNCTION__);
      result = AVERROR(EFAULT);
    }
    m_timeout.SetInfinite();

    if (result == AVERROR(EINTR) || result == AVERROR(EAGAIN))
    {
      // timeout, probably no real error, return empty packet
      bReturnEmpty = true;
    }
    else if (result < 0)
    {
      Flush();
    }
    else if (pkt.size < 0 || pkt.stream_index >= MAX_STREAMS)
    {
      // XXX, in some cases ffmpeg returns a negative packet size
      if(m_pFormatContext->pb && !m_pFormatContext->pb->eof_reached)
      {
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() no valid packet");
        bReturnEmpty = true;
        Flush();
      }
      else
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() returned invalid packet and eof reached");

      m_dllAvCodec.av_free_packet(&pkt);
    }
    else
    {
      AVStream *stream = m_pFormatContext->streams[pkt.stream_index];

      if (m_program != UINT_MAX)
      {
        /* check so packet belongs to selected program */
        for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
        {
          if(pkt.stream_index == (int)m_pFormatContext->programs[m_program]->stream_index[i])
          {
            pPacket = CDVDDemuxUtils::AllocateDemuxPacket(pkt.size);
            break;
          }
        }

        if (!pPacket)
          bReturnEmpty = true;
      }
      else
        pPacket = CDVDDemuxUtils::AllocateDemuxPacket(pkt.size);

      if (pPacket)
      {
        // lavf sometimes bugs out and gives 0 dts/pts instead of no dts/pts
        // since this could only happens on initial frame under normal
        // circomstances, let's assume it is wrong all the time
        if(pkt.dts == 0)
          pkt.dts = AV_NOPTS_VALUE;
        if(pkt.pts == 0)
          pkt.pts = AV_NOPTS_VALUE;

        if(m_bMatroska && stream->codec && stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        { // matroska can store different timestamps
          // for different formats, for native stored
          // stuff it is pts, but for ms compatibility
          // tracks, it is really dts. sadly ffmpeg
          // sets these two timestamps equal all the
          // time, so we select it here instead
          if(stream->codec->codec_tag == 0)
            pkt.dts = AV_NOPTS_VALUE;
          else
            pkt.pts = AV_NOPTS_VALUE;
        }

        // we need to get duration slightly different for matroska embedded text subtitels
        if(m_bMatroska && stream->codec->codec_id == CODEC_ID_TEXT && pkt.convergence_duration != 0)
          pkt.duration = pkt.convergence_duration;

        if(m_bAVI && stream->codec && stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
          // AVI's always have borked pts, specially if m_pFormatContext->flags includes
          // AVFMT_FLAG_GENPTS so always use dts
          pkt.pts = AV_NOPTS_VALUE;
        }

        // copy contents into our own packet
        pPacket->iSize = pkt.size;

        // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
        if (pkt.data)
          memcpy(pPacket->pData, pkt.data, pPacket->iSize);

        pPacket->pts = ConvertTimestamp(pkt.pts, stream->time_base.den, stream->time_base.num);
        pPacket->dts = ConvertTimestamp(pkt.dts, stream->time_base.den, stream->time_base.num);
        pPacket->duration =  DVD_SEC_TO_TIME((double)pkt.duration * stream->time_base.num / stream->time_base.den);

        // used to guess streamlength
        if (pPacket->dts != DVD_NOPTS_VALUE && (pPacket->dts > m_iCurrentPts || m_iCurrentPts == DVD_NOPTS_VALUE))
          m_iCurrentPts = pPacket->dts;


        // check if stream has passed full duration, needed for live streams
        if(pkt.dts != (int64_t)AV_NOPTS_VALUE)
        {
          int64_t duration;
          duration = pkt.dts;
          if(stream->start_time != (int64_t)AV_NOPTS_VALUE)
            duration -= stream->start_time;

          if(duration > stream->duration)
          {
            stream->duration = duration;
            duration = m_dllAvUtil.av_rescale_rnd(stream->duration, stream->time_base.num * AV_TIME_BASE, stream->time_base.den, AV_ROUND_NEAR_INF);
            if ((m_pFormatContext->duration == (int64_t)AV_NOPTS_VALUE && m_pFormatContext->file_size > 0)
                ||  (m_pFormatContext->duration != (int64_t)AV_NOPTS_VALUE && duration > m_pFormatContext->duration))
              m_pFormatContext->duration = duration;
          }
        }

        // check if stream seem to have grown since start
        if(m_pFormatContext->file_size > 0 && m_pFormatContext->pb)
        {
          if(m_pFormatContext->pb->pos > m_pFormatContext->file_size)
            m_pFormatContext->file_size = m_pFormatContext->pb->pos;
        }

        pPacket->iStreamId = pkt.stream_index; // XXX just for now
      }
      m_dllAvCodec.av_free_packet(&pkt);
    }
  }
  } // end of lock scope
  if (bReturnEmpty && !pPacket)
    pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);

  if (!pPacket) return NULL;

  // check streams, can we make this a bit more simple?
  if (pPacket && pPacket->iStreamId >= 0 && pPacket->iStreamId <= MAX_STREAMS)
  {
    if (!m_streams[pPacket->iStreamId] ||
        m_streams[pPacket->iStreamId]->pPrivate != m_pFormatContext->streams[pPacket->iStreamId] ||
        m_streams[pPacket->iStreamId]->codec != m_pFormatContext->streams[pPacket->iStreamId]->codec->codec_id)
    {
      // content has changed, or stream did not yet exist
      AddStream(pPacket->iStreamId);
    }
    // we already check for a valid m_streams[pPacket->iStreamId] above
    else if (m_streams[pPacket->iStreamId]->type == STREAM_AUDIO)
    {
      if (((CDemuxStreamAudio*)m_streams[pPacket->iStreamId])->iChannels != m_pFormatContext->streams[pPacket->iStreamId]->codec->channels ||
          ((CDemuxStreamAudio*)m_streams[pPacket->iStreamId])->iSampleRate != m_pFormatContext->streams[pPacket->iStreamId]->codec->sample_rate)
      {
        // content has changed
        AddStream(pPacket->iStreamId);
      }
    }
    else if (m_streams[pPacket->iStreamId]->type == STREAM_VIDEO)
    {
      if (((CDemuxStreamVideo*)m_streams[pPacket->iStreamId])->iWidth != m_pFormatContext->streams[pPacket->iStreamId]->codec->width ||
          ((CDemuxStreamVideo*)m_streams[pPacket->iStreamId])->iHeight != m_pFormatContext->streams[pPacket->iStreamId]->codec->height)
      {
        // content has changed
        AddStream(pPacket->iStreamId);
      }
    }
  }
  return pPacket;
}

bool CDVDDemuxFFmpeg::SeekTime(int time, bool backwords, double *startpts)
{
  g_demuxer = this;

  if(time < 0)
    time = 0;

  CDVDInputStream::ISeekTime* ist = dynamic_cast<CDVDInputStream::ISeekTime*>(m_pInput);
  if (ist)
  {
    if (!ist->SeekTime(time))
      return false;

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;

    Flush();

    // also empty the internal ffmpeg buffer
    m_ioContext->buf_ptr = m_ioContext->buf_end;

    return true;
  }

  if(!m_pInput->Seek(0, SEEK_POSSIBLE)
  && !m_pInput->IsStreamType(DVDSTREAM_TYPE_FFMPEG))
  {
    CLog::Log(LOGDEBUG, "%s - input stream reports it is not seekable", __FUNCTION__);
    return false;
  }

  __int64 seek_pts = (__int64)time * (AV_TIME_BASE / 1000);
  if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    seek_pts += m_pFormatContext->start_time;

  int ret;
  {
    CSingleLock lock(m_critSection);
    ret = m_dllAvFormat.av_seek_frame(m_pFormatContext, -1, seek_pts, backwords ? AVSEEK_FLAG_BACKWARD : 0);

    if(ret >= 0)
      UpdateCurrentPTS();
  }

  if(m_iCurrentPts == DVD_NOPTS_VALUE)
    CLog::Log(LOGDEBUG, "%s - unknown position after seek", __FUNCTION__);
  else
    CLog::Log(LOGDEBUG, "%s - seek ended up on time %d", __FUNCTION__, (int)(m_iCurrentPts / DVD_TIME_BASE * 1000));

  // in this case the start time is requested time
  if(startpts)
    *startpts = DVD_MSEC_TO_TIME(time);

  // demuxer will return failure, if you seek to eof
  if (m_pInput->IsEOF() && ret <= 0)
    return true;

  return (ret >= 0);
}

bool CDVDDemuxFFmpeg::SeekByte(__int64 pos)
{
  g_demuxer = this;

  CSingleLock lock(m_critSection);
  int ret = m_dllAvFormat.av_seek_frame(m_pFormatContext, -1, pos, AVSEEK_FLAG_BYTE);

  if(ret >= 0)
    UpdateCurrentPTS();

  return (ret >= 0);
}

void CDVDDemuxFFmpeg::UpdateCurrentPTS()
{
  m_iCurrentPts = DVD_NOPTS_VALUE;
  for(unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    AVStream *stream = m_pFormatContext->streams[i];
    if(stream && stream->cur_dts != (int64_t)AV_NOPTS_VALUE)
    {
      double ts = ConvertTimestamp(stream->cur_dts, stream->time_base.den, stream->time_base.num);
      if(m_iCurrentPts == DVD_NOPTS_VALUE || m_iCurrentPts > ts )
        m_iCurrentPts = ts;
    }
  }
}

int CDVDDemuxFFmpeg::GetStreamLength()
{
  if (!m_pFormatContext)
    return 0;

  /* apperently ffmpeg messes up sometimes, so check for negative value too */
  if (m_pFormatContext->duration == (int64_t)AV_NOPTS_VALUE || m_pFormatContext->duration < 0LL)
  {
    // no duration is available for us
    // try to calculate it
    int iLength = 0;
    if (m_iCurrentPts != DVD_NOPTS_VALUE && m_pFormatContext->file_size > 0 && m_pFormatContext->pb && m_pFormatContext->pb->pos > 0)
    {
      iLength = (int)(((m_iCurrentPts * m_pFormatContext->file_size) / m_pFormatContext->pb->pos) / 1000) & 0xFFFFFFFF;
    }
    return iLength;
  }

  return (int)(m_pFormatContext->duration / (AV_TIME_BASE / 1000));
}

CDemuxStream* CDVDDemuxFFmpeg::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS) return NULL;
  return m_streams[iStreamId];
}

int CDVDDemuxFFmpeg::GetNrOfStreams()
{
  int i = 0;
  while (i < MAX_STREAMS && m_streams[i]) i++;
  return i;
}

static double SelectAspect(AVStream* st)
{
  /* if stream aspect is 1:1 or 0:0 use codec aspect */
  if((st->sample_aspect_ratio.den == 1 || st->sample_aspect_ratio.den == 0)
  && (st->sample_aspect_ratio.num == 1 || st->sample_aspect_ratio.num == 0)
  && st->codec->sample_aspect_ratio.num != 0)
    return av_q2d(st->codec->sample_aspect_ratio);

  if(st->sample_aspect_ratio.num != 0)
    return av_q2d(st->sample_aspect_ratio);

  return 0.0;
}

void CDVDDemuxFFmpeg::AddStream(int iId)
{
  AVStream* pStream = m_pFormatContext->streams[iId];
  if (pStream)
  {
    CDemuxStream* old = m_streams[iId];

    switch (pStream->codec->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
      {
        CDemuxStreamAudioFFmpeg* st = new CDemuxStreamAudioFFmpeg(this, pStream);
        m_streams[iId] = st;
        st->iChannels = pStream->codec->channels;
        st->iSampleRate = pStream->codec->sample_rate;
        st->iBlockAlign = pStream->codec->block_align;
        st->iBitRate = pStream->codec->bit_rate;
        st->iBitsPerSample = pStream->codec->bits_per_coded_sample;
	
        if(m_dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0))
          st->m_description = m_dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0)->value;

        break;
      }
    case AVMEDIA_TYPE_VIDEO:
      {
        CDemuxStreamVideoFFmpeg* st = new CDemuxStreamVideoFFmpeg(this, pStream);
        m_streams[iId] = st;
        if(strcmp(m_pFormatContext->iformat->name, "flv") == 0)
          st->bVFR = true;
        else
          st->bVFR = false;

        // never trust pts in avi files with h264.
        if (m_bAVI && pStream->codec->codec_id == CODEC_ID_H264)
          st->bPTSInvalid = true;

        //average fps is more accurate for mkv files
        if (m_bMatroska && pStream->avg_frame_rate.den && pStream->avg_frame_rate.num)
        {
          st->iFpsRate = pStream->avg_frame_rate.num;
          st->iFpsScale = pStream->avg_frame_rate.den;
        }
        else if(pStream->r_frame_rate.den && pStream->r_frame_rate.num)
        {
          st->iFpsRate = pStream->r_frame_rate.num;
          st->iFpsScale = pStream->r_frame_rate.den;
        }
        else
        {
          st->iFpsRate  = 0;
          st->iFpsScale = 0;
        }

        if (pStream->codec_info_nb_frames >  0
        &&  pStream->codec_info_nb_frames <= 2
        &&  m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
        {
          CLog::Log(LOGDEBUG, "%s - fps may be unreliable since ffmpeg decoded only %d frame(s)", __FUNCTION__, pStream->codec_info_nb_frames);
          st->iFpsRate  = 0;
          st->iFpsScale = 0;
        }

        st->iWidth = pStream->codec->width;
        st->iHeight = pStream->codec->height;
        st->fAspect = SelectAspect(pStream) * pStream->codec->width / pStream->codec->height;
        st->iLevel = pStream->codec->level;
        st->iProfile = pStream->codec->profile;

        if ( m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) )
        {
          if (pStream->codec->codec_id == CODEC_ID_PROBE)
          {
            // fix MPEG-1/MPEG-2 video stream probe returning CODEC_ID_PROBE for still frames.
            // ffmpeg issue 1871, regression from ffmpeg r22831.
            if ((pStream->id & 0xF0) == 0xE0)
            {
              pStream->codec->codec_id = CODEC_ID_MPEG2VIDEO;
              pStream->codec->codec_tag = MKTAG('M','P','2','V');
              CLog::Log(LOGERROR, "%s - CODEC_ID_PROBE detected, forcing CODEC_ID_MPEG2VIDEO", __FUNCTION__);
            }
          }
        }
        break;
      }
    case AVMEDIA_TYPE_DATA:
      {
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_DATA;
        break;
      }
    case AVMEDIA_TYPE_SUBTITLE:
      {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,38,1)
        if (pStream->codec->codec_id == CODEC_ID_DVB_TELETEXT && g_guiSettings.GetBool("videoplayer.teletextenabled"))
        {
          CDemuxStreamTeletext* st = new CDemuxStreamTeletext();
          m_streams[iId] = st;
          m_streams[iId]->type = STREAM_TELETEXT;
          break;
        }
        else
#endif
        {
          CDemuxStreamSubtitleFFmpeg* st = new CDemuxStreamSubtitleFFmpeg(this, pStream);
          m_streams[iId] = st;
          if(pStream->codec)
            st->identifier = pStream->codec->sub_id;
	    
          if(m_dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0))
            st->m_description = m_dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0)->value;
	
          break;
        }
      }
    case AVMEDIA_TYPE_ATTACHMENT:
      { //mkv attachments. Only bothering with fonts for now.
        if(pStream->codec->codec_id == CODEC_ID_TTF)
        {
          std::string fileName = "special://temp/fonts/";
          XFILE::CDirectory::Create(fileName);
          AVMetadataTag *nameTag = m_dllAvFormat.av_metadata_get(pStream->metadata, "filename", NULL, 0);
          if (!nameTag) {
            CLog::Log(LOGERROR, "%s: TTF attachment has no name", __FUNCTION__);
            break;
          }
          fileName += nameTag->value;
          XFILE::CFile file;
          if(pStream->codec->extradata && file.OpenForWrite(fileName))
          {
            file.Write(pStream->codec->extradata, pStream->codec->extradata_size);
            file.Close();
          }
        }
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_NONE;
        break;
      }
    default:
      {
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_NONE;
        break;
      }
    }

    // delete old stream after new is created
    // since dvdplayer uses the pointer to know
    // if something changed in the demuxer
    if (old)
    {
      if( old->ExtraData ) delete[] (BYTE*)(old->ExtraData);
      delete old;
    }

    // generic stuff
    if (pStream->duration != (int64_t)AV_NOPTS_VALUE) m_streams[iId]->iDuration = (int)((pStream->duration / AV_TIME_BASE) & 0xFFFFFFFF);

    m_streams[iId]->codec = pStream->codec->codec_id;
    m_streams[iId]->codec_fourcc = pStream->codec->codec_tag;
    m_streams[iId]->profile = pStream->codec->profile;
    m_streams[iId]->iId = iId;
    m_streams[iId]->source = STREAM_SOURCE_DEMUX;
    m_streams[iId]->pPrivate = pStream;
    m_streams[iId]->flags = (CDemuxStream::EFlags)pStream->disposition;

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,83,0)
    // API added on: 2010-10-15
    // (Note that while the function was available earlier, the generic
    // metadata tags were not populated by default)
    AVMetadataTag *langTag = m_dllAvFormat.av_metadata_get(pStream->metadata, "language", NULL, 0);
    if (langTag)
      strncpy(m_streams[iId]->language, langTag->value, 3);
#else
    strcpy( m_streams[iId]->language, pStream->language );
#endif

    if( pStream->codec->extradata && pStream->codec->extradata_size > 0 )
    {
      m_streams[iId]->ExtraSize = pStream->codec->extradata_size;
      m_streams[iId]->ExtraData = new BYTE[pStream->codec->extradata_size];
      memcpy(m_streams[iId]->ExtraData, pStream->codec->extradata, pStream->codec->extradata_size);
    }

#ifdef HAVE_LIBBLURAY
    if( m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY) )
      static_cast<CDVDInputStreamBluray*>(m_pInput)->GetStreamInfo(pStream->id, m_streams[iId]->language);
#endif
    if( m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) )
    {
      // this stuff is really only valid for dvd's.
      // this is so that the physicalid matches the
      // id's reported from libdvdnav
      switch(m_streams[iId]->codec)
      {
        case CODEC_ID_AC3:
          m_streams[iId]->iPhysicalId = pStream->id - 128;
          break;
        case CODEC_ID_DTS:
          m_streams[iId]->iPhysicalId = pStream->id - 136;
          break;
        case CODEC_ID_MP2:
          m_streams[iId]->iPhysicalId = pStream->id - 448;
          break;
        case CODEC_ID_PCM_S16BE:
          m_streams[iId]->iPhysicalId = pStream->id - 160;
          break;
        case CODEC_ID_DVD_SUBTITLE:
          m_streams[iId]->iPhysicalId = pStream->id - 0x20;
          break;
        default:
          m_streams[iId]->iPhysicalId = pStream->id & 0x1f;
          break;
      }
    }
    else
      m_streams[iId]->iPhysicalId = pStream->id;
  }
}

std::string CDVDDemuxFFmpeg::GetFileName()
{
  if(m_pInput && m_pInput)
    return m_pInput->GetFileName();
  else
    return "";
}

int CDVDDemuxFFmpeg::GetChapterCount()
{
  CDVDInputStream::IChapter* ich = dynamic_cast<CDVDInputStream::IChapter*>(m_pInput);
  if(ich)
    return ich->GetChapterCount();

  if(m_pFormatContext == NULL)
    return 0;
  #if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,14,0)
    return m_pFormatContext->nb_chapters;
  #else
    return 0;
  #endif
}

int CDVDDemuxFFmpeg::GetChapter()
{
  CDVDInputStream::IChapter* ich = dynamic_cast<CDVDInputStream::IChapter*>(m_pInput);
  if(ich)
    return ich->GetChapter();

  if(m_pFormatContext == NULL
  || m_iCurrentPts == DVD_NOPTS_VALUE)
    return 0;

  #if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,14,0)
    for(unsigned i = 0; i < m_pFormatContext->nb_chapters; i++)
    {
      AVChapter *chapter = m_pFormatContext->chapters[i];
      if(m_iCurrentPts >= ConvertTimestamp(chapter->start, chapter->time_base.den, chapter->time_base.num)
      && m_iCurrentPts <  ConvertTimestamp(chapter->end,   chapter->time_base.den, chapter->time_base.num))
        return i + 1;
    }
  #endif
  return 0;
}

void CDVDDemuxFFmpeg::GetChapterName(std::string& strChapterName)
{
  CDVDInputStream::IChapter* ich = dynamic_cast<CDVDInputStream::IChapter*>(m_pInput);
  if(ich)  
    ich->GetChapterName(strChapterName);
  else
  {
    #if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,14,0)
      int chapterIdx = GetChapter();
      if(chapterIdx <= 0)
        return;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,83,0)
      // API added on: 2010-10-15
      // (Note that while the function was available earlier, the generic
      // metadata tags were not populated by default)
      AVMetadataTag *titleTag = m_dllAvFormat.av_metadata_get(m_pFormatContext->chapters[chapterIdx-1]->metadata,
                                                              "title", NULL, 0);
      if (titleTag)
        strChapterName = titleTag->value;
#else
      if (m_pFormatContext->chapters[chapterIdx-1]->title)
        strChapterName = m_pFormatContext->chapters[chapterIdx-1]->title;
#endif
    #endif
  }
}

bool CDVDDemuxFFmpeg::SeekChapter(int chapter, double* startpts)
{
  if(chapter < 1)
    chapter = 1;

  CDVDInputStream::IChapter* ich = dynamic_cast<CDVDInputStream::IChapter*>(m_pInput);
  if(ich)
  {
    CLog::Log(LOGDEBUG, "%s - chapter seeking using input stream", __FUNCTION__);
    if(!ich->SeekChapter(chapter))
      return false;

    if(startpts)
      *startpts = DVD_NOPTS_VALUE;

    Flush();
    return true;
  }

  if(m_pFormatContext == NULL)
    return false;

    #if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52,14,0)
        if(chapter < 1 || chapter > (int)m_pFormatContext->nb_chapters)
            return false;

        AVChapter *ch = m_pFormatContext->chapters[chapter-1];
        double dts = ConvertTimestamp(ch->start, ch->time_base.den, ch->time_base.num);
        return SeekTime(DVD_TIME_TO_MSEC(dts), true, startpts);
    #else
        return false;
    #endif
}

void CDVDDemuxFFmpeg::GetStreamCodecName(int iStreamId, CStdString &strName)
{
  CDemuxStream *stream = GetStream(iStreamId);
  if (stream)
  {
    unsigned int in = stream->codec_fourcc;
    // FourCC codes are only valid on video streams, audio codecs in AVI/WAV
    // are 2 bytes and audio codecs in transport streams have subtle variation
    // e.g AC-3 instead of ac3
    if (stream->type == STREAM_VIDEO && in != 0)
    {
      char fourcc[5];
#if defined(__powerpc__)
      fourcc[0] = in & 0xff;
      fourcc[1] = (in >> 8) & 0xff;
      fourcc[2] = (in >> 16) & 0xff;
      fourcc[3] = (in >> 24) & 0xff;
#else
      memcpy(fourcc, &in, 4);
#endif
      fourcc[4] = 0;
      // fourccs have to be 4 characters
      if (strlen(fourcc) == 4)
      {
        strName = fourcc;
        strName.MakeLower();
        return;
      }
    }

#ifdef FF_PROFILE_DTS_HD_MA
    /* use profile to determine the DTS type */
    if (stream->codec == CODEC_ID_DTS)
    {
      if (stream->profile == FF_PROFILE_DTS_HD_MA)
        strName = "dtshd_ma";
      else if (stream->profile == FF_PROFILE_DTS_HD_HRA)
        strName = "dtshd_hra";
      else
        strName = "dca";
      return;
    }
#endif

    AVCodec *codec = m_dllAvCodec.avcodec_find_decoder(stream->codec);
    if (codec)
      strName = codec->name;
  }
}
