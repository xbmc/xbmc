/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "MP3codec.h"
#include "FileItem.h"
#include "utils/log.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "music/tags/TagLoaderTagLib.h"

#define BYTES2INT(b1,b2,b3,b4) (((b1 & 0xFF) << (3*8)) | \
((b2 & 0xFF) << (2*8)) | \
((b3 & 0xFF) << (1*8)) | \
((b4 & 0xFF) << (0*8)))

#define UNSYNC(b1,b2,b3,b4) (((b1 & 0x7F) << (3*7)) | \
((b2 & 0x7F) << (2*7)) | \
((b3 & 0x7F) << (1*7)) | \
((b4 & 0x7F) << (0*7)))

#define MPEG_VERSION2_5 0
#define MPEG_VERSION1   1
#define MPEG_VERSION2   2

/* Xing header information */
#define VBR_FRAMES_FLAG 0x01
#define VBR_BYTES_FLAG  0x02
#define VBR_TOC_FLAG    0x04

// mp3 header flags
#define SYNC_MASK (0x7ff << 21)
#define VERSION_MASK (3 << 19)
#define LAYER_MASK (3 << 17)
#define PROTECTION_MASK (1 << 16)
#define BITRATE_MASK (0xf << 12)
#define SAMPLERATE_MASK (3 << 10)
#define PADDING_MASK (1 << 9)
#define PRIVATE_MASK (1 << 8)
#define CHANNELMODE_MASK (3 << 6)
#define MODE_EXT_MASK (3 << 4)
#define COPYRIGHT_MASK (1 << 3)
#define ORIGINAL_MASK (1 << 2)
#define EMPHASIS_MASK 3

#define DECODER_DELAY 529 // decoder delay in samples

#define DEFAULT_CHUNK_SIZE 16384

#define DECODING_ERROR    -1
#define DECODING_SUCCESS   0
#define DECODING_CALLAGAIN 1

#define SAMPLESPERFRAME   1152
#define CHANNELSPERSAMPLE 2
#define BITSPERSAMPLE     32
#define OUTPUTFRAMESIZE   (SAMPLESPERFRAME * CHANNELSPERSAMPLE * (BITSPERSAMPLE >> 3))

MP3Codec::MP3Codec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = "mp3";

  // mp3 related
  m_CallAgainWithSameBuffer = false;
  m_readRetries = 5;
  m_lastByteOffset = 0;
  m_InputBufferSize = 64*1024;         // 64k is a reasonable amount, considering that we actual are
                                       // using a background reader thread now that caches in advance.
  m_InputBuffer = new BYTE[m_InputBufferSize];
  m_InputBufferPos = 0;

  memset(&m_Formatdata,0,sizeof(m_Formatdata));
  m_DataFormat = AE_FMT_S32NE;

  // create our output buffer
  m_OutputBufferSize = OUTPUTFRAMESIZE * 4;        // enough for 4 frames
  m_OutputBuffer = new BYTE[m_OutputBufferSize];
  m_OutputBufferPos = 0;
  m_Decoding = false;
  m_IgnoreFirst = true; // we want to be gapless
  m_IgnoredBytes = 0;
  m_IgnoreLast = true;
  m_eof = false;

  // VBR stuff
  m_iSeekOffsets = 0;
  m_fTotalDuration = 0.0f;
  m_SeekOffset = NULL;
  m_iFirstSample = 0;
  m_iLastSample = 0;

  memset(&mxhouse, 0, sizeof(madx_house));
  memset(&mxstat,  0, sizeof(madx_stat));
  mxsig = ERROR_OCCURED;
  
  m_HaveData = false;
  flushcnt = 0;

  if (m_dll.Load())
    madx_init(&mxhouse);
}

MP3Codec::~MP3Codec()
{
  DeInit();

  delete[] m_InputBuffer;
  m_InputBuffer = NULL;

  delete[] m_OutputBuffer;
  m_OutputBuffer = NULL;

  delete[] m_SeekOffset;
  m_SeekOffset = NULL;

  if (m_dll.IsLoaded())
    madx_deinit(&mxhouse);
}

//Eventhandler if filereader is clearedwe flush the decoder.
void MP3Codec::OnFileReaderClearEvent()
{
  FlushDecoder();
}

bool MP3Codec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
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
  m_readRetries = 5;

  int id3v2Size = 0;
  int result = -1;
  int64_t length = 0;
  bool bTags = false;

  if (!m_file.Open(strFile, READ_CACHED))
  {
    CLog::Log(LOGERROR, "MP3Codec: Unable to open file %s", strFile.c_str());
    return false;
  }

  // Guess Bitrate and obtain replayGain information etc.
  length = m_file.GetLength();
  if (length != 0)
  {
    CTagLoaderTagLib tagLoaderTagLib; //opens the file so needs to be after m_file.Open or lastfm radio breaks.
    bTags = tagLoaderTagLib.Load(strFile, m_tag);

    if (bTags)
      ReadDuration();

    if (bTags)
      m_TotalTime = (int64_t)( m_fTotalDuration * 1000.0f);

    // Read in some data so we can determine the sample size and so on
    // This needs to be made more intelligent - possibly use a temp output buffer
    // and cycle around continually reading until we have the necessary data
    // as a first workaround skip the id3v2 tag at the beginning of the file
    if (bTags)
    {
      if (m_iSeekOffsets > 0)
      {
        id3v2Size=(int)m_SeekOffset[0];
        m_file.Seek(id3v2Size);
      }
      else
      {
        CLog::Log(LOGERROR, "MP3Codec: Seek info unavailable for file <%s> (corrupt?)", strFile.c_str());
        goto error;
      }
    }
    
    if ( m_TotalTime && (length - id3v2Size > 0) )
    {
      m_Bitrate = (int)(((length - id3v2Size) / m_fTotalDuration) * 8);  // average bitrate
    }
  }

  m_eof = false;
  while ((result != DECODING_SUCCESS) && !m_eof && (m_OutputBufferPos < OUTPUTFRAMESIZE)) // eof can be set from outside (when stopping playback)
  {
    result = Read(8192, true);
    if (result == DECODING_ERROR)
    {
      CLog::Log(LOGERROR, "MP3Codec: Unable to determine file format of %s (corrupt start of mp3?)", strFile.c_str());
      goto error;
    }
    if (bTags && !m_Bitrate) //use tag bitrate if average bitrate is not available
      m_Bitrate = m_Formatdata[4];
  }

  return true;

error:
  m_file.Close();
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
  Flush();
  m_InputBufferPos = 0;
  m_OutputBufferPos = 0;
  m_CallAgainWithSameBuffer = false;
}

int64_t MP3Codec::Seek(int64_t iSeekTime)
{
  // calculate our offset to seek to in the file
  m_lastByteOffset = GetByteOffset(0.001f * iSeekTime);
  m_file.Seek(m_lastByteOffset, SEEK_SET);
  FlushDecoder();
  return iSeekTime;
}

int MP3Codec::Read(int size, bool init)
{
  int inputBufferToRead = (int)(m_InputBufferSize - m_InputBufferPos);
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
      // See if there is an ID3v2 tag at the beginning of the stream.
      // For file-based internet streams (i.e UPnP/HTTP), it is very likely to happen.
      // If we don't skip it, we may never be able to snyc to the MPEG stream
      if (init)
      {
        // Check for an ID3v2 tag header
        unsigned int tagSize = MP3Codec::IsID3v2Header(m_InputBuffer,m_InputBufferPos);
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
      result = Decode(&outputsize);
      if ( result != DECODING_ERROR)
      {
        if (init)
        {
          if (result == 0 && m_readRetries-- > 0)
            return Read(size,init);
          // Make sure some data was decoded. Without a valid frame, we cannot determine the audio format
          if (!outputsize)
            return DECODING_ERROR;

          m_Channels              = m_Formatdata[2];
          m_SampleRate            = m_Formatdata[1];
          m_BitsPerSample         = m_Formatdata[3];
        }

        // let's check if we need to ignore the decoded data.
        if ( m_IgnoreFirst && outputsize && m_iFirstSample )
        {
          // starting up - lets ignore the first (typically 576) samples
          int iDelay = DECODER_DELAY + m_iFirstSample;  // decoder delay + encoder delay
          iDelay *= m_Channels * (m_BitsPerSample >> 3);            // sample size
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
          //m_InputBufferPos = 0;
          // Check for the end of file (as we need to remove data from the end of the track)
          if (m_eof)
          {
            m_Decoding = false;
            // EOF reached - let's remove any unused samples from our frame buffers
            if (m_IgnoreLast && m_iLastSample)
            {
              unsigned int samplestoremove = (m_iLastSample - DECODER_DELAY);
              samplestoremove *= m_Channels * (m_BitsPerSample >> 3);
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
  m_readRetries = 5;
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
  int move;
  if ((m_eof && !m_Decoding) || m_OutputBufferPos <= OUTPUTFRAMESIZE)
    move = m_OutputBufferPos;
  else
    move = m_OutputBufferPos - OUTPUTFRAMESIZE;
  move = std::min(move, size);

  memcpy(pBuffer, m_OutputBuffer, move);
  m_OutputBufferPos -= move;
  memmove(m_OutputBuffer, m_OutputBuffer + move, m_OutputBufferPos);
  *actualsize = move;

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

int MP3Codec::Decode(int *out_len)
{
  if (!m_HaveData)
  {
    if (!m_dll.IsLoaded())
      m_dll.Load();
    
    //MAD needs padding at the end of the stream to decode the last frame, this doesn't hurt winamps in_mp3.dll
    int madguard = 0;
    if (m_eof)
    {
      madguard = 8;
      if (m_InputBufferPos + madguard > m_InputBufferSize)
        madguard = m_InputBufferSize - m_InputBufferPos;
      memset(m_InputBuffer + m_InputBufferPos, 0, madguard);
    }

    m_dll.mad_stream_buffer( &mxhouse.stream, m_InputBuffer, m_InputBufferPos + madguard );
    mxhouse.stream.error = (mad_error)0;
    m_dll.mad_stream_sync(&mxhouse.stream);
    if ((mxstat.flushed) && (flushcnt == 2))
    {
      int skip;
      skip = 2;
      do
      {
        if (m_dll.mad_frame_decode(&mxhouse.frame, &mxhouse.stream) == 0)
        {
          if (--skip == 0)
            m_dll.mad_synth_frame(&mxhouse.synth, &mxhouse.frame);
        }
        else if (!MAD_RECOVERABLE(mxhouse.stream.error))
          break;
      }
      while (skip);
      mxstat.flushed = false;
    }
  }
  int maxtowrite = *out_len;
  *out_len = 0;
  mxsig = ERROR_OCCURED;
  while ((mxsig != FLUSH_BUFFER) && (*out_len + mxstat.framepcmsize < (size_t)maxtowrite))
  {
    mxsig = madx_read(&mxhouse, &mxstat, maxtowrite);
    switch (mxsig)
    {
    case ERROR_OCCURED: 
      *out_len = 0;
      m_HaveData = false;
      return -1;
    case MORE_INPUT: 
      if (mxstat.remaining > 0)
      {
        memcpy(m_InputBuffer, mxhouse.stream.next_frame, mxstat.remaining);
        m_InputBufferPos = mxstat.remaining;
      }
      m_HaveData = false;
      return 0;
    case FLUSH_BUFFER:
      m_Formatdata[2] = mxhouse.synth.pcm.channels;
      m_Formatdata[1] = mxhouse.synth.pcm.samplerate;
      m_Formatdata[3] = BITSPERSAMPLE;
      m_Formatdata[4] = mxhouse.frame.header.bitrate;
      *out_len += (int)mxstat.write_size;
      mxstat.write_size = 0;
      break;
    default:
      break;
    }
  }
  if (!mxhouse.stream.next_frame || (mxhouse.stream.bufend - mxhouse.stream.next_frame <= 0))
  {
    m_HaveData = false;
    return 0;
  }
  m_HaveData = true;
  return 1;
}

void MP3Codec::Flush()
{
  if (!m_dll.IsLoaded())
    m_dll.Load();
  m_dll.mad_frame_mute(&mxhouse.frame);
  m_dll.mad_synth_mute(&mxhouse.synth);
  m_dll.mad_stream_finish(&mxhouse.stream);
  m_dll.mad_stream_init(&mxhouse.stream);
  ZeroMemory(&mxstat, sizeof(madx_stat)); 
  mxstat.flushed = true;
  if (flushcnt < 2) flushcnt++;
  m_HaveData = false;
  m_InputBufferPos = 0;
}

int MP3Codec::madx_init (madx_house *mxhouse )
{
  if (!m_dll.IsLoaded())
    m_dll.Load();
  // Initialize libmad structures 
  m_dll.mad_stream_init(&mxhouse->stream);
  mxhouse->stream.options = MAD_OPTION_IGNORECRC;
  m_dll.mad_frame_init(&mxhouse->frame);
  m_dll.mad_synth_init(&mxhouse->synth);
  mxhouse->timer = m_dll.Get_mad_timer_zero();

  return(1);
}

madx_sig MP3Codec::madx_read(madx_house *mxhouse, madx_stat *mxstat, int maxwrite)
{
  if (!m_dll.IsLoaded())
    m_dll.Load();
  mxhouse->output_ptr = m_OutputBuffer + m_OutputBufferPos;

  if( m_dll.mad_frame_decode(&mxhouse->frame, &mxhouse->stream) )
  {
    if( !MAD_RECOVERABLE(mxhouse->stream.error) )
    {
      if( mxhouse->stream.error == MAD_ERROR_BUFLEN )
      {    
        //printf("Need more input (%s)",  mad_stream_errorstr(&mxhouse->stream));
        mxstat->remaining = mxhouse->stream.bufend - mxhouse->stream.next_frame;

        return(MORE_INPUT);      
      }
      else
      {
        CLog::Log(LOGERROR, "(MAD)Unrecoverable frame level error (%s).", m_dll.mad_stream_errorstr(&mxhouse->stream));
        return(ERROR_OCCURED); 
      }
    }
    return(SKIP_FRAME); 
  }

  m_dll.mad_synth_frame( &mxhouse->synth, &mxhouse->frame );
  
  mxstat->framepcmsize = mxhouse->synth.pcm.length * mxhouse->synth.pcm.channels * (int)(BITSPERSAMPLE >> 3);
  mxhouse->frame_cnt++;
  m_dll.mad_timer_add( &mxhouse->timer, mxhouse->frame.header.duration );

  int32_t *dest = (int32_t*)mxhouse->output_ptr;
  for(int i=0; i < mxhouse->synth.pcm.length; i++)
  {
    // Left channel
    *dest++ = (int32_t)(mxhouse->synth.pcm.samples[0][i] << 2);

    // Right channel
    if(MAD_NCHANNELS(&mxhouse->frame.header) == 2)
      *dest++ = (int32_t)(mxhouse->synth.pcm.samples[1][i] << 2);
  }

  // Tell calling code buffer size
  mxhouse->output_ptr = (unsigned char*)dest;
  mxstat->write_size  = mxhouse->output_ptr - (m_OutputBuffer + m_OutputBufferPos);

  return(FLUSH_BUFFER);
}

void MP3Codec::madx_deinit( madx_house *mxhouse )
{
  if (!m_dll.IsLoaded())
    m_dll.Load();
  mad_synth_finish(&mxhouse->synth);
  m_dll.mad_frame_finish(&mxhouse->frame);
  m_dll.mad_stream_finish(&mxhouse->stream);
}

/* check if 'head' is a valid mp3 frame header and return the framesize if it is (0 otherwise) */
int MP3Codec::IsMp3FrameHeader(unsigned long head)
{
  const long freqs[9] = { 44100, 48000, 32000,
    22050, 24000, 16000 ,
    11025 , 12000 , 8000 };
  
  const int tabsel_123[2][3][16] = {
    { {128,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
      {128,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
      {128,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },
    
    { {128,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
      {128,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
      {128,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
  };
  
  
  if ((head & SYNC_MASK) != (unsigned long)SYNC_MASK) /* bad sync? */
    return 0;
  if ((head & VERSION_MASK) == (1 << 19)) /* bad version? */
    return 0;
  if (!(head & LAYER_MASK)) /* no layer? */
    return 0;
  if ((head & BITRATE_MASK) == BITRATE_MASK) /* bad bitrate? */
    return 0;
  if (!(head & BITRATE_MASK)) /* no bitrate? */
    return 0;
  if ((head & SAMPLERATE_MASK) == SAMPLERATE_MASK) /* bad sample rate? */
    return 0;
  if (((head >> 19) & 1) == 1 &&
      ((head >> 17) & 3) == 3 &&
      ((head >> 16) & 1) == 1)
    return 0;
  if ((head & 0xffff0000) == 0xfffe0000)
    return 0;
  
  int srate = 0;
  if(!((head >> 20) &  1))
    srate = 6 + ((head>>10)&0x3);
  else
    srate = ((head>>10)&0x3) + ((1-((head >> 19) &  1)) * 3);
  
  int framesize = tabsel_123[1 - ((head >> 19) &  1)][(4-((head>>17)&3))-1][((head>>12)&0xf)]*144000/(freqs[srate]<<(1 - ((head >> 19) &  1)))+((head>>9)&0x1);
  return framesize;
}

//TODO: merge duplicate, but slitely different implemented) code and consts in IsMp3FrameHeader(above) and ReadDuration (below).
// Inspired by http://rockbox.haxx.se/ and http://www.xs4all.nl/~rwvtveer/scilla
int MP3Codec::ReadDuration()
{
#define SCANSIZE  8192
#define CHECKNUMFRAMES 5
#define ID3V2HEADERSIZE 10

  unsigned char* xing;
  unsigned char* vbri;
  unsigned char buffer[SCANSIZE + 1];

  const int freqtab[][4] =
  {
    {11025, 12000, 8000, 0}
    ,   /* MPEG version 2.5 */
    {44100, 48000, 32000, 0},  /* MPEG Version 1 */
    {22050, 24000, 16000, 0},  /* MPEG version 2 */
  };

  /* Check if the file has an ID3v1 tag */
  m_file.Seek(m_file.GetLength()-128, SEEK_SET);
  m_file.Read(buffer, 3);

  bool hasid3v1=false;
  if (buffer[0] == 'T' &&
      buffer[1] == 'A' &&
      buffer[2] == 'G')
  {
    hasid3v1=true;
  }

  /* Check if the file has an ID3v2 tag (or multiple tags) */
  unsigned int id3v2Size = 0;
  m_file.Seek(0, SEEK_SET);
  m_file.Read(buffer, ID3V2HEADERSIZE);
  unsigned int size = IsID3v2Header(buffer, ID3V2HEADERSIZE);
  while (size)
  {
    id3v2Size += size;
    if (id3v2Size != m_file.Seek(id3v2Size, SEEK_SET))
      return 0;
    if (ID3V2HEADERSIZE != m_file.Read(buffer, ID3V2HEADERSIZE))
      return 0;
    size = IsID3v2Header(buffer, ID3V2HEADERSIZE);
  }

  //skip any padding
  //already read ID3V2HEADERSIZE bytes so take it into account
  int iScanSize = m_file.Read(buffer + ID3V2HEADERSIZE, SCANSIZE - ID3V2HEADERSIZE) + ID3V2HEADERSIZE;
  int iBufferDataStart;
  do
  {
    iBufferDataStart = -1;
    for(int i = 0; i < iScanSize; i++)
    {
      //all 0x00's after id3v2 tag until first mpeg-frame are padding
      if (buffer[i] != 0)
      {
        iBufferDataStart = i;
        break;
      }
    }
    if (iBufferDataStart == -1)
    {
      id3v2Size += iScanSize;
      iScanSize = m_file.Read(buffer, SCANSIZE);
    }
    else
    {
      id3v2Size += iBufferDataStart;
    }
  } while (iBufferDataStart == -1 && iScanSize > 0);
  if (iScanSize <= 0)
    return 0;
  if (iBufferDataStart > 0)
  {
    //move data to front of buffer
    iScanSize -= iBufferDataStart;
    memcpy(buffer, buffer + iBufferDataStart, iScanSize);
    //fill remainder of buffer with new data
    iScanSize += m_file.Read(buffer + iScanSize, SCANSIZE - iScanSize);
  }

  int firstFrameOffset = id3v2Size;


  //raw mp3Data = FileSize - ID3v1 tag - ID3v2 tag
  int nMp3DataSize = (int)m_file.GetLength() - id3v2Size;
  if (hasid3v1)
    nMp3DataSize -= 128;

  //*** find the first frame in the buffer, we do this by checking if the calculated framesize leads to the next frame a couple of times.
  //the first frame that leads to a valid next frame a couple of times is where we should start decoding.
  int firstValidFrameLocation = 0;
  for(int i = 0; i + 3 < iScanSize; i++)
  {
    int j = i;
    int framesize = 1;
    int numFramesCheck = 0;

    for (numFramesCheck = 0; (numFramesCheck < CHECKNUMFRAMES) && framesize; numFramesCheck++)
    {
      unsigned long mpegheader = (unsigned long)(
                                                 ( (buffer[j] & 255) << 24) |
                                                 ( (buffer[j + 1] & 255) << 16) |
                                                 ( (buffer[j + 2] & 255) << 8) |
                                                 ( (buffer[j + 3] & 255) )
                                                 );
      framesize = IsMp3FrameHeader(mpegheader);

      j += framesize;

      if ((j + 4) >= iScanSize)
      {
        //no valid frame found in buffer
        firstValidFrameLocation = -1;
        break;
      }
    }

    if (numFramesCheck == CHECKNUMFRAMES)
    { //found it
      firstValidFrameLocation = i;
      break;
    }
  }

  if (firstValidFrameLocation != -1)
  {
    firstFrameOffset += firstValidFrameLocation;
    nMp3DataSize -= firstValidFrameLocation;
  }
  //*** done finding first valid frame

  //find lame/xing info
  int frequency = 0, bitrate = 0, bittable = 0;
  int frame_count = 0;
  double tpf = 0.0, bpf = 0.0;
  for (int i = 0; i < iScanSize; i++)
  {
    unsigned long mpegheader = (unsigned long)(
                                               ( (buffer[i] & 255) << 24) |
                                               ( (buffer[i + 1] & 255) << 16) |
                                               ( (buffer[i + 2] & 255) << 8) |
                                               ( (buffer[i + 3] & 255) )
                                               );
    
    // Do we have a Xing header before the first mpeg frame?
    if (buffer[i ] == 'X' &&
        buffer[i + 1] == 'i' &&
        buffer[i + 2] == 'n' &&
        buffer[i + 3] == 'g')
    {
      if (buffer[i + 7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
      {
        frame_count = BYTES2INT(buffer[i + 8], buffer[i + 8 + 1], buffer[i + 8 + 2], buffer[i + 8 + 3]);
        if (buffer[i + 7] & VBR_TOC_FLAG)
        {
          int iOffset = i + 12;
          if (buffer[i + 7] & VBR_BYTES_FLAG)
          {
            nMp3DataSize = BYTES2INT(buffer[i + 12], buffer[i + 12 + 1], buffer[i + 12 + 2], buffer[i + 12 + 3]);
            iOffset += 4;
          }
          float *offset = new float[101];
          for (int j = 0; j < 100; j++)
            offset[j] = (float)buffer[iOffset + j]/256.0f * nMp3DataSize + firstFrameOffset;
          offset[100] = (float)nMp3DataSize + firstFrameOffset;
          SetOffsets(100, offset);
          delete[] offset;
        }
      }
    }
    /*else if (buffer[i] == 'I' && buffer[i + 1] == 'n' && buffer[i + 2] == 'f' && buffer[i + 3] == 'o') //Info is used when CBR
     {
     //should we do something with this?
     }*/
    
    if (
        (i == firstValidFrameLocation) ||
        (
         (firstValidFrameLocation == -1) &&
         (IsMp3FrameHeader(mpegheader))
         )
        )
    {
      // skip mpeg header
      i += 4;
      int version = 0;
      /* MPEG Audio Version */
      switch (mpegheader & VERSION_MASK)
      {
        case 0:
          /* MPEG version 2.5 is not an official standard */
          version = MPEG_VERSION2_5;
          bittable = MPEG_VERSION2 - 1; /* use the V2 bit rate table */
          break;

        case (1 << 19):
          return 0;

        case (2 << 19):
          /* MPEG version 2 (ISO/IEC 13818-3) */
          version = MPEG_VERSION2;
          bittable = MPEG_VERSION2 - 1;
          break;
          
        case (3 << 19):
          /* MPEG version 1 (ISO/IEC 11172-3) */
          version = MPEG_VERSION1;
          bittable = MPEG_VERSION1 - 1;
          break;
      }

      int layer = 0;
      switch (mpegheader & LAYER_MASK)
      {
        case (3 << 17):  // LAYER_I
          layer = 1;
          break;
        case (2 << 17):  // LAYER_II
          layer = 2;
          break;
        case (1 << 17):  // LAYER_III
          layer = 3;
          break;
      }

      /* Table of bitrates for MP3 files, all values in kilo.
       * Indexed by version, layer and value of bit 15-12 in header.
       */
      const int bitrate_table[2][4][16] =
      {
        {
          {0},
          {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0},
          {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
          {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}
        },
        {
          {0},
          {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0},
          {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
          {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}
        }
      };

      /* Bitrate */
      int bitindex = (mpegheader & 0xf000) >> 12;
      int freqindex = (mpegheader & 0x0C00) >> 10;
      bitrate = bitrate_table[bittable][layer][bitindex];

      /* Calculate bytes per frame, calculation depends on layer */
      switch (layer)
      {
        case 1:
          bpf = bitrate;
          bpf *= 48000;
          bpf /= freqtab[version][freqindex] << (version - 1);
          break;
        case 2:
        case 3:
          bpf = bitrate;
          bpf *= 144000;
          bpf /= freqtab[version][freqindex] << (version - 1);
          break;
        default:
          bpf = 1;
      }
      double tpfbs[] = { 0, 384.0f, 1152.0f, 1152.0f };
      frequency = freqtab[version][freqindex];
      tpf = tpfbs[layer] / (double) frequency;
      if (version == MPEG_VERSION2_5 || version == MPEG_VERSION2)
        tpf /= 2;

      if (frequency == 0)
        return 0;

      /* Channel mode (stereo/mono) */
      int chmode = (mpegheader & 0xc0) >> 6;
      /* calculate position of Xing VBR header */
      if (version == MPEG_VERSION1)
      {
        if (chmode == 3) /* mono */
          xing = buffer + i + 17;
        else
          xing = buffer + i + 32;
      }
      else
      {
        if (chmode == 3) /* mono */
          xing = buffer + i + 9;
        else
          xing = buffer + i + 17;
      }

      /* calculate position of VBRI header */
      vbri = buffer + i + 32;

      // Do we have a Xing header
      if (xing[0] == 'X' &&
          xing[1] == 'i' &&
          xing[2] == 'n' &&
          xing[3] == 'g')
      {
        if (xing[7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
        {
          frame_count = BYTES2INT(xing[8], xing[8 + 1], xing[8 + 2], xing[8 + 3]);
          if (xing[7] & VBR_TOC_FLAG)
          {
            int iOffset = 12;
            if (xing[7] & VBR_BYTES_FLAG)
            {
              nMp3DataSize = BYTES2INT(xing[12], xing[12 + 1], xing[12 + 2], xing[12 + 3]);
              iOffset += 4;
            }
            float *offset = new float[101];
            for (int j = 0; j < 100; j++)
              offset[j] = (float)xing[iOffset + j]/256.0f * nMp3DataSize + firstFrameOffset;
            //first offset should be the location of the first frame, usually it is but some files have seektables that are a little off.
            offset[0]   = (float)firstFrameOffset;
            offset[100] = (float)nMp3DataSize + firstFrameOffset;
            SetOffsets(100, offset);
            delete[] offset;
          }
        }
      }
      // Get the info from the Lame header (if any)
      if ((xing[0] == 'X' && xing[1] == 'i' && xing[2] == 'n' && xing[3] == 'g') ||
          (xing[0] == 'I' && xing[1] == 'n' && xing[2] == 'f' && xing[3] == 'o'))
      {
        if (ReadLAMETagInfo(xing - 0x24))
        {
          // calculate new (more accurate) duration:
          int64_t lastSample = (int64_t)frame_count * (int64_t)tpfbs[layer] - m_iFirstSample - m_iLastSample;
          m_fTotalDuration = (float)lastSample / frequency;
        }
      }
      if (vbri[0] == 'V' &&
          vbri[1] == 'B' &&
          vbri[2] == 'R' &&
          vbri[3] == 'I')
      {
        frame_count = BYTES2INT(vbri[14], vbri[14 + 1],
                                vbri[14 + 2], vbri[14 + 3]);
        nMp3DataSize = BYTES2INT(vbri[10], vbri[10 + 1], vbri[10 + 2], vbri[10 + 3]);
        int iSeekOffsets = (((vbri[18] & 0xFF) << 8) | (vbri[19] & 0xFF)) + 1;
        float *offset = new float[iSeekOffsets + 1];
        int iScaleFactor = ((vbri[20] & 0xFF) << 8) | (vbri[21] & 0xFF);
        int iOffsetSize = ((vbri[22] & 0xFF) << 8) | (vbri[23] & 0xFF);
        offset[0] = (float)firstFrameOffset;
        for (int j = 0; j < iSeekOffsets; j++)
        {
          DWORD dwOffset = 0;
          for (int k = 0; k < iOffsetSize; k++)
          {
            dwOffset = dwOffset << 8;
            dwOffset += vbri[26 + j*iOffsetSize + k];
          }
          offset[j] += (float)dwOffset * iScaleFactor;
          offset[j + 1] = offset[j];
        }
        offset[iSeekOffsets] = (float)firstFrameOffset + nMp3DataSize;
        SetOffsets(iSeekOffsets, offset);
        delete[] offset;
      }
      // We are done!
      break;
    }
  }

  if (m_iSeekOffsets == 0)
  {
    float offset[2];
    offset[0] = (float)firstFrameOffset;
    offset[1] = (float)(firstFrameOffset + nMp3DataSize);
    SetOffsets(1, offset);
  }

  // Calculate duration if we have a Xing/VBRI VBR file
  if (frame_count > 0)
  {
    double d = tpf * frame_count;
    m_fTotalDuration = (float)d;
    return (int)d;
  }

  // Normal mp3 with constant bitrate duration
  // Now song length is (filesize without id3v1/v2 tag)/((bitrate)/(8))
  double d = 0;
  if (bitrate > 0)
    d = (double)(nMp3DataSize / ((bitrate * 1000) / 8));
  m_fTotalDuration = (float)d;
  return (int)d;
}

bool MP3Codec::ReadLAMETagInfo(BYTE *b)
{
  if (b[0x9c] != 'L' ||
      b[0x9d] != 'A' ||
      b[0x9e] != 'M' ||
      b[0x9f] != 'E')
    return false;
  
  // Found LAME tag - extract the start and end offsets
  int iDelay = ((b[0xb1] & 0xFF) << 4) + ((b[0xb2] & 0xF0) >> 4);
  iDelay += 1152; // This header is going to be decoded as a silent frame
  int iPadded = ((b[0xb2] & 0x0F) << 8) + (b[0xb3] & 0xFF);
  m_iFirstSample = iDelay;
  m_iLastSample = iPadded;
  
  /* Don't read replaygain information from here as no other player respects this.
   
   // Now do the ReplayGain stuff
   if (!m_replayGainInfo.iHasGainInfo)
   { // haven't found the gain info - let's test here for it
   BYTE *p = b + 0xA7;
   #define REPLAY_GAIN_RADIO 1
   #define REPLAY_GAIN_AUDIOPHILE 2
   m_replayGainInfo.fTrackPeak = *(float *)p;
   if (m_replayGainInfo.fTrackPeak != 0.0f) m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
   for (int i = 0; i <= 6; i+=2)
   {
   BYTE gainType = (*(p+i) & 0xE0) >> 5;
   BYTE gainFrom = (*(p+i) & 0x1C) >> 2;  // where the gain is from
   if (gainFrom && (gainType == REPLAY_GAIN_RADIO || gainType == REPLAY_GAIN_AUDIOPHILE))
   { // have some replay gain stuff
   int sign = (*(p+i) & 0x02) ? -1 : 1;
   int gainLevel = ((*(p+i) & 0x01) << 8) | (*(p+i+1) & 0xFF);
   gainLevel *= sign;
   if (gainType == REPLAY_GAIN_RADIO)
   {
   m_replayGainInfo.iTrackGain = gainLevel*10;
   m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
   }
   if (gainType == REPLAY_GAIN_AUDIOPHILE)
   {
   m_replayGainInfo.iAlbumGain = gainLevel*10;
   m_replayGainInfo.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
   }
   }
   }
   }*/
  return true;
}

// \brief Check to see if the specified buffer contains an ID3v2 tag header
// \param pBuf Pointer to the buffer to be examined. Must be at least 10 bytes long.
// \param bufLen Size of the buffer pointer to by pBuf. Must be at least 10.
// \return The length of the ID3v2 tag (including the header) if one is present, otherwise 0
unsigned int MP3Codec::IsID3v2Header(unsigned char* pBuf, size_t bufLen)
{
  unsigned int tagLen = 0;
  if (bufLen < 10 || pBuf[0] != 'I' || pBuf[1] != 'D' || pBuf[2] != '3')
    return 0; // Buffer is too small for complete header, or no header signature detected

  // Retrieve the tag size (including this header)
  tagLen = UNSYNC(pBuf[6], pBuf[7], pBuf[8], pBuf[9]) + 10;

  if (pBuf[5] & 0x10) // Header is followed by a footer
    tagLen += 10; //Add footer size

  return tagLen;
}

void MP3Codec::SetOffsets(int iSeekOffsets, const float *offsets)
{
  m_iSeekOffsets = iSeekOffsets;
  delete[] m_SeekOffset;
  m_SeekOffset = NULL;
  if (m_iSeekOffsets <= 0)
    return;
  m_SeekOffset = new float[m_iSeekOffsets + 1];
  for (int i = 0; i <= m_iSeekOffsets; i++)
    m_SeekOffset[i] = offsets[i];
};

int64_t MP3Codec::GetByteOffset(float fTime)
{
  if (!m_iSeekOffsets) return 0;  // no seek info
  if (fTime > m_fTotalDuration)
    fTime = m_fTotalDuration;
  float fOffset = (fTime / m_fTotalDuration) * m_iSeekOffsets;
  int iOffset = (int)floor(fOffset);
  if (iOffset > m_iSeekOffsets-1) iOffset = m_iSeekOffsets - 1;
  float fa = m_SeekOffset[iOffset];
  float fb = m_SeekOffset[iOffset + 1];
  return (int64_t)(fa + (fb - fa) * (fOffset - iOffset));
};

int64_t MP3Codec::GetTimeOffset(int64_t iBytes)
{
  if (!m_iSeekOffsets) return 0;  // no seek info
  float fBytes = (float)iBytes;
  if (fBytes > m_SeekOffset[m_iSeekOffsets])
    fBytes = m_SeekOffset[m_iSeekOffsets];
  if (fBytes < m_SeekOffset[0])
    fBytes = m_SeekOffset[0];
  // run through our byte offsets searching for our times...
  int iOffset = 1;
  while (iOffset < m_iSeekOffsets && fBytes > m_SeekOffset[iOffset])
    iOffset++;
  // iOffset will be the last of the two offsets and will be bigger than 1.
  float fTimeOffset = (float)iOffset - 1 + (fBytes - m_SeekOffset[iOffset - 1])/(m_SeekOffset[iOffset] - m_SeekOffset[iOffset - 1]);
  float fTime = fTimeOffset / m_iSeekOffsets * m_fTotalDuration;
  return (int64_t)(fTime * 1000.0f);
};

CAEChannelInfo MP3Codec::GetChannelInfo()
{
  static enum AEChannel map[2][3] = {
    {AE_CH_FC, AE_CH_NULL},
    {AE_CH_FL, AE_CH_FR  , AE_CH_NULL}
  };

  if (m_Channels > 2)
    return CAEUtil::GuessChLayout(m_Channels);

  return CAEChannelInfo(map[m_Channels - 1]);
}
