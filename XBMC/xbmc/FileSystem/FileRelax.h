// FileRelax.h: interface for the CFileRelax class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILERELAX_H__7DA6AD38_2EA8_4106_933C_EF5FC6D581E4__INCLUDED_)
#define AFX_FILERELAX_H__7DA6AD38_2EA8_4106_933C_EF5FC6D581E4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IFile.h"
#include "../autoptrhandle.h"

using namespace XFILE;
using namespace AUTOPTR;

namespace XFILE
{

	class CFileRelax : public IFile  
	{
	public:
		CFileRelax();
		virtual ~CFileRelax();
		virtual __int64			GetPosition();
		virtual __int64			GetLength();
		virtual bool					Open(const char* strUserName, const char* strPassword,const char* strHostName, const char* strFileName,int iport, bool bBinary=true);
		virtual unsigned int	Read(void* lpBuf, __int64 uiBufSize);
		virtual bool					ReadString(char *szLine, int iLineLength);
		virtual __int64			Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
		virtual void					Close();
	protected:
		UINT64								m_fileSize ;
		UINT64							  m_filePos;
		CAutoPtrSocket        m_socket;
	private:
		bool m_bOpened;
		bool									Send(byte* pBuffer, int iLen);
		bool									Recv(byte* pBuffer, int iLen);
	};
};

#endif // !defined(AFX_FILERELAX_H__7DA6AD38_2EA8_4106_933C_EF5FC6D581E4__INCLUDED_)
