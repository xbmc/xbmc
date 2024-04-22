/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VirtualDirectory.h"

#include "Directory.h"
#include "DirectoryFactory.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "SourcesDirectory.h"
#include "URL.h"
#include "Util.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;

namespace XFILE
{

CVirtualDirectory::CVirtualDirectory(void)
{
  m_flags = DIR_FLAG_ALLOW_PROMPT;
  m_allowNonLocalSources = true;
}

CVirtualDirectory::~CVirtualDirectory(void) = default;

/*!
 \brief Add shares to the virtual directory
 \param VECSOURCES Shares to add
 \sa CMediaSource, VECSOURCES
 */
void CVirtualDirectory::SetSources(const VECSOURCES& vecSources)
{
  m_vecSources = vecSources;
}

/*!
 \brief Retrieve the shares or the content of a directory.
 \param strPath Specifies the path of the directory to retrieve or pass an empty string to get the shares.
 \param items Content of the directory.
 \return Returns \e true, if directory access is successful.
 \note If \e strPath is an empty string, the share \e items have thumbnails and icons set, else the thumbnails
    and icons have to be set manually.
 */

bool CVirtualDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  return GetDirectory(url, items, true, false);
}

bool CVirtualDirectory::GetDirectory(const CURL& url, CFileItemList &items, bool bUseFileDirectories, bool keepImpl)
{
  std::string strPath = url.Get();
  int flags = m_flags;
  if (!bUseFileDirectories)
    flags |= DIR_FLAG_NO_FILE_DIRS;
  if (!strPath.empty() && strPath != "files://")
  {
    CURL realURL = URIUtils::SubstitutePath(url);
    if (!m_pDir)
      m_pDir.reset(CDirectoryFactory::Create(realURL));
    bool ret = CDirectory::GetDirectory(strPath, m_pDir, items, m_strFileMask, flags);
    if (!keepImpl)
      m_pDir.reset();
    return ret;
  }

  // if strPath is blank, clear the list (to avoid parent items showing up)
  if (strPath.empty())
    items.Clear();

  // return the root listing
  items.SetPath(strPath);

  // grab our shares
  VECSOURCES shares;
  GetSources(shares);
  CSourcesDirectory dir;
  return dir.GetDirectory(shares, items);
}

void CVirtualDirectory::CancelDirectory()
{
  if (m_pDir)
    m_pDir->CancelDirectory();
}

/*!
 \brief Is the share \e strPath in the virtual directory.
 \param strPath Share to test
 \return Returns \e true, if share is in the virtual directory.
 \note The parameter \e strPath can not be a share with directory. Eg. "iso9660://dir" will return \e false.
    It must be "iso9660://".
 */
bool CVirtualDirectory::IsSource(const std::string& strPath, VECSOURCES *sources, std::string *name) const
{
  std::string strPathCpy = strPath;
  StringUtils::TrimRight(strPathCpy, "/\\");

  // just to make sure there's no mixed slashing in share/default defines
  // ie. f:/video and f:\video was not be recognised as the same directory,
  // resulting in navigation to a lower directory then the share.
  if(URIUtils::IsDOSPath(strPathCpy))
    StringUtils::Replace(strPathCpy, '/', '\\');

  VECSOURCES shares;
  if (sources)
    shares = *sources;
  else
    GetSources(shares);
  for (int i = 0; i < (int)shares.size(); ++i)
  {
    const CMediaSource& share = shares.at(i);
    std::string strShare = share.strPath;
    StringUtils::TrimRight(strShare, "/\\");
    if(URIUtils::IsDOSPath(strShare))
      StringUtils::Replace(strShare, '/', '\\');
    if (strShare == strPathCpy)
    {
      if (name)
        *name = share.strName;
      return true;
    }
  }
  return false;
}

/*!
 \brief Is the share \e path in the virtual directory.
 \param path Share to test
 \return Returns \e true, if share is in the virtual directory.
 \note The parameter \e path CAN be a share with directory. Eg. "iso9660://dir" will
       return the same as "iso9660://".
 */
bool CVirtualDirectory::IsInSource(const std::string &path) const
{
  bool isSourceName;
  VECSOURCES shares;
  GetSources(shares);
  int iShare = CUtil::GetMatchingSource(path, shares, isSourceName);
  if (URIUtils::IsOnDVD(path))
  { // check to see if our share path is still available and of the same type, as it changes during autodetect
    // and GetMatchingSource() is too naive at it's matching
    for (unsigned int i = 0; i < shares.size(); i++)
    {
      CMediaSource &share = shares[i];
      if (URIUtils::IsOnDVD(share.strPath) &&
          URIUtils::PathHasParent(path, share.strPath))
        return true;
    }
    return false;
  }
  //! @todo May need to handle other special cases that GetMatchingSource() fails on
  return (iShare > -1);
}

void CVirtualDirectory::GetSources(VECSOURCES &shares) const
{
  shares = m_vecSources;
  // add our plug n play shares

  if (m_allowNonLocalSources)
    CServiceBroker::GetMediaManager().GetRemovableDrives(shares);

#ifdef HAS_OPTICAL_DRIVE
  // and update our dvd share
  for (unsigned int i = 0; i < shares.size(); ++i)
  {
    CMediaSource& share = shares[i];
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD)
    {
      if (CServiceBroker::GetMediaManager().IsAudio(share.strPath))
      {
        share.strStatus = "Audio-CD";
        share.strPath = "cdda://local/";
        share.strDiskUniqueId = "";
      }
      else
      {
        share.strStatus = CServiceBroker::GetMediaManager().GetDiskLabel(share.strPath);
        share.strDiskUniqueId = CServiceBroker::GetMediaManager().GetDiskUniqueId(share.strPath);
        if (!share.strPath.length()) // unmounted CD
        {
          if (CServiceBroker::GetMediaManager().GetDiscPath() == "iso9660://")
            share.strPath = "iso9660://";
          else
            // share is unmounted and not iso9660, discard it
            shares.erase(shares.begin() + i--);
        }
      }
    }
  }
#endif
}
}

