
#include "stdafx.h"

#include "FileFactory.h"
#include "FileShoutcast.h"
#include "../url.h"
#include "FileISO.h"
#include "FileRelax.h"
#include "FileHD.h"
#include "FileSMB.h"
#include "FileXBMSP.h"
#include "FileRTV.h"
using namespace XFILE;

CFileFactory::CFileFactory()
{

}

CFileFactory::~CFileFactory()
{

}
IFile* CFileFactory::CreateLoader(const CStdString& strFileName)
{
	CURL url(strFileName);
	CStdString strProtocol=url.GetProtocol();
	if (strProtocol=="iso9660")
	{
		return (IFile*)new CFileISO();
	}
	
	if (strProtocol=="xns")
	{
		return (IFile*)new CFileRelax();
	}

	if (strProtocol=="smb")
	{
		return (IFile*)new CFileSMB();
	}

	if (strProtocol=="xbms")
	{
		return (IFile*)new CFileXBMSP();
	}
	if (strProtocol=="shout" || strProtocol=="http")
	{
		return (IFile*)new CFileShoutcast();
	}
	if (strProtocol=="rtv")
	{
		return (IFile*)new CFileRTV();
	}

  return (IFile*)new CFileHD();
}