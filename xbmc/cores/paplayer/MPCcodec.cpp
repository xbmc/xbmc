#include "stdafx.h"
#include "MPCcodec.h"

using namespace XFILE;

// Callbacks for file reading
mpc_int32_t Mpc_Callback_Read(void *data, void * buffer, mpc_int32_t bytes)
{
  CFile *file = (CFile *)data;
  if (!file || !buffer) return 0;
  return (mpc_int32_t)file->Read(buffer, bytes);
}

mpc_bool_t Mpc_Callback_Seek(void *data, mpc_int32_t position)
{
  CFile *file = (CFile *)data;
  if (!file) return 0;

  int seek = (int)file->Seek(position, SEEK_SET);
  if (seek >= 0)
    return 1;
  CLog::Log(LOGERROR, "MPCCodec:Seek callback.  Seeking to position %lu failed.", position);
  return 0;
}

mpc_bool_t Mpc_Callback_CanSeek(void *data)
{
  return 1;
}

mpc_int32_t Mpc_Callback_GetLength(void *data)
{
  CFile *file = (CFile *)data;
  if (!file) return 0;
  return (int)file->GetLength();
}

mpc_int32_t Mpc_Callback_GetPosition(void *data)
{
  CFile *file = (CFile *)data;
  if (!file) return 0;
  int position = (int)file->GetPosition();
	if (position >= 0)
		return position;
	return -1;
}

MPCCodec::MPCCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = "MPC";

  m_sampleBufferSize = 0;
  m_handle = NULL;
}

MPCCodec::~MPCCodec()
{
  DeInit();
}

bool MPCCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false;

  if (!m_file.Open(strFile,true,READ_CACHED))
    return false;

  // setup our callbacks
  m_reader.data = &m_file;
  m_reader.read = Mpc_Callback_Read;
  m_reader.seek = Mpc_Callback_Seek;
  m_reader.canseek = Mpc_Callback_CanSeek;
  m_reader.get_size = Mpc_Callback_GetLength;
  m_reader.tell = Mpc_Callback_GetPosition;

  mpc_streaminfo info;
  double timeinseconds = 0.0;
  if (!m_dll.Open(&m_handle, &m_reader, &info, &timeinseconds))
    return false;

  m_TotalTime = (__int64)(timeinseconds * 1000.0 + 0.5);
  m_BitsPerSample = 16;
	m_Channels = 2;
  m_SampleRate = (int)info.sample_freq;

  m_Bitrate = info.bitrate;
  if (m_Bitrate == 0)
  {
	  m_Bitrate = (int)info.average_bitrate;
  }
  if (m_Bitrate == 0)
  {
	  m_Bitrate = (int)((info.total_file_length * 8) / (m_TotalTime / 1000));
  }

  // Replay gain
  if (info.gain_title || info.peak_title)
  {
		m_replayGain.iTrackGain = info.gain_title;
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
		if (info.peak_title)
    {
      m_replayGain.fTrackPeak = info.peak_title / 32768.0f;
      m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
    }
	}

	if (info.gain_album || info.gain_album)
  {
		m_replayGain.iAlbumGain = info.gain_album;
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
		if (info.gain_album)
    {
      m_replayGain.fAlbumPeak = info.gain_album / 32768.0f;
      m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
    }
	}

  return true;
}

void MPCCodec::DeInit()
{
  if (m_handle)
    m_dll.Close(m_handle);
  m_handle = NULL;

  m_file.Close();
  if (m_reader.data)
    m_reader.data = NULL;
}

__int64 MPCCodec::Seek(__int64 iSeekTime)
{
  if (!m_handle)
    return -1;
  if (!m_dll.Seek(m_handle, (double)iSeekTime/1000.0))
    return -1;
  return iSeekTime;
}

int MPCCodec::ReadSamples(float *pBuffer, int numsamples, int *actualsamples)
{
  if (!m_handle)
    return READ_ERROR;
  *actualsamples = 0;
  // start by emptying out our frame buffer
  int copied = min(m_sampleBufferSize, numsamples);
  memcpy(pBuffer, m_sampleBuffer, copied*sizeof(float));
  numsamples -= copied;
  m_sampleBufferSize -= copied;
  *actualsamples = copied;
  pBuffer += copied;

  // copy down any additional data if we have some
  if (m_sampleBufferSize)
  { // didn't require as much as was in our sample buffer - copy data down and return.
    memmove(m_sampleBuffer, &m_sampleBuffer[copied], m_sampleBufferSize * sizeof(float));
    return READ_SUCCESS;
  }

  // emptied our sample buffer - let's fill it up again
  int ret = m_dll.Read(m_handle, m_sampleBuffer, FRAMELEN * 2);
  if (ret == -2)
    return READ_EOF;
  if (ret == -1)
    return READ_ERROR;

  // have valid float data - copy it across
  copied = min(ret * 2, numsamples);
  ASSERT(ret <= FRAMELEN * 2);

  memcpy(pBuffer, m_sampleBuffer, copied*sizeof(float));
  *actualsamples += copied;
  m_sampleBufferSize = ret * 2 - copied;
  if (m_sampleBufferSize)
  { // copy data down
    memmove(m_sampleBuffer, &m_sampleBuffer[copied], m_sampleBufferSize * sizeof(float));
  }
  return READ_SUCCESS;
}

int MPCCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (!m_handle)
    return READ_ERROR;

  short *pShort = (short *)pBuffer;
  *actualsize = 0;
  // start by emptying out our frame buffer
  int copied = 0;
  for (copied = 0; copied < m_sampleBufferSize && copied < size / 2; copied++)
  {
    float sample = m_sampleBuffer[copied]*32768.0f + 0.5f;
    if (sample > 32767.0f) sample = 32767.0f;
    if (sample < -32768.0f) sample = -32768.0f;
    *pShort++ = (short)sample;
  }
  size -= 2 * copied;
  m_sampleBufferSize -= copied;
  *actualsize = 2 * copied;
  if (m_sampleBufferSize)
  { // didn't require as much as was in our sample buffer - copy data down and return.
    memmove(m_sampleBuffer, &m_sampleBuffer[copied], m_sampleBufferSize * sizeof(float));
    return READ_SUCCESS;
  }
  // emptied our sample buffer - let's fill it up again
  int ret = m_dll.Read(m_handle, m_sampleBuffer, FRAMELEN * 2);
  if (ret == -2)
    return READ_EOF;
  if (ret == -1)
    return READ_ERROR;
  // have valid float data - let's convert back to normal 16bit info
  // (FIXME - should transfer directly for better quality)
  copied = 0;
  ASSERT(ret <= FRAMELEN * 2);
  for (copied = 0; copied < ret * 2 && copied < size / 2; copied++)
  {
    float sample = m_sampleBuffer[copied]*32768.0f + 0.5f;
    if (sample > 32767.0f) sample = 32767.0f;
    if (sample < -32768.0f) sample = -32768.0f;
    *pShort++ = (short)sample;
  }
  *actualsize += copied * 2;
  m_sampleBufferSize = ret * 2 - copied;
  if (m_sampleBufferSize)
  { // copy data down
    memmove(m_sampleBuffer, &m_sampleBuffer[copied], m_sampleBufferSize * sizeof(float));
  }
  return READ_SUCCESS;
}

bool MPCCodec::CanInit()
{
  return m_dll.CanLoad();
}
