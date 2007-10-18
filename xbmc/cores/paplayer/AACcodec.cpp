#include "stdafx.h"
#include "AACcodec.h"


AACCodec::AACCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = "AAC";

  m_Handle=AAC_INVALID_HANDLE;

  m_BufferSize=0;
  m_BufferPos = 0;
  m_Buffer=NULL;

}

AACCodec::~AACCodec()
{
  DeInit();
}

bool AACCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false;

  // setup our callbacks
  AACIOCallbacks callbacks;
  callbacks.userData=this;
  callbacks.Open=AACOpenCallback;
  callbacks.Read=AACReadCallback;
  callbacks.Close=AACCloseCallback;
  callbacks.Filesize=AACFilesizeCallback;
  callbacks.Seek=AACSeekCallback;

  m_Handle=m_dll.AACOpen(strFile.c_str(), callbacks);
  if (m_Handle==AAC_INVALID_HANDLE)
  {
    CLog::Log(LOGERROR,"AACCodec: Unable to open file %s (%s)", strFile.c_str(), m_dll.AACGetErrorMessage());
    return false;
  }

	AACInfo info;
	if (m_dll.AACGetInfo(m_Handle, &info))
	{
    m_Channels = info.channels;
		m_SampleRate = info.samplerate;
		m_BitsPerSample = info.bitspersample;
    m_TotalTime = info.totaltime;
	  m_Bitrate = info.bitrate;

    m_Buffer = new BYTE[AAC_PCM_SIZE*m_Channels*2];

    //  setup codec name
    CStdString strType;
    if (info.objecttype==AAC_MAIN)
      strType="MAIN";
    else if (info.objecttype==AAC_LC)
      strType="LC";
    else if (info.objecttype==AAC_SSR)
      strType="SSR";
    else if (info.objecttype==AAC_LTP)
      strType="LTP";
    else if (info.objecttype==AAC_HE)
      strType="HE";
    else if (info.objecttype==AAC_ER_LC)
      strType="ER-LC";
    else if (info.objecttype==AAC_ER_LTP)
      strType="ER-LTP";
    else if (info.objecttype==AAC_LD)
      strType="LD";
    else if (info.objecttype == ALAC)
      m_CodecName = "ALAC";

    if (!strType.IsEmpty())
      m_CodecName.Format("%s-AAC", strType.c_str());

    //  Get replay gain info
    if (info.replaygain_track_gain)
    {
      m_replayGain.iTrackGain = (int)(atof(info.replaygain_track_gain) * 100 + 0.5);
      m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
    }
    if (info.replaygain_track_peak)
    {
      m_replayGain.fTrackPeak = (float)atof(info.replaygain_track_peak);
      m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
    }
    if (info.replaygain_album_gain)
    {
      m_replayGain.iAlbumGain = (int)(atof(info.replaygain_album_gain) * 100 + 0.5);
      m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
    }
    if (info.replaygain_album_peak)
    {
      m_replayGain.fAlbumPeak = (float)atof(info.replaygain_album_peak);
      m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
    }
	}
	else
	{
    CLog::Log(LOGERROR,"AACCodec: No stream info found in file %s (%s)", strFile.c_str(), m_dll.AACGetErrorMessage());
    return false;
	}
  return true;
}

void AACCodec::DeInit()
{
  if (m_Handle!=AAC_INVALID_HANDLE)
    m_dll.AACClose(m_Handle);

  m_file.Close();

  if (m_Buffer)
    delete[] m_Buffer;
  m_Buffer=NULL;
}

__int64 AACCodec::Seek(__int64 iSeekTime)
{
  if (m_Handle==AAC_INVALID_HANDLE)
    return -1;

  if (!m_dll.AACSeek(m_Handle, (int)iSeekTime))
    return -1;

  m_BufferSize=m_BufferPos=0;

  return iSeekTime;
}

int AACCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
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
    int iAmountRead=m_dll.AACRead(m_Handle, m_Buffer+m_BufferSize, AAC_PCM_SIZE*m_Channels);
    if (iAmountRead==AAC_READ_EOF)
      bEof=true;
    else if (iAmountRead<=AAC_READ_ERROR)
    {
      CLog::Log(LOGERROR, "AACCodec: Unable to read data (%s)", m_dll.AACGetErrorMessage());
      return READ_ERROR;
    } 
    else
      m_BufferSize+=iAmountRead;
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

bool AACCodec::CanInit()
{
  return m_dll.CanLoad();
}

unsigned __int32 AACCodec::AACOpenCallback(const char *pName, const char *mode, void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return 0;

  return codec->m_file.Open(pName, true, READ_CACHED);
}

void AACCodec::AACCloseCallback(void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return;

  codec->m_file.Close();
}

unsigned __int32 AACCodec::AACReadCallback(void *userData, void *pBuffer, unsigned long nBytesToRead)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return 0;

  return codec->m_file.Read(pBuffer, nBytesToRead);
}

__int32 AACCodec::AACSeekCallback(void *userData, unsigned __int64 pos)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return -1;

  codec->m_file.Seek(pos);

  return 0;
}

__int64 AACCodec::AACFilesizeCallback(void *userData)
{
  AACCodec* codec=(AACCodec*) userData;

  if (!codec)
    return 0;

  return codec->m_file.GetLength();
}
