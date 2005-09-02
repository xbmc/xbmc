
#include "../../../stdafx.h"
#include "DVDFactoryInputStream.h"
#include "DVDInputStream.h"
#include "DVDInputStreamFile.h"
#include "DVDInputStreamNavigator.h"
#include "DVDInputStreamHttp.h"

CDVDInputStream* CDVDFactoryInputStream::CreateInputStream(IDVDPlayer* pPlayer, const char* strFile)
{
  CFileItem item(strFile, false);
  if (item.IsDVDFile(false, true) || item.IsDVDImage() ||
      strncmp(strFile, "\\Device\\Cdrom0", 14) == 0)
  {
    return (new CDVDInputStreamNavigator(pPlayer));
  }
  else if (item.IsInternetStream())
  {
    return (new CDVDInputStreamHttp());
  }
  
  return (new CDVDInputStreamFile());
}
