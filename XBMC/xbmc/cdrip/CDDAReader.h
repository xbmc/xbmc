#ifndef _CCDDAREADER_H
#define _CCDDAREADER_H

#define CDDARIP_OK    0
#define CDDARIP_ERR   1
#define CDDARIP_DONE  2

#include "..\utils\thread.h"
#include "..\lib\libcdrip\cdrip.h"

struct RipBuffer{
	int iRipError;
	long lBytesRead;
	BYTE* pbtStream;
	HANDLE hEvent;
};

class CCDDAReader : public CThread
{
public:
	CCDDAReader();
	~CCDDAReader();
	int         GetData(BYTE** stream, long& lBytes);
	bool        Init(int iTrack);
	bool        DeInit();
	int					GetPercent();
protected:
	void        Process();
	int         ReadChunk();

	int         m_iPercent;
	long        m_lBufferSize;

	RipBuffer   m_sRipBuffer[2]; // hold space for 2 buffers
	int					m_iCurrentBuffer;   // 0 or 1

	HANDLE			m_hReadEvent;       // data is fetched
	HANDLE			m_hDataReadyEvent;  // data is ready to be fetched
	HANDLE			m_hStopEvent;       // stop event

	bool				m_iInitialized;

	CDROMPARAMS m_cdParams;
};

#endif // _CCDDAREADER_H
