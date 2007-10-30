
#include "stdafx.h"
#include "VirtualDirectory.h"
#include "FactoryDirectory.h"
#include "../Util.h"
#include "DirectoryCache.h"
#ifdef HAS_XBOX_HARDWARE
#include "../utils/MemoryUnitManager.h"
#endif
#include "../DetectDVDType.h"

using namespace XFILE;

namespace DIRECTORY
{

CVirtualDirectory::CVirtualDirectory(void) : m_vecShares(NULL)
{
  m_allowPrompting = true;  // by default, prompting is allowed.
  m_cacheDirectory = true;  // by default, caching is done.
  m_allowNonLocalShares = true;
}

CVirtualDirectory::~CVirtualDirectory(void)
{}

/*!
 \brief Add shares to the virtual directory
 \param vecShares Shares to add
 \sa CShare, VECSHARES
 */
void CVirtualDirectory::SetShares(VECSHARES& vecShares)
{
  m_vecShares = &vecShares;
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
  if (!m_vecShares)
  {
    items.m_strPath=strPath;
    return true;
  }

  CStdString strPath2 = strPath;
  CStdString strPath3 = strPath;
  strPath2 += "/";
  strPath3 += "\\";

  // i assumed the intention of this section was to confirm that strPath is part of an existing book.
  // in order to work with virtualpath:// subpaths, i had replaced this with the GetMatchingShare function
  // which does the same thing.
  /*
  for (int i = 0; i < (int)m_vecShares->size(); ++i)
  {
    CShare& share = m_vecShares->at(i);
    CLog::Log(LOGDEBUG,"CVirtualDirectory... Checking [%s]", share.strPath.c_str());
    if ( share.strPath == strPath.Left( share.strPath.size() ) ||
         share.strPath == strPath2.Left( share.strPath.size() ) ||
         share.strPath == strPath3.Left( share.strPath.size() ) ||
         strPath.Mid(1, 1) == ":" )
    {
      // Only cache directory we are getting now
      g_directoryCache.Clear();
      CLog::Log(LOGDEBUG,"CVirtualDirectory... FOUND MATCH [%s]", share.strPath.c_str());
      
      return CDirectory::GetDirectory(strPath, items, m_strFileMask);
    }
  }
  */

  VECSHARES shares;
  GetShares(shares);
  if (!strPath.IsEmpty() && strPath != "files://")
  {
    bool bIsSourceName = false;
    int iIndex = CUtil::GetMatchingShare(strPath, shares, bIsSourceName);
    // added exception for various local hd items
    // function doesn't work for http/shout streams with options..
    if (iIndex > -1 || strPath.Mid(1, 1) == ":" 
      || strPath.Left(8).Equals("shout://") 
      || strPath.Left(8).Equals("https://") 
      || strPath.Left(7).Equals("http://") 
      || strPath.Left(7).Equals("daap://")
      || strPath.Left(9).Equals("tuxbox://")
      || strPath.Left(7).Equals("upnp://")
      || strPath.Left(10).Equals("musicdb://")
      || strPath.Left(14).Equals("musicsearch://"))
    {
      // Only cache directory we are getting now
      if (!strPath.Left(7).Equals("lastfm:") && !strPath.Left(8).Equals("shout://") && !strPath.Left(9).Equals("tuxbox://"))
        g_directoryCache.Clear();
      return CDirectory::GetDirectory(strPath, items, m_strFileMask, bUseFileDirectories, m_allowPrompting, m_cacheDirectory);
    }

    // what do with an invalid path?
    // return false so the calling window can deal with the error accordingly
    // otherwise the root listing is returned which seems incorrect but was the previous behaviour
    CLog::Log(LOGERROR,"CVirtualDirectory::GetDirectory(%s) matches no valid source, getting root source list instead", strPath.c_str());
    return false;
  }

  // if strPath is blank, clear the list (to avoid parent items showing up)
  if (strPath.IsEmpty())
    items.Clear();

  // return the root listing
  items.m_strPath=strPath;

  // grab our shares
  for (unsigned int i = 0; i < shares.size(); ++i)
  {
    CShare& share = shares[i];
    CFileItem* pItem = new CFileItem(share);
    CStdString strPathUpper = pItem->m_strPath;
    strPathUpper.ToUpper();

    CStdString strIcon=share.m_strThumbnailImage;
    if (share.m_strThumbnailImage.IsEmpty())
    {
      // We have the real DVD-ROM, set icon on disktype
      if (share.m_iDriveType == SHARE_TYPE_DVD)
      {
        CUtil::GetDVDDriveIcon( pItem->m_strPath, strIcon );
        // CDetectDVDMedia::SetNewDVDShareUrl() caches disc thumb as Z:\dvdicon.tbn
        CStdString strThumb = "Z:\\dvdicon.tbn";
        if (CFile::Exists(strThumb))
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
    }

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
bool CVirtualDirectory::IsShare(const CStdString& strPath) const
{
  CStdString strPathCpy = strPath;
  strPathCpy.TrimRight("/");
  strPathCpy.TrimRight("\\");

  // just to make sure there's no mixed slashing in share/default defines
  // ie. f:/video and f:\video was not be recognised as the same directory,
  // resulting in navigation to a lower directory then the share.
  strPathCpy.Replace("/", "\\");

  VECSHARES shares;
  GetShares(shares);
  for (int i = 0; i < (int)shares.size(); ++i)
  {
    const CShare& share = shares.at(i);
    CStdString strShare = share.strPath;
    strShare.TrimRight("/");
    strShare.TrimRight("\\");
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
bool CVirtualDirectory::IsInShare(const CStdString &path) const
{
  bool isSourceName;
  VECSHARES shares;
  GetShares(shares);
  int iShare = CUtil::GetMatchingShare(path, shares, isSourceName);
  // TODO: May need to handle other special cases that GetMatchingShare() fails on
  return (iShare > -1);
}

void CVirtualDirectory::GetShares(VECSHARES &shares) const
{
  shares = *m_vecShares;
  // add our plug n play shares

  if (m_allowNonLocalShares)
  {
#ifdef HAS_XBOX_HARDWARE
    g_memoryUnitManager.GetMemoryUnitShares(shares);
#endif
    CUtil::AutoDetectionGetShare(shares);
  }

  // and update our dvd share
  for (unsigned int i = 0; i < shares.size(); ++i)
  {
    CShare& share = shares[i];
    if (share.m_iDriveType == SHARE_TYPE_DVD)
    {
      share.strStatus = MEDIA_DETECT::CDetectDVDMedia::GetDVDLabel();
      share.strPath = MEDIA_DETECT::CDetectDVDMedia::GetDVDPath();
    }
  }
}
}

