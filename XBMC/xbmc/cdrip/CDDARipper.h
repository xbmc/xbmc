#ifndef _CCDDARIPPER_H
#define _CCDDARIPPER_H

#include "..\lib\libcdrip\cdrip.h"
#include "..\MusicInfotag.h"
#include "..\FileItem.h"
#include "Encoder.h"

#define CDDARIP_OK    0
#define CDDARIP_ERR   1
#define CDDARIP_DONE  2

class CCDDARipper
{
public:
	CCDDARipper();
	~CCDDARipper();

	bool        RipTrack(CFileItem* pItem);
	bool        RipCD();

private:
	bool        Init(int iTrack, const char* strFile);
	bool        DeInit();
	int         RipChunk(int& nPercent, int& nPeakValue, int& nJitterErrors, int&	nJitterPos);
	bool        Rip(int iTrack, const char* strFileName, MUSIC_INFO::CMusicInfoTag& infoTag);
	char*       GetExtension(int iEncoder);

	long        m_lBufferSize;
	CEncoder*   m_pEncoder;
	BYTE*       m_pbtStream;
	CDROMPARAMS m_cdParams;
};

#endif // _CCDDARIPPERMP3_H
