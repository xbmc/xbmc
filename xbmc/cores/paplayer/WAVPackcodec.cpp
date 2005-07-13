#include "../../stdafx.h"
#include "WAVPackCodec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WAVPACK_DLL "Q:\\system\\players\\paplayer\\WAVPack.dll"

WAVPackCodec::WAVPackCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_Bitrate = 0;
  m_CodecName = L"WAVPack";

  m_Handle=NULL;

  m_BufferSize=0;
  m_BufferPos = 0;
  m_Buffer=NULL;
  m_ReadBuffer=NULL;

  // dll stuff
  ZeroMemory(&m_dll, sizeof(WAVPackdll));
  m_bDllLoaded = false;
}

WAVPackCodec::~WAVPackCodec()
{
  DeInit();
  if (m_bDllLoaded)
    CSectionLoader::UnloadDLL(WAVPACK_DLL);
}

bool WAVPackCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_file.Initialize(filecache);

  if (!LoadDLL())
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
  m_Handle=m_dll.WavpackOpenFileInputEx(&m_Callbacks, this, NULL, m_errormsg, OPEN_TAGS | OPEN_2CH_MAX | OPEN_NORMALIZE, 23);
  if (!m_Handle)
  {
    CLog::Log(LOGERROR, "WAVPackCodec: Can't open decoder for %s", strFile.c_str());
    return false;
  }

  m_SampleRate = m_dll.WavpackGetSampleRate(m_Handle);
  m_Channels = m_dll.WavpackGetNumChannels(m_Handle);
  m_BitsPerSample = m_dll.WavpackGetBitsPerSample(m_Handle);
  m_TotalTime = (__int64)(m_dll.WavpackGetNumSamples(m_Handle) * 1000.0 / m_SampleRate); 
  m_Bitrate = (int)m_dll.WavpackGetAverageBitrate(m_Handle, TRUE);

  if (m_SampleRate==0 || m_Channels==0 || m_BitsPerSample==0 || m_TotalTime==0)
  {
    CLog::Log(LOGERROR, "WAVPackCodec: incomplete stream info from %s, SampleRate=%i, Channels=%i, BitsPerSample=%i, TotalTime=%i", strFile.c_str(), m_SampleRate, m_Channels, m_BitsPerSample, m_TotalTime);
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
  int NumSamples=m_dll.WavpackGetNumSamples(m_Handle);
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
	  int tsamples = m_dll.WavpackUnpackSamples(m_Handle, (int32_t*)m_ReadBuffer, 576) * m_dll.WavpackGetReducedChannels(m_Handle);
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

bool WAVPackCodec::LoadDLL()
{
  if (m_bDllLoaded)
    return true;

  DllLoader* pDll=CSectionLoader::LoadDLL(WAVPACK_DLL);

  if (!pDll)
  {
    CLog::Log(LOGERROR, "WAVPackCodec: Unable to load dll %s", WAVPACK_DLL);
    return false;
  }

  pDll->ResolveExport("WavpackOpenFileInputEx", (void**)&m_dll.WavpackOpenFileInputEx);
  pDll->ResolveExport("WavpackOpenFileInput", (void**)&m_dll.WavpackOpenFileInput);
  pDll->ResolveExport("WavpackGetVersion", (void**)&m_dll.WavpackGetVersion);
  pDll->ResolveExport("WavpackUnpackSamples", (void**)&m_dll.WavpackUnpackSamples);
  pDll->ResolveExport("WavpackGetNumSamples", (void**)&m_dll.WavpackGetNumSamples);
  pDll->ResolveExport("WavpackGetSampleIndex", (void**)&m_dll.WavpackGetSampleIndex);
  pDll->ResolveExport("WavpackGetNumErrors", (void**)&m_dll.WavpackGetNumErrors);
  pDll->ResolveExport("WavpackLossyBlocks", (void**)&m_dll.WavpackLossyBlocks);
  pDll->ResolveExport("WavpackSeekSample", (void**)&m_dll.WavpackSeekSample);
  pDll->ResolveExport("WavpackCloseFile", (void**)&m_dll.WavpackCloseFile);
  pDll->ResolveExport("WavpackGetSampleRate", (void**)&m_dll.WavpackGetSampleRate);
  pDll->ResolveExport("WavpackGetBitsPerSample", (void**)&m_dll.WavpackGetBitsPerSample);
  pDll->ResolveExport("WavpackGetBytesPerSample", (void**)&m_dll.WavpackGetBytesPerSample);
  pDll->ResolveExport("WavpackGetNumChannels", (void**)&m_dll.WavpackGetNumChannels);
  pDll->ResolveExport("WavpackGetReducedChannels", (void**)&m_dll.WavpackGetReducedChannels);
  pDll->ResolveExport("WavpackGetMD5Sum", (void**)&m_dll.WavpackGetMD5Sum);
  pDll->ResolveExport("WavpackGetWrapperBytes", (void**)&m_dll.WavpackGetWrapperBytes);
  pDll->ResolveExport("WavpackGetWrapperData", (void**)&m_dll.WavpackGetWrapperData);
  pDll->ResolveExport("WavpackFreeWrapper", (void**)&m_dll.WavpackFreeWrapper);
  pDll->ResolveExport("WavpackGetProgress", (void**)&m_dll.WavpackGetProgress);
  pDll->ResolveExport("WavpackGetFileSize", (void**)&m_dll.WavpackGetFileSize);
  pDll->ResolveExport("WavpackGetRatio", (void**)&m_dll.WavpackGetRatio);
  pDll->ResolveExport("WavpackGetAverageBitrate", (void**)&m_dll.WavpackGetAverageBitrate);
  pDll->ResolveExport("WavpackGetInstantBitrate", (void**)&m_dll.WavpackGetInstantBitrate);
  pDll->ResolveExport("WavpackGetTagItem", (void**)&m_dll.WavpackGetTagItem);
  pDll->ResolveExport("WavpackAppendTagItem", (void**)&m_dll.WavpackAppendTagItem);
  pDll->ResolveExport("WavpackWriteTag", (void**)&m_dll.WavpackWriteTag);

  pDll->ResolveExport("WavpackOpenFileOutput", (void**)&m_dll.WavpackOpenFileOutput);
  pDll->ResolveExport("WavpackSetConfiguration", (void**)&m_dll.WavpackSetConfiguration);
  pDll->ResolveExport("WavpackAddWrapper", (void**)&m_dll.WavpackAddWrapper);
  pDll->ResolveExport("WavpackStoreMD5Sum", (void**)&m_dll.WavpackStoreMD5Sum);
  pDll->ResolveExport("WavpackPackInit", (void**)&m_dll.WavpackPackInit);
  pDll->ResolveExport("WavpackPackSamples", (void**)&m_dll.WavpackPackSamples);
  pDll->ResolveExport("WavpackFlushSamples", (void**)&m_dll.WavpackFlushSamples);
  pDll->ResolveExport("WavpackUpdateNumSamples", (void**)&m_dll.WavpackUpdateNumSamples);
  pDll->ResolveExport("WavpackGetWrapperLocation", (void**)&m_dll.WavpackGetWrapperLocation);

  // Check resolves
  if (!m_dll.WavpackGetVersion || !m_dll.WavpackUnpackSamples || !m_dll.WavpackGetNumSamples || 
      !m_dll.WavpackGetSampleIndex || !m_dll.WavpackGetNumErrors || !m_dll.WavpackLossyBlocks || 
      !m_dll.WavpackSeekSample || !m_dll.WavpackCloseFile || !m_dll.WavpackGetSampleRate || 
      !m_dll.WavpackGetBitsPerSample ||   !m_dll.WavpackGetBytesPerSample || !m_dll.WavpackGetNumChannels || 
      !m_dll.WavpackGetReducedChannels || !m_dll.WavpackGetMD5Sum || !m_dll.WavpackGetWrapperBytes || 
      !m_dll.WavpackGetWrapperData || !m_dll.WavpackFreeWrapper || !m_dll.WavpackGetProgress || 
      !m_dll.WavpackGetFileSize || !m_dll.WavpackGetRatio || !m_dll.WavpackGetAverageBitrate || 
      !m_dll.WavpackGetInstantBitrate || !m_dll.WavpackGetTagItem || !m_dll.WavpackAppendTagItem || 
      !m_dll.WavpackWriteTag || !m_dll.WavpackOpenFileOutput || !m_dll.WavpackSetConfiguration || 
      !m_dll.WavpackAddWrapper || !m_dll.WavpackStoreMD5Sum || !m_dll.WavpackPackInit || 
      !m_dll.WavpackPackSamples || !m_dll.WavpackFlushSamples || !m_dll.WavpackUpdateNumSamples || 
      !m_dll.WavpackOpenFileInputEx || !m_dll.WavpackOpenFileInput || !m_dll.WavpackGetWrapperLocation) 
  {
    CLog::Log(LOGERROR, "WAVPackCodec: Unable to resolve exports from %s", WAVPACK_DLL);
    CSectionLoader::UnloadDLL(WAVPACK_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}

bool WAVPackCodec::CanInit()
{
  return CFile::Exists(WAVPACK_DLL);
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
      dst [0] = 0;
      dst [1] = (BYTE)(temp & 0xFF);
      dst [2] = (BYTE)(temp >> 8);
      dst [3] = (BYTE)(temp >> 16);
      dst += 4;
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
