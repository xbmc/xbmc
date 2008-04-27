#include "stdafx.h"
#include "ASAPFileDirectory.h"

using namespace DIRECTORY;

CASAPFileDirectory::CASAPFileDirectory()
{
  m_strExt = "asapstream";
}

CASAPFileDirectory::~CASAPFileDirectory()
{
}

int CASAPFileDirectory::GetTrackCount(const CStdString &strPath)
{
  if (!m_dll.Load())
    return 0;
  return m_dll.asapGetSongs(strPath.c_str());
}
