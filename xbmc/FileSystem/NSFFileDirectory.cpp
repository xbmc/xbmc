
#include "stdafx.h"
#include "NSFFileDirectory.h"
#include "MusicInfoTagLoaderNSF.h"
#include "MusicInfoTag.h"

using namespace MUSIC_INFO;
using namespace DIRECTORY;

CNSFFileDirectory::CNSFFileDirectory(void)
{
  m_strExt = "nfsstream";
}

CNSFFileDirectory::~CNSFFileDirectory(void)
{
}

int CNSFFileDirectory::GetTrackCount(const CStdString& strPath)
{
  CMusicInfoTagLoaderNSF nsf;
  nsf.Load(strPath,m_tag);
  m_tag.SetDuration(4*60); // 4 mins

  return nsf.GetStreamCount(strPath);
}
