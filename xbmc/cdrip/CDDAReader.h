#ifndef _CCDDAREADER_H
#define _CCDDAREADER_H

#define CDDARIP_OK    0
#define CDDARIP_ERR   1
#define CDDARIP_DONE  2

#include "..\utils\thread.h"
#include "../lib/libcdio/cdio.h"

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
	virtual ~CCDDAReader();
	int         GetData(BYTE** stream, long& lBytes);
	bool        Init(const char* strFileName);
	bool        DeInit();
	int					GetPercent();
protected:
	void        Process();
	int         ReadChunk();

	long        m_lBufferSize;

	RipBuffer   m_sRipBuffer[2]; // hold space for 2 buffers
	int					m_iCurrentBuffer;   // 0 or 1

	HANDLE			m_hReadEvent;       // data is fetched
	HANDLE			m_hDataReadyEvent;  // data is ready to be fetched
	HANDLE			m_hStopEvent;       // stop event

	bool				m_iInitialized;

	CFile				m_fileCdda;
};

#endif // _CCDDAREADER_H
