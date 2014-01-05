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
  CStdString strPath = URIUtils::GetDirectory(strFileAndPath);
  if (CGUIKeyboardFactory::ShowAndGetInput(strFileName, g_localizeStrings.Get(16013), false))
  {
    strPath = URIUtils::AddFileToFolder(strPath, strFileName);
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

  if (StringUtils::StartsWithNoCase(realPath, "virtualpath://upnproot/"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "musicdb://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "videodb://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "library://video"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "sources://video"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://musicplaylists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://profile/playlists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "special://videoplaylists"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "addons://sources"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "upnp://"))
    return true;
  else if (StringUtils::StartsWithNoCase(realPath, "plugin://"))
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
  XFILE::auto_buffer buffer;
  XFILE::CFile file;

  const unsigned int total_read = file.LoadFile(filename, buffer);
  outputBuffer = buffer.detach();

  return total_read;
}

CFileUtils::EFileType CFileUtils::GetFileTypeFromMime(const std::string& mimeType)
{
  // based on http://mimesniff.spec.whatwg.org/

  std::string type, subtype;
  if (!parseMimeType(mimeType, type, subtype))
    return FileTypeUnknown;

  if (type == "application")
  {
    if (subtype == "zip")
      return FileTypeZip;
    if (subtype == "x-gzip")
      return FileTypeGZip;
    if (subtype == "x-rar-compressed")
      return FileTypeRar;
    
    if (subtype == "xml")
      return FileTypeXml;
  }
  else if (type == "text")
  {
    if (subtype == "xml")
      return FileTypeXml;
    if (subtype == "html")
      return FileTypeHtml;
    if (subtype == "plain")
      return FileTypePlainText;
  }
  else if (type == "image")
  {
    if (subtype == "bmp")
      return FileTypeBmp;
    if (subtype == "gif")
      return FileTypeGif;
    if (subtype == "png")
      return FileTypePng;
    if (subtype == "jpeg" || subtype == "pjpeg")
      return FileTypeJpeg;
  }

  if (StringUtils::EndsWith(subtype, "+zip"))
    return FileTypeZip;
  if (StringUtils::EndsWith(subtype, "+xml"))
    return FileTypeXml;

  return FileTypeUnknown;
}

bool CFileUtils::parseMimeType(const std::string& mimeType, std::string& type, std::string& subtype)
{
  // this is an modified implementation of http://mimesniff.spec.whatwg.org/#parsing-a-mime-type with additional checks for non-empty type and subtype
  // note: only type and subtype are parsed, parameters are ignored

  static const char* const whitespaceChars = "\x09\x0A\x0C\x0D\x20"; // tab, LF, FF, CR and space
  static const std::string whitespaceSmclnChars("\x09\x0A\x0C\x0D\x20\x3B"); // tab, LF, FF, CR, space and semicolon

  type.clear();
  subtype.clear();

  const size_t len = mimeType.length();
  if (len < 1)
    return false;

  const char* const mimeTypeC = mimeType.c_str();
  size_t pos = mimeType.find_first_not_of(whitespaceChars);
  if (pos == std::string::npos)
    return false;

  // find "type"
  size_t t = 0;
  do
  {
    const char chr = mimeTypeC[pos];
    if (t > 127 || !chr)
    {
      type.clear();
      return false;
    }

    if (chr >= 'A' && chr <= 'Z')
      type.push_back(chr + ('a' - 'A')); // convert to lowercase
    else
      type.push_back(chr);
    t++;
    pos++;
  } while (mimeTypeC[pos] != '/');

  pos++; // skip '/'
  t = 0;

  while (mimeTypeC[pos] && whitespaceSmclnChars.find(mimeTypeC[pos]) != std::string::npos && t++ <= 127)
  {
    const char chr = mimeTypeC[pos];
    if (chr >= 'A' && chr <= 'Z')
      subtype.push_back(chr + ('a' - 'A')); // convert to lowercase
    else
      subtype.push_back(chr);
    pos++;
  }

  if (subtype.empty() || t > 127)
  {
    type.clear();
    subtype.clear();
    return false;
  }

  return true;
}
