#include "stdafx.h"
#include "SHNcodec.h"


// Callbacks for file reading
int Shn_Callback_Read(ShnPlayStream * stream, void * buffer, int bytes, int * bytes_read)
{
	ShnPlayFileStream *filestream = (ShnPlayFileStream *)stream;
  if (!filestream || !buffer || !filestream->file) return 0;
//  CLog::Log(LOGERROR, "Reading from SHN dll - stream @ %x, - file @ %x", (int)stream, filestream->file);
  int amountread = (int)filestream->file->Read(buffer, bytes);
  if (bytes_read)
    *bytes_read = amountread;
	if (amountread == bytes || filestream->file->GetPosition() == filestream->file->GetLength())
		return 1;
	return 0;
}

int Shn_Callback_Seek(ShnPlayStream * stream, int position)
{
	ShnPlayFileStream *filestream = (ShnPlayFileStream *)stream;
  if (!filestream || !filestream->file) return 0;

  __int64 seek = (int)filestream->file->Seek(position, SEEK_SET);
  if (seek >= 0)
    return 1;
  return 0;
}

int Shn_Callback_CanSeek(ShnPlayStream * stream)
{
  return 1;
}

int Shn_Callback_GetLength(ShnPlayStream * stream)
{
	ShnPlayFileStream *filestream = (ShnPlayFileStream *)stream;
  if (!filestream || !filestream->file) return 0;
  return (int)filestream->file->GetLength();
}

int Shn_Callback_GetPosition(ShnPlayStream * stream)
{
	ShnPlayFileStream *filestream = (ShnPlayFileStream *)stream;
  if (!filestream || !filestream->file) return 0;
  int position = (int)filestream->file->GetPosition();
	if (position >= 0)
		return position;
	return -1;
}

// SHNCodec class

SHNCodec::SHNCodec()
{
  m_SampleRate = 0;
  m_Channels = 0;
  m_BitsPerSample = 0;
  m_TotalTime = 0;
  m_Bitrate = 0;
  m_CodecName = "SHN";

  m_handle = NULL;
}

SHNCodec::~SHNCodec()
{
  DeInit();
}

bool SHNCodec::Init(const CStdString &strFile, unsigned int filecache)
{
  if (!m_dll.Load())
    return false;

  if (!m_file.Open(strFile,true, READ_CACHED))
    return false;

  // setup our callbacks
  m_stream.file = &m_file;
  m_stream.vtbl.Read = Shn_Callback_Read;
  m_stream.vtbl.Seek = Shn_Callback_Seek;
  m_stream.vtbl.CanSeek = Shn_Callback_CanSeek;
  m_stream.vtbl.GetLength = Shn_Callback_GetLength;
  m_stream.vtbl.GetPosition = Shn_Callback_GetPosition;

  if (!m_dll.OpenStream(&m_handle, (ShnPlayStream *)&m_stream, 0) || !m_handle)
  {
    CLog::Log(LOGERROR,"SHNCodec: Unable to open file %s (%s)", strFile.c_str(), m_handle ? m_dll.ErrorMessage(m_handle) : "Invalid handle");
    return false;
  }

	ShnPlayInfo info;
	if (m_dll.GetInfo(m_handle, &info))
	{
		m_Channels = info.channels;
		m_SampleRate = info.sample_rate;
		m_BitsPerSample = info.bits_per_sample;
    m_TotalTime = (__int64)info.sample_count * 1000 / info.sample_rate;
	  m_Bitrate = (int)(m_file.GetLength() * 8 / (m_TotalTime / 1000));
	}
	else
	{
    CLog::Log(LOGERROR,"SHNCodec: No stream info found in file %s (%s)", strFile.c_str(), m_dll.ErrorMessage(m_handle));
    return false;
	}
  return true;
}

void SHNCodec::DeInit()
{
  if (m_handle)
    m_dll.Close(m_handle);
  m_handle = NULL;

  m_file.Close();
  if (m_stream.file)
    m_stream.file = NULL;
}

__int64 SHNCodec::Seek(__int64 iSeekTime)
{
  if (!m_handle)
    return -1;
  int sample = (int)(iSeekTime * m_SampleRate / 1000);
  if (!m_dll.Seek(m_handle, sample))
    return -1;
  return iSeekTime;
}
#define BLOCK_READ_SIZE 588*4  // read 4 frames of samples at a time

static int total_samples = 0;
int SHNCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
  if (!m_handle)
    return READ_ERROR;

  int samplesToRead = min(size * 8 / m_BitsPerSample / m_Channels, BLOCK_READ_SIZE);
  int samplesRead = 0;
  if (m_dll.Read(m_handle, pBuffer, samplesToRead, &samplesRead))
  {
    if (actualsize)
      *actualsize = samplesRead * m_Channels * m_BitsPerSample / 8;
    total_samples += samplesRead;
    if (samplesRead == samplesToRead)
      return READ_SUCCESS;
    else
      return READ_EOF;
  }
  return READ_ERROR;
}

bool SHNCodec::CanInit()
{
  return m_dll.CanLoad();
}
