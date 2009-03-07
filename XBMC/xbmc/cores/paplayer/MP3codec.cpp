/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "MP3codec.h"
#include "FileItem.h"

using namespace MUSIC_INFO;

#define DECODER_DELAY 529 // decoder delay in samples

#define XMIN(a,b) (a)<(b)?(a):(b)
#define DEFAULT_CHUNK_SIZE 16384

#define DECODING_ERROR    -1
#define DECODING_SUCCESS   0
#define DECODING_CALLAGAIN 1

MP3Codec::MP3Codec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_BitsPerSampleInternal = 0;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = "MP3";

  // mp3 related
  m_pDecoder = NULL;
  m_CallAgainWithSameBuffer = false;
  m_lastByteOffset = 0;
  m_InputBufferSize = 64*1024;         // 64k is a reasonable amount, considering that we actual are
                                       // using a background reader thread now that caches in advance.
  m_InputBuffer = new BYTE[m_InputBufferSize];
  m_InputBufferPos = 0;

  memset(&m_Formatdata,0,sizeof(m_Formatdata));

  // create our output buffer
  m_OutputBufferSize = 1152*4*8;        // enough for 4 frames
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
}

//Eventhandler if filereader is clearedwe flush the decoder.
void MP3Codec::OnFileReaderClearEvent()
{
  if (m_pDecoder) {
    FlushDecoder();
  }
}

bool MP3Codec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.IsLoaded())
    m_dll.Load();

  // TODO:  add file extension checking and HTTP/Icecast/Shoutcast reading
  if (m_pDecoder)
  {
    delete m_pDecoder;
    m_pDecoder = NULL;
  }
  m_pDecoder = m_dll.CreateAudioDecoder(' ',NULL);

  if ( m_pDecoder )
    CLog::Log(LOGINFO, "MP3Codec: Loaded decoder at %p", (void *)m_pDecoder);
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

  CFileItem item(strFile, false);
  bool bIsInternetStream = item.IsInternetStream();
  if (!bIsInternetStream)
  {
    // Guess Bitrate and obtain replayGain information etc.
    CMusicInfoTagLoaderMP3 mp3info;
    mp3info.ReadSeekAndReplayGainInfo(strFile);
    mp3info.GetSeekInfo(m_seekInfo);
    mp3info.GetReplayGain(m_replayGain);
  }

  int id3v2Size = 0;
  int result = -1;
  __int64 length = 0;

  if (!m_file.Open(strFile, true, READ_CACHED))
  {
    CLog::Log(LOGERROR, "MP3Codec: Unable to open file %s", strFile.c_str());
    goto error;
  }

  length = m_file.GetLength();
  if (!bIsInternetStream)
  {
    m_TotalTime = (__int64)(m_seekInfo.GetDuration() * 1000.0f);
  }
  if ( m_TotalTime && length )
  {
    m_Bitrate = (int)((length / m_seekInfo.GetDuration()) * 8);  // average bitrate
  }

  // Read in some data so we can determine the sample size and so on
  // This needs to be made more intelligent - possibly use a temp output buffer
  // and cycle around continually reading until we have the necessary data
  // as a first workaround skip the id3v2 tag at the beginning of the file
  if (!bIsInternetStream)
  {
    if (m_seekInfo.GetNumOffsets() > 0)
    {
      const float* offsets=m_seekInfo.GetOffsets();
      id3v2Size=(int)offsets[0];
      m_file.Seek(id3v2Size);
    }
    else
    {
      CLog::Log(LOGERROR, "MP3Codec: Seek info unavailable for file <%s> (corrupt?)", strFile.c_str());
      goto error;
    }
  }

  m_eof = false;
  while ((result != DECODING_SUCCESS) && !m_eof && (m_OutputBufferPos < 1152*8)) // eof can be set from outside (when stopping playback)
  {
    result = Read(8192, true);
    if (result == DECODING_ERROR)
    {
      CLog::Log(LOGERROR, "MP3Codec: Unable to determine file format of %s (corrupt start of mp3?)", strFile.c_str());
      goto error;
    }
    if (bIsInternetStream) m_Bitrate = m_Formatdata[4];
  } ;
  return true;

error:
  m_file.Close();
  if (m_pDecoder)
  {
    delete m_pDecoder;
    m_pDecoder = NULL;
  }
  return false;
}

void MP3Codec::DeInit()
{
  m_file.Close();
  m_eof = true;
}

void MP3Codec::FlushDecoder()
{
  // Flush the decoder
  m_pDecoder->flush();
  m_InputBufferPos = 0;
  m_OutputBufferPos = 0;
  m_CallAgainWithSameBuffer = false;
}

__int64 MP3Codec::Seek(__int64 iSeekTime)
{
  // calculate our offset to seek to in the file
  m_lastByteOffset = m_seekInfo.GetByteOffset(0.001f * iSeekTime);
  m_file.Seek(m_lastByteOffset, SEEK_SET);
  FlushDecoder();
  return iSeekTime;
}

int MP3Codec::ReadSamples(float *pBuffer, int numsamples, int *actualsamples)
{
  int result = ReadPCM((BYTE *)pBuffer, numsamples * sizeof(float), actualsamples);
  *actualsamples /= sizeof(float);
  return result;
}

int MP3Codec::Read(int size, bool init)
{
  // First read in any extra info we need from our MP3
  int nChunkSize = m_file.GetChunkSize();
  if (nChunkSize == 0)
    nChunkSize = DEFAULT_CHUNK_SIZE;

  int inputBufferToRead = XMIN(nChunkSize, (int)(m_InputBufferSize - m_InputBufferPos));
  if ( inputBufferToRead && !m_CallAgainWithSameBuffer && !m_eof )
  {
    if (m_file.GetLength() > 0)
    {
      int fileLeft=(int)(m_file.GetLength() - m_file.GetPosition());
      if (inputBufferToRead >  fileLeft ) inputBufferToRead = fileLeft;
    }

    DWORD dwBytesRead = m_file.Read(m_InputBuffer + m_InputBufferPos , inputBufferToRead);
    if (!dwBytesRead)
    {
      CLog::Log(LOGERROR, "MP3Codec: Error reading file");
      return DECODING_ERROR;
    }
    // add the size of read PAP data to the buffer size
    m_InputBufferPos += dwBytesRead;
    if (m_file.GetLength() > 0 && m_file.GetLength() == m_file.GetPosition() )
      m_eof = true;
  }
  // Decode data if we have some to decode
  if ( m_InputBufferPos || m_CallAgainWithSameBuffer || (m_eof && m_Decoding) )
  {
    int result;

    m_Decoding = true;

    if ( size )
    {
      m_CallAgainWithSameBuffer = false;
      int outputsize = m_OutputBufferSize - m_OutputBufferPos;
      //MAD needs padding at the end of the stream to decode the last frame, this doesn't hurt winamps in_mp3.dll
      int madguard = 0;
      if (m_eof)
      {
        madguard = 8;
        if (m_InputBufferPos + madguard > m_InputBufferSize)
          madguard = m_InputBufferSize - m_InputBufferPos;
        memset(m_InputBuffer + m_InputBufferPos, 0, madguard);
      }

      // See if there is an ID3v2 tag at the beginning of the stream.
      // For file-based internet streams (i.e UPnP/HTTP), it is very likely to happen.
      // If we don't skip it, we may never be able to snyc to the MPEG stream
      if (init)
      {
        // Check for an ID3v2 tag header
        unsigned int tagSize = CMusicInfoTagLoaderMP3::IsID3v2Header(m_InputBuffer,m_InputBufferPos);
        if(tagSize)
        {
          if (tagSize != m_file.Seek(tagSize, SEEK_SET))
            return DECODING_ERROR;

          // Reset the read state before we return
          m_InputBufferPos = 0;
          m_CallAgainWithSameBuffer = false;

          // Please try your call again later...
          return DECODING_CALLAGAIN;
        }
      }

      // Now decode data into the vacant frame buffer.
      result = m_pDecoder->decode( m_InputBuffer, m_InputBufferPos + madguard, m_OutputBuffer + m_OutputBufferPos, &outputsize, (unsigned int *)&m_Formatdata);
      if ( result != DECODING_ERROR)
      {
        if (init)
        {
          // Make sure some data was decoded. Without a valid frame, we cannot determine the audio format
          if (!outputsize)
            return DECODING_ERROR;

          m_Channels              = m_Formatdata[2];
          m_SampleRate            = m_Formatdata[1];
          m_BitsPerSampleInternal = m_Formatdata[3];
          //m_BitsPerSample holds display value when using 32-bits floats (source is 24 bits), real value otherwise
          m_BitsPerSample         = m_BitsPerSampleInternal>16?24:m_BitsPerSampleInternal;
        }
        // let's check if we need to ignore the decoded data.
        if ( m_IgnoreFirst && outputsize && m_seekInfo.GetFirstSample() )
        {
          // starting up - lets ignore the first (typically 576) samples
          int iDelay = DECODER_DELAY + m_seekInfo.GetFirstSample();  // decoder delay + encoder delay
          iDelay *= m_Channels * m_BitsPerSampleInternal / 8;            // sample size
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
        // Do we still have data in the buffer to decode?
        if ( result == DECODING_CALLAGAIN )
          m_CallAgainWithSameBuffer = true;
        else
        { // There are no more complete frames in the input buffer
          m_InputBufferPos = 0;
          // Check for the end of file (as we need to remove data from the end of the track)
          if (m_eof)
          {
            m_Decoding = false;
            // EOF reached - let's remove any unused samples from our frame buffers
            if (m_IgnoreLast && m_seekInfo.GetLastSample())
            {
              unsigned int samplestoremove = (m_seekInfo.GetLastSample() - DECODER_DELAY);
              samplestoremove *= m_Channels * m_BitsPerSampleInternal / 8;
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
      return result;
    }
  }
  return DECODING_SUCCESS;
}

int MP3Codec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize = 0;
  if (Read(size) == DECODING_ERROR)
    return READ_ERROR;

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
  // only return READ_EOF when we've reached the end of the mp3 file, we've finished decoding, and our output buffer is depleated.
  if (m_eof && !m_Decoding && !m_OutputBufferPos)
    return READ_EOF;
  return READ_SUCCESS;
}

bool MP3Codec::CanInit()
{
  return m_dll.CanLoad();
}

bool MP3Codec::SkipNext()
{
  return m_file.SkipNext();
}

bool MP3Codec::CanSeek()
{
  return true;
}
