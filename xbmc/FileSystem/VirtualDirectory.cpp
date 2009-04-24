/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "stdafx.h"
#include "VirtualDirectory.h"
#include "FactoryDirectory.h"
#include "Util.h"
#include "Profile.h"
#include "Directory.h"
#include "DirectoryCache.h"
#include "DetectDVDType.h"
#include "../MediaManager.h"
#ifdef HAS_HAL // This should be ifdef _LINUX when hotplugging is supported on osx
#include "linux/LinuxFileSystem.h"
#include <vector>
#endif
#include "File.h"
#include "FileItem.h"
#ifdef _WIN32PC
using namespace MEDIA_DETECT;
#include "cdioSupport.h"
#endif

using namespace XFILE;

namespace DIRECTORY
{

CVirtualDirectory::CVirtualDirectory(void) : m_vecSources(NULL)
{
  m_allowPrompting = true;  // by default, prompting is allowed.
  m_cacheDirectory = DIR_CACHE_ONCE;  // by default, caching is done.
  m_allowNonLocalSources = true;
}

CVirtualDirectory::~CVirtualDirectory(void)
{}

/*!
 \brief Add shares to the virtual directory
 \param VECSOURCES Shares to add
 \sa CMediaSource, VECSOURCES
 */
void CVirtualDirectory::SetSources(VECSOURCES& vecSources)
{
  m_vecSources = &vecSources;
}

/*!
 \brief Retrieve the shares or the content of a directory.
 \param strPath Specifies the path of the directory to retrieve or pass an empty string to get the shares.
 \param items Content of the directory.
 \return Returns \e true, if directory access is successfull.
 \note If \e strPath is an empty string, the share \e items have thumbnails and icons set, else the thumbnails
    and icons have to be set manually.
 */

bool CVirtualDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  return GetDirectory(strPath,items,true);
}
bool CVirtualDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, bool bUseFileDirectories)
{
  if (!m_vecSources)
  {
    items.m_strPath=strPath;
    return true;
  }

  VECSOURCES shares;
  GetSources(shares);
  if (!strPath.IsEmpty() && strPath != "files://")
    return CDirectory::GetDirectory(strPath, items, m_strFileMask, bUseFileDirectories, m_allowPrompting, m_cacheDirectory, m_extFileInfo);

  // if strPath is blank, clear the list (to avoid parent items showing up)
  if (strPath.IsEmpty())
    items.Clear();

  // return the root listing
  items.m_strPath=strPath;

  // grab our shares
  for (unsigned int i = 0; i < shares.size(); ++i)
  {
    CMediaSource& share = shares[i];
    CFileItemPtr pItem(new CFileItem(share));
    if (pItem->IsLastFM() || pItem->IsShoutCast() || (pItem->m_strPath.Left(14).Equals("musicsearch://")))
      pItem->SetCanQueue(false);
    CStdString strPathUpper = pItem->m_strPath;
    strPathUpper.ToUpper();

    CStdString strIcon;
    // We have the real DVD-ROM, set icon on disktype
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD && share.m_strThumbnailImage.IsEmpty())
    {
      CUtil::GetDVDDriveIcon( pItem->m_strPath, strIcon );
      // CDetectDVDMedia::SetNewDVDShareUrl() caches disc thumb as special://temp/dvdicon.tbn
      CStdString strThumb = "special://temp/dvdicon.tbn";
      if (XFILE::CFile::Exists(strThumb))
        pItem->SetThumbnailImage(strThumb);
    }
    else if (strPathUpper.Left(11) == "SOUNDTRACK:")
      strIcon = "defaultHardDisk.png";
    else if (pItem->IsRemote())
      strIcon = "defaultNetwork.png";
    else if (pItem->IsISO9660())
      strIcon = "defaultDVDRom.png";
    else if (pItem->IsDVD())
      strIcon = "defaultDVDRom.png";
    else if (pItem->IsCDDA())
      strIcon = "defaultCDDA.png";
    else
      strIcon = "defaultHardDisk.png";

    pItem->SetIconImage(strIcon);
    if (share.m_iHasLock == 2 && g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_LOCKED);
    else
      pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_NONE);

    items.Add(pItem);
  }

  return true;
}

/*!
 \brief Is the share \e strPath in the virtual directory.
 \param strPath Share to test
 \return Returns \e true, if share is in the virtual directory.
 \note The parameter \e strPath can not be a share with directory. Eg. "iso9660://dir" will return \e false.
    It must be "iso9660://".
 */
bool CVirtualDirectory::IsSource(const CStdString& strPath) const
{
  CStdString strPathCpy = strPath;
  strPathCpy.TrimRight("/");
  strPathCpy.TrimRight("\\");

  // just to make sure there's no mixed slashing in share/default defines
  // ie. f:/video and f:\video was not be recognised as the same directory,
  // resulting in navigation to a lower directory then the share.
  if(CUtil::IsDOSPath(strPathCpy))
    strPathCpy.Replace("/", "\\");

  VECSOURCES shares;
  GetSources(shares);
  for (int i = 0; i < (int)shares.size(); ++i)
  {
    const CMediaSource& share = shares.at(i);
    CStdString strShare = share.strPath;
    strShare.TrimRight("/");
    strShare.TrimRight("\\");
    if(CUtil::IsDOSPath(strShare))
      strShare.Replace("/", "\\");
    if (strShare == strPathCpy) return true;
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
bool CVirtualDirectory::IsInSource(const CStdString &path) const
{
  bool isSourceName;
  VECSOURCES shares;
  GetSources(shares);
  int iShare = CUtil::GetMatchingSource(path, shares, isSourceName);
  if (CUtil::IsOnDVD(path))
  { // check to see if our share path is still available and of the same type, as it changes during autodetect
    // and GetMatchingSource() is too naive at it's matching
    for (unsigned int i = 0; i < shares.size(); i++)
    {
      CMediaSource &share = shares[i];
      if (CUtil::IsOnDVD(share.strPath) && share.strPath.Equals(path.Left(share.strPath.GetLength())))
        return true;
    }
    return false;
  }
  // TODO: May need to handle other special cases that GetMatchingSource() fails on
  return (iShare > -1);
}

void CVirtualDirectory::GetSources(VECSOURCES &shares) const
{
  shares = *m_vecSources;
  // add our plug n play shares

  if (m_allowNonLocalSources)
  {
    g_mediaManager.GetRemovableDrives(shares);

#ifdef HAS_HAL
    int type = CMediaSource::SOURCE_TYPE_DVD;
    CLinuxFileSystem::GetDrives(&type, 1, shares);
#endif
    CUtil::AutoDetectionGetSource(shares);
  }

  // and update our dvd share
  for (unsigned int i = 0; i < shares.size(); ++i)
  {
    CMediaSource& share = shares[i];
    if (share.m_iDriveType == CMediaSource::SOURCE_TYPE_DVD)
    {
#ifdef _WIN32PC
      CCdIoSupport cdio;
      CStdString strDevice;
      strDevice.Format("\\\\.\\%c:",share.strPath[0]);
      CCdInfo* pCdInfo = cdio.GetCdInfo((char*)strDevice.c_str());
      if (pCdInfo != NULL)
      {
        if(pCdInfo->IsAudio(1))
        {
          share.strStatus = "Audio-CD";
          share.strPath = "cdda://local/";
        }
        else
          share.strStatus = pCdInfo->GetDiscLabel().c_str();

        delete pCdInfo;
      }
#else
      share.strStatus = MEDIA_DETECT::CDetectDVDMedia::GetDVDLabel();
      share.strPath = MEDIA_DETECT::CDetectDVDMedia::GetDVDPath();
#endif
    }
  }
}
}

