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
	virtual ~CCDDARipper();

	bool        RipTrack(CFileItem* pItem);
	bool        RipCD();

private:
	bool        Init(const CStdString& strTrackFile, const CStdString& strFile, MUSIC_INFO::CMusicInfoTag* infoTag = NULL);
	bool        DeInit();
	int         RipChunk(int& nPercent);
	bool        Rip(const CStdString& strTrackFile, const CStdString& strFileName, MUSIC_INFO::CMusicInfoTag& infoTag);
	char*       GetExtension(int iEncoder);

	CEncoder*   m_pEncoder;
	CCDDAReader m_cdReader;
};

#endif // _CCDDARIPPERMP3_H
