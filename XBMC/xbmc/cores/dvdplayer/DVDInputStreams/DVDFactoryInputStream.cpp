
#include "stdafx.h"
#include "DVDFactoryInputStream.h"
#include "DVDInputStream.h"
#include "DVDInputStreamFile.h"
#include "DVDInputStreamNavigator.h"
#include "..\..\..\util.h"

CDVDInputStream* CDVDFactoryInputStream::CreateInputStream(IDVDPlayer* pPlayer, const char* strFile)
{
  if (CUtil::IsDVDFile(strFile, false, true) || CUtil::IsDVDImage(strFile) ||
      strncmp(strFile, "\\Device\\Cdrom0", 14) == 0)
  {
    return (new CDVDInputStreamNavigator(pPlayer));
  }
  
  return (new CDVDInputStreamFile());
}