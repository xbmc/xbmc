#include "Win32StorageProvider.h"
#include "WIN32Util.h"
#include "LocalizeStrings.h"

void CWin32StorageProvider::GetLocalDrives(VECSOURCES &localDrives)
{
  CMediaSource share;
  share.strPath = "special://xbmc/";
  share.strName.Format(g_localizeStrings.Get(21438),'Q');
  share.m_ignore = true;
  localDrives.push_back(share);

  CWIN32Util::GetDrivesByType(localDrives, LOCAL_DRIVES);
}

void CWin32StorageProvider::GetRemovableDrives(VECSOURCES &removableDrives)
{
}

std::vector<CStdString> CWin32StorageProvider::GetDiskUsage()
{
  return CWIN32Util::GetDiskUsage();
}
