#include "stdafx.h"
#include "WAVPackcodec.h"


WAVPackCodec::WAVPackCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_Bitrate = 0;
  m_CodecName = "WAVPack";

  m_Handle=NULL;

  m_BufferSize=0;
  m_BufferPos = 0;
  m_Buffer=NULL;
  m_ReadBuffer=NULL;
}

WAVPackCodec::~WAVPackCodec()
{
  DeInit();
}

bool WAVPackCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_file.Initialize(filecache);

  if (!m_dll.Load())
    return false;
  
  //  Open the file to play
  if (!m_file.Open(strFile))
  {
    CLog::Log(LOGERROR, "WAVPackCodec: Can't open %s", strFile.c_str());
    return false;
  }

  //  setup i/o callbacks
  m_Callbacks.read_bytes=ReadCallback;
  m_Callbacks.get_pos=GetPosCallback;
  m_Callbacks.set_pos_abs=SetPosAbsCallback;
  m_Callbacks.set_pos_rel=SetPosRelCallback;
  m_Callbacks.push_back_byte=PushBackByteCallback;
  m_Callbacks.get_length=GetLenghtCallback;
  m_Callbacks.can_seek=CanSeekCallback;

  //  open file with decoder
  //  NOTE: We restrict to 2 channel here, as otherwise xbox doesn't have the CPU
  m_Handle=m_dll.WavpackOpenFileInputEx(&m_Callbacks, this, NULL, m_errormsg, OPEN_TAGS | OPEN_2CH_MAX | OPEN_NORMALIZE, 23);
  if (!m_Handle)
  {
    CLog::Log(LOGERROR, "WAVPackCodec: Can't open decoder for %s", strFile.c_str());
    return false;
  }

  m_SampleRate = m_dll.WavpackGetSampleRate(m_Handle);
  m_Channels = m_dll.WavpackGetReducedChannels(m_Handle);
  m_BitsPerSample = m_dll.WavpackGetBitsPerSample(m_Handle);
  m_TotalTime = (__int64)(m_dll.WavpackGetNumSamples(m_Handle) * 1000.0 / m_SampleRate); 
  m_Bitrate = (int)m_dll.WavpackGetAverageBitrate(m_Handle, TRUE);

  if (m_SampleRate==0 || m_Channels==0 || m_BitsPerSample==0 || m_TotalTime==0)
  {
    CLog::Log(LOGERROR, "WAVPackCodec: incomplete stream info from %s, SampleRate=%i, Channels=%i, BitsPerSample=%i, TotalTime=%llu", strFile.c_str(), m_SampleRate, m_Channels, m_BitsPerSample, m_TotalTime);
    return false;
  }

  m_Buffer = new BYTE[576*m_Channels*(m_BitsPerSample/8)*2];

  m_ReadBuffer = new BYTE[4*576*m_Channels];

  //  Get replay gain info
	char value [32];
  if (m_dll.WavpackGetTagItem (m_Handle, "replaygain_track_gain", value, sizeof (value)))
  {
    m_replayGain.iTrackGain = (int)(atof(value) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  if (m_dll.WavpackGetTagItem (m_Handle, "replaygain_album_gain", value, sizeof (value)))
  {
    m_replayGain.iAlbumGain = (int)(atof(value) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  if (m_dll.WavpackGetTagItem (m_Handle, "replaygain_track_peak", value, sizeof (value)))
  {
    m_replayGain.fTrackPeak = (float)atof(value);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  if (m_dll.WavpackGetTagItem (m_Handle, "replaygain_album_peak", value, sizeof (value)))
  {
    m_replayGain.fAlbumPeak = (float)atof(value);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }

  return true;
}

void WAVPackCodec::DeInit()
{
  if (m_Handle)
    m_dll.WavpackCloseFile(m_Handle);
  m_Handle = NULL;

  if (m_Buffer)
    delete[] m_Buffer;
  m_Buffer=NULL;

  if (m_ReadBuffer)
    delete[] m_ReadBuffer;
  m_ReadBuffer=NULL;

  m_file.Close();
}

__int64 WAVPackCodec::Seek(__int64 iSeekTime)
{
  m_BufferSize=m_BufferPos=0;
  int SeekSample=(int)(m_SampleRate / 1000.0 * iSeekTime);
  if (m_dll.WavpackSeekSample(m_Handle, SeekSample))
  {
    return iSeekTime;
  }
  return -1;
}

int WAVPackCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;

  bool bEof=false;

  //  Do we have to refill our audio buffer?
  if (m_BufferSize-m_BufferPos<size)
  {
    //  Move the remaining audio data to the beginning of the buffer
    memmove(m_Buffer, m_Buffer + m_BufferPos, m_BufferSize-m_BufferPos);
    m_BufferSize=m_BufferSize-m_BufferPos;
    m_BufferPos = 0;

    //  Fill our buffer with a chunk of audio data
	  int tsamples = m_dll.WavpackUnpackSamples(m_Handle, (int32_t*)m_ReadBuffer, 576) * m_Channels;
	  int tbytes = tsamples * (m_BitsPerSample/8);

	  if (!tsamples)
      bEof=true;
	  else
    {
      FormatSamples(m_Buffer + m_BufferSize, m_BitsPerSample/8, (long *) m_ReadBuffer, tsamples);
      m_BufferSize+=tbytes;
    }
  }

  //  Our buffer is empty and no data left to read
  if (m_BufferSize-m_BufferPos==0 && bEof)
    return READ_EOF;

  //  Try to give the player the amount of audio data he wants
  if (m_BufferSize-m_BufferPos>=size)
  { //  we have enough data in our buffer
    memcpy(pBuffer, m_Buffer + m_BufferPos, size);
    m_BufferPos+=size;
    *actualsize=size;
  }
  else
  { //  Only a smaller amount of data left as the player wants
    memcpy(pBuffer, m_Buffer + m_BufferPos, m_BufferSize-m_BufferPos);
    *actualsize=m_BufferSize-m_BufferPos;
    m_BufferPos+=m_BufferSize-m_BufferPos;
  }

  return READ_SUCCESS;
}

bool WAVPackCodec::CanInit()
{
  return m_dll.CanLoad();
}

void WAVPackCodec::FormatSamples (BYTE *dst, int bps, long *src, unsigned long samcnt)
{
  long temp;

  switch (bps)
  {
  case 1:
    while (samcnt--)
    {
      temp = *src++;
      dst [0] = 0;
      dst [1] = (BYTE)(temp & 0xFF);
      dst += 2;
    }

    break;

  case 2:
    while (samcnt--)
    {
      temp = *src++;
      dst [0] = (BYTE)(temp & 0xFF);
      dst [1] = (BYTE)(temp >> 8);
      dst += 2;
    }

    break;

  case 3:
    while (samcnt--)
    {
      temp = *src++;
      dst [0] = (BYTE)(temp & 0xFF);
      dst [1] = (BYTE)((temp >> 8) & 0xFF);
      dst [2] = (BYTE)((temp >> 16) & 0xFF);
      dst += 3;
    }

    break;

  case 4:
    while (samcnt--)
    {
      temp = *src++;
      dst [0] = (BYTE)temp;
      dst [1] = (BYTE)(temp >> 8);
      dst [2] = (BYTE)(temp >> 16);
      dst [3] = (BYTE)(temp >> 24);
      dst += 4;
    }

    break;
  }
}

int WAVPackCodec::ReadCallback(void *id, void *data, int bcount)
{
  WAVPackCodec* pCodec=(WAVPackCodec*)id;
  if (!pCodec)
    return 0;

  return pCodec->m_file.Read(data, bcount);
}

int WAVPackCodec::SetPosAbsCallback(void *id, unsigned int pos)
{
  WAVPackCodec* pCodec=(WAVPackCodec*)id;
  if (!pCodec)
    return 0;

  if (pCodec->m_file.Seek(pos, SEEK_SET)>=0)
    return 0;

  return -1;
}

int WAVPackCodec::SetPosRelCallback(void *id, int delta, int mode)
{
  WAVPackCodec* pCodec=(WAVPackCodec*)id;
  if (!pCodec)
    return 0;

  if (pCodec->m_file.Seek(delta, mode)>=0)
    return 0;

  return -1;
}

unsigned int WAVPackCodec::GetPosCallback(void *id)
{
  WAVPackCodec* pCodec=(WAVPackCodec*)id;
  if (!pCodec)
    return 0;

  return (unsigned int)pCodec->m_file.GetPosition();
}

int WAVPackCodec::CanSeekCallback(void *id)
{
  WAVPackCodec* pCodec=(WAVPackCodec*)id;
  if (!pCodec)
    return 0;

  return 1;
}

unsigned int WAVPackCodec::GetLenghtCallback(void *id)
{
  WAVPackCodec* pCodec=(WAVPackCodec*)id;
  if (!pCodec)
    return 0;

  return (unsigned int)pCodec->m_file.GetLength();
}

int WAVPackCodec::PushBackByteCallback(void *id, int c)
{
  WAVPackCodec* pCodec=(WAVPackCodec*)id;
  if (!pCodec)
    return 0;

  if (pCodec->m_file.Seek(pCodec->m_file.GetPosition()-1, SEEK_SET))
    return c;

  return EOF;
}
