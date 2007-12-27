#include "stdafx.h"
#include "EncoderWav.h"


CEncoderWav::CEncoderWav()
{
  m_iBytesWritten = 0;
}

CEncoderWav::~CEncoderWav()
{
  FileClose();
}

bool CEncoderWav::Init(const char* strFile, int iInChannels, int iInRate, int iInBits)
{
  m_iBytesWritten = 0;

  // we only accept 2 / 44100 / 16 atm
  if (iInChannels != 2 || iInRate != 44100 || iInBits != 16) return false;

  // set input stream information and open the file
  if (!CEncoder::Init(strFile, iInChannels, iInRate, iInBits)) return false;

  // write dummy header file
  WAVHDR dummyheader;
  memset(&dummyheader, 0, sizeof(dummyheader));
  FileWrite(&dummyheader, sizeof(dummyheader));

  return true;
}

int CEncoderWav::Encode(int nNumBytesRead, BYTE* pbtStream)
{
  // write stream to file (no conversion needed at this time)
  if (FileWrite(pbtStream, nNumBytesRead) == -1)
  {
    CLog::Log(LOGERROR, "Error writing buffer to file");
    return 0;
  }

  m_iBytesWritten += nNumBytesRead;
  return 1;
}

bool CEncoderWav::Close()
{
  WriteWavHeader();
  FileClose();
  return true;
}

bool CEncoderWav::WriteWavHeader()
{
  WAVHDR wav;
  int bps = 1;

  if (m_hFile == INVALID_HANDLE_VALUE) return false;

  memcpy(wav.riff, "RIFF", 4);
  wav.len = m_iBytesWritten + 44 - 8;
  memcpy(wav.cWavFmt, "WAVEfmt ", 8);
  wav.dwHdrLen = 16;
  wav.wFormat = WAVE_FORMAT_PCM;
  wav.wNumChannels = m_iInChannels;
  wav.dwSampleRate = m_iInSampleRate;
  wav.wBitsPerSample = m_iInBitsPerSample;
  if (wav.wBitsPerSample == 16) bps = 2;
  wav.dwBytesPerSec = m_iInBitsPerSample * m_iInChannels * bps;
  wav.wBlockAlign = 4;
  memcpy(wav.cData, "data", 4);
  wav.dwDataLen = m_iBytesWritten;

  // write header to beginning of stream
  SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN);
  FileWrite(&wav, sizeof(wav));

  return true;
}
