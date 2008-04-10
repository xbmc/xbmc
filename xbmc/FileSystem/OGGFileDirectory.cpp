
#include "stdafx.h"
#include "OGGFileDirectory.h"
#include "OggTag.h"

using namespace MUSIC_INFO;
using namespace DIRECTORY;

COGGFileDirectory::COGGFileDirectory(void)
{
  m_strExt = "oggstream";
}

COGGFileDirectory::~COGGFileDirectory(void)
{
}

int COGGFileDirectory::GetTrackCount(const CStdString& strPath)
{
  COggTag tag;
  return tag.GetStreamCount(strPath);
}
