#include "../../stdafx.h"
#include "WAVCodec.h"

typedef struct wav_header{ /* RIFF */
 char riff[4];         /* "RIFF" (4 bytes) */
 long TotLen;          /* File length - 8 bytes  (4 bytes) */
 char wavefmt[8];      /* "WAVEfmt "  (8 bytes) */
 long Len;             /* Remaining length  (4 bytes) */
 short format_tag;     /* Tag (1 = PCM) (2 bytes) */
 short channels;       /* Mono=1 Stereo=2 (2 bytes) */
 long SamplesPerSec;   /* No samples/sec (4 bytes) */
 long AvgBytesPerSec;  /* Average bytes/sec (4 bytes) */
 short BlockAlign;     /* Block align (2 bytes) */
 short FormatSpecific; /* 8 or 16 bit (2 bytes) */
 char data[4];         /* "data" (4 bytes) */
 long datalen;         /* Raw data length (4 bytes) */
 /* ..data follows. Total header size = 44 bytes */
} wav_header_s;

WAVCodec::WAVCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_iDataLen=0;
}

WAVCodec::~WAVCodec()
{
  DeInit();
}

bool WAVCodec::Init(const CStdString &strFile)
{
  if (!m_file.Open(strFile.c_str()))
    return false;

  wav_header_s wavHeader;
  ZeroMemory(&wavHeader, sizeof(wav_header_s));

  if (m_file.Read(&wavHeader, sizeof(wav_header_s))<=0)
    return false;

  //  Valid wav file and PCM format?
  if (strcmp(wavHeader.riff, "RIFF")!=0 && wavHeader.format_tag!=1)
    return false;

  //  Get file info
  m_SampleRate = wavHeader.SamplesPerSec;
  m_Channels = wavHeader.channels;
  m_BitsPerSample = wavHeader.FormatSpecific;
  m_iDataLen = wavHeader.datalen;
  m_TotalTime = m_iDataLen/(m_SampleRate*m_Channels*(m_BitsPerSample/8))*1000;

  if (m_SampleRate==0 || m_Channels==0 || m_BitsPerSample==0 || m_TotalTime==0)
    return false;

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
  m_file.Seek(sizeof(wav_header_s)+((iSeekTime/1000)*iSampleSize));

  return iSeekTime;
}

int WAVCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize=0;
  int iPos=(int)m_file.GetPosition();
  if (iPos == m_iDataLen)
    return READ_EOF;

  int iSize=size;
  if (iPos+size>m_iDataLen)
  {
    iSize=m_iDataLen-iPos;
    iSize-=iSize%2;
  }

  int iAmountRead=m_file.Read(pBuffer, iSize);
  if (iAmountRead>0)
  {
    *actualsize=iAmountRead;
    return READ_SUCCESS;
  }

  return READ_ERROR;
}

bool WAVCodec::HandlesType(const char *type)
{
  return ( strcmp(type, "wav") == 0 );
}
