

#pragma once


class CSmartStr
{
	char * m_szPtr;
public:
	CSmartStr( int iCount ) : m_szPtr(NULL)
	{
		if ( iCount )
		{
			m_szPtr = new char [iCount];
		}
	}
	~CSmartStr()
	{
		if ( m_szPtr )
		{
			delete [] m_szPtr;
		}
	}
	operator char *() { return m_szPtr; }
};

class CSmartStrW
{
	wchar * m_szPtr;
public:
	CSmartStrW( int iCount ) : m_szPtr(NULL)
	{
		if ( iCount )
		{
			m_szPtr = new wchar [iCount];
		}
	}
	~CSmartStrW()
	{
		if ( m_szPtr )
		{
			delete [] m_szPtr;
		}
	}
	operator wchar *() { return m_szPtr; }
};

