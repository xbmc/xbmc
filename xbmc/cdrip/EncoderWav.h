#ifndef _ENCODERWAV_H
#define _ENCODERWAV_H

#include "Encoder.h"
#include <stdio.h>

typedef struct {
	BYTE  riff[4];         /* must be "RIFF"    */
	DWORD len;             /* #bytes + 44 - 8   */
	BYTE  cWavFmt[8];      /* must be "WAVEfmt " */
	DWORD dwHdrLen;
	WORD  wFormat;
	WORD  wNumChannels;
	DWORD dwSampleRate;
	DWORD dwBytesPerSec;
	WORD  wBlockAlign;
	WORD  wBitsPerSample;
	BYTE  cData[4];        /* must be "data"   */
	DWORD dwDataLen;       /* #bytes           */
} WAVHDR, *PWAVHDR, *LPWAVHDR;

class CEncoderWav : public CEncoder
{
public:
	CEncoderWav();
	~CEncoderWav();
	bool               Init(const char* strFile);
	int                Encode(int nNumBytesRead, BYTE* pbtStream);
	bool               Close();
	void               AddTag(int key,const char* value);

private:
	bool               WriteWavHeader();

	HANDLE             m_hFile;
	int                m_iChannels;
	int                m_iSampleRate;
	int                m_iBitsPerSample;
	int                m_iBytesWritten;
};

#endif // _ENCODERWAV_H