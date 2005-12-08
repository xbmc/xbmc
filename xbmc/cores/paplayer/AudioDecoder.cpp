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
  unsigned int filecache = g_guiSettings.GetInt("CacheAudio.LAN");
  if ( file.IsHD() )
    filecache = g_guiSettings.GetInt("Cache.HardDisk");
  else if ( file.IsOnDVD() )
    filecache = g_guiSettings.GetInt("CacheAudio.DVDRom");
  else if ( file.IsInternetStream() )
    filecache = g_guiSettings.GetInt("CacheAudio.Internet");

  // create our codec
  m_codec=CodecFactory::CreateCodecDemux(file.m_strPath, filecache * 1024);

  if (!m_codec || !m_codec->Init(file.m_strPath, filecache * 1024))
  {
    CLog::Log(LOGERROR, "CAudioDecoder: Unable to Init Codec!");
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
  return m_pcmBuffer.GetMaxReadSize();
}

void *CAudioDecoder::GetData(unsigned int size)
{
  if (size > PACKET_SIZE)
  {
    CLog::Log(LOGWARNING, "CAudioDecoder::GetData() more bytes (%i) requested than we have to give (%i)!", size, PACKET_SIZE);
    ASSERT(true);
    size = PACKET_SIZE;
  }
  if (m_outputBufferSize > size)
  {
    m_outputBufferSize -= size;
    return m_outputBuffer;
  }

  if (m_pcmBuffer.ReadBinary( (char *)m_outputBuffer + m_outputBufferSize, size - m_outputBufferSize))
  {
    m_outputBufferSize = 0;
    // check for end of file + end of buffer
    if ( m_status == STATUS_ENDING && m_pcmBuffer.GetMaxReadSize() < PACKET_SIZE)
    {
      CLog::Log(LOGINFO, "CAudioDecoder::GetData() ending track - only have %i bytes left", m_pcmBuffer.GetMaxReadSize());
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

#define MAX_SHORT_VALUE 32767
#define MIN_SHORT_VALUE -32768

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
