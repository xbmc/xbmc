#ifndef _ENCODERLAME_H
#define _ENCODERLAME_H

#include "Encoder.h"
#include "..\lib\liblame\lame.h"

// merge sections from liblame and liblamed
#pragma comment(linker, "/merge:LAME_TEX=LIBLAME")
#pragma comment(linker, "/merge:LAME_DAT=LIBLAME")
#pragma comment(linker, "/merge:LAME_BSS=LIBLAME")
// we have a problem here, SECTION loading doesn't work ok when using stringpooling,
// this is disabled for all projects in xbmc now. But it seemes the same happens for
// static doubles. These crash xbmc to. Only solution for now is to prevent this section
// from merging into LIBLAME
//#pragma comment(linker, "/merge:LAME_RDA=LIBLAME")
#pragma comment(linker, "/section:LIBLAME,RWE")

class CEncoderLame : public CEncoder
{
public:
	CEncoderLame();
	bool               Init(const char* strFile);
	int                Encode(int nNumBytesRead, BYTE* pbtStream);
	bool               Close();
	void               AddTag(int key,const char* value);

private:
	lame_global_flags* m_pGlobalFlags;
	FILE*              m_pFile;
	unsigned char      m_buffer[48160]; // mp3buf_size in bytes = 1.25*(chunk size / 4) + 7200
	char               m_inPath[MAX_PATH + 1];
	char               m_outPath[MAX_PATH + 1];
};

#endif // _ENCODERLAME_H