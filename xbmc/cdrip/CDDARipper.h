#ifndef _CCDDARIPPER_H
#define _CCDDARIPPER_H

#include "CDDAReader.h"
#include "..\MusicInfotag.h"
#include "..\FileItem.h"
#include "Encoder.h"

class CCDDARipper
{
public:
	CCDDARipper();
	~CCDDARipper();

	bool        RipTrack(CFileItem* pItem);
	bool        RipCD();

private:
	bool        Init(int iTrack, const char* strFile, MUSIC_INFO::CMusicInfoTag* infoTag = NULL);
	bool        DeInit();
	int         RipChunk(int& nPercent);
	bool        Rip(int iTrack, const char* strFileName, MUSIC_INFO::CMusicInfoTag& infoTag);
	char*       GetExtension(int iEncoder);

	CEncoder*   m_pEncoder;
	CCDDAReader m_cdReader;
};

#endif // _CCDDARIPPERMP3_H
