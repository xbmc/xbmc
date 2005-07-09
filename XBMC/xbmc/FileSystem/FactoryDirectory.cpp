
#include "../stdafx.h"
#include "factorydirectory.h"
#include "factoryfiledirectory.h"
#include "HDDirectory.h"
#include "ISO9660Directory.h"
#include "XNSDirectory.h"
#include "SMBDirectory.h"
#include "XBMSDirectory.h"
#include "CDDADirectory.h"
#include "RTVDirectory.h"
#include "SndtrkDirectory.h"
#include "DAAPDirectory.h"
#include "shoutcastdirectory.h"
#include "zipdirectory.h"
#include "rardirectory.h"
#include "FTPDirectory.h"


/*!
 \brief Create a IDirectory object of the share type specified in \e strPath .
 \param strPath Specifies the share type to access, can be a share or share with path.
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CFactoryDirectory::Create(const CStdString& strPath)
{
  CURL url(strPath);

  IFileDirectory* pDir=CFactoryFileDirectory::Create(strPath);
  if (pDir)
    return pDir;

  CStdString strProtocol = url.GetProtocol();
  if (strProtocol == "iso9660") return new CISO9660Directory();
  else if (strProtocol == "xns") return new CXNSDirectory();
  else if (strProtocol == "smb") return new CSMBDirectory();
  else if (strProtocol == "xbms") return new CXBMSDirectory();
  else if (strProtocol == "cdda") return new CCDDADirectory();
  else if (strProtocol == "rtv") return new CRTVDirectory();
  else if (strProtocol == "soundtrack") return new CSndtrkDirectory();
  else if (strProtocol == "daap") return new CDAAPDirectory();
  else if (strProtocol == "shout") return new CShoutcastDirectory();
  else if (strProtocol == "zip") return new CZipDirectory();
  else if (strProtocol == "rar") return new CRarDirectory();
  else if (strProtocol == "ftp") return new CFTPDirectory();
  else return new CHDDirectory();
}
