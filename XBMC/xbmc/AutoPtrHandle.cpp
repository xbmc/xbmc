#include ".\autoptrhandle.h"
using namespace AUTOPTR;
CAutoPtrHandle::CAutoPtrHandle(HANDLE hHandle)
:m_hHandle(hHandle)
{
}

CAutoPtrHandle::~CAutoPtrHandle(void)
{
	Cleanup();
}

CAutoPtrHandle::operator HANDLE()
{
	return m_hHandle;
}

void CAutoPtrHandle::attach(HANDLE hHandle)
{
	Cleanup();
	m_hHandle=hHandle;
}

HANDLE CAutoPtrHandle::release()
{
	HANDLE hTmp=m_hHandle;
	m_hHandle=INVALID_HANDLE_VALUE;
	return hTmp;
}

void CAutoPtrHandle::Cleanup()
{
	if ( isValid() )
	{
		CloseHandle(m_hHandle);
		m_hHandle=INVALID_HANDLE_VALUE;
	}
}

bool CAutoPtrHandle::isValid() const
{
	if ( INVALID_HANDLE_VALUE != m_hHandle)
		return true;
	return false;
}
void CAutoPtrHandle::reset()
{
	Cleanup();
}

//-------------------------------------------------------------------------------
CAutoPtrFind ::CAutoPtrFind(HANDLE hHandle)
:CAutoPtrHandle(hHandle)
{
}
CAutoPtrFind::~CAutoPtrFind(void)
{
	Cleanup();
}

void CAutoPtrFind::Cleanup()
{
	if ( isValid() )
	{
		FindClose(m_hHandle);
		m_hHandle=INVALID_HANDLE_VALUE;
	}
}

//-------------------------------------------------------------------------------
CAutoPtrSocket::CAutoPtrSocket(SOCKET hSocket)
:m_hSocket(hSocket)
{
}

CAutoPtrSocket::~CAutoPtrSocket(void)
{
	Cleanup();
}

CAutoPtrSocket::operator SOCKET()
{
	return m_hSocket;
}

void CAutoPtrSocket::attach(SOCKET hSocket)
{
	Cleanup();
	m_hSocket=hSocket;
}

SOCKET CAutoPtrSocket::release()
{
	SOCKET hTmp=m_hSocket;
	m_hSocket=INVALID_SOCKET;
	return hTmp;
}

void CAutoPtrSocket::Cleanup()
{
	if ( isValid() )
	{
		closesocket(m_hSocket);
		m_hSocket=INVALID_SOCKET;
	}
}

bool CAutoPtrSocket::isValid() const
{
	if ( INVALID_SOCKET != m_hSocket)
		return true;
	return false;
}
void CAutoPtrSocket::reset()
{
	Cleanup();
}
