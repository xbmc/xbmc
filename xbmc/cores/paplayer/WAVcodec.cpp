#include "stdafx.h"
#include "WAVcodec.h"

typedef struct
{
  char chunk_id[4];
  long chunksize;
} WAVE_CHUNK;

typedef struct
{
  char riff[4];
  long filesize;
  char rifftype[4];
} WAVE_RIFFHEADER;

WAVCodec::WAVCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_iDataStart=0;
  m_iDataLen=0;
  m_Bitrate = 0;
  m_CodecName = "WAV";
}

WAVCodec::~WAVCodec()
{
  DeInit();
}

bool WAVCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_file.Open(strFile, true, READ_CACHED))
    return false;

  // read header
  WAVE_RIFFHEADER riffh;
  m_file.Read(&riffh, sizeof(WAVE_RIFFHEADER));

  // file valid?
  if (strncmp(riffh.riff, "RIFF", 4)!=0 && strncmp(riffh.rifftype, "WAVE", 4)!=0)
    return false;

  long offset=0;
  offset += sizeof(WAVE_RIFFHEADER);
  offset -= sizeof(WAVE_CHUNK);

  // parse chunks
  do
  {
    WAVE_CHUNK chunk;

    // always seeking to the start of a chunk
    m_file.Seek(offset + sizeof(WAVE_CHUNK), SEEK_SET);
    m_file.Read(&chunk, sizeof(WAVE_CHUNK));

    if (!strncmp(chunk.chunk_id, "fmt ", 4))
    { // format chunk
      WAVEFORMATEX wfx;
      memset(&wfx, 0, sizeof(WAVEFORMATEX));
      m_file.Read(&wfx, 16);

      //  Get file info
      m_SampleRate = wfx.nSamplesPerSec;
      m_Channels = wfx.nChannels;
      m_BitsPerSample = wfx.wBitsPerSample;

      // we only need 16 bytes of the fmt chunk
      if (chunk.chunksize-16>0)
        m_file.Seek(chunk.chunksize-16, SEEK_CUR);
    }
    else if (!strncmp(chunk.chunk_id, "data", 4))
    { // data chunk
      m_iDataStart=(long)m_file.GetPosition();
      m_iDataLen=chunk.chunksize;

      if (chunk.chunksize & 1)
        offset++;
    }
    else
    { // other chunk - unused, just skip
      m_file.Seek(chunk.chunksize, SEEK_CUR);
    }

    offset+=(chunk.chunksize+sizeof(WAVE_CHUNK));

    if (offset & 1)
      offset++;

  } while (offset+(int)sizeof(WAVE_CHUNK) < riffh.filesize);

  m_TotalTime = (int)(((float)m_iDataLen/(m_SampleRate*m_Channels*(m_BitsPerSample/8)))*1000);
  m_Bitrate = (int)(((float)m_iDataLen * 8) / ((float)m_TotalTime / 1000));

  if (m_SampleRate==0 || m_Channels==0 || m_BitsPerSample==0 || m_TotalTime==0 || m_iDataStart==0 || m_iDataLen==0)
    return false;

  //  Seek to the start of the data chunk
  m_file.Seek(m_iDataStart);

  return true;
}

void WAVCodec::DeInit()
{
  m_file.Close();
}

__int64 WAVCodec::Seek(__int64 iSeekTime)
{
  //  Calculate size of a single sample of the file
  int iSampleSize=m_SampleRate*m_Channels*(m_BitsPerSample/8);

  //  Seek to the position in the file
  m_file.Seek(m_iDataStart+((iSeekTime/1000)*iSampleSize));

  return iSeekTime;
}

int WAVCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;
  int iPos=(int)m_file.GetPosition();
  if (iPos >= m_iDataStart+m_iDataLen)
    return READ_EOF;

  int iAmountRead=m_file.Read(pBuffer, size);
  if (iAmountRead>0)
  {
    *actualsize=iAmountRead;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool WAVCodec::CanInit()
{
  return true;
}
