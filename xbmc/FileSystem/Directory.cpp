
#include "stdafx.h"
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

/*!
	\brief Test a file for an extension specified with SetMask().
	\param strFile File to test
	\return Returns \e true, if file is allowed.
	*/
bool	CDirectory::IsAllowed(const CStdString& strFile)
{
	CStdString strExtension;
	if ( !m_strFileMask.size() ) return true;
	if ( !strFile.size() ) return true;

	CUtil::GetExtension(strFile,strExtension);
	// if (!strExtension.size()) return true;
	if (!strExtension.size()) return false;
  CUtil::Lower(strExtension);
	if ( m_strFileMask.Find(strExtension.c_str() )>=0 )
	{
		return true;
	}
	return false;
}

/*!
	\brief Set a mask of extensions for the files in the directory.
	\param strMask Mask of file extensions that are allowed.

	The mask has to look like the following: \n
	\verbatim
	.m4a|.flac|.aac|
	\endverbatim
	So only *.m4a, *.flac, *.aac files will be retrieved with GetDirectory().
	*/
void  CDirectory::SetMask(const CStdString& strMask)
{
	m_strFileMask=strMask;
  CUtil::Lower(m_strFileMask);
}