
#include "stdafx.h"
#include "..\DVDPlayerDLL.h"

#include "..\ffmpeg\ffmpeg.h"

#include "DVDDemuxFFmpeg.h"
#include "..\DVDInputStreams\DVDInputStream.h"
#include "DVDdemuxUtils.h"
#include "..\..\..\utils\log.h"
#include <errno.h>
#include "..\DVDClock.h" // for DVD_TIME_BASE

// class CDemuxStreamVideoFFmpeg
void CDemuxStreamVideoFFmpeg::GetStreamInfo(std::string& strInfo)
{
  char temp[128];
  avcodec_string(temp, 128, &((AVStream*)pPrivate)->codec, 0);
  strInfo = temp;
}

// class CDemuxStreamVideoFFmpeg
void CDemuxStreamAudioFFmpeg::GetStreamInfo(std::string& strInfo)
{
  char temp[128];
  avcodec_string(temp, 128, &((AVStream*)pPrivate)->codec, 0);
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

int dvd_input_stream_seek(void *opaque, __int64 offset, int whence)
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
}

CDVDDemuxFFmpeg::~CDVDDemuxFFmpeg()
{
  Dispose();
  DeleteCriticalSection(&m_critSection);
}

void CDVDDemuxFFmpeg::Lock()
{
  EnterCriticalSection(&m_critSection);
}

void CDVDDemuxFFmpeg::Unlock()
{
  LeaveCriticalSection(&m_critSection);
}
  
bool CDVDDemuxFFmpeg::Open(CDVDInputStream* pInput)
{
  AVInputFormat* iformat = NULL;
  const char* strFile;
  
  if (!pInput) return false;
  
  // set ffmpeg logging, dvdplayer_log is staticly defined in ffmpeg.h
  av_log_set_callback(dvdplayer_log);
  
  // register codecs
  av_register_all();
  
  // could be used for interupting ffmpeg while opening a file (eg internet streams)
  // url_set_interrupt_cb(NULL);
  
  m_pInput = pInput;
  strFile = m_pInput->GetFileName();
  
  if (m_pInput->m_streamType == DVDSTREAM_TYPE_DVD)
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
    if((pd.buf_size = m_pInput->Read(pd.buf, 2048)) <= 0 )
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
  if (m_pInput->m_streamType == DVDSTREAM_TYPE_DVD) iBufferSize = FFMPEG_DVDNAV_BUFFER_SIZE;
  
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
  if (m_pInput->m_streamType == DVDSTREAM_TYPE_FILE)
  {
    // disable the AVFMT_NOFILE just once, else ffmpeg isn't able to find stream info
    iformat->flags &= ~AVFMT_NOFILE;
    int iErr = av_find_stream_info(m_pFormatContext);
    iformat->flags |= AVFMT_NOFILE;
    
    if (iErr < 0)
    {
      CLog::DebugLog("could not find codec parameters for %s", strFile);
      Dispose();
      return false;
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

CDVDDemux::DemuxPacket* CDVDDemuxFFmpeg::Read()
{
  AVPacket pkt;
  CDVDDemux::DemuxPacket* pPacket = NULL;
  Lock();
  if (m_pFormatContext)
  {
    if (av_read_frame(m_pFormatContext, &pkt))
    {
      // error reading from stream
      // XXX, currently there is a bug when reading from libdvdnav at the moment of a STILL_PICTURE
      // XXX, just reset eof for now, and let the dvd player decide what todo
      m_pFormatContext->pb.eof_reached = 0;
      pPacket = NULL;
    }
    else 
    {
      pPacket = CDVDDemuxUtils::AllocateDemuxPacket();
      
      // copy contents into our own packet
      if (pkt.pts == AV_NOPTS_VALUE) pPacket->pts = DVD_NOPTS_VALUE;
      else pPacket->pts = pkt.pts;
      if (pkt.dts == AV_NOPTS_VALUE) pPacket->dts = DVD_NOPTS_VALUE;
      else pPacket->dts = pkt.dts;
      
      pPacket->iStreamId = pkt.stream_index; // XXX just for now
      
      // maybe we can avoid a memcpy here by detecting where pkt.destruct is pointing too?
      pPacket->iSize = pkt.size;
      // need to allocate a few bytes more.
      // From avcodec.h (ffmpeg)
      /**
        * Required number of additionally allocated bytes at the end of the input bitstream for decoding.
        * this is mainly needed because some optimized bitstream readers read 
        * 32 or 64 bit at once and could read over the end<br>
        * Note, if the first 23 bits of the additional bytes are not 0 then damaged
        * MPEG bitstreams could cause overread and segfault
        */
      // #define FF_INPUT_BUFFER_PADDING_SIZE 8
      pPacket->pData = new BYTE[pPacket->iSize + 8];
      if (!pPacket->pData)
      {
        // out of memory, free as much as possible and return NULL (read error)
        CDVDDemuxUtils::FreeDemuxPacket(pPacket);
        av_free_packet(&pkt);
        Unlock();
        return NULL;
      }
      fast_memcpy(pPacket->pData, pkt.data, pPacket->iSize);

      av_free_packet(&pkt);
    }
  }
  Unlock();
  
  // check streams, can we make this a bit more simple?
  if (pPacket && pPacket->iStreamId >= 0 && pPacket->iStreamId <= MAX_STREAMS)
  {
    if (!m_streams[pPacket->iStreamId] ||
        m_streams[pPacket->iStreamId]->pPrivate != m_pFormatContext->streams[pPacket->iStreamId] ||
        m_streams[pPacket->iStreamId]->codec != m_pFormatContext->streams[pPacket->iStreamId]->codec.codec_id)
    {
      // content has changed, or stream did not yet exist
      AddStream(pPacket->iStreamId);
    }
    // we already check for a valid m_streams[pPacket->iStreamId] above
    else if (m_streams[pPacket->iStreamId]->type == STREAM_AUDIO)
    {
      if (((CDemuxStreamAudio*)m_streams[pPacket->iStreamId])->iChannels != m_pFormatContext->streams[pPacket->iStreamId]->codec.channels ||
          ((CDemuxStreamAudio*)m_streams[pPacket->iStreamId])->iSampleRate != m_pFormatContext->streams[pPacket->iStreamId]->codec.sample_rate)
      {
        // content has changed
        AddStream(pPacket->iStreamId);
      }
    }
    else if (m_streams[pPacket->iStreamId]->type == STREAM_VIDEO)
    {
      if (((CDemuxStreamVideo*)m_streams[pPacket->iStreamId])->iWidth != m_pFormatContext->streams[pPacket->iStreamId]->codec.width ||
      ((CDemuxStreamVideo*)m_streams[pPacket->iStreamId])->iHeight != m_pFormatContext->streams[pPacket->iStreamId]->codec.height)
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
  Lock();
  __int64 seek_pts = (__int64)iTime * AV_TIME_BASE;
  int ret = av_seek_frame(m_pFormatContext, -1, seek_pts, AVSEEK_FLAG_BACKWARD);
  Unlock();
  return (ret >= 0);
}

int CDVDDemuxFFmpeg::GetStreamLenght()
{
  if (m_pFormatContext->duration == DVD_NOPTS_VALUE) return 0;
  
  return (int)(m_pFormatContext->duration / AV_TIME_BASE);
}

CDemuxStream* CDVDDemuxFFmpeg::GetStream(int iStreamId)
{
  if (iStreamId < 0 || iStreamId >= MAX_STREAMS) return NULL;
  return m_streams[iStreamId];
}

int CDVDDemuxFFmpeg::GetNrOfStreams()
{
  int i = 0;
  while (m_streams[i]) i++;
  return i;
}

void CDVDDemuxFFmpeg::AddStream(int iId)
{
  AVStream* pStream = m_pFormatContext->streams[iId];
  if (pStream)
  {
    if (m_streams[iId]) delete m_streams[iId];
    
    switch (pStream->codec.codec_type)
    {
      case CODEC_TYPE_AUDIO:
      {
        m_streams[iId] = new CDemuxStreamAudioFFmpeg();
        m_streams[iId]->type = STREAM_AUDIO;
        ((CDemuxStreamAudio*)m_streams[iId])->iChannels = pStream->codec.channels;
        ((CDemuxStreamAudio*)m_streams[iId])->iSampleRate = pStream->codec.sample_rate;
        break;
      }
      case CODEC_TYPE_VIDEO:
      {
        m_streams[iId] = new CDemuxStreamVideoFFmpeg();
        m_streams[iId]->type = STREAM_VIDEO;
        ((CDemuxStreamVideo*)m_streams[iId])->iFpsRate = pStream->codec.frame_rate;
        ((CDemuxStreamVideo*)m_streams[iId])->iFpsScale = pStream->codec.frame_rate_base;
        ((CDemuxStreamVideo*)m_streams[iId])->iWidth = pStream->codec.width;
        ((CDemuxStreamVideo*)m_streams[iId])->iHeight = pStream->codec.height;
        break;
      }
      case CODEC_TYPE_DATA:
      {
        m_streams[iId] = new CDemuxStream();
        m_streams[iId]->type = STREAM_DATA;
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
    
    m_streams[iId]->codec = pStream->codec.codec_id;
    m_streams[iId]->iId = pStream->id;
    
    // we set this pointer to detect a stream changed inside ffmpeg
    // used to extract info too
    m_streams[iId]->pPrivate = pStream;
  }
}
