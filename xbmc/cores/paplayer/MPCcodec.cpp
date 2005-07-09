#include "../../stdafx.h"
#include "MPCCodec.h"

// Callbacks for file reading
int Mpc_Callback_Read(MpcPlayStream * stream, void * buffer, int bytes, int * bytes_read)
{
	MpcPlayFileStream *filestream = (MpcPlayFileStream *)stream;
  if (!filestream || !buffer || !filestream->file) return 0;
//  CLog::Log(LOGERROR, "Reading from MPC dll - stream @ %x, - file @ %x", (int)stream, filestream->file);
  int amountread = (int)filestream->file->Read(buffer, bytes);
  if (bytes_read)
    *bytes_read = amountread;
	if (amountread == bytes || filestream->file->GetPosition() == filestream->file->GetLength())
		return 1;
	return 0;
}

int Mpc_Callback_Seek(MpcPlayStream * stream, int position)
{
	MpcPlayFileStream *filestream = (MpcPlayFileStream *)stream;
  if (!filestream || !filestream->file) return 0;

  __int64 seek = (int)filestream->file->Seek(position, SEEK_SET);
  if (seek >= 0)
    return 1;
  return 0;
}

int Mpc_Callback_CanSeek(MpcPlayStream * stream)
{
  return 1;
}

int Mpc_Callback_GetLength(MpcPlayStream * stream)
{
	MpcPlayFileStream *filestream = (MpcPlayFileStream *)stream;
  if (!filestream || !filestream->file) return 0;
  return (int)filestream->file->GetLength();
}

int Mpc_Callback_GetPosition(MpcPlayStream * stream)
{
	MpcPlayFileStream *filestream = (MpcPlayFileStream *)stream;
  if (!filestream || !filestream->file) return 0;
  int position = (int)filestream->file->GetPosition();
	if (position >= 0)
		return position;
	return -1;
}

MPCCodec::MPCCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = L"MPC";

  // dll stuff
  m_bDllLoaded = false;
  ZeroMemory(&m_dll, sizeof(MPCdll));
  m_sampleBufferSize = 0;
  m_handle = NULL;
}

MPCCodec::~MPCCodec()
{
  DeInit();
  if (m_bDllLoaded)
    CSectionLoader::UnloadDLL(MPC_DLL);
}

bool MPCCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  m_file.Initialize(filecache);

  if (!LoadDLL())
    return false;

  if (!m_file.Open(strFile))
    return false;

  // setup our callbacks
  m_stream.file = &m_file;
  m_stream.vtbl.Read = Mpc_Callback_Read;
  m_stream.vtbl.Seek = Mpc_Callback_Seek;
  m_stream.vtbl.CanSeek = Mpc_Callback_CanSeek;
  m_stream.vtbl.GetLength = Mpc_Callback_GetLength;
  m_stream.vtbl.GetPosition = Mpc_Callback_GetPosition;

  StreamInfo::BasicData data;
  double timeinseconds = 0.0;
  if (!m_dll.Open(&m_handle, (MpcPlayStream *)&m_stream, &data, &timeinseconds))
    return false;

  m_TotalTime = (__int64)(timeinseconds * 1000.0 + 0.5);
  m_BitsPerSample = 16;
	m_Channels = 2;
  m_SampleRate = (int)data.SampleFreq;

  m_Bitrate = data.Bitrate;
  if (m_Bitrate == 0)
  {
	  m_Bitrate = (int)data.AverageBitrate;
  }
  if (m_Bitrate == 0)
  {
	  m_Bitrate = (int)((data.TotalFileLength * 8) / (m_TotalTime / 1000));
  }

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

  return true;
}

void MPCCodec::DeInit()
{
  if (m_handle)
    m_dll.Close(m_handle);
  m_handle = NULL;

  m_file.Close();
  if (m_stream.file)
    m_stream.file = NULL;
}

__int64 MPCCodec::Seek(__int64 iSeekTime)
{
  if (!m_handle)
    return -1;
  if (!m_dll.Seek(m_handle, (double)iSeekTime/1000.0))
    return -1;
  return iSeekTime;
}

int MPCCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (!m_handle)
    return READ_ERROR;

  short *pShort = (short *)pBuffer;
  *actualsize = 0;
  // start by emptying out our frame buffer
  int copied = 0;
  for (copied = 0; copied < m_sampleBufferSize && copied < size / 2; copied++)
  {
    float sample = m_sampleBuffer[copied]*32768.0f + 0.5f;
    if (sample > 32767.0f) sample = 32767.0f;
    if (sample < -32768.0f) sample = -32768.0f;
    *pShort++ = (short)sample;
  }
  size -= 2 * copied;
  m_sampleBufferSize -= copied;
  *actualsize = 2 * copied;
  if (m_sampleBufferSize)
  { // didn't require as much as was in our sample buffer - copy data down and return.
    memmove(m_sampleBuffer, &m_sampleBuffer[copied], m_sampleBufferSize * sizeof(float));
    return READ_SUCCESS;
  }
  // emptied our sample buffer - let's fill it up again
  int ret = m_dll.Read(m_handle, m_sampleBuffer, FRAMELEN * 2);
  if (ret == -2)
    return READ_EOF;
  if (ret == -1)
    return READ_ERROR;
  // have valid float data - let's convert back to normal 16bit info
  // (FIXME - should transfer directly for better quality)
  copied = 0;
  ASSERT(ret <= FRAMELEN * 2);
  for (copied = 0; copied < ret * 2 && copied < size / 2; copied++)
  {
    float sample = m_sampleBuffer[copied]*32768.0f + 0.5f;
    if (sample > 32767.0f) sample = 32767.0f;
    if (sample < -32768.0f) sample = -32768.0f;
    *pShort++ = (short)sample;
  }
  *actualsize += copied * 2;
  m_sampleBufferSize = ret * 2 - copied;
  if (m_sampleBufferSize)
  { // copy data down
    memmove(m_sampleBuffer, &m_sampleBuffer[copied], m_sampleBufferSize * sizeof(float));
  }
  return READ_SUCCESS;
}

bool MPCCodec::CanInit()
{
  return CFile::Exists(MPC_DLL);
}

bool MPCCodec::LoadDLL()
{
  if (m_bDllLoaded)
    return true;

  DllLoader* pDll = CSectionLoader::LoadDLL(MPC_DLL);
  if (!pDll)
  {
    CLog::Log(LOGERROR, "MPCCodec: Unable to load dll %s", MPC_DLL);
    return false;
  }

  // get handle to the functions in the dll
  pDll->ResolveExport("Open", (void**)&m_dll.Open);
  pDll->ResolveExport("Close", (void**)&m_dll.Close);
  pDll->ResolveExport("Read", (void**)&m_dll.Read);
  pDll->ResolveExport("Seek", (void**)&m_dll.Seek);

  // Check resolves + version number
  if (!m_dll.Open || !m_dll.Close || !m_dll.Read || !m_dll.Seek)
  {
    CLog::Log(LOGERROR, "MPCCodec: Unable to resolve exports from %s", MPC_DLL);
    CSectionLoader::UnloadDLL(MPC_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}
