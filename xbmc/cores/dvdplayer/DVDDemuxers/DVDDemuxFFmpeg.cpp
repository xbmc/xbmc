
#include "stdafx.h"
#include "DVDDemuxFFmpeg.h"
#include "../DVDInputStreams/DVDInputStream.h"
#include "DVDDemuxUtils.h"
#include "../DVDClock.h" // for DVD_TIME_BASE

// class CDemuxStreamVideoFFmpeg
void CDemuxStreamVideoFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!pPrivate) return;
  char temp[128];
  m_pDll->avcodec_string(temp, 128, ((AVStream*)pPrivate)->codec, 0);
  strInfo = temp;
}

// class CDemuxStreamVideoFFmpeg
void CDemuxStreamAudioFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!pPrivate) return;
  char temp[128];
  m_pDll->avcodec_string(temp, 128, ((AVStream*)pPrivate)->codec, 0);
  strInfo = temp;
}

// these need to be put somewhere, they are prototyped together with avutil
void ff_avutil_log(void* ptr, int level, const char* format, va_list va)
{
  static CStdString buffer;

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

  prefix = "ffmpeg: ";
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


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

static int dvd_file_open(URLContext *h, const char *filename, int flags)
{
  return -1;
}

static int dvd_file_read(URLContext *h, BYTE* buf, int size)
{
  CDVDInputStream* pInputStream = (CDVDInputStream*)h->priv_data;
  return pInputStream->Read(buf, size);
}

static int dvd_file_write(URLContext *h, BYTE* buf, int size)
{
  return -1;
}

static __int64 dvd_file_seek(URLContext *h, __int64 pos, int whence)
{  
  CDVDInputStream* pInputStream = (CDVDInputStream*)h->priv_data;
  if(whence == AVSEEK_SIZE)
    return pInputStream->GetLength();
  else
    return pInputStream->Seek(pos, whence);
}

static int dvd_file_close(URLContext *h)
{
  return 0;
}

static DWORD g_urltimeout = 0;
static int interrupt_cb(void)
{
  if(!g_urltimeout)
    return 0;
  
  if(GetTickCount() > g_urltimeout)
    return 1;

  return 0;
}

URLProtocol dvd_file_protocol = {
                                  "CDVDInputStream",
                                  NULL,
                                  dvd_file_read,
                                  NULL,
                                  dvd_file_seek,
                                  dvd_file_close,
                                  NULL
                                };

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

CDVDDemuxFFmpeg::CDVDDemuxFFmpeg() : CDVDDemux()
{
  m_pFormatContext = NULL;
  m_pInput = NULL;
  memset(&m_ioContext, 0, sizeof(ByteIOContext));
  InitializeCriticalSection(&m_critSection);
  for (int i = 0; i < MAX_STREAMS; i++) m_streams[i] = NULL;
  m_iCurrentPts = 0LL;
}

CDVDDemuxFFmpeg::~CDVDDemuxFFmpeg()
{
  Dispose();
  DeleteCriticalSection(&m_critSection);
}

bool CDVDDemuxFFmpeg::Open(CDVDInputStream* pInput)
{
  AVInputFormat* iformat = NULL;
  const char* strFile;
  m_iCurrentPts = 0LL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  if (!pInput) return false;

  if (!m_dllAvFormat.Load() || !m_dllAvCodec.Load() || !m_dllAvUtil.Load() )
  {
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
    if( content.compare("audio/aacp") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("aac");
    else if( content.compare("audio/aac") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("aac");
    else if( content.compare("audio/mpeg") == 0  )  
      iformat = m_dllAvFormat.av_find_input_format("mp3");
    else if( content.compare("video/mpeg") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("mpeg");
    else if( content.compare("video/flv") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("flv");
    else if( content.compare("video/x-flv") == 0 )
      iformat = m_dllAvFormat.av_find_input_format("flv");

    /* these are likely pure streams, and as such we don't */
    /* want to try to look for streaminfo before playback */
    if( iformat )
      streaminfo = false;
  }

  if( m_pInput->IsStreamType(DVDSTREAM_TYPE_FFMPEG) )
  {
    g_urltimeout = GetTickCount() + 10000;

    // special stream type that makes avformat handle file opening
    // allows internal ffmpeg protocols to be used
    if( m_dllAvFormat.av_open_input_file(&m_pFormatContext, strFile, iformat, FFMPEG_FILE_BUFFER_SIZE, NULL) < 0 )
    {
      CLog::DebugLog("Error, could not open file %s", strFile);
      Dispose();
      return false;
    }
  }
  else
  {
    g_urltimeout = 0;

    // initialize url context to be used as filedevice
    URLContext* context = (URLContext*)m_dllAvUtil.av_mallocz(sizeof(struct URLContext) + strlen(strFile) + 1);
    context->prot = &dvd_file_protocol;
    context->priv_data = (void*)m_pInput;
    context->max_packet_size = FFMPEG_FILE_BUFFER_SIZE;

    if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
    {
      context->max_packet_size = FFMPEG_DVDNAV_BUFFER_SIZE;
      context->is_streamed = 1;
    }
    else if( m_pInput->IsStreamType(DVDSTREAM_TYPE_FILE) )
    {
      if(m_pInput->Seek(0, SEEK_CUR) < 0)
        context->is_streamed = 1;
    }
    
    // skip finding stream info for any streamed content
    if(context->is_streamed)
      streaminfo = false;  

    strcpy(context->filename, strFile);  

    // open our virtual file device
    if(m_dllAvFormat.url_fdopen(&m_ioContext, context) < 0)
    {
      CLog::Log(LOGERROR, "%s - Unable to init io context", __FUNCTION__);
      m_dllAvUtil.av_free(context);
      Dispose();
      return false;
    }

    if( iformat == NULL )
    {
      // let ffmpeg decide which demuxer we have to open
      AVProbeData pd;
      BYTE probe_buffer[2048];

      // init probe data
      pd.buf = probe_buffer;
      pd.filename = strFile;

      // read data using avformat's buffers
      pd.buf_size = m_dllAvFormat.get_buffer(&m_ioContext, pd.buf, sizeof(probe_buffer));
      m_dllAvFormat.url_fseek(&m_ioContext , 0, SEEK_SET);
      
      if (pd.buf_size == 0)
      {
        CLog::Log(LOGERROR, "%s - error reading from input stream, %s", __FUNCTION__, strFile);
        return false;
      }
      
      iformat = m_dllAvFormat.av_probe_input_format(&pd, 1);
      if (!iformat)
      {
        CLog::Log(LOGERROR, "%s - error probing input format, %s", __FUNCTION__, strFile);
        return false;
      }
    }


    // open the demuxer
    if (m_dllAvFormat.av_open_input_stream(&m_pFormatContext, &m_ioContext, strFile, iformat, NULL) < 0)
    {
      CLog::DebugLog("Error, could not open file", strFile);
      Dispose();
      return false;
    }
  }

  // in combination with libdvdnav seek, av_find_stream_info wont work
  // so we do this for files only
  if (streaminfo)
  {
    int iErr = m_dllAvFormat.av_find_stream_info(m_pFormatContext);
    if (iErr < 0)
    {
      CLog::Log(LOGDEBUG,"could not find codec parameters for %s", strFile);
      if (m_pFormatContext->nb_streams == 1 && m_pFormatContext->streams[0]->codec->codec_id == CODEC_ID_AC3)
      {
        // special case, our codecs can still handle it.
      }
      else
      {
        Dispose();
        return false;
      }
    }
  }
  // reset any timeout
  g_urltimeout = 0;

  // print some extra information
  m_dllAvFormat.dump_format(m_pFormatContext, 0, strFile, 0);

  // add the ffmpeg streams to our own stream array
  for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    AddStream(i);
  }

  return true;
}

void CDVDDemuxFFmpeg::Dispose()
{
  if (m_pFormatContext) 
    m_dllAvFormat.av_close_input_file(m_pFormatContext);  
  else if (m_ioContext.opaque) // can only do this as an alternate, m_pFormatContex has copy
    m_dllAvFormat.url_fclose(&m_ioContext);

  memset(&m_ioContext, 0, sizeof(ByteIOContext));
  m_pFormatContext = NULL;
  m_speed = DVD_PLAYSPEED_NORMAL;

  for (int i = 0; i < MAX_STREAMS; i++)
  {
    if (m_streams[i]) delete m_streams[i];
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
  if (m_pFormatContext)
  {
    m_dllAvFormat.av_read_frame_flush(m_pFormatContext);
  }
}

void CDVDDemuxFFmpeg::Abort()
{
  g_urltimeout = 1;
}

void CDVDDemuxFFmpeg::SetSpeed(int iSpeed)
{
  if(!m_pFormatContext)
    return;

  if(m_speed != DVD_PLAYSPEED_PAUSE && iSpeed == DVD_PLAYSPEED_PAUSE)
    m_dllAvFormat.av_read_pause(m_pFormatContext);
  else if(m_speed == DVD_PLAYSPEED_PAUSE && iSpeed != DVD_PLAYSPEED_PAUSE)
    m_dllAvFormat.av_read_play(m_pFormatContext);

  m_speed = iSpeed;
}

__int64 CDVDDemuxFFmpeg::ConvertTimestamp(__int64 pts, int den, int num)
{
  if (pts == AV_NOPTS_VALUE)
    return DVD_NOPTS_VALUE;

  // do calculations in floats as they can easily overflow otherwise
  // we don't care for having a completly exact timestamp anyway
  double timestamp = (double)pts * num  / den;
  double starttime = 0.0f;

  if (m_pFormatContext->start_time != AV_NOPTS_VALUE)
    starttime = (double)m_pFormatContext->start_time / AV_TIME_BASE;

  if(timestamp > starttime)
    timestamp -= starttime;
  else if( timestamp + 0.1f > starttime )
    timestamp = 0;

  return (__int64)(timestamp*DVD_TIME_BASE+0.5f);
}

CDVDDemux::DemuxPacket* CDVDDemuxFFmpeg::Read()
{
  AVPacket pkt;
  CDVDDemux::DemuxPacket* pPacket = NULL;
  Lock();
  if (m_pFormatContext)
  {
    // assume we are not eof
    m_pFormatContext->pb.eof_reached = 0;

    // timeout reads after 100ms
    g_urltimeout = GetTickCount() + 100;
    int result = m_dllAvFormat.av_read_frame(m_pFormatContext, &pkt);
    g_urltimeout = 0;

    if (result < 0)
    {
      // we are likely atleast at an discontinuity
      m_dllAvFormat.av_read_frame_flush(m_pFormatContext);

      // reset any dts interpolation      
      for(int i=0;i<MAX_STREAMS;i++)
      {
        if(m_pFormatContext->streams[i])
        {
          m_pFormatContext->streams[i]->cur_dts = AV_NOPTS_VALUE;
          m_pFormatContext->streams[i]->last_IP_duration = 0;
          m_pFormatContext->streams[i]->last_IP_pts = AV_NOPTS_VALUE;
        }
      }

      m_iCurrentPts = 0LL;
    }
    else
    {        
      // XXX, in some cases ffmpeg returns a negative packet size
      if (pkt.size <= 0 || pkt.stream_index >= MAX_STREAMS)
      {
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() no valid packet");
      }
      else
      {
        AVStream *stream = m_pFormatContext->streams[pkt.stream_index];

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

          // lavf lies about delayed frames in h264 since parser doesn't set it properly
          // dts values are then invalid
          if(stream->codec && stream->codec->codec_id == CODEC_ID_H264 && !stream->codec->has_b_frames)
            pkt.dts = AV_NOPTS_VALUE;

          // copy contents into our own packet
          pPacket->iSize = pkt.size;
          
          // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
          memcpy(pPacket->pData, pkt.data, pPacket->iSize);
          
          pPacket->pts = ConvertTimestamp(pkt.pts, stream->time_base.den, stream->time_base.num);
          pPacket->dts = ConvertTimestamp(pkt.dts, stream->time_base.den, stream->time_base.num);

          // used to guess streamlength
          if (pPacket->dts != DVD_NOPTS_VALUE && pPacket->dts > m_iCurrentPts)
            m_iCurrentPts = pPacket->dts;
          
          pPacket->iStreamId = pkt.stream_index; // XXX just for now
        }
      }
      av_free_packet(&pkt);
    }
  }
  Unlock();

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

bool CDVDDemuxFFmpeg::Seek(int iTime)
{
  __int64 seek_pts = (__int64)iTime * (AV_TIME_BASE / 1000);
  if (m_pFormatContext->start_time != AV_NOPTS_VALUE && seek_pts < m_pFormatContext->start_time)
  {
    seek_pts += m_pFormatContext->start_time;
  }
  
  Lock();
  int ret = m_dllAvFormat.av_seek_frame(m_pFormatContext, -1, seek_pts, seek_pts < 0 ? AVSEEK_FLAG_BACKWARD : 0);
  m_iCurrentPts = 0LL;
  Unlock();
  
  return (ret >= 0);
}

int CDVDDemuxFFmpeg::GetStreamLenght()
{
  /* apperently ffmpeg messes up sometimes, so check for negative value too */
  if (m_pFormatContext->duration == AV_NOPTS_VALUE || m_pFormatContext->duration < 0LL)
  {
    // no duration is available for us
    // try to calculate it
    int iLength = 0;
    if (m_iCurrentPts > 0 && m_pFormatContext->file_size > 0 && m_pFormatContext->pb.pos > 0)
    {
      iLength = (int)(((m_iCurrentPts * m_pFormatContext->file_size) / m_pFormatContext->pb.pos) / 1000) & 0xFFFFFFFF;
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

void CDVDDemuxFFmpeg::AddStream(int iId)
{
  AVStream* pStream = m_pFormatContext->streams[iId];
  if (pStream)
  {
    if (m_streams[iId]) 
    {
      if( m_streams[iId]->ExtraData ) delete[] m_streams[iId]->ExtraData;
      delete m_streams[iId];
    }

    switch (pStream->codec->codec_type)
    {
    case CODEC_TYPE_AUDIO:
      {
        m_streams[iId] = new CDemuxStreamAudioFFmpeg(&m_dllAvCodec);
        m_streams[iId]->type = STREAM_AUDIO;
        ((CDemuxStreamAudio*)m_streams[iId])->iChannels = pStream->codec->channels;
        ((CDemuxStreamAudio*)m_streams[iId])->iSampleRate = pStream->codec->sample_rate;
        break;
      }
    case CODEC_TYPE_VIDEO:
      {
        m_streams[iId] = new CDemuxStreamVideoFFmpeg(&m_dllAvCodec);
        m_streams[iId]->type = STREAM_VIDEO;
        ((CDemuxStreamVideo*)m_streams[iId])->iFpsRate = pStream->codec->time_base.den;
        ((CDemuxStreamVideo*)m_streams[iId])->iFpsScale = pStream->codec->time_base.num;
        ((CDemuxStreamVideo*)m_streams[iId])->iWidth = pStream->codec->width;
        ((CDemuxStreamVideo*)m_streams[iId])->iHeight = pStream->codec->height;
        break;
      }
    case CODEC_TYPE_DATA:
      {
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_DATA;
        break;
      }
    case CODEC_TYPE_SUBTITLE:
      {
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_SUBTITLE;
        break;
      }
    default:
      {
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_NONE;
        break;
      }
    }

    // generic stuff
    if (pStream->duration != AV_NOPTS_VALUE) m_streams[iId]->iDuration = (int)((pStream->duration / AV_TIME_BASE) & 0xFFFFFFFF);

    m_streams[iId]->codec = pStream->codec->codec_id;
    m_streams[iId]->iId = iId;

    strcpy( m_streams[iId]->language, pStream->language );

    if( pStream->codec->extradata && pStream->codec->extradata_size > 0 )
    {
      m_streams[iId]->ExtraSize = pStream->codec->extradata_size;
      m_streams[iId]->ExtraData = new BYTE[pStream->codec->extradata_size];
      memcpy(m_streams[iId]->ExtraData, pStream->codec->extradata, pStream->codec->extradata_size);
    }

    //FFMPEG has an error doesn't set type properly for DTS
    if( m_streams[iId]->codec == CODEC_ID_AC3 && (pStream->id >= 136 && pStream->id <= 143) )
      m_streams[iId]->codec = CODEC_ID_DTS;

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

    // we set this pointer to detect a stream changed inside ffmpeg
    // used to extract info too
    m_streams[iId]->pPrivate = pStream;
  }
}

