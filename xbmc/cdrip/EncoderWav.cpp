#include <xtl.h>
#include "EncoderWav.h"
#include "..\utils\log.h"

CEncoderWav::CEncoderWav()
{
	m_iBytesWritten = 0;

	m_iChannels = 2;
	m_iSampleRate = 44100;
	m_iBitsPerSample = 16;
}

CEncoderWav::~CEncoderWav()
{
	if (m_hFile != INVALID_HANDLE_VALUE) CloseHandle(m_hFile);
}

bool CEncoderWav::Init(const char* strFile)
{
	m_iBytesWritten = 0;

	m_hFile = CreateFile(strFile, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL |	FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if(m_hFile == INVALID_HANDLE_VALUE)
	{
		CLog::Log("Error: Cannot open file: %s", strFile);
		return false;
	}

	// write dummy header file
	WAVHDR dummyheader;
	DWORD dwBytesWritten;
	memset(&dummyheader, 0, sizeof(dummyheader));
	WriteFile(m_hFile, &dummyheader, sizeof(dummyheader), &dwBytesWritten, NULL);

	return true;
}

int CEncoderWav::Encode(int nNumBytesRead, BYTE* pbtStream)
{
	// write stream to file (no conversion needed at this time)
	DWORD dwBytesWritten;
	if (!WriteFile(m_hFile, pbtStream, nNumBytesRead, &dwBytesWritten, NULL))
	{ 
		CLog::Log("Error writing buffer to file");
		return 0;
	}

	m_iBytesWritten += nNumBytesRead;
	return 1;
}

bool CEncoderWav::Close()
{
	WriteWavHeader();
	CloseHandle(m_hFile);
	return true;
}

void CEncoderWav::AddTag(int key,const char* value)
{
	// wave does not support tag's, just return
	return;
}

bool CEncoderWav::WriteWavHeader()
{
	WAVHDR wav;
	int bps=1;

	if (m_hFile == INVALID_HANDLE_VALUE) return false;

	memcpy(wav.riff, "RIFF", 4);
	wav.len = m_iBytesWritten + 44 - 8;
	memcpy(wav.cWavFmt, "WAVEfmt ", 8);
	wav.dwHdrLen = 16;
	wav.wFormat = WAVE_FORMAT_PCM;
	wav.wNumChannels = m_iChannels;
	wav.dwSampleRate = m_iSampleRate;
	wav.wBitsPerSample = m_iBitsPerSample;
	if (wav.wBitsPerSample == 16) bps = 2;
	wav.dwBytesPerSec = m_iBitsPerSample * m_iChannels * bps;
	wav.wBlockAlign = 4;
	memcpy(wav.cData, "data", 4);
	wav.dwDataLen = m_iBytesWritten;

	DWORD dwBytesWritten;
	SetFilePointer(m_hFile, NULL, NULL, FILE_BEGIN);
	WriteFile(m_hFile, &wav, sizeof(wav), &dwBytesWritten, NULL);

	return true;
}
