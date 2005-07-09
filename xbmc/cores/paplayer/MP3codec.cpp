#include "../../stdafx.h"
#include "MP3Codec.h"

#define MP3_DLL "Q:\\system\\players\\PAPlayer\\in_mp3.dll"

#define DECODER_DELAY 529 // decoder delay in samples

MP3Codec::MP3Codec()
{
  CreateDecoder = NULL;
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = L"MP3";

  // dll stuff
  m_bDllLoaded = false;

  // mp3 related
  m_pDecoder = NULL;
  m_CallAgainWithSameBuffer = false;
  m_AverageInputBytesPerSecond = 20000; // 160k , good place to start i guess
  m_bGuessByterate = false;
  m_lastByteOffset = 0;
  m_InputBufferSize = 64*1024;         // 64k is a reasonable amount, considering that we actual are
                                       // using a background reader thread now that caches in advance.
  m_InputBuffer = new BYTE[m_InputBufferSize];  
  m_InputBufferPos = 0;

  // create our output buffer
  m_OutputBufferSize = 1152*4*4;        // enough for 4 frames
  m_OutputBuffer = new BYTE[m_OutputBufferSize];
  m_OutputBufferPos = 0;
  m_Decoding = false;
  m_IgnoreFirst = true; // we want to be gapless
  m_IgnoredBytes = 0;
  m_IgnoreLast = true;
  m_eof = false;
}

MP3Codec::~MP3Codec()
{
  DeInit();

  if (m_pDecoder )
    delete m_pDecoder;
  m_pDecoder = NULL;

  if ( m_InputBuffer )
    delete[] m_InputBuffer;
  m_InputBuffer = NULL;

  if ( m_OutputBuffer )
    delete[] m_OutputBuffer;
  m_OutputBuffer = NULL;

  if (m_bDllLoaded)
    CSectionLoader::UnloadDLL(MP3_DLL);
}

bool MP3Codec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_file.Initialize(filecache);

  if (!LoadDLL())
    return false;

  // TODO:  add file extension checking and HTTP/Icecast/Shoutcast reading
  if (m_pDecoder)
  {
    delete m_pDecoder;
    m_pDecoder = NULL;
  }
  m_pDecoder = CreateDecoder(' 3PM',NULL);

  if ( m_pDecoder )
    CLog::Log(LOGINFO, "MP3Codec: Loaded decoder at %p", m_pDecoder);
  else
    return false;

  // set defaults...
  m_InputBufferPos = 0;
  m_OutputBufferPos = 0;
  m_IgnoreFirst = true; // we want to be gapless
  m_IgnoredBytes = 0;
  m_IgnoreLast = true;
  m_lastByteOffset = 0;
  m_eof = false;
  m_CallAgainWithSameBuffer = false;

  // Guess Bitrate and obtain replayGain information etc.
  CMusicInfoTagLoaderMP3 mp3info;
  CMusicInfoTag tag;
  mp3info.Load(strFile, tag);
  mp3info.GetSeekInfo(m_seekInfo);
  mp3info.GetReplayGain(m_replayGain);

  if (!m_file.Open(strFile))
  {
    CLog::Log(LOGERROR, "MP3Codec: Unable to open file %s", strFile.c_str());
    delete m_pDecoder;
    m_pDecoder = NULL;
    return false;
  }
  
  __int64 length = m_file.GetLength();
  m_TotalTime = (__int64)(m_seekInfo.GetDuration() * 1000.0f);
  if ( m_TotalTime )
  {
    m_AverageInputBytesPerSecond = (DWORD)(length / m_seekInfo.GetDuration());
    m_Bitrate = m_AverageInputBytesPerSecond * 8;  // average bitrate
  }
  else
    m_bGuessByterate = true;  // If we don't have the TrackDuration we'll have to guess later.

  // Read in some data so we can determine the sample size and so on
  // This needs to be made more intelligent - possibly use a temp output buffer
  // and cycle around continually reading until we have the necessary data
  // as a first workaround skip the id3v2 tag at the beginning of the file
  m_file.Seek(mp3info.GetID3v2Size());
  m_file.Read(m_InputBuffer, 8192);
  int sendsize = 8192;
  unsigned int formatdata[8];
  int result = m_pDecoder->decode(m_InputBuffer, 8192, m_InputBuffer + 8192, &sendsize, (unsigned int *)&formatdata);
  if ( (result == 0 || result == 1) && sendsize )
  {
    m_Channels = formatdata[2];
    m_SampleRate = formatdata[1];
    m_BitsPerSample = formatdata[3];
  }
  else
  {
    CLog::Log(LOGERROR, "MP3Codec: Unable to determine file format of %s (corrupt start of mp3?)", strFile.c_str());
    m_file.Close();
    delete m_pDecoder;
    m_pDecoder = NULL;
    return false;
  }
  m_pDecoder->flush();
  m_file.Seek(mp3info.GetID3v2Size());
  return true;
}

void MP3Codec::DeInit()
{
  m_file.Close();
}

__int64 MP3Codec::Seek(__int64 iSeekTime)
{
  // calculate our offset to seek to in the file
  m_lastByteOffset = m_seekInfo.GetByteOffset(0.001f * iSeekTime);
  m_file.Seek(m_lastByteOffset, SEEK_SET);
  // Flush the decoder
  m_pDecoder->flush();
  m_InputBufferPos = 0;
  m_OutputBufferPos = 0;
  m_CallAgainWithSameBuffer = false;
  return iSeekTime;
}

int MP3Codec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize = 0;
  // First read in any extra info we need from our MP3
  int inputBufferToRead = m_InputBufferSize - m_InputBufferPos;
  if ( inputBufferToRead && !m_CallAgainWithSameBuffer && !m_eof ) 
  {
    int fileLeft=(int)(m_file.GetLength() - m_file.GetPosition());
    if (inputBufferToRead >  fileLeft ) inputBufferToRead = fileLeft;

    DWORD dwBytesRead = m_file.Read(m_InputBuffer + m_InputBufferPos , inputBufferToRead);
    if (!dwBytesRead)
    {
      CLog::Log(LOGERROR, "MP3Codec: Error reading file");
      return READ_ERROR;
    }
    // add the size of read PAP data to the buffer size
    m_InputBufferPos += dwBytesRead;
    if ( m_file.GetLength() == m_file.GetPosition() )
      m_eof = true;
  }
  // Decode data if we have some to decode
  if ( m_InputBufferPos || m_CallAgainWithSameBuffer || m_eof )
  {
    int result;
      
    m_Decoding = true;

    if ( size )
    {
      m_CallAgainWithSameBuffer = false;
      unsigned int formatdata[8];
      int outputsize = m_OutputBufferSize - m_OutputBufferPos;
      // Now decode data into the vacant frame buffer
      result = m_pDecoder->decode( m_InputBuffer, m_InputBufferPos, m_OutputBuffer + m_OutputBufferPos, &outputsize, (unsigned int *)&formatdata);
      if ( result == 1 || result == 0) 
      {
        // let's check if we need to ignore the decoded data.
        if ( m_IgnoreFirst && outputsize && m_seekInfo.GetFirstSample() )
        {
          // starting up - lets ignore the first (typically 576) samples
          int iDelay = DECODER_DELAY + m_seekInfo.GetFirstSample();  // decoder delay + encoder delay
          iDelay *= m_Channels * m_BitsPerSample / 8;            // sample size
          if (outputsize + m_IgnoredBytes >= iDelay)
          {
            // have enough data to ignore - let's move the valid data to the start
            int iAmountToMove = outputsize + m_IgnoredBytes - iDelay;
            memmove(m_OutputBuffer, m_OutputBuffer + outputsize - iAmountToMove, iAmountToMove);
            outputsize = iAmountToMove;
            m_IgnoreFirst = false;
            m_IgnoredBytes = 0;
          }
          else
          { // not enough data yet - ignore all of this
            m_IgnoredBytes += outputsize;
            outputsize = 0;
          }
        }
        // Do we need to call back with the same set of data?
        if ( result )
          m_CallAgainWithSameBuffer = true;
        else 
        { // Read more from the file
          m_InputBufferPos = 0;
          // Check for the end of file (as we need to remove data from the end of the track)
          if (m_eof)
          {
            m_Decoding = false;
            // EOF reached - let's see remove any unused samples from our frame buffers
            if (m_IgnoreLast && m_seekInfo.GetLastSample())
            {
              unsigned int samplestoremove = (m_seekInfo.GetLastSample() - DECODER_DELAY);
              samplestoremove *= m_Channels * m_BitsPerSample / 8;
              if (samplestoremove > m_OutputBufferPos)
                samplestoremove = m_OutputBufferPos;
              m_OutputBufferPos -= samplestoremove;
              m_IgnoreLast = false;
            }
          }
        }
        m_OutputBufferPos += outputsize;
        ASSERT(m_OutputBufferPos <= m_OutputBufferSize);
      }
      else if (result == -1)
      {
        return READ_ERROR;
      }
    }
  }
  // check whether we can move data out of our output buffer
  // we leave some data in our output buffer to allow us to remove samples
  // at the end of the track for gapless playback
  int amounttomove = 0;
  if (m_OutputBufferPos > 1152 * 4 * 2)
    amounttomove = m_OutputBufferPos - 1152 * 4 * 2;
  if (m_eof && !m_Decoding)
    amounttomove = m_OutputBufferPos;
  if (amounttomove > size) amounttomove = size;
  if (amounttomove)
  {
    memcpy(pBuffer, m_OutputBuffer, amounttomove);
    m_OutputBufferPos -= amounttomove;
    memmove(m_OutputBuffer, m_OutputBuffer + amounttomove, m_OutputBufferPos);
    *actualsize = amounttomove;
  }
  if (m_eof && !m_Decoding)
    return READ_EOF;
  return READ_SUCCESS;
}

bool MP3Codec::LoadDLL()
{
  if (m_bDllLoaded)
    return true;

  DllLoader* pDll = CSectionLoader::LoadDLL(MP3_DLL);
  if (!pDll)
  {
    CLog::Log(LOGERROR, "MP3Codec: Unable to load dll %s", MP3_DLL);
    return false;
  }

  // get handle to the functions in the dll
  pDll->ResolveExport("CreateAudioDecoder", (void**)&CreateDecoder);

  // Check resolves + version number
  if ( !CreateDecoder )
  {
    CLog::Log(LOGERROR, "MP3Codec: Unable to resolve exports from %s", MP3_DLL);
    CSectionLoader::UnloadDLL(MP3_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}

bool MP3Codec::CanInit()
{
  return CFile::Exists(MP3_DLL);
}
