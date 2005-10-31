
#include "../../../stdafx.h"
#include "..\DVDPlayerDLL.h"

#include "..\ffmpeg\ffmpeg.h"

#include "DVDDemuxFFmpeg.h"
#include "..\DVDInputStreams\DVDInputStream.h"
#include "DVDdemuxUtils.h"
#include "..\DVDClock.h" // for DVD_TIME_BASE

// class CDemuxStreamVideoFFmpeg
void CDemuxStreamVideoFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!pPrivate) return;
  char temp[128];
  avcodec_string(temp, 128, ((AVStream*)pPrivate)->codec, 0);
  strInfo = temp;
}

// class CDemuxStreamVideoFFmpeg
void CDemuxStreamAudioFFmpeg::GetStreamInfo(std::string& strInfo)
{
  if(!pPrivate) return;
  char temp[128];
  avcodec_string(temp, 128, ((AVStream*)pPrivate)->codec, 0);
  strInfo = temp;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

static int dvd_file_open(URLContext *h, const char *filename, int flags)
{
  return -1;
}

int dvd_file_read(URLContext *h, BYTE* buf, int size)
{
  CDVDInputStream* pInputStream = (CDVDInputStream*)h->priv_data;
  return pInputStream->Read(buf, size);
}

int dvd_file_write(URLContext *h, BYTE* buf, int size)
{
  CDVDInputStream* pInputStream = (CDVDInputStream*)h->priv_data;
  return -1;
}

__int64 dvd_file_seek(URLContext *h, __int64 pos, int whence)
{
  CDVDInputStream* pInputStream = (CDVDInputStream*)h->priv_data;
  return pInputStream->Seek(pos, whence);
}

int dvd_file_close(URLContext *h)
{
  return 0;
}

int dvd_input_stream_read_packet(void* opaque, BYTE* buf, int buf_size)
{
  return dvd_file_read((URLContext*)opaque, buf, buf_size);
}

int dvd_input_stream_write_packet(void* opaque, BYTE* buf, int buf_size)
{
  return dvd_file_write((URLContext*)opaque, buf, buf_size);
}

__int64 dvd_input_stream_seek(void *opaque, __int64 offset, int whence)
{
  return (int)(dvd_file_seek((URLContext*)opaque, offset, whence) & 0xFFFFFFFF);
}

URLProtocol dvd_file_protocol = {
                                  "file",
                                  dvd_file_open,
                                  dvd_file_read,
                                  dvd_file_write,
                                  dvd_file_seek,
                                  dvd_file_close,
                                };

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

CDVDDemuxFFmpeg::CDVDDemuxFFmpeg() : CDVDDemux()
{
  m_pUrlContext = NULL;
  m_pFormatContext = NULL;
  m_pInput = NULL;
  InitializeCriticalSection(&m_critSection);
  for (int i = 0; i < MAX_STREAMS; i++) m_streams[i] = NULL;
  m_bLoadedDllAvFormat = false;
  m_bLoadedDllAvCodec = false;
  m_iCurrentPts = 0LL;
}

CDVDDemuxFFmpeg::~CDVDDemuxFFmpeg()
{
  Dispose();
  DeleteCriticalSection(&m_critSection);
}

bool CDVDDemuxFFmpeg::LoadDlls()
{
  if (!m_bLoadedDllAvCodec)
  {
    DllLoader* pDll = g_sectionLoader.LoadDLL(DVD_AVCODEC_DLL);
    if (!pDll)
    {
      CLog::Log(LOGERROR, "CDVDDemuxFFmpeg: Unable to load dll %s", DVD_AVCODEC_DLL);
      return false;
    }
    
    if (!dvdplayer_load_dll_avcodec(*pDll))
    {
      CLog::Log(LOGERROR, "CDVDDemuxFFmpeg: Unable to resolve exports from %s", DVD_AVFORMAT_DLL);
      UnloadDlls();
      return false;
    }
    m_bLoadedDllAvCodec = true;
  }
  
  if (!m_bLoadedDllAvFormat)
  {
    DllLoader* pDll = g_sectionLoader.LoadDLL(DVD_AVFORMAT_DLL);
    if (!pDll)
    {
      CLog::Log(LOGERROR, "CDVDDemuxFFmpeg: Unable to load dll %s", DVD_AVFORMAT_DLL);
      UnloadDlls();
      return false;
    }
    
    if (!dvdplayer_load_dll_avformat(*pDll))
    {
      CLog::Log(LOGERROR, "CDVDDemuxFFmpeg: Unable to resolve exports from %s", DVD_AVCODEC_DLL);
      UnloadDlls();
      return false;
    }
    m_bLoadedDllAvFormat = true;
  }
  
  return true;
}

void CDVDDemuxFFmpeg::UnloadDlls()
{
  if (m_bLoadedDllAvCodec)
  {
    g_sectionLoader.UnloadDLL(DVD_AVCODEC_DLL);
    m_bLoadedDllAvCodec = false;
  }

  if (m_bLoadedDllAvFormat)
  {
    g_sectionLoader.UnloadDLL(DVD_AVFORMAT_DLL);
    m_bLoadedDllAvFormat = false;
  }
}
  
bool CDVDDemuxFFmpeg::Open(CDVDInputStream* pInput)
{
  AVInputFormat* iformat = NULL;
  const char* strFile;
  m_iCurrentPts = 0LL;
  
  if (!pInput) return false;

  if (!LoadDlls()) return false;
  
  // set ffmpeg logging, dvdplayer_log is staticly defined in ffmpeg.h
  av_log_set_callback(dvdplayer_log);

  // register codecs
  av_register_all();

  // could be used for interupting ffmpeg while opening a file (eg internet streams)
  // url_set_interrupt_cb(NULL);

  m_pInput = pInput;
  strFile = m_pInput->GetFileName();

  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD))
  {
    // we are playing form a dvd, just open the mpeg demuxer here
    iformat = av_find_input_format("mpeg");
    if (!iformat)
    {
      CLog::DebugLog("error opening ffmpeg's mpeg demuxer");
      return false;
    }
  }
  else
  {
    // let ffmpeg decide which demuxer we have to open
    AVProbeData pd;
    BYTE probe_buffer[2048];

    /* Init Probe data */
    pd.buf = probe_buffer;
    pd.filename = strFile;

    // read a bit from the file to see which demuxer we need
    if ((pd.buf_size = m_pInput->Read(pd.buf, 2048)) <= 0 )
    {
      CLog::DebugLog("error reading from input stream, %s", strFile);
      return false;
    }
    
    // and reset to the beginning
    m_pInput->Seek(0, SEEK_SET);

    iformat = av_probe_input_format(&pd, 1);
    if (!iformat)
    {
      CLog::DebugLog("error probing input format, %s", strFile);
      return false;
    }
  }

  // set this flag to avoid some input stream handling we don't want at this time
  iformat->flags |= AVFMT_NOFILE;

  int iBufferSize = FFMPEG_FILE_BUFFER_SIZE;
  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_DVD)) iBufferSize = FFMPEG_DVDNAV_BUFFER_SIZE;

  if (!ContextInit(strFile, m_ffmpegBuffer, iBufferSize))
  {
    Dispose();
    return false;
  }

  // open the demuxer
  if (av_open_input_stream(&m_pFormatContext, &m_ioContext, strFile, iformat, NULL) < 0)
  {
    CLog::DebugLog("Error, could not open file", strFile);
    Dispose();
    return false;
  }

  // in combination with libdvdnav seek, av_find_stream_info wont work
  // so we do this for files only
  if (m_pInput->IsStreamType(DVDSTREAM_TYPE_FILE))// &&
  //    !m_pInput->HasExtension("vob"))
  {
    // disable the AVFMT_NOFILE just once, else ffmpeg isn't able to find stream info
    iformat->flags &= ~AVFMT_NOFILE;
    int iErr = av_find_stream_info(m_pFormatContext);
    iformat->flags |= AVFMT_NOFILE;

    if (iErr < 0)
    {
      CLog::DebugLog("could not find codec parameters for %s", strFile);
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

  // print some extra information
  dump_format(m_pFormatContext, 0, strFile, 0);

  // add the ffmpeg streams to our own stream array
  for (int i = 0; i < m_pFormatContext->nb_streams; i++)
  {
    AddStream(i);
  }

  return true;
}

void CDVDDemuxFFmpeg::Dispose()
{
  if (m_pFormatContext) av_close_input_file(m_pFormatContext);

  for (int i = 0; i < MAX_STREAMS; i++)
  {
    if (m_streams[i]) delete m_streams[i];
    m_streams[i] = NULL;
  }

  m_pFormatContext = NULL;
  m_pInput = NULL;

  ContextDeInit();
  
  UnloadDlls();
}

/*
 * Init Byte IO Context
 * We need this for ffmpeg, cause it's the best / only way for reading files
 */
bool CDVDDemuxFFmpeg::ContextInit(const char* strFile, BYTE* buffer, int iBufferSize)
{
  if (m_pUrlContext) ContextDeInit();

  // create context and set all data to 0
  int iContextSize = sizeof(struct URLContext) + strlen(strFile) + 1;
  m_pUrlContext = (URLContext*)calloc(iContextSize, 1);

  // extrac filename part form context and copy
  char* filePos = (char*)m_pUrlContext + sizeof(URLContext) - 4;
  strcpy(filePos, strFile);

  // initialize context
  m_pUrlContext->prot = &dvd_file_protocol;
  m_pUrlContext->flags = AVFMT_NOFILE; // we open and close the file ourself
  m_pUrlContext->is_streamed = 0;      // default = not streamed
  m_pUrlContext->max_packet_size = 0;  // default: stream file
  m_pUrlContext->priv_data = (void*)m_pInput;

  m_pUrlContext->max_packet_size = iBufferSize;
  init_put_byte(&m_ioContext, buffer, iBufferSize, 0, m_pUrlContext, dvd_input_stream_read_packet,
                dvd_input_stream_write_packet, dvd_input_stream_seek);

  return true;
}

void CDVDDemuxFFmpeg::ContextDeInit()
{
  if (m_pUrlContext) free(m_pUrlContext);
  m_pUrlContext = NULL;
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
    //av_read_frame_flush(m_pFormatContext);
  }
}

CDVDDemux::DemuxPacket* CDVDDemuxFFmpeg::Read()
{
  AVPacket pkt;
  CDVDDemux::DemuxPacket* pPacket = NULL;
  Lock();
  if (m_pFormatContext)
  {
    if (av_read_frame(m_pFormatContext, &pkt) < 0)
    {
      // error reading from stream
      // XXX, just reset eof for now, and let the dvd player decide what todo
      m_pFormatContext->pb.eof_reached = 0;
      pPacket = NULL;
    }
    else
    {
      // pkt.pts is not the real pts, but a frame number.
      // to get our pts we need to multiply the frame delay with that number
      int num = m_pFormatContext->streams[pkt.stream_index]->time_base.num;
      int den = m_pFormatContext->streams[pkt.stream_index]->time_base.den;
        
      // XXX, in some cases ffmpeg returns a negative packet size
      if (pkt.size <= 0 || num == 0 || den == 0 || pkt.stream_index >= MAX_STREAMS)
      {
        CLog::Log(LOGERROR, "CDVDDemuxFFmpeg::Read() no valid packet");          
        return NULL;
      }
      
      pPacket = CDVDDemuxUtils::AllocateDemuxPacket(pkt.size);
      if (pPacket)
      {
        // copy contents into our own packet
        pPacket->iSize = pkt.size;
        
        // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
        fast_memcpy(pPacket->pData, pkt.data, pPacket->iSize);
        
        if (pkt.pts == AV_NOPTS_VALUE) pPacket->pts = DVD_NOPTS_VALUE;
        else
        {
          pPacket->pts = (num * pkt.pts * AV_TIME_BASE) / den;
          if (m_pFormatContext->start_time != DVD_NOPTS_VALUE &&
              pPacket->pts > (unsigned int)m_pFormatContext->start_time)
          {
            pPacket->pts -= m_pFormatContext->start_time;
          }
          
          // convert to dvdplayer clock ticks
          pPacket->pts = (pPacket->pts * DVD_TIME_BASE) / AV_TIME_BASE;
        }
        
        if (pkt.dts == AV_NOPTS_VALUE) pPacket->dts = DVD_NOPTS_VALUE;
        else
        {
          pPacket->dts = (num * pkt.dts * AV_TIME_BASE) / den;
          if (m_pFormatContext->start_time != DVD_NOPTS_VALUE &&
              pPacket->dts > (unsigned int)m_pFormatContext->start_time)
          {
            pPacket->dts -= m_pFormatContext->start_time;
          }
          
          // convert to dvdplayer clock ticks
          pPacket->dts = (pPacket->dts * DVD_TIME_BASE) / AV_TIME_BASE;
          
          // used to guess streamlength
          if (pPacket->dts > m_iCurrentPts)
          {
            m_iCurrentPts = pPacket->dts;
          }
        }

        pPacket->iStreamId = pkt.stream_index; // XXX just for now
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
  if (m_pFormatContext->start_time != DVD_NOPTS_VALUE && seek_pts < m_pFormatContext->start_time)
  {
    seek_pts += m_pFormatContext->start_time;
  }
  
  Lock();
  int ret = av_seek_frame(m_pFormatContext, -1, seek_pts, seek_pts < 0 ? AVSEEK_FLAG_BACKWARD : 0);
  m_iCurrentPts = 0LL;
  Unlock();
  
  return (ret >= 0);
}

int CDVDDemuxFFmpeg::GetStreamLenght()
{
  if (m_pFormatContext->duration == DVD_NOPTS_VALUE)
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
    if (m_streams[iId]) delete m_streams[iId];

    switch (pStream->codec->codec_type)
    {
    case CODEC_TYPE_AUDIO:
      {
        m_streams[iId] = new CDemuxStreamAudioFFmpeg();
        m_streams[iId]->type = STREAM_AUDIO;
        ((CDemuxStreamAudio*)m_streams[iId])->iChannels = pStream->codec->channels;
        ((CDemuxStreamAudio*)m_streams[iId])->iSampleRate = pStream->codec->sample_rate;
        break;
      }
    case CODEC_TYPE_VIDEO:
      {
        m_streams[iId] = new CDemuxStreamVideoFFmpeg();
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

    //FFMPEG has an error doesn't set type properly for DTS
    if( m_streams[iId]->codec == CODEC_ID_AC3 && (pStream->id >= 136 && pStream->id <= 143) )
      m_streams[iId]->codec = CODEC_ID_DTS;

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
      default:
        m_streams[iId]->iPhysicalId = pStream->id;
        break;
    }

    // we set this pointer to detect a stream changed inside ffmpeg
    // used to extract info too
    m_streams[iId]->pPrivate = pStream;
  }
}
