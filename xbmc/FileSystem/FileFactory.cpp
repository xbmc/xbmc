
#include "../stdafx.h"
#include "FileFactory.h"
#include "FileShoutcast.h"
#include "FileISO.h"
#include "FileRelax.h"
#include "FileHD.h"
#include "FileSMB.h"
#include "FileXBMSP.h"
#include "FileRTV.h"
#include "FileSndtrk.h"
#include "FileDAAP.h"
#include "FileCDDA.h"
#include "FileZip.h"
#include "FileRar.h"
#include "FileFTP.h"

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
  CStdString strProtocol = url.GetProtocol();
  strProtocol.MakeLower();

  if (strProtocol == "http") return NULL;
  else if (strProtocol == "iso9660") return new CFileISO();
  else if (strProtocol == "xns") return new CFileRelax();
  else if (strProtocol == "smb") return new CFileSMB();
  else if (strProtocol == "xbms") return new CFileXBMSP();
  else if (strProtocol == "shout") return new CFileShoutcast();
  else if (strProtocol == "rtv") return new CFileRTV();
  else if (strProtocol == "soundtrack") return new CFileSndtrk();
  else if (strProtocol == "daap") return new CFileDAAP();
  else if (strProtocol == "cdda") return new CFileCDDA();
  else if (strProtocol == "zip") return new CFileZip();
  else if (strProtocol == "rar") return new CFileRar();
  else if (strProtocol == "ftp") return new CFileFTP();
  else return new CFileHD();
}
