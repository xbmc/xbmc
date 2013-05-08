/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "FileUtils.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIKeyboardFactory.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"
#include "JobManager.h"
#include "FileOperationJob.h"
#include "URIUtils.h"
#include "filesystem/MultiPathDirectory.h"
#include <vector>
#include "settings/MediaSourceSettings.h"
#include "Util.h"
#include "StringUtils.h"
#include "URL.h"

using namespace XFILE;
using namespace std;

bool CFileUtils::DeleteItem(const CStdString &strPath, bool force)
{
  CFileItemPtr item(new CFileItem(strPath));
  item->SetPath(strPath);
  item->m_bIsFolder = URIUtils::HasSlashAtEnd(strPath);
  item->Select(true);
  return DeleteItem(item, force);
}

bool CFileUtils::DeleteItem(const CFileItemPtr &item, bool force)
{
  if (!item || item->IsParentFolder())
    return false;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!force && pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, URIUtils::GetFileName(item->GetPath()));
    pDialog->SetLine(2, "");
    pDialog->DoModal();
    if (!pDialog->IsConfirmed()) return false;
  }

  // Create a temporary item list containing the file/folder for deletion
  CFileItemPtr pItemTemp(new CFileItem(*item));
  pItemTemp->Select(true);
  CFileItemList items;
  items.Add(pItemTemp);

  // grab the real filemanager window, set up the progress bar,
  // and process the delete action
  CFileOperationJob op(CFileOperationJob::ActionDelete, items, "");

  return op.DoWork();
}

bool CFileUtils::RenameFile(const CStdString &strFile)
{
  CStdString strFileAndPath(strFile);
  URIUtils::RemoveSlashAtEnd(strFileAndPath);
  CStdString strFileName = URIUtils::GetFileName(strFileAndPath);
  CStdString strPath = strFile.Left(strFileAndPath.size() - strFileName.size());
  if (CGUIKeyboardFactory::ShowAndGetInput(strFileName, g_localizeStrings.Get(16013), false))
  {
    strPath += strFileName;
    CLog::Log(LOGINFO,"FileUtils: rename %s->%s\n", strFileAndPath.c_str(), strPath.c_str());
    if (URIUtils::IsMultiPath(strFileAndPath))
    { // special case for multipath renames - rename all the paths.
      vector<CStdString> paths;
      CMultiPathDirectory::GetPaths(strFileAndPath, paths);
      bool success = false;
      for (unsigned int i = 0; i < paths.size(); ++i)
      {
        CStdString filePath(paths[i]);
        URIUtils::RemoveSlashAtEnd(filePath);
        filePath = URIUtils::GetDirectory(filePath);
        filePath = URIUtils::AddFileToFolder(filePath, strFileName);
        if (CFile::Rename(paths[i], filePath))
          success = true;
      }
      return success;
    }
    return CFile::Rename(strFileAndPath, strPath);
  }
  return false;
}

bool CFileUtils::RemoteAccessAllowed(const CStdString &strPath)
{
  const unsigned int SourcesSize = 5;
  CStdString SourceNames[] = { "programs", "files", "video", "music", "pictures" };

  string realPath = URIUtils::GetRealPath(strPath);
  // for rar:// and zip:// paths we need to extract the path to the archive
  // instead of using the VFS path
  while (URIUtils::IsInArchive(realPath))
    realPath = CURL(realPath).GetHostName();

  if (StringUtils::StartsWith(realPath, "virtualpath://upnproot/"))
    return true;
  else if (StringUtils::StartsWith(realPath, "musicdb://"))
    return true;
  else if (StringUtils::StartsWith(realPath, "videodb://"))
    return true;
  else if (StringUtils::StartsWith(realPath, "library://video"))
    return true;
  else if (StringUtils::StartsWith(realPath, "sources://video"))
    return true;
  else if (StringUtils::StartsWith(realPath, "special://musicplaylists"))
    return true;
  else if (StringUtils::StartsWith(realPath, "special://profile/playlists"))
    return true;
  else if (StringUtils::StartsWith(realPath, "special://videoplaylists"))
    return true;
  else if (StringUtils::StartsWith(realPath, "addons://sources"))
    return true;
  else if (StringUtils::StartsWith(realPath, "upnp://"))
    return true;
  else if (StringUtils::StartsWith(realPath, "plugin://"))
    return true;
  bool isSource;
  for (unsigned int index = 0; index < SourcesSize; index++)
  {
    VECSOURCES* sources = CMediaSourceSettings::Get().GetSources(SourceNames[index]);
    int sourceIndex = CUtil::GetMatchingSource(realPath, *sources, isSource);
    if (sourceIndex >= 0 && sourceIndex < (int)sources->size() && sources->at(sourceIndex).m_iHasLock != 2 && sources->at(sourceIndex).m_allowSharing)
      return true;
  }
  return false;
}


unsigned int CFileUtils::LoadFile(const std::string &filename, void* &outputBuffer)
{
  static const unsigned int max_file_size = 0x7FFFFFFF;
  static const unsigned int min_chunk_size = 64*1024U;
  static const unsigned int max_chunk_size = 2048*1024U;

  outputBuffer = NULL;
  if (filename.empty())
    return 0;

  XFILE::CFile file;
  if (!file.Open(filename, READ_TRUNCATED))
    return 0;

  /*
   GetLength() will typically return values that fall into three cases:
   1. The real filesize. This is the typical case.
   2. Zero. This is the case for some http:// streams for example.
   3. Some value smaller than the real filesize. This is the case for an expanding file.

   In order to handle all three cases, we read the file in chunks, relying on Read()
   returning 0 at EOF.  To minimize (re)allocation of the buffer, the chunksize in
   cases 1 and 3 is set to one byte larger than the value returned by GetLength().
   The chunksize in case 2 is set to the lowest value larger than min_chunk_size aligned
   to GetChunkSize().

   We fill the buffer entirely before reallocation.  Thus, reallocation never occurs in case 1
   as the buffer is larger than the file, so we hit EOF before we hit the end of buffer.

   To minimize reallocation, we double the chunksize each read while chunksize is lower
   than max_chunk_size.
   */
  int64_t filesize = file.GetLength();
  if (filesize > max_file_size)
  { /* file is too large for this function */
    file.Close();
    return 0;
  }
  unsigned int chunksize = (filesize > 0) ? (unsigned int)(filesize + 1) : CFile::GetChunkSize(file.GetChunkSize(), min_chunk_size);
  unsigned char *inputBuff = NULL;
  unsigned int inputBuffSize = 0;

  unsigned int total_read = 0, free_space = 0;
  while (true)
  {
    if (!free_space)
    { // (re)alloc
      inputBuffSize += chunksize;
      unsigned char *tempinputBuff = NULL;
      if (inputBuffSize <= max_file_size)
        tempinputBuff = (unsigned char *)realloc(inputBuff, inputBuffSize);
      if (!tempinputBuff)
      {
        CLog::Log(LOGERROR, "%s unable to (re)allocate buffer of size %u for file \"%s\"", __FUNCTION__, inputBuffSize, filename.c_str());
        free(inputBuff);
        file.Close();
        return 0;
      }
      inputBuff = tempinputBuff;
      free_space = chunksize;
      if (chunksize < max_chunk_size)
        chunksize *= 2;
    }
    unsigned int read = file.Read(inputBuff + total_read, free_space);
    free_space -= read;
    total_read += read;
    if (!read)
      break;
  }

  file.Close();

  if (total_read == 0)
  {
    free(inputBuff);
    return 0;
  }

  if (total_read + 1 < inputBuffSize)
  {
    /* free extra memory if more than 1 byte (cases 1 and 3) */
    unsigned char *tempinputBuff = (unsigned char *)realloc(inputBuff, total_read);
    if (!tempinputBuff)
    {
      /* just a precaution, shouldn't really happen */
      CLog::Log(LOGERROR, "%s unable to reallocate buffer for file \"%s\"", __FUNCTION__, filename.c_str());
      free(inputBuff);
      return 0;
    }
    inputBuff = tempinputBuff;
  }

  outputBuffer = (void *) inputBuff;
  return total_read;
}

