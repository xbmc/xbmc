#include "../../stdafx.h"
#include "MPCCodec.h"

MPCCodec::MPCCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime = 0;
  // dll stuff
  m_pDll = NULL;
  m_bDllLoaded = false;
  ZeroMemory(&m_dll, sizeof(MPCdll));
}

MPCCodec::~MPCCodec()
{
  DeInit();
}

bool MPCCodec::Init(const CStdString &strFile)
{
  if (!LoadDLL())
    return false;

  StreamInfo::BasicData data;
  double timeinseconds = 0.0;
  if (!m_dll.Open(strFile.c_str(), &data, &timeinseconds))
    return false;

  m_TotalTime = (__int64)(timeinseconds * 1000.0 + 0.5);
  m_BitsPerSample = 16;
	m_Channels = 2;
  m_SampleRate = (int)data.SampleFreq;

  // Replay gain
  if (data.GainTitle || data.PeakTitle)
  {
		m_replayGain.iTrackGain = data.GainTitle;
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
		if (data.PeakTitle)
    {
      m_replayGain.fTrackPeak = data.PeakTitle / 32768.0f;
      m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
    }
	}

	if (data.GainAlbum || data.PeakAlbum)
  {
		m_replayGain.iAlbumGain = data.GainAlbum;
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
		if (data.PeakAlbum)
    {
      m_replayGain.fAlbumPeak = data.PeakAlbum / 32768.0f;
      m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
    }
	}

//  delete info;

  return true;
}

void MPCCodec::DeInit()
{
  m_dll.Close();
}

__int64 MPCCodec::Seek(__int64 iSeekTime)
{
  if (!m_dll.Seek((double)iSeekTime/1000.0))
    return -1;
  return iSeekTime;
}

int MPCCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  *actualsize = 0;
  int ret = m_dll.Read( sample_buffer, size / 2);
  if (ret == -2)
    return READ_EOF;
  if (ret == -1)
    return READ_ERROR;
  // have valid float data - let's convert back to normal 16bit info
  // (FIXME - should transfer directly for better quality)
  short *pShort = (short *)pBuffer;
  for (int i=0; i < ret * 2 && i < FRAMELEN * 2; i++)
  {
    float sample = sample_buffer[i]*32786.0f + 0.5f;
    if (sample > 32767.0f) sample = 32767.0f;
    if (sample < -32768.0f) sample = -32768.0f;
    *pShort++ = (short)sample;
  }
  *actualsize = ret * 2 * 2;
  return READ_SUCCESS;
}

bool MPCCodec::HandlesType(const char *type)
{
  return ( strcmp(type, "mpc") == 0 || strcmp(type, "mp+") == 0 || strcmp(type, "mpp") == 0);
}

bool MPCCodec::LoadDLL()
{
  if (m_bDllLoaded)
    return true;
  m_pDll = new DllLoader(MPC_DLL, true);
  if (!m_pDll)
  {
    CLog::Log(LOGERROR, "MPCCodec: Unable to load dll %s", MPC_DLL);
    return false;
  }
  if (!m_pDll->Parse())
  {
    // failed,
    CLog::Log(LOGERROR, "MPCCodec: Unable to load dll %s", MPC_DLL);
    delete m_pDll;
    m_pDll = NULL;
    return false;
  }
  m_pDll->ResolveImports();

  // get handle to the functions in the dll
  m_pDll->ResolveExport("Open", (void**)&m_dll.Open);
  m_pDll->ResolveExport("Close", (void**)&m_dll.Close);
  m_pDll->ResolveExport("Read", (void**)&m_dll.Read);
  m_pDll->ResolveExport("Seek", (void**)&m_dll.Seek);

  // Check resolves + version number
  if (!m_dll.Open || !m_dll.Close || !m_dll.Read || !m_dll.Seek)
  {
    CLog::Log(LOGERROR, "MPCCodec: Unable to load our dll %s", MPC_DLL);
    delete m_pDll;
    m_pDll = NULL;
    return false;
  }

  m_bDllLoaded = true;
  return true;
}
