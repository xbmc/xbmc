
#include "../stdafx.h"
#include "StackDirectory.h"
#include "../utils/log.h"
#include "../Util.h"


namespace DIRECTORY
{
  CStackDirectory::CStackDirectory()
  {
  }

  CStackDirectory::~CStackDirectory()
  {
  }

  bool CStackDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {
    // format is:
    // stack://<path>/file1,file2,file3,file4
    CStdStringArray files;
    StringUtils::SplitString(strPath, ",", files);
    if (files.empty())
      return false;   // error in path
    CStdString directory;
    CUtil::GetDirectory(files[0], directory);
    // remove "stack://" from directory
    directory = directory.Mid(8);
    files[0] = CUtil::GetFileName(files[0]);
    for (unsigned int i = 0; i < files.size(); i++)
    {
      CStdString path = directory + "/" + files[i];
      CFileItem *item = new CFileItem(files[i]);
      item->m_strPath = path;
      item->m_bIsFolder = false;
      items.Add(item);
    }
    return true;
  }

  CStdString CStackDirectory::GetStackedTitlePath(const CStdString &strPath)
  {
    CStdString path;
    int pos = strPath.Find(',');
    if (pos > 0)
    {
      path = strPath.Mid(8, pos - 8);
      CStdString file, folder, title, volume;
      CUtil::Split(path, folder, file);
      CUtil::GetVolumeFromFileName(file, title, volume);
      CUtil::AddFileToFolder(folder, title, path);
    }
    return path;
  }

}

