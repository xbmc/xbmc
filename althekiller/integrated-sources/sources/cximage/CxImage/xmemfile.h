#if !defined(__xmemfile_h)
#define __xmemfile_h

#include "xfile.h"

//////////////////////////////////////////////////////////
class DLL_EXP CxMemFile : public CxFile
	{
public:
	CxMemFile(BYTE* pBuffer = NULL, DWORD size = 0)
	{
		m_pBuffer = pBuffer;
		m_Position = 0;
		m_Size = m_Edge = size;
		m_bFreeOnClose = (bool)(pBuffer==0);
	}
//////////////////////////////////////////////////////////
	~CxMemFile()
	{
		Close();
	}
//////////////////////////////////////////////////////////
	virtual bool Close()
	{
		if ( (m_pBuffer) && (m_bFreeOnClose) ){
			free(m_pBuffer);
			m_pBuffer = NULL;
			m_Size = 0;
		}
		return true;
	}
//////////////////////////////////////////////////////////
	bool Open()
	{
		if (m_pBuffer) return false;	// Can't re-open without closing first

		m_Position = m_Size = m_Edge = 0;
		m_pBuffer=(BYTE*)malloc(0);
		m_bFreeOnClose = true;

		return (m_pBuffer!=0);
	}
//////////////////////////////////////////////////////////
BYTE* GetBuffer(bool bDetachBuffer = true) { m_bFreeOnClose = !bDetachBuffer; return m_pBuffer;}
//////////////////////////////////////////////////////////
	virtual size_t	Read(void *buffer, size_t size, size_t count);
	virtual size_t	Write(const void *buffer, size_t size, size_t count);
	virtual bool	Seek(long offset, int origin);
	virtual long	Tell();
	virtual long	Size();
	virtual bool	Flush();
	virtual bool	Eof();
	virtual long	Error();
	virtual bool	PutC(unsigned char c);
	virtual long	GetC();

protected:
	void	Alloc(DWORD nBytes);
	void	Free();

protected:
	BYTE*	m_pBuffer;
	DWORD	m_Size;
	bool	m_bFreeOnClose;
	long	m_Position;	//current position
	long	m_Edge;		//buffer size
	};

#endif
