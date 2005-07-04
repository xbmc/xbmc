
#include "../stdafx.h"
#include "../util.h"
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

  IFileDirectory* pDir=CFactoryFileDirectory().Create(strPath);
  if (pDir)
    return pDir;

  CStdString strProtocol = url.GetProtocol();
  if (strProtocol == "iso9660")
  {
    return (IDirectory*)new CISO9660Directory();
  }

  if (strProtocol == "xns")
  {
    return (IDirectory*)new CXNSDirectory();
  }

  if (strProtocol == "smb")
  {
    return (IDirectory*)new CSMBDirectory();
  }

  if (strProtocol == "xbms")
  {
    return (IDirectory*)new CXBMSDirectory();
  }
  if (strProtocol == "cdda")
  {
    return (IDirectory*)new CCDDADirectory();
  }
  if (strProtocol == "rtv")
  {
    return (IDirectory*)new CRTVDirectory();
  }
  if (strProtocol == "soundtrack")
  {
    return (IDirectory*)new CSndtrkDirectory();
  }
  if (strProtocol == "daap")
  {
    return (IDirectory*)new CDAAPDirectory();
  }
  if (strProtocol == "shout")
  {
    return (IDirectory*)new CShoutcastDirectory();
  }
  if (strProtocol == "zip")
  {
    return (IDirectory*)new CZipDirectory();
  }
  if (strProtocol == "rar")
    return (IDirectory*)new CRarDirectory();
  
  if (strProtocol == "ftp")
  {
    return (IDirectory*)new CFTPDirectory();
  }


  return (IDirectory*)new CHDDirectory();
}
