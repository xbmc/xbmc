#include "../../stdafx.h"
#include "FLACCodec.h"
#include "../../FLACtag.h"


FLACCodec::FLACCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime=0;
  m_Bitrate = 0;
  m_CodecName = L"FLAC";

  m_pFlacDecoder=NULL;

  m_pBuffer=NULL;
  m_BufferSize=0; 
  m_MaxFrameSize=0;

}

FLACCodec::~FLACCodec()
{
  DeInit();
}

bool FLACCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_file.Initialize(filecache);

  if (!m_dll.Load())
    return false;

  if (!m_file.Open(strFile))
    return false;

  m_pFlacDecoder=m_dll.FLAC__seekable_stream_decoder_new();

  if (!m_pFlacDecoder)
  {
    CLog::Log(LOGERROR, "FLACCodec: Error creating decoder");
    return false;
  }

  m_dll.FLAC__seekable_stream_decoder_set_read_callback (m_pFlacDecoder, DecoderReadCallback);
  m_dll.FLAC__seekable_stream_decoder_set_seek_callback (m_pFlacDecoder, DecoderSeekCallback);
  m_dll.FLAC__seekable_stream_decoder_set_tell_callback (m_pFlacDecoder, DecoderTellCallback);
  m_dll.FLAC__seekable_stream_decoder_set_length_callback (m_pFlacDecoder, DecoderLengthCallback);
  m_dll.FLAC__seekable_stream_decoder_set_eof_callback (m_pFlacDecoder, DecoderEofCallback);
  m_dll.FLAC__seekable_stream_decoder_set_write_callback (m_pFlacDecoder, DecoderWriteCallback);
  m_dll.FLAC__seekable_stream_decoder_set_metadata_callback (m_pFlacDecoder, DecoderMetadataCallback);
  m_dll.FLAC__seekable_stream_decoder_set_error_callback (m_pFlacDecoder, DecoderErrorCallback);
  m_dll.FLAC__seekable_stream_decoder_set_client_data (m_pFlacDecoder, this);

  if (m_dll.FLAC__seekable_stream_decoder_init(m_pFlacDecoder)!=FLAC__SEEKABLE_STREAM_DECODER_OK)
  {
    CLog::Log(LOGERROR, "FLACCodec: Error initializing decoder");
    FreeDecoder();
    return false;
  }

  //  Process metadata like mum of channels...
  if (!m_dll.FLAC__seekable_stream_decoder_process_until_end_of_metadata(m_pFlacDecoder))
  {
    CLog::Log(LOGERROR, "FLACCodec: Error while processing metadata");
    FreeDecoder();
    return false;
  }

  //  These are filled by the metadata callback
  if (m_SampleRate==0 || m_Channels==0 || m_BitsPerSample==0 || m_TotalTime==0 || m_MaxFrameSize==0)
  {
    CLog::Log(LOGERROR, "FLACCodec: Can't get stream info, SampleRate=%i, Channels=%i, BitsPerSample=%i, TotalTime=%i, MaxFrameSize=%i", m_SampleRate, m_Channels, m_BitsPerSample, m_TotalTime, m_MaxFrameSize);
    FreeDecoder();
    return false;
  }

  //  Extract ReplayGain info
  CFlacTag tag;
  if (tag.Read(strFile))
    m_replayGain=tag.GetReplayGain();

  m_Bitrate = (int)((m_file.GetLength() * 8) / (m_TotalTime/1000));

  if (m_pBuffer)
  {
    delete[] m_pBuffer;
    m_pBuffer=NULL;
  }
  //  allocate the buffer to hold the audio data,
  //  it is 5 times bigger then a single decoded frame
  m_pBuffer=new BYTE[m_MaxFrameSize*5];

  return true;
}

void FLACCodec::DeInit()
{
  FreeDecoder();
  m_file.Close();

  if (m_pBuffer)
  {
    delete[] m_pBuffer;
    m_pBuffer=NULL;
  }
}

__int64 FLACCodec::Seek(__int64 iSeekTime)
{
  //  Seek to the nearest sample
  // set the buffer size to 0 first, as this invokes a WriteCallback which
  // may be called when the buffer is almost full (resulting in a buffer
  // overrun unless we reset m_BufferSize first).
  m_BufferSize=0;
  m_dll.FLAC__seekable_stream_decoder_seek_absolute(m_pFlacDecoder,
		(__int64)(iSeekTime*m_SampleRate)/1000); 	
  return iSeekTime;
}

int FLACCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;

  bool eof=false;
  if (m_dll.FLAC__seekable_stream_decoder_get_state(m_pFlacDecoder)==FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
    eof=true;

  if (!eof)
  {
    //  fill our buffer 4 decoded frame (the buffer could hold 5)
    while(m_BufferSize < m_MaxFrameSize*4 && 
          m_dll.FLAC__seekable_stream_decoder_get_state(m_pFlacDecoder)!=FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
    {
      if (!m_dll.FLAC__seekable_stream_decoder_process_single(m_pFlacDecoder))
      {
        CLog::Log(LOGERROR, "FLACCodec: Error decoding single block");
        return READ_ERROR;
      }
    }
  }

  if (size<m_BufferSize)
  { //  do we need less audio data then in our buffer
    memcpy(pBuffer, m_pBuffer, size);
    memcpy(m_pBuffer, m_pBuffer+size, m_BufferSize-size);
    m_BufferSize-=size;
    *actualsize=size;
  }
  else
  {
    memcpy(pBuffer, m_pBuffer, m_BufferSize);
    *actualsize=m_BufferSize;
    m_BufferSize=0;
  }

  if (eof && m_BufferSize==0)
    return READ_EOF;

  return READ_SUCCESS;
}

bool FLACCodec::CanInit()
{
  return m_dll.CanLoad();
}

void FLACCodec::FreeDecoder()
{
  if (m_pFlacDecoder)
  {
    m_dll.FLAC__seekable_stream_decoder_finish(m_pFlacDecoder);
    m_dll.FLAC__seekable_stream_decoder_delete(m_pFlacDecoder);
    m_pFlacDecoder=NULL;
  }
}

FLAC__SeekableStreamDecoderReadStatus FLACCodec::DecoderReadCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
  FLACCodec* pThis=(FLACCodec*)client_data;
  if (!pThis)
    return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;

  *bytes=pThis->m_file.Read(buffer, *bytes);

  return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
}

FLAC__SeekableStreamDecoderSeekStatus FLACCodec::DecoderSeekCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
  FLACCodec* pThis=(FLACCodec*)client_data;
  if (!pThis)
    return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;

  if (pThis->m_file.Seek(absolute_byte_offset, SEEK_SET)<0)
    return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;


  return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__SeekableStreamDecoderTellStatus FLACCodec::DecoderTellCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
  FLACCodec* pThis=(FLACCodec*)client_data;
  if (!pThis)
    return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;

  *absolute_byte_offset=pThis->m_file.GetPosition();

  return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__SeekableStreamDecoderLengthStatus FLACCodec::DecoderLengthCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
  FLACCodec* pThis=(FLACCodec*)client_data;
  if (!pThis)
    return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;

  *stream_length=pThis->m_file.GetLength();

  return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool FLACCodec::DecoderEofCallback(const FLAC__SeekableStreamDecoder *decoder, void *client_data)
{
  FLACCodec* pThis=(FLACCodec*)client_data;
  if (!pThis)
    return true;

  return (pThis->m_file.GetLength()==pThis->m_file.GetPosition());
}

FLAC__StreamDecoderWriteStatus FLACCodec::DecoderWriteCallback(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
  FLACCodec* pThis=(FLACCodec*)client_data;
  if (!pThis)
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

  const int bytes_per_sample = frame->header.bits_per_sample/8;
  BYTE* outptr = pThis->m_pBuffer+pThis->m_BufferSize;

  for(unsigned int current_sample = 0; current_sample < frame->header.blocksize; current_sample++)
  {
    for(unsigned int channel = 0; channel < frame->header.channels; channel++)
    {
      int sample = buffer[channel][current_sample];
      memcpy(outptr,&sample,bytes_per_sample);
      outptr += bytes_per_sample;
    }
  }

  if (bytes_per_sample == 1)
  {
    for(unsigned int i=0;i<current_sample;i++)
    {
      BYTE* outptr=pThis->m_pBuffer+pThis->m_BufferSize;
      outptr[i]^=0x80;
    }
  }

  pThis->m_BufferSize += current_sample*bytes_per_sample*frame->header.channels;

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACCodec::DecoderMetadataCallback(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
  FLACCodec* pThis=(FLACCodec*)client_data;
  if (!pThis)
    return;

  if (metadata->type==FLAC__METADATA_TYPE_STREAMINFO)
  {
    pThis->m_SampleRate    = metadata->data.stream_info.sample_rate;
    pThis->m_Channels      = metadata->data.stream_info.channels;
    pThis->m_BitsPerSample = metadata->data.stream_info.bits_per_sample;
    pThis->m_TotalTime     = (__int64)metadata->data.stream_info.total_samples * 1000 / metadata->data.stream_info.sample_rate;
    pThis->m_MaxFrameSize  = metadata->data.stream_info.max_blocksize*(pThis->m_BitsPerSample/8)*pThis->m_Channels;
  }
}

void FLACCodec::DecoderErrorCallback(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
  CLog::Log(LOGERROR, "FLACCodec: Read error %i", status);
}
