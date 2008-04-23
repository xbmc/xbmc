#include "stdafx.h"
#include "SIDFileDirectory.h"

using namespace MUSIC_INFO;
using namespace DIRECTORY;

CSIDFileDirectory::CSIDFileDirectory(void)
{
  m_strExt = "sidstream";
}

CSIDFileDirectory::~CSIDFileDirectory(void)
{
}

int CSIDFileDirectory::GetTrackCount(const CStdString& strPath)
{
  DllSidplay2 m_dll;
  if (!m_dll.Load())
    return 0;

  return m_dll.GetNumberOfSongs(strPath.c_str());
}
