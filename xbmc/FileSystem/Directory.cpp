#include "directory.h"
#include "../util.h"

using namespace DIRECTORY;

CDirectory::CDirectory(void)
{
	m_strFileMask="";
}

CDirectory::~CDirectory(void)
{
}

bool	CDirectory::IsAllowed(const CStdString& strFile)
{
	CStdString strExtension;
	if ( !m_strFileMask.size() ) return true;
	if ( !strFile.size() ) return true;

	CUtil::GetExtension(strFile,strExtension);
	if (!strExtension.size()) return true;
  CUtil::Lower(strExtension);
	if ( m_strFileMask.Find(strExtension.c_str() )>=0 )
	{
		return true;
	}
	return false;
}

void  CDirectory::SetMask(const CStdString& strMask)
{
	m_strFileMask=strMask;
  CUtil::Lower(m_strFileMask);
}