#include "factorydirectory.h"
#include "HDDirectory.h"
#include "ISO9660Directory.h"
#include "XNSDirectory.h"
#include "SMBDirectory.h"
#include "XBMSDirectory.h"
#include "../url.h"

CFactoryDirectory::CFactoryDirectory(void)
{
}

CFactoryDirectory::~CFactoryDirectory(void)
{
}

CDirectory* CFactoryDirectory::Create(const CStdString& strPath)
{
	CURL url(strPath);
	CStdString strProtocol=url.GetProtocol();
	if (strProtocol=="iso9660")
	{
		return (CDirectory*)new CISO9660Directory();
	}
	
	if (strProtocol=="xns")
	{
		return (CDirectory*)new CXNSDirectory();
	}

	if (strProtocol=="smb")
	{
		return (CDirectory*)new CSMBDirectory();
	}

	if (strProtocol=="xbms")
	{
		return (CDirectory*)new CXBMSDirectory();
	}

  return (CDirectory*)new CHDDirectory();
}
