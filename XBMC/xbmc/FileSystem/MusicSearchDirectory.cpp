#include "stdafx.h"
#include "MusicSearchDirectory.h"
#include "../MusicDatabase.h"

using namespace XFILE;
using namespace DIRECTORY;

CMusicSearchDirectory::CMusicSearchDirectory(void)
{
}

CMusicSearchDirectory::~CMusicSearchDirectory(void)
{
}

bool CMusicSearchDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // break up our path
  // format is:  musicsearch://<url encoded search string>
  CURL url(strPath);
  CStdString search(url.GetHostName());

  if (search.IsEmpty())
    return false;

  // and retrieve the search details
  items.m_strPath = strPath;
  DWORD time = timeGetTime();
  CMusicDatabase db;
  db.Open();
  db.Search(search, items);
  db.Close();
  CLog::Log(LOGDEBUG, "%s (%s) took %lu ms", __FUNCTION__, strPath.c_str(), timeGetTime() - time);
  return true;
}

bool CMusicSearchDirectory::Exists(const char* strPath)
{
  return true;
}
