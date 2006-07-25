#include "../../stdafx.h"
#include "AudioDecoder.h"
#include "../../util.h"
#include "CodecFactory.h"


#define INTERNAL_BUFFER_LENGTH  4*44100       // 1 second

CAudioDecoder::CAudioDecoder()
{
  m_codec = NULL;

  m_eof = false;

  m_status = STATUS_NO_FILE;
  m_canPlay = false;

  m_outputBufferSize = 0;
  m_blockSize = 4;
}

CAudioDecoder::~CAudioDecoder()
{
  Destroy();
}

void CAudioDecoder::Destroy()
{
  CSingleLock lock(m_critSection);
  m_status = STATUS_NO_FILE;

  m_pcmBuffer.Destroy();
  m_outputBufferSize = 0;

  if ( m_codec )
    delete m_codec;
  m_codec = NULL;

  m_canPlay = false;
}

bool CAudioDecoder::Create(const CFileItem &file, __int64 seekOffset, unsigned int nBufferSize)
{
  Destroy();

  CSingleLock lock(m_critSection);
  // create our pcm buffer
  m_pcmBuffer.Create( max(1, nBufferSize) * INTERNAL_BUFFER_LENGTH, 0 );

  // reset our playback timing variables
  m_eof = false;

  // get correct cache size
  unsigned int filecache = g_guiSettings.GetInt("cacheaudio.lan");
  if ( file.IsHD() )
    filecache = g_guiSettings.GetInt("cache.harddisk");
  else if ( file.IsOnDVD() )
    filecache = g_guiSettings.GetInt("cacheaudio.dvdrom");
  else if ( file.IsInternetStream() )
    filecache = g_guiSettings.GetInt("cacheaudio.internet");

  // create our codec
  m_codec=CodecFactory::CreateCodecDemux(file.m_strPath, file.GetContentType(), filecache * 1024);

  if (!m_codec || !m_codec->Init(file.m_strPath, filecache * 1024))
  {
    CLog::Log(LOGERROR, "CAudioDecoder: Unable to Init Codec while loading file %s", file.m_strPath.c_str());
    Destroy();
    return false;
  }
  m_blockSize = m_codec->m_Channels * m_codec->m_BitsPerSample / 8;

  if (seekOffset)
    m_codec->Seek(seekOffset);

  m_status = STATUS_QUEUING;

  return true;
}

void CAudioDecoder::GetDataFormat(unsigned int *channels, unsigned int *samplerate, unsigned int *bitspersample)
{
  if (!m_codec)
    return;

  if (channels) *channels = m_codec->m_Channels;
  if (samplerate) *samplerate = m_codec->m_SampleRate;
  if (bitspersample) *bitspersample = m_codec->m_BitsPerSample;
}

__int64 CAudioDecoder::Seek(__int64 time)
{
  m_pcmBuffer.Clear();
  if (!m_codec)
    return 0;
  if (time < 0) time = 0;
  if (time > m_codec->m_TotalTime) time = m_codec->m_TotalTime;
  return m_codec->Seek(time);
}

__int64 CAudioDecoder::TotalTime()
{
  if (m_codec)
    return m_codec->m_TotalTime;
  return 0;
}

unsigned int CAudioDecoder::GetDataSize()
{
  if (m_status == STATUS_QUEUING || m_status == STATUS_NO_FILE)
    return 0;
  // check for end of file and end of buffer
  if (m_status == STATUS_ENDING && m_pcmBuffer.GetMaxReadSize() < PACKET_SIZE)
    m_status = STATUS_ENDED;
#ifdef USE_FLOAT_BUFFERS
  return m_pcmBuffer.GetMaxReadSize() / sizeof(float);
#else
  return m_pcmBuffer.GetMaxReadSize();
#endif
}

void *CAudioDecoder::GetData(unsigned int size)
{
  if (size > OUTPUT_SAMPLES)
  {
    CLog::Log(LOGWARNING, "CAudioDecoder::GetData() more bytes/samples (%i) requested than we have to give (%i)!", size, OUTPUT_SAMPLES);
    assert(false);
    size = OUTPUT_SAMPLES;
  }
  if (m_outputBufferSize > size)
  {
    m_outputBufferSize -= size;
    return m_outputBuffer;
  }

#ifdef USE_FLOAT_BUFFERS
  if (m_pcmBuffer.ReadBinary( (char *)(m_outputBuffer + m_outputBufferSize), (size - m_outputBufferSize) * sizeof(float)))
#else
  if (m_pcmBuffer.ReadBinary( (char *)m_outputBuffer + m_outputBufferSize, size - m_outputBufferSize))
#endif
  {
    m_outputBufferSize = 0;
    // check for end of file + end of buffer
#ifdef USE_FLOAT_BUFFERS
    if ( m_status == STATUS_ENDING && m_pcmBuffer.GetMaxReadSize() < OUTPUT_SAMPLES * sizeof(float))
#else
    if ( m_status == STATUS_ENDING && m_pcmBuffer.GetMaxReadSize() < OUTPUT_SAMPLES)
#endif
    {
#ifdef USE_FLOAT_BUFFERS
      CLog::Log(LOGINFO, "CAudioDecoder::GetData() ending track - only have %i bytes left", m_pcmBuffer.GetMaxReadSize() * sizeof(float));
#else
      CLog::Log(LOGINFO, "CAudioDecoder::GetData() ending track - only have %i bytes left", m_pcmBuffer.GetMaxReadSize());
#endif
      m_status = STATUS_ENDED;
    }
    return m_outputBuffer;
  }
  CLog::Log(LOGERROR, "CAudioDecoder::GetData() ReadBinary failed with size %i", size - m_outputBufferSize);
  return NULL;
}

void CAudioDecoder::PrefixData(void *data, unsigned int size)
{
  if (!data)
  {
    CLog::Log(LOGERROR, "CAudioDecoder::PrefixData() failed - null data pointer");
    return;
  }
  m_outputBufferSize = min(PACKET_SIZE, size);
  fast_memcpy(m_outputBuffer, data, m_outputBufferSize);
  if (m_outputBufferSize != size)
    CLog::Log(LOGWARNING, "CAudioDecoder::PrefixData - losing %i bytes of audio data in track transistion", size - m_outputBufferSize);
}

#ifdef USE_FLOAT_BUFFERS
int CAudioDecoder::ReadSamples(int numsamples)
{
  if (m_status == STATUS_NO_FILE || m_status == STATUS_ENDING || m_status == STATUS_ENDING)
    return RET_SLEEP;             // nothing loaded yet

  // start playing once we're fully queued and we're ready to go
  if (m_status == STATUS_QUEUED && m_canPlay)
    m_status = STATUS_PLAYING;

  // grab a lock to ensure the codec is created at this point.
  CSingleLock lock(m_critSection);

  // Read in more data
  int maxsize = min(INPUT_SAMPLES, m_pcmBuffer.GetMaxWriteSize() / sizeof(float));
  numsamples = min(numsamples, maxsize);
  numsamples -= (numsamples % m_codec->m_Channels);  // make sure it's divisible by our number of channels
  if ( numsamples )
  {
    int actualsamples = 0;
    // if our codec sends floating point, then read it
    int result = READ_ERROR;
    if (m_codec->HasFloatData())
      result = m_codec->ReadSamples(m_inputBuffer, numsamples, &actualsamples);
    else
      result = ReadPCMSamples(m_inputBuffer, numsamples, &actualsamples);

    if ( result != READ_ERROR && actualsamples ) 
    {
      // do any post processing of the audio (eg replaygain etc.)
      ProcessAudio(m_inputBuffer, actualsamples);

      // move it into our buffer
      m_pcmBuffer.WriteBinary((char *)m_inputBuffer, actualsamples * sizeof(float));

      // update status
      if (m_status == STATUS_QUEUING && m_pcmBuffer.GetMaxReadSize() > m_pcmBuffer.Size() * 0.9)
      {
        CLog::Log(LOGINFO, "AudioDecoder: File is queued");
        m_status = STATUS_QUEUED;
      }

      if (result == READ_EOF) // EOF reached
      {
        // setup ending if we're within set time of the end (currently just EOF)
        m_eof = true;
        if (m_status < STATUS_ENDING)
          m_status = STATUS_ENDING;
      }

      return RET_SUCCESS;
    }
    if (result == READ_ERROR)
    {
      // error decoding, lets finish up and get out
      CLog::Log(LOGERROR, "CAudioDecoder: Error while decoding %i", result);
      return RET_ERROR;
    }
    if (result == READ_EOF)
    {
      m_eof = true;
      // setup ending if we're within set time of the end (currently just EOF)
      if (m_status < STATUS_ENDING)
        m_status = STATUS_ENDING;
    }
  }
  return RET_SLEEP; // nothing to do
}
#else
int CAudioDecoder::ReadData(int sendsize)
{
  if (m_status == STATUS_NO_FILE || m_status == STATUS_ENDING || m_status == STATUS_ENDING)
    return RET_SLEEP;             // nothing loaded yet

  // start playing once we're fully queued and we're ready to go
  if (m_status == STATUS_QUEUED && m_canPlay)
    m_status = STATUS_PLAYING;

  // grab a lock to ensure the codec is created at this point.
  CSingleLock lock(m_critSection);

  // Read in more data
  int maxsize = min(INPUT_SIZE, m_pcmBuffer.GetMaxWriteSize());
  sendsize = min(sendsize, maxsize);
  sendsize -= (sendsize % m_blockSize);  // make sure it's divisible by our block size!
  if ( sendsize )
  {
    int actualdatasent = 0;
    int result = m_codec->ReadPCM(m_inputBuffer, sendsize, &actualdatasent);
    if ( result != READ_ERROR && actualdatasent ) 
    {
      ProcessAudio(m_inputBuffer, actualdatasent);
      // move it into our buffer
      m_pcmBuffer.WriteBinary((char *)m_inputBuffer, actualdatasent);

      // update status
      if (m_status == STATUS_QUEUING && m_pcmBuffer.GetMaxReadSize() > m_pcmBuffer.Size() * 0.9)
      {
        CLog::Log(LOGINFO, "AudioDecoder: File is queued");
        m_status = STATUS_QUEUED;
      }

      if (result == READ_EOF) // EOF reached
      {
        // setup ending if we're within set time of the end (currently just EOF)
        m_eof = true;
        if (m_status < STATUS_ENDING)
          m_status = STATUS_ENDING;
      }

      return RET_SUCCESS;
    }
    if (result == READ_ERROR)
    {
      // error decoding, lets finish up and get out
      CLog::Log(LOGERROR, "CAudioDecoder: Error while decoding %i", result);
      return RET_ERROR;
    }
    if (result == READ_EOF)
    {
      m_eof = true;
      // setup ending if we're within set time of the end (currently just EOF)
      if (m_status < STATUS_ENDING)
        m_status = STATUS_ENDING;
    }
  }
  return RET_SLEEP; // nothing to do
}
#endif

#define MAX_SHORT_VALUE 32767
#define MIN_SHORT_VALUE -32768

#ifdef USE_FLOAT_BUFFERS
void CAudioDecoder::ProcessAudio(float *data, int numsamples)
{
  if (g_guiSettings.m_replayGain.iType != REPLAY_GAIN_NONE)
  {
    float gainFactor = GetReplayGain();
    for (int i = 0; i < numsamples; i++)
    {
      data[i] *= gainFactor;
      // check the range (is this needed here?)
      if (data[i] > 1.0f) data[i] = 1.0f;
      if (data[i] < -1.0f) data[i] = -1.0f;
    }
  }
}
#else
void CAudioDecoder::ProcessAudio(void *data, int size)
{
  if (size & 1)
  {
    CLog::Log(LOGERROR, "ProcessAudio called with non-even size (%i)", size);
    return;
  }
  float gainFactor = 1.0f;
  // convert data to floats
  if (g_guiSettings.m_replayGain.iType != REPLAY_GAIN_NONE)
    gainFactor = GetReplayGain();
  // multiply this by the fadelevel, and by the volume level of the stream
  short *shortData = (short *)data;
  for (int i = size; i; i-=2)
  {
    float result = *shortData * gainFactor;
    if (result > MAX_SHORT_VALUE) result = MAX_SHORT_VALUE;
    if (result < MIN_SHORT_VALUE) result = MIN_SHORT_VALUE;
    *shortData++ = (short)result;
  }
}
#endif

float CAudioDecoder::GetReplayGain()
{
#define REPLAY_GAIN_DEFAULT_LEVEL 89.0f
  // Compute amount of gain
  float replaydB = (float)g_guiSettings.m_replayGain.iNoGainPreAmp;
  float peak = 0.0f;
  if (g_guiSettings.m_replayGain.iType == REPLAY_GAIN_ALBUM)
  {
    if (m_codec->m_replayGain.iHasGainInfo & REPLAY_GAIN_HAS_ALBUM_INFO)
    {
      replaydB = (float)g_guiSettings.m_replayGain.iPreAmp + (float)m_codec->m_replayGain.iAlbumGain / 100.0f;
      peak = m_codec->m_replayGain.fAlbumPeak;
    }
    else if (m_codec->m_replayGain.iHasGainInfo & REPLAY_GAIN_HAS_TRACK_INFO)
    {
      replaydB = (float)g_guiSettings.m_replayGain.iPreAmp + (float)m_codec->m_replayGain.iTrackGain / 100.0f;
      peak = m_codec->m_replayGain.fTrackPeak;
    }
  }
  else if (g_guiSettings.m_replayGain.iType == REPLAY_GAIN_TRACK)
  {
    if (m_codec->m_replayGain.iHasGainInfo & REPLAY_GAIN_HAS_TRACK_INFO)
    {
      replaydB = (float)g_guiSettings.m_replayGain.iPreAmp + (float)m_codec->m_replayGain.iTrackGain / 100.0f;
      peak = m_codec->m_replayGain.fTrackPeak;
    }
    else if (m_codec->m_replayGain.iHasGainInfo & REPLAY_GAIN_HAS_ALBUM_INFO)
    {
      replaydB = (float)g_guiSettings.m_replayGain.iPreAmp + (float)m_codec->m_replayGain.iAlbumGain / 100.0f;
      peak = m_codec->m_replayGain.fAlbumPeak;
    }
  }
  // convert to a gain type
  float replaygain = pow(10.0f, (replaydB - REPLAY_GAIN_DEFAULT_LEVEL)* 0.05f);
  // check peaks
  if (g_guiSettings.m_replayGain.bAvoidClipping)
  {
    if (fabs(peak * replaygain) > 1.0f)
      replaygain = 1.0f / fabs(peak);
  }
  return replaygain;
}

int CAudioDecoder::ReadPCMSamples(float *buffer, int numsamples, int *actualsamples)
{
  // convert samples to bytes
  numsamples *= (m_codec->m_BitsPerSample / 8);

  // read in our PCM data
  int result = m_codec->ReadPCM(m_pcmInputBuffer, numsamples, actualsamples);

  // convert to floats (-1 ... 1) range
  int i;
  switch (m_codec->m_BitsPerSample)
  {
  case 8:
    for (i = 0; i < *actualsamples; i++)
      m_inputBuffer[i] = 1.0f / 0x7f * m_pcmInputBuffer[i];
    break;
  case 16:
    *actualsamples /= 2;
    for (i = 0; i < *actualsamples; i++)
      m_inputBuffer[i] = 1.0f / 0x7fff * ((short *)m_pcmInputBuffer)[i];
    break;
  case 24:
    *actualsamples /= 3;
    for (i = 0; i < *actualsamples; i++)
      m_inputBuffer[i] = 1.0f / 0x7fffff * (((int)m_pcmInputBuffer[3*i] << 0) | ((int)m_pcmInputBuffer[3*i+1] << 8) | (((int)((char *)m_pcmInputBuffer)[3*i+2]) << 16));
    break;
  }
  return result;
}

