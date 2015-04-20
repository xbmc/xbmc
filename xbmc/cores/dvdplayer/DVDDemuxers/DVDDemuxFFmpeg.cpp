/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifdef TARGET_POSIX
#include "stdint.h"
#endif
#include "DVDDemuxFFmpeg.h"
#include "DVDInputStreams/DVDInputStream.h"
#ifdef HAVE_LIBBLURAY
#include "DVDInputStreams/DVDInputStreamBluray.h"
#endif
#include "DVDInputStreams/DVDInputStreamFFmpeg.h"
#include "DVDDemuxUtils.h"
#include "DVDClock.h" // for DVD_TIME_BASE
#include "commons/Exception.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "filesystem/File.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "utils/log.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "cores/FFmpeg.h"

extern "C" {
#include "libavutil/opt.h"
}

struct StereoModeConversionMap
{
  const char*          name;
  const char*          mode;
};

// we internally use the matroska string representation of stereoscopic modes.
// This struct is a conversion map to convert stereoscopic mode values
// from asf/wmv to the internally used matroska ones
static const struct StereoModeConversionMap WmvToInternalStereoModeMap[] =
{
  { "SideBySideRF",             "right_left" },
  { "SideBySideLF",             "left_right" },
  { "OverUnderRT",              "bottom_top" },
  { "OverUnderLT",              "top_bottom" },
  {}
};

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - FF_INPUT_BUFFER_PADDING_SIZE)

void CDemuxStreamAudioFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!m_stream) return;
  char temp[128];
  avcodec_string(temp, 128, m_stream->codec, 0);
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
  avcodec_string(temp, 128, m_stream->codec, 0);
  strInfo = temp;
}

void CDemuxStreamSubtitleFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!m_stream) return;
  char temp[128];
  avcodec_string(temp, 128, m_stream->codec, 0);
  strInfo = temp;
}

static int interrupt_cb(void* ctx)
{
  CDVDDemuxFFmpeg* demuxer = static_cast<CDVDDemuxFFmpeg*>(ctx);
  if(demuxer && demuxer->Aborted())
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
  if(interrupt_cb(h))
    return AVERROR_EXIT;

  CDVDInputStream* pInputStream = static_cast<CDVDDemuxFFmpeg*>(h)->m_pInput;
  return pInputStream->Read(buf, size);
}
/*
static int dvd_file_write(URLContext *h, uint8_t* buf, int size)
{
  return -1;
}
*/
static int64_t dvd_file_seek(void *h, int64_t pos, int whence)
{
  if(interrupt_cb(h))
    return AVERROR_EXIT;

  CDVDInputStream* pInputStream = static_cast<CDVDDemuxFFmpeg*>(h)->m_pInput;
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
  m_currentPts = DVD_NOPTS_VALUE;
  m_bMatroska = false;
  m_bAVI = false;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_program = UINT_MAX;
  m_pkt.result = -1;
  memset(&m_pkt.pkt, 0, sizeof(AVPacket));
  m_streaminfo = true; /* set to true if we want to look for streams before playback */
  m_checkvideo = false;
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

  CDVDInputStreamFFmpeg * input = dynamic_cast<CDVDInputStreamFFmpeg*>(m_pInput);
  if(input && input->Aborted())
    return true;

  return false;
}

bool CDVDDemuxFFmpeg::Open(CDVDInputStream* pInput, bool streaminfo, bool fileinfo)
{
  AVInputFormat* iformat = NULL;
  std::string strFile;
  m_streaminfo = streaminfo;
  m_currentPts = DVD_NOPTS_VALUE;
  m_speed = DVD_PLAYSPEED_NORMAL;
  m_program = UINT_MAX;
  const AVIOInterruptCB int_cb = { interrupt_cb, this };

  if (!pInput) return false;

  m_pInput = pInput;
  strFile = m_pInput->GetFileName();

  if( m_pInput->GetContent().length() > 0 )
  {
    std::string content = m_pInput->GetContent();
    StringUtils::ToLower(content);

    /* check if we can get a hint from content */
    if     ( content.compare("video/x-vobsub") == 0 )
      iformat = av_find_input_format("mpeg");
    else if( content.compare("video/x-dvd-mpeg") == 0 )
      iformat = av_find_input_format("mpeg");
    else if( content.compare("video/mp2t") == 0 )
      iformat = av_find_input_format("mpegts");
    else if( content.compare("multipart/x-mixed-replace") == 0 )
      iformat = av_find_input_format("mjpeg");
  }

  // open the demuxer
  m_pFormatContext  = avformat_alloc_context();
  m_pFormatContext->interrupt_callback = int_cb;

  // try to abort after 30 seconds
  m_timeout.Set(30000);

  if( m_pInput->IsStreamType(DVDSTREAM_TYPE_FFMPEG) )
  {
    // special stream type that makes avformat handle file opening
    // allows internal ffmpeg protocols to be used
    CURL url = m_pInput->GetURL();

    AVDictionary *options = GetFFMpegOptionsFromURL(url);

    int result=-1;
    if (url.IsProtocol("mms"))
    {
      // try mmsh, then mmst
      url.SetProtocol("mmsh");
      url.SetProtocolOptions("");
      result = avformat_open_input(&m_pFormatContext, url.Get().c_str(), iformat, &options);
      if (result < 0)
      {
        url.SetProtocol("mmst");
        strFile = url.Get();
      } 
    }
    if (result < 0 && avformat_open_input(&m_pFormatContext, strFile.c_str(), iformat, &options) < 0 )
    {
      CLog::Log(LOGDEBUG, "Error, could not open file %s", CURL::GetRedacted(strFile).c_str());
      Dispose();
      av_dict_free(&options);
      return false;
    }
    av_dict_free(&options);
  }
  else
  {
    unsigned char* buffer = (unsigned char*)av_malloc(FFMPEG_FILE_BUFFER_SIZE);
    m_ioContext = avio_alloc_context(buffer, FFMPEG_FILE_BUFFER_SIZE, 0, this, dvd_file_read, NULL, dvd_file_seek);
    m_ioContext->max_packet_size = m_pInput->GetBlockSize();
    if(m_ioContext->max_packet_size)
      m_ioContext->max_packet_size *= FFMPEG_FILE_BUFFER_SIZE / m_ioContext->max_packet_size;

    if(m_pInput->Seek(0, SEEK_POSSIBLE) == 0)
      m_ioContext->seekable = 0;

    std::string content = m_pInput->GetContent();
    StringUtils::ToLower(content);
    if (StringUtils::StartsWith(content, "audio/l16"))
      iformat = av_find_input_format("s16be");

    if( iformat == NULL )
    {
      // let ffmpeg decide which demuxer we have to open

      bool trySPDIFonly = (m_pInput->GetContent() == "audio/x-spdif-compressed");

      if (!trySPDIFonly)
        av_probe_input_buffer(m_ioContext, &iformat, strFile.c_str(), NULL, 0, 0);

      // Use the more low-level code in case we have been built against an old
      // FFmpeg without the above av_probe_input_buffer(), or in case we only
      // want to probe for spdif (DTS or IEC 61937) compressed audio
      // specifically, or in case the file is a wav which may contain DTS or
      // IEC 61937 (e.g. ac3-in-wav) and we want to check for those formats.
      if (trySPDIFonly || (iformat && strcmp(iformat->name, "wav") == 0))
      {
        AVProbeData pd;
        uint8_t probe_buffer[FFMPEG_FILE_BUFFER_SIZE + AVPROBE_PADDING_SIZE];

        // init probe data
        pd.buf = probe_buffer;
        pd.filename = strFile.c_str();

        // av_probe_input_buffer might have changed the buffer_size beyond our allocated amount
        int buffer_size = std::min((int) FFMPEG_FILE_BUFFER_SIZE, m_ioContext->buffer_size);
        // read data using avformat's buffers
        pd.buf_size = avio_read(m_ioContext, pd.buf, m_ioContext->max_packet_size ? m_ioContext->max_packet_size : buffer_size);
        if (pd.buf_size <= 0)
        {
          CLog::Log(LOGERROR, "%s - error reading from input stream, %s", __FUNCTION__, CURL::GetRedacted(strFile).c_str());
          return false;
        }
        memset(pd.buf+pd.buf_size, 0, AVPROBE_PADDING_SIZE);

        // restore position again
        avio_seek(m_ioContext , 0, SEEK_SET);

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
          iformat2 = av_find_input_format("spdif");

          if (iformat2 && iformat2->read_probe(&pd) > AVPROBE_SCORE_MAX / 4)
          {
            iformat = iformat2;
          }
          else
          {
            // not spdif or no spdif demuxer, try dts
            iformat2 = av_find_input_format("dts");

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
          iformat = av_find_input_format("aac");
        else if( content.compare("audio/aac") == 0 )
          iformat = av_find_input_format("aac");
        else if( content.compare("video/flv") == 0 )
          iformat = av_find_input_format("flv");
        else if( content.compare("video/x-flv") == 0 )
          iformat = av_find_input_format("flv");
      }

      if (!iformat)
      {
        CLog::Log(LOGERROR, "%s - error probing input format, %s", __FUNCTION__, CURL::GetRedacted(strFile).c_str());
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


    m_pFormatContext->pb = m_ioContext;

    AVDictionary *options = NULL;
    if (iformat->name && (strcmp(iformat->name, "mp3") == 0 || strcmp(iformat->name, "mp2") == 0))
    {
      CLog::Log(LOGDEBUG, "%s - setting usetoc to 0 for accurate VBR MP3 seek", __FUNCTION__);
      av_dict_set(&options, "usetoc", "0", 0);
    }

    if (StringUtils::StartsWith(content, "audio/l16"))
    {
      int channels = 2;
      int samplerate = 44100;
      GetL16Parameters(channels, samplerate);
      av_dict_set_int(&options, "channels", channels, 0);
      av_dict_set_int(&options, "sample_rate", samplerate, 0);
    }

    if (avformat_open_input(&m_pFormatContext, strFile.c_str(), iformat, &options) < 0)
    {
      CLog::Log(LOGERROR, "%s - Error, could not open file %s", __FUNCTION__, CURL::GetRedacted(strFile).c_str());
      Dispose();
      av_dict_free(&options);
      return false;
    }
    av_dict_free(&options);
  }

  // Avoid detecting framerate if advancedsettings.xml says so
  if (g_advancedSettings.m_videoFpsDetect == 0) 
      m_pFormatContext->fps_probe_size = 0;
  
  // analyse very short to speed up mjpeg playback start
  if (iformat && (strcmp(iformat->name, "mjpeg") == 0) && m_ioContext->seekable == 0)
    av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);

  bool skipCreateStreams = false;
  bool isBluray = pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY);
  if (iformat && (strcmp(iformat->name, "mpegts") == 0) && !fileinfo && !isBluray)
  {
    av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);
    m_checkvideo = true;
    skipCreateStreams = true;
  }
  else if (!iformat || (strcmp(iformat->name, "mpegts") != 0))
  {
    m_streaminfo = true;
  }

  // we need to know if this is matroska or avi later
  m_bMatroska = strncmp(m_pFormatContext->iformat->name, "matroska", 8) == 0;	// for "matroska.webm"
  m_bAVI = strcmp(m_pFormatContext->iformat->name, "avi") == 0;

  if (m_streaminfo)
  {
    /* to speed up dvd switches, only analyse very short */
    if(m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
      av_opt_set_int(m_pFormatContext, "analyzeduration", 500000, 0);

    CLog::Log(LOGDEBUG, "%s - avformat_find_stream_info starting", __FUNCTION__);
    int iErr = avformat_find_stream_info(m_pFormatContext, NULL);
    if (iErr < 0)
    {
      CLog::Log(LOGWARNING,"could not find codec parameters for %s", CURL::GetRedacted(strFile).c_str());
      if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD)
      ||  m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY)
      || (m_pFormatContext->nb_streams == 1 && m_pFormatContext->streams[0]->codec->codec_id == AV_CODEC_ID_AC3)
      || m_checkvideo)
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

    if (m_checkvideo)
    {
      // make sure we start video with an i-frame
      ResetVideoStreams();
    }
  }
  else
  {
    m_program = 0;
    m_checkvideo = true;
    skipCreateStreams = true;
  }

  // reset any timeout
  m_timeout.SetInfinite();

  // if format can be nonblocking, let's use that
  m_pFormatContext->flags |= AVFMT_FLAG_NONBLOCK;

  // print some extra information
  av_dump_format(m_pFormatContext, 0, strFile.c_str(), 0);

  UpdateCurrentPTS();

  // in case of mpegts and we have not seen pat/pmt, defer creation of streams
  if (!skipCreateStreams || m_pFormatContext->nb_programs > 0)
    CreateStreams();

  // allow IsProgramChange to return true
  if (skipCreateStreams && GetNrOfStreams() == 0)
    m_program = 0;

  return true;
}

void CDVDDemuxFFmpeg::Dispose()
{
  m_pkt.result = -1;
  av_free_packet(&m_pkt.pkt);

  if (m_pFormatContext)
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
      avcodec_close(m_pFormatContext->streams[i]->codec);
    }

    if (m_ioContext && m_pFormatContext->pb && m_pFormatContext->pb != m_ioContext)
    {
      CLog::Log(LOGWARNING, "CDVDDemuxFFmpeg::Dispose - demuxer changed our byte context behind our back, possible memleak");
      m_ioContext = m_pFormatContext->pb;
    }
    avformat_close_input(&m_pFormatContext);
  }

  if(m_ioContext)
  {
    av_free(m_ioContext->buffer);
    av_free(m_ioContext);
  }

  m_ioContext = NULL;
  m_pFormatContext = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  DisposeStreams();

  m_pInput = NULL;
}

void CDVDDemuxFFmpeg::Reset()
{
  CDVDInputStream* pInputStream = m_pInput;
  Dispose();
  Open(pInputStream, m_streaminfo);
}

void CDVDDemuxFFmpeg::Flush()
{
  // naughty usage of an internal ffmpeg function
  if (m_pFormatContext)
    avformat_flush(m_pFormatContext);

  m_currentPts = DVD_NOPTS_VALUE;

  m_pkt.result = -1;
  av_free_packet(&m_pkt.pkt);
}

void CDVDDemuxFFmpeg::Abort()
{
  m_timeout.SetExpired();
}

void CDVDDemuxFFmpeg::SetSpeed(int iSpeed)
{
  if(!m_pFormatContext)
    return;

  if(m_speed != DVD_PLAYSPEED_PAUSE && iSpeed == DVD_PLAYSPEED_PAUSE)
  {
    m_pInput->Pause(m_currentPts);
    av_read_pause(m_pFormatContext);
  }
  else if(m_speed == DVD_PLAYSPEED_PAUSE && iSpeed != DVD_PLAYSPEED_PAUSE)
  {
    m_pInput->Pause(m_currentPts);
    av_read_play(m_pFormatContext);
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

AVDictionary *CDVDDemuxFFmpeg::GetFFMpegOptionsFromURL(const CURL &url)
{

  AVDictionary *options = NULL;

  if (url.IsProtocol("http") || url.IsProtocol("https"))
  {
    std::map<std::string, std::string> protocolOptions;
    url.GetProtocolOptions(protocolOptions);
    std::string headers;
    bool hasUserAgent = false;
    for(std::map<std::string, std::string>::const_iterator it = protocolOptions.begin(); it != protocolOptions.end(); ++it)
    {
      std::string name = it->first; StringUtils::ToLower(name);
      const std::string &value = it->second;

      if (name == "seekable")
        av_dict_set(&options, "seekable", value.c_str(), 0);
      else if (name == "user-agent")
      {
        av_dict_set(&options, "user-agent", value.c_str(), 0);
        hasUserAgent = true;
      }
      else if (name != "auth" && name != "encoding")
        // all other protocol options can be added as http header.
        headers.append(it->first).append(": ").append(value).append("\r\n");
    }
    if (!hasUserAgent)
      // set default xbmc user-agent.
      av_dict_set(&options, "user-agent", g_advancedSettings.m_userAgent.c_str(), 0);

    if (!headers.empty())
      av_dict_set(&options, "headers", headers.c_str(), 0);

    std::string cookies;
    if (XFILE::CCurlFile::GetCookies(url, cookies))
      av_dict_set(&options, "cookies", cookies.c_str(), 0);

  }
  return options;
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
  if(CDVDInputStream::IMenus* menu = dynamic_cast<CDVDInputStream::IMenus*>(m_pInput))
    starttime = menu->GetTimeStampCorrection() / DVD_TIME_BASE;
  else if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE)
    starttime = (double)m_pFormatContext->start_time / AV_TIME_BASE;

  if(timestamp > starttime)
    timestamp -= starttime;
  // allow for largest possible difference in pts and dts for a single packet
  else if( timestamp + 0.5f > starttime )
    timestamp = 0;

  return timestamp*DVD_TIME_BASE;
}

DemuxPacket* CDVDDemuxFFmpeg::Read()
{
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

    // check for saved packet after a program change
    if (m_pkt.result < 0)
    {
      // keep track if ffmpeg doesn't always set these
      m_pkt.pkt.size = 0;
      m_pkt.pkt.data = NULL;

      // timeout reads after 100ms
      m_timeout.Set(20000);
      m_pkt.result = av_read_frame(m_pFormatContext, &m_pkt.pkt);
      m_timeout.SetInfinite();
    }

    if (m_pkt.result == AVERROR(EINTR) || m_pkt.result == AVERROR(EAGAIN))
    {
      // timeout, probably no real error, return empty packet
      bReturnEmpty = true;
    }
    else if (m_pkt.result < 0)
    {
      Flush();
    }
    else if (IsProgramChange())
    {
      // update streams
      CreateStreams(m_program);

      pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);
      pPacket->iStreamId = DMX_SPECIALID_STREAMCHANGE;

      return pPacket;
    }
    // check size and stream index for being in a valid range
    else if (m_pkt.pkt.size < 0 ||
             m_pkt.pkt.stream_index < 0 ||
             m_pkt.pkt.stream_index >= (int)m_pFormatContext->nb_streams)
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

      m_pkt.result = -1;
      av_free_packet(&m_pkt.pkt);
    }
    else
    {
      ParsePacket(&m_pkt.pkt);

      AVStream *stream = m_pFormatContext->streams[m_pkt.pkt.stream_index];

      if (IsVideoReady())
      {
        if (m_program != UINT_MAX)
        {
          /* check so packet belongs to selected program */
          for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
          {
            if(m_pkt.pkt.stream_index == (int)m_pFormatContext->programs[m_program]->stream_index[i])
            {
              pPacket = CDVDDemuxUtils::AllocateDemuxPacket(m_pkt.pkt.size);
              break;
            }
          }

          if (!pPacket)
            bReturnEmpty = true;
        }
        else
          pPacket = CDVDDemuxUtils::AllocateDemuxPacket(m_pkt.pkt.size);
      }
      else
        bReturnEmpty = true;

      if (pPacket)
      {
        // lavf sometimes bugs out and gives 0 dts/pts instead of no dts/pts
        // since this could only happens on initial frame under normal
        // circomstances, let's assume it is wrong all the time
        if(m_pkt.pkt.dts == 0)
          m_pkt.pkt.dts = AV_NOPTS_VALUE;
        if(m_pkt.pkt.pts == 0)
          m_pkt.pkt.pts = AV_NOPTS_VALUE;

        if(m_bMatroska && stream->codec && stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        { // matroska can store different timestamps
          // for different formats, for native stored
          // stuff it is pts, but for ms compatibility
          // tracks, it is really dts. sadly ffmpeg
          // sets these two timestamps equal all the
          // time, so we select it here instead
          if(stream->codec->codec_tag == 0)
            m_pkt.pkt.dts = AV_NOPTS_VALUE;
          else
            m_pkt.pkt.pts = AV_NOPTS_VALUE;
        }

        // we need to get duration slightly different for matroska embedded text subtitels
        if(m_bMatroska && stream->codec && stream->codec->codec_id == AV_CODEC_ID_TEXT && m_pkt.pkt.convergence_duration != 0)
          m_pkt.pkt.duration = m_pkt.pkt.convergence_duration;

        if(m_bAVI && stream->codec && stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
          // AVI's always have borked pts, specially if m_pFormatContext->flags includes
          // AVFMT_FLAG_GENPTS so always use dts
          m_pkt.pkt.pts = AV_NOPTS_VALUE;
        }

        // copy contents into our own packet
        pPacket->iSize = m_pkt.pkt.size;

        // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
        if (m_pkt.pkt.data)
          memcpy(pPacket->pData, m_pkt.pkt.data, pPacket->iSize);

        pPacket->pts = ConvertTimestamp(m_pkt.pkt.pts, stream->time_base.den, stream->time_base.num);
        pPacket->dts = ConvertTimestamp(m_pkt.pkt.dts, stream->time_base.den, stream->time_base.num);
        pPacket->duration =  DVD_SEC_TO_TIME((double)m_pkt.pkt.duration * stream->time_base.num / stream->time_base.den);

        // used to guess streamlength
        if (pPacket->dts != DVD_NOPTS_VALUE && (pPacket->dts > m_currentPts || m_currentPts == DVD_NOPTS_VALUE))
          m_currentPts = pPacket->dts;


        // check if stream has passed full duration, needed for live streams
        bool bAllowDurationExt = (stream->codec && (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO || stream->codec->codec_type == AVMEDIA_TYPE_AUDIO));
        if(bAllowDurationExt && m_pkt.pkt.dts != (int64_t)AV_NOPTS_VALUE)
        {
          int64_t duration;
          duration = m_pkt.pkt.dts;
          if(stream->start_time != (int64_t)AV_NOPTS_VALUE)
            duration -= stream->start_time;

          if(duration > stream->duration)
          {
            stream->duration = duration;
            duration = av_rescale_rnd(stream->duration, (int64_t)stream->time_base.num * AV_TIME_BASE, stream->time_base.den, AV_ROUND_NEAR_INF);
            if ((m_pFormatContext->duration == (int64_t)AV_NOPTS_VALUE)
                ||  (m_pFormatContext->duration != (int64_t)AV_NOPTS_VALUE && duration > m_pFormatContext->duration))
              m_pFormatContext->duration = duration;
          }
        }

        // store internal id until we know the continuous id presented to player
        // the stream might not have been created yet
        pPacket->iStreamId = m_pkt.pkt.stream_index;
      }
      m_pkt.result = -1;
      av_free_packet(&m_pkt.pkt);
    }
  }
  } // end of lock scope
  if (bReturnEmpty && !pPacket)
    pPacket = CDVDDemuxUtils::AllocateDemuxPacket(0);

  if (!pPacket) return NULL;

  // check streams, can we make this a bit more simple?
  if (pPacket && pPacket->iStreamId >= 0)
  {
    CDemuxStream *stream = GetStreamInternal(pPacket->iStreamId);
    if (!stream ||
        stream->pPrivate != m_pFormatContext->streams[pPacket->iStreamId] ||
        stream->codec != m_pFormatContext->streams[pPacket->iStreamId]->codec->codec_id)
    {
      // content has changed, or stream did not yet exist
      stream = AddStream(pPacket->iStreamId);
    }
    // we already check for a valid m_streams[pPacket->iStreamId] above
    else if (stream->type == STREAM_AUDIO)
    {
      if (((CDemuxStreamAudio*)stream)->iChannels != m_pFormatContext->streams[pPacket->iStreamId]->codec->channels ||
          ((CDemuxStreamAudio*)stream)->iSampleRate != m_pFormatContext->streams[pPacket->iStreamId]->codec->sample_rate)
      {
        // content has changed
        stream = AddStream(pPacket->iStreamId);
      }
    }
    else if (stream->type == STREAM_VIDEO)
    {
      if (((CDemuxStreamVideo*)stream)->iWidth != m_pFormatContext->streams[pPacket->iStreamId]->codec->width ||
          ((CDemuxStreamVideo*)stream)->iHeight != m_pFormatContext->streams[pPacket->iStreamId]->codec->height)
      {
        // content has changed
        stream = AddStream(pPacket->iStreamId);
      }
    }
    if (!stream)
    {
      CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::AddStream - internal error, stream is null");
      CDVDDemuxUtils::FreeDemuxPacket(pPacket);
      return NULL;
    }
    // set continuous stream id for player
    pPacket->iStreamId = stream->iId;
  }
  return pPacket;
}

bool CDVDDemuxFFmpeg::SeekTime(int time, bool backwords, double *startpts)
{
  if (!m_pInput)
    return false;

  if(time < 0)
    time = 0;

  m_pkt.result = -1;
  av_free_packet(&m_pkt.pkt);

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

  int64_t seek_pts = (int64_t)time * (AV_TIME_BASE / 1000);
  bool ismp3 = m_pFormatContext->iformat && (strcmp(m_pFormatContext->iformat->name, "mp3") == 0);
  if (m_pFormatContext->start_time != (int64_t)AV_NOPTS_VALUE && !ismp3)
    seek_pts += m_pFormatContext->start_time;

  int ret;
  {
    CSingleLock lock(m_critSection);
    ret = av_seek_frame(m_pFormatContext, -1, seek_pts, backwords ? AVSEEK_FLAG_BACKWARD : 0);

    // demuxer will return failure, if you seek behind eof
    if (ret < 0 && m_pFormatContext->duration && seek_pts >= (m_pFormatContext->duration + m_pFormatContext->start_time))
      ret = 0;
    else if (ret < 0 && m_pInput->IsEOF())
      ret = 0;

    if(ret >= 0)
      UpdateCurrentPTS();
  }

  if(m_currentPts == DVD_NOPTS_VALUE)
    CLog::Log(LOGDEBUG, "%s - unknown position after seek", __FUNCTION__);
  else
    CLog::Log(LOGDEBUG, "%s - seek ended up on time %d", __FUNCTION__, (int)(m_currentPts / DVD_TIME_BASE * 1000));

  // in this case the start time is requested time
  if(startpts)
    *startpts = DVD_MSEC_TO_TIME(time);

  return (ret >= 0);
}

bool CDVDDemuxFFmpeg::SeekByte(int64_t pos)
{
  CSingleLock lock(m_critSection);
  int ret = av_seek_frame(m_pFormatContext, -1, pos, AVSEEK_FLAG_BYTE);

  if(ret >= 0)
    UpdateCurrentPTS();

  m_pkt.result = -1;
  av_free_packet(&m_pkt.pkt);

  return (ret >= 0);
}

void CDVDDemuxFFmpeg::UpdateCurrentPTS()
{
  m_currentPts = DVD_NOPTS_VALUE;
  
  int idx = av_find_default_stream_index(m_pFormatContext);
  if (idx >= 0)
  {
    AVStream *stream = m_pFormatContext->streams[idx];
    if(stream && stream->cur_dts != (int64_t)AV_NOPTS_VALUE)
    {
      double ts = ConvertTimestamp(stream->cur_dts, stream->time_base.den, stream->time_base.num);
      if(m_currentPts == DVD_NOPTS_VALUE || m_currentPts > ts )
        m_currentPts = ts;
    }
  }
}

int CDVDDemuxFFmpeg::GetStreamLength()
{
  if (!m_pFormatContext)
    return 0;

  if (m_pFormatContext->duration < 0)
    return 0;

  return (int)(m_pFormatContext->duration / (AV_TIME_BASE / 1000));
}

/**
 * @brief Finds stream based on demuxer index
 */
CDemuxStream* CDVDDemuxFFmpeg::GetStream(int iStreamId)
{
  if(iStreamId >= 0 && (size_t)iStreamId < m_stream_index.size())
    return m_stream_index[iStreamId]->second;
  else
    return NULL;
}

/**
 * @brief Finds stream based on ffmpeg index
 */
CDemuxStream* CDVDDemuxFFmpeg::GetStreamInternal(int iId)
{
  std::map<int, CDemuxStream*>::iterator it = m_streams.find(iId);
  if (it == m_streams.end())
    return NULL;
  else
    return it->second;
}

int CDVDDemuxFFmpeg::GetNrOfStreams()
{
  return m_stream_index.size();
}

static double SelectAspect(AVStream* st, bool* forced)
{
  *forced = false;
  /* if stream aspect is 1:1 or 0:0 use codec aspect */
  if((st->sample_aspect_ratio.den == 1 || st->sample_aspect_ratio.den == 0)
  && (st->sample_aspect_ratio.num == 1 || st->sample_aspect_ratio.num == 0)
  && st->codec->sample_aspect_ratio.num != 0)
    return av_q2d(st->codec->sample_aspect_ratio);

  *forced = true;
  if(st->sample_aspect_ratio.num != 0)
    return av_q2d(st->sample_aspect_ratio);

  return 0.0;
}

void CDVDDemuxFFmpeg::CreateStreams(unsigned int program)
{
  DisposeStreams();

  // add the ffmpeg streams to our own stream map
  if (m_pFormatContext->nb_programs)
  {
    // check if desired program is available
    if (program < m_pFormatContext->nb_programs && m_pFormatContext->programs[program]->nb_stream_indexes > 0)
    {
      m_program = program;
    }
    else
      m_program = UINT_MAX;

    // look for first non empty stream and discard nonselected programs
    for (unsigned int i = 0; i < m_pFormatContext->nb_programs; i++)
    {
      if(m_program == UINT_MAX && m_pFormatContext->programs[i]->nb_stream_indexes > 0)
      {
        m_program = i;
      }

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
  else
    m_program = UINT_MAX;

  // if there were no programs or they were all empty, add all streams
  if (m_program == UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
      AddStream(i);
  }
}

void CDVDDemuxFFmpeg::DisposeStreams()
{
  std::map<int, CDemuxStream*>::iterator it;
  for(it = m_streams.begin(); it != m_streams.end(); ++it)
    delete it->second;
  m_streams.clear();
  m_stream_index.clear();
}

CDemuxStream* CDVDDemuxFFmpeg::AddStream(int iId)
{
  AVStream* pStream = m_pFormatContext->streams[iId];
  if (pStream)
  {
    CDemuxStream* stream = NULL;

    switch (pStream->codec->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
      {
        CDemuxStreamAudioFFmpeg* st = new CDemuxStreamAudioFFmpeg(this, pStream);
        stream = st;
        st->iChannels = pStream->codec->channels;
        st->iSampleRate = pStream->codec->sample_rate;
        st->iBlockAlign = pStream->codec->block_align;
        st->iBitRate = pStream->codec->bit_rate;
        st->iBitsPerSample = pStream->codec->bits_per_raw_sample;
        if (st->iBitsPerSample == 0)
          st->iBitsPerSample = pStream->codec->bits_per_coded_sample;
	
        if(av_dict_get(pStream->metadata, "title", NULL, 0))
          st->m_description = av_dict_get(pStream->metadata, "title", NULL, 0)->value;

        break;
      }
    case AVMEDIA_TYPE_VIDEO:
      {
        CDemuxStreamVideoFFmpeg* st = new CDemuxStreamVideoFFmpeg(this, pStream);
        stream = st;
        if(strcmp(m_pFormatContext->iformat->name, "flv") == 0)
          st->bVFR = true;
        else
          st->bVFR = false;

        // never trust pts in avi files with h264.
        if (m_bAVI && pStream->codec->codec_id == AV_CODEC_ID_H264)
          st->bPTSInvalid = true;

#if defined(AVFORMAT_HAS_STREAM_GET_R_FRAME_RATE)
        AVRational r_frame_rate = av_stream_get_r_frame_rate(pStream);
#else
        AVRational r_frame_rate = pStream->r_frame_rate;
#endif

        //average fps is more accurate for mkv files
        if (m_bMatroska && pStream->avg_frame_rate.den && pStream->avg_frame_rate.num)
        {
          st->iFpsRate = pStream->avg_frame_rate.num;
          st->iFpsScale = pStream->avg_frame_rate.den;
        }
        else if(r_frame_rate.den && r_frame_rate.num)
        {
          st->iFpsRate = r_frame_rate.num;
          st->iFpsScale = r_frame_rate.den;
        }
        else
        {
          st->iFpsRate  = 0;
          st->iFpsScale = 0;
        }

        // added for aml hw decoder, mkv frame-rate can be wrong.
        if (r_frame_rate.den && r_frame_rate.num)
        {
          st->irFpsRate = r_frame_rate.num;
          st->irFpsScale = r_frame_rate.den;
        }
        else
        {
          st->irFpsRate = 0;
          st->irFpsScale = 0;
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
        st->fAspect = SelectAspect(pStream, &st->bForcedAspect) * pStream->codec->width / pStream->codec->height;
        st->iOrientation = 0;
        st->iBitsPerPixel = pStream->codec->bits_per_coded_sample;

        AVDictionaryEntry *rtag = av_dict_get(pStream->metadata, "rotate", NULL, 0);
        if (rtag) 
          st->iOrientation = atoi(rtag->value);

        // detect stereoscopic mode
        std::string stereoMode = GetStereoModeFromMetadata(pStream->metadata);
          // check for metadata in file if detection in stream failed
        if (stereoMode.empty())
          stereoMode = GetStereoModeFromMetadata(m_pFormatContext->metadata);
        if (!stereoMode.empty())
          st->stereo_mode = stereoMode;

        
        if ( m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) )
        {
          if (pStream->codec->codec_id == AV_CODEC_ID_PROBE)
          {
            // fix MPEG-1/MPEG-2 video stream probe returning AV_CODEC_ID_PROBE for still frames.
            // ffmpeg issue 1871, regression from ffmpeg r22831.
            if ((pStream->id & 0xF0) == 0xE0)
            {
              pStream->codec->codec_id = AV_CODEC_ID_MPEG2VIDEO;
              pStream->codec->codec_tag = MKTAG('M','P','2','V');
              CLog::Log(LOGERROR, "%s - AV_CODEC_ID_PROBE detected, forcing AV_CODEC_ID_MPEG2VIDEO", __FUNCTION__);
            }
          }
        }
        break;
      }
    case AVMEDIA_TYPE_DATA:
      {
        stream = new CDemuxStream();
        stream->type = STREAM_DATA;
        break;
      }
    case AVMEDIA_TYPE_SUBTITLE:
      {
        if (pStream->codec->codec_id == AV_CODEC_ID_DVB_TELETEXT && CSettings::Get().GetBool("videoplayer.teletextenabled"))
        {
          CDemuxStreamTeletext* st = new CDemuxStreamTeletext();
          stream = st;
          stream->type = STREAM_TELETEXT;
          break;
        }
        else
        {
          CDemuxStreamSubtitleFFmpeg* st = new CDemuxStreamSubtitleFFmpeg(this, pStream);
          stream = st;
	    
          if(av_dict_get(pStream->metadata, "title", NULL, 0))
            st->m_description = av_dict_get(pStream->metadata, "title", NULL, 0)->value;
	
          break;
        }
      }
    case AVMEDIA_TYPE_ATTACHMENT:
      { //mkv attachments. Only bothering with fonts for now.
        if(pStream->codec->codec_id == AV_CODEC_ID_TTF
          || pStream->codec->codec_id == AV_CODEC_ID_OTF
          )
        {
          std::string fileName = "special://temp/fonts/";
          XFILE::CDirectory::Create(fileName);
          AVDictionaryEntry *nameTag = av_dict_get(pStream->metadata, "filename", NULL, 0);
          if (!nameTag)
          {
            CLog::Log(LOGERROR, "%s: TTF attachment has no name", __FUNCTION__);
          }
          else
          {
            fileName += nameTag->value;
            XFILE::CFile file;
            if(pStream->codec->extradata && file.OpenForWrite(fileName))
            {
              if (file.Write(pStream->codec->extradata, pStream->codec->extradata_size) != pStream->codec->extradata_size)
              {
                file.Close();
                XFILE::CFile::Delete(fileName);
                CLog::Log(LOGDEBUG, "%s: Error saving font file \"%s\"", __FUNCTION__, fileName.c_str());
              }
            }
          }
        }
        stream = new CDemuxStream();
        stream->type = STREAM_NONE;
        break;
      }
    default:
      {
        stream = new CDemuxStream();
        stream->type = STREAM_NONE;
        break;
      }
    }

    // set ffmpeg type
    stream->orig_type = pStream->codec->codec_type;

    // generic stuff
    if (pStream->duration != (int64_t)AV_NOPTS_VALUE)
      stream->iDuration = (int)((pStream->duration / AV_TIME_BASE) & 0xFFFFFFFF);

    stream->codec = pStream->codec->codec_id;
    stream->codec_fourcc = pStream->codec->codec_tag;
    stream->profile = pStream->codec->profile;
    stream->level   = pStream->codec->level;

    stream->source = STREAM_SOURCE_DEMUX;
    stream->pPrivate = pStream;
    stream->flags = (CDemuxStream::EFlags)pStream->disposition;

    AVDictionaryEntry *langTag = av_dict_get(pStream->metadata, "language", NULL, 0);
    if (langTag)
      strncpy(stream->language, langTag->value, 3);

    if( pStream->codec->extradata && pStream->codec->extradata_size > 0 )
    {
      stream->ExtraSize = pStream->codec->extradata_size;
      stream->ExtraData = new uint8_t[pStream->codec->extradata_size];
      memcpy(stream->ExtraData, pStream->codec->extradata, pStream->codec->extradata_size);
    }

#ifdef HAVE_LIBBLURAY
    if( m_pInput->IsStreamType(DVDSTREAM_TYPE_BLURAY) )
      static_cast<CDVDInputStreamBluray*>(m_pInput)->GetStreamInfo(pStream->id, stream->language);
#endif
    if( m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD) )
    {
      // this stuff is really only valid for dvd's.
      // this is so that the physicalid matches the
      // id's reported from libdvdnav
      switch(stream->codec)
      {
        case AV_CODEC_ID_AC3:
          stream->iPhysicalId = pStream->id - 128;
          break;
        case AV_CODEC_ID_DTS:
          stream->iPhysicalId = pStream->id - 136;
          break;
        case AV_CODEC_ID_MP2:
          stream->iPhysicalId = pStream->id - 448;
          break;
        case AV_CODEC_ID_PCM_S16BE:
          stream->iPhysicalId = pStream->id - 160;
          break;
        case AV_CODEC_ID_DVD_SUBTITLE:
          stream->iPhysicalId = pStream->id - 0x20;
          break;
        default:
          stream->iPhysicalId = pStream->id & 0x1f;
          break;
      }
    }
    else
      stream->iPhysicalId = pStream->id;

    AddStream(iId, stream);
    return stream;
  }
  else
    return NULL;
}

/**
 * @brief Adds or updates a demux stream based in ffmpeg id
 */
void CDVDDemuxFFmpeg::AddStream(int iId, CDemuxStream* stream)
{
  std::pair<std::map<int, CDemuxStream*>::iterator, bool> res;

  res = m_streams.insert(std::make_pair(iId, stream));
  if(res.second)
  {
    /* was new stream */
    stream->iId = m_stream_index.size();
    m_stream_index.push_back(res.first);
  }
  else
  {
    /* replace old stream, keeping old index */
    stream->iId = res.first->second->iId;

    delete res.first->second;
    res.first->second = stream;
  }
  if(g_advancedSettings.m_logLevel > LOG_LEVEL_NORMAL)
    CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::AddStream(%d, ...) -> %d", iId, stream->iId);
}


std::string CDVDDemuxFFmpeg::GetFileName()
{
  if(m_pInput)
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

  return m_pFormatContext->nb_chapters;
}

int CDVDDemuxFFmpeg::GetChapter()
{
  CDVDInputStream::IChapter* ich = dynamic_cast<CDVDInputStream::IChapter*>(m_pInput);
  if(ich)
    return ich->GetChapter();

  if(m_pFormatContext == NULL
  || m_currentPts == DVD_NOPTS_VALUE)
    return 0;

  for(unsigned i = 0; i < m_pFormatContext->nb_chapters; i++)
  {
    AVChapter *chapter = m_pFormatContext->chapters[i];
    if(m_currentPts >= ConvertTimestamp(chapter->start, chapter->time_base.den, chapter->time_base.num)
    && m_currentPts <  ConvertTimestamp(chapter->end,   chapter->time_base.den, chapter->time_base.num))
      return i + 1;
  }

  return 0;
}

void CDVDDemuxFFmpeg::GetChapterName(std::string& strChapterName, int chapterIdx)
{
  if (chapterIdx <= 0 || chapterIdx > GetChapterCount())
    chapterIdx = GetChapter();
  CDVDInputStream::IChapter* ich = dynamic_cast<CDVDInputStream::IChapter*>(m_pInput);
  if(ich)  
    ich->GetChapterName(strChapterName, chapterIdx);
  else
  {
    if(chapterIdx <= 0)
      return;

    AVDictionaryEntry *titleTag = av_dict_get(m_pFormatContext->chapters[chapterIdx-1]->metadata,
                                                          "title", NULL, 0);
    if (titleTag)
      strChapterName = titleTag->value;
  }
}

int64_t CDVDDemuxFFmpeg::GetChapterPos(int chapterIdx)
{
  if (chapterIdx <= 0 || chapterIdx > GetChapterCount())
    chapterIdx = GetChapter();
  if(chapterIdx <= 0)
    return 0;

  CDVDInputStream::IChapter* ich = dynamic_cast<CDVDInputStream::IChapter*>(m_pInput);
  if(ich)  
    return ich->GetChapterPos(chapterIdx);

  return m_pFormatContext->chapters[chapterIdx-1]->start*av_q2d(m_pFormatContext->chapters[chapterIdx-1]->time_base);
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
    {
      *startpts = DVD_SEC_TO_TIME(ich->GetChapterPos(chapter));
    }

    Flush();
    return true;
  }

  if(m_pFormatContext == NULL)
    return false;

  if(chapter < 1 || chapter > (int)m_pFormatContext->nb_chapters)
    return false;

  AVChapter *ch = m_pFormatContext->chapters[chapter-1];
  double dts = ConvertTimestamp(ch->start, ch->time_base.den, ch->time_base.num);
  return SeekTime(DVD_TIME_TO_MSEC(dts), true, startpts);
}

void CDVDDemuxFFmpeg::GetStreamCodecName(int iStreamId, std::string &strName)
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
        StringUtils::ToLower(strName);
        return;
      }
    }

#ifdef FF_PROFILE_DTS_HD_MA
    /* use profile to determine the DTS type */
    if (stream->codec == AV_CODEC_ID_DTS)
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

    AVCodec *codec = avcodec_find_decoder(stream->codec);
    if (codec)
      strName = codec->name;
  }
}

bool CDVDDemuxFFmpeg::IsProgramChange()
{
  if (m_program == UINT_MAX)
    return false;

  if (m_program == 0 && !m_pFormatContext->nb_programs)
    return false;

  if(m_pFormatContext->programs[m_program]->nb_stream_indexes != m_streams.size())
    return true;

  if (m_program >= m_pFormatContext->nb_programs)
    return true;

  for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
  {
    int idx = m_pFormatContext->programs[m_program]->stream_index[i];
    CDemuxStream *stream = GetStreamInternal(idx);
    if(!stream)
      return true;
    if(m_pFormatContext->streams[idx]->codec->codec_type != stream->orig_type)
      return true;
  }
  return false;
}

std::string CDVDDemuxFFmpeg::GetStereoModeFromMetadata(AVDictionary *pMetadata)
{
  std::string stereoMode;
  AVDictionaryEntry *tag = NULL;

  // matroska
  tag = av_dict_get(pMetadata, "stereo_mode", NULL, 0);
  if (tag && tag->value)
    stereoMode = tag->value;

  // asf / wmv
  if (stereoMode.empty())
  {
    tag = av_dict_get(pMetadata, "Stereoscopic", NULL, 0);
    if (tag && tag->value)
    {
      tag = av_dict_get(pMetadata, "StereoscopicLayout", NULL, 0);
      if (tag && tag->value)
        stereoMode = ConvertCodecToInternalStereoMode(tag->value, WmvToInternalStereoModeMap);
    }
  }

  return stereoMode;
}

std::string CDVDDemuxFFmpeg::ConvertCodecToInternalStereoMode(const std::string &mode, const StereoModeConversionMap *conversionMap)
{
  size_t i = 0;
  while (conversionMap[i].name)
  {
    if (mode == conversionMap[i].name)
      return conversionMap[i].mode;
    i++;
  }
  return "";
}

void CDVDDemuxFFmpeg::ParsePacket(AVPacket *pkt)
{
  AVStream *st = m_pFormatContext->streams[pkt->stream_index];
  CDemuxStream *stream = GetStreamInternal(pkt->stream_index);

  // if the stream is new, tell ffmpeg to parse the stream
  if (!stream && !st->parser)
  {
    st->need_parsing = AVSTREAM_PARSE_FULL;
  }

  // split extradata
  if(st->parser && st->parser->parser->split && !st->codec->extradata)
  {
    int i = st->parser->parser->split(st->codec, pkt->data, pkt->size);
    if (i > 0 && i < FF_MAX_EXTRADATA_SIZE)
    {
      // Found extradata, fill it in. This will cause
      // a new stream to be created and used.
      st->codec->extradata_size = i;
      st->codec->extradata = (uint8_t*)av_malloc(st->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
      if (st->codec->extradata)
      {
        CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::Read() fetching extradata, extradata_size(%d)", st->codec->extradata_size);
        memcpy(st->codec->extradata, pkt->data, st->codec->extradata_size);
        memset(st->codec->extradata + i, 0, FF_INPUT_BUFFER_PADDING_SIZE);
      }
      else
      {
        st->codec->extradata_size = 0;
      }
    }
  }

  // for video we need a decoder to get desired information into codec context
  if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO && st->codec->extradata &&
      (!st->codec->width || st->codec->pix_fmt == PIX_FMT_NONE))
  {
    // open a decoder, it will be cleared down by ffmpeg on closing the stream
    if (!st->codec->codec)
    {
      const AVCodec* codec;
      AVDictionary *thread_opt = NULL;
      codec = avcodec_find_decoder(st->codec->codec_id);
      // Force thread count to 1 since the h264 decoder will not extract
      // SPS and PPS to extradata during multi-threaded decoding
      av_dict_set(&thread_opt, "threads", "1", 0);
      int res = avcodec_open2(st->codec, codec, &thread_opt);
      if(res < 0)
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::ParsePacket() unable to open codec %d", res);
      av_dict_free(&thread_opt);
    }

    // We don't need to actually decode here
    // we just want to transport SPS data into codec context
    st->codec->skip_idct = AVDISCARD_ALL;
    // extradata is not decoded if skip_frame >= AVDISCARD_NONREF
//    st->codec->skip_frame = AVDISCARD_ALL;
    st->codec->skip_loop_filter = AVDISCARD_ALL;

    // We are looking for an IDR frame
    AVFrame picture;
    memset(&picture, 0, sizeof(AVFrame));
    picture.pts = picture.pkt_dts = picture.pkt_pts = picture.best_effort_timestamp = AV_NOPTS_VALUE;
    picture.pkt_pos = -1;
    picture.key_frame = 1;
    picture.format = -1;

    int got_picture = 0;
    avcodec_decode_video2(st->codec, &picture, &got_picture, pkt);
    av_frame_unref(&picture);
  }
}

bool CDVDDemuxFFmpeg::IsVideoReady()
{
  AVStream *st;
  bool hasVideo = false;

  if(!m_checkvideo)
    return true;

  if (m_program == 0 && !m_pFormatContext->nb_programs)
    return false;

  if(m_program != UINT_MAX)
  {
    for (unsigned int i = 0; i < m_pFormatContext->programs[m_program]->nb_stream_indexes; i++)
    {
      int idx = m_pFormatContext->programs[m_program]->stream_index[i];
      st = m_pFormatContext->streams[idx];
      if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        if (st->codec->width && st->codec->pix_fmt != PIX_FMT_NONE)
          return true;
        hasVideo = true;
      }
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
      st = m_pFormatContext->streams[i];
      if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
      {
        if (st->codec->width && st->codec->pix_fmt != PIX_FMT_NONE)
          return true;
        hasVideo = true;
      }
    }
  }
  return !hasVideo;
}

void CDVDDemuxFFmpeg::ResetVideoStreams()
{
  AVStream *st;
  for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    st = m_pFormatContext->streams[i];
    if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      if (st->codec->extradata)
        av_free(st->codec->extradata);
      st->codec->extradata = NULL;
      st->codec->width = 0;
    }
  }
}

void CDVDDemuxFFmpeg::GetL16Parameters(int &channels, int &samplerate)
{
  std::string content;
  if (XFILE::CCurlFile::GetContentType(m_pInput->GetURL(), content))
  {
    StringUtils::ToLower(content);
    const size_t len = content.length();
    size_t pos = content.find(';');
    while (pos < len)
    {
      // move to the next non-whitespace character
      pos = content.find_first_not_of(" \t", pos + 1);

      if (pos != std::string::npos)
      {
        if (content.compare(pos, 9, "channels=", 9) == 0)
        {
          pos += 9; // move position to char after 'channels='
          size_t len = content.find(';', pos);
          if (len != std::string::npos)
            len -= pos;
          std::string no_channels(content, pos, len);
          // as we don't support any charset with ';' in name
          StringUtils::Trim(no_channels, " \t");
          if (!no_channels.empty())
          {
            int val = strtol(no_channels.c_str(), NULL, 0);
            if (val > 0)
              channels = val;
            else
              CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::%s - no parameter for channels", __FUNCTION__);
          }
        }
        else if (content.compare(pos, 5, "rate=", 5) == 0)
        {
          pos += 5; // move position to char after 'rate='
          size_t len = content.find(';', pos);
          if (len != std::string::npos)
            len -= pos;
          std::string rate(content, pos, len);
          // as we don't support any charset with ';' in name
          StringUtils::Trim(rate, " \t");
          if (!rate.empty())
          {
            int val = strtol(rate.c_str(), NULL, 0);
            if (val > 0)
              samplerate = val;
            else
              CLog::Log(LOGDEBUG, "CDVDDemuxFFmpeg::%s - no parameter for samplerate", __FUNCTION__);
          }
        }
        pos = content.find(';', pos); // find next parameter
      }
    }
  }
}
