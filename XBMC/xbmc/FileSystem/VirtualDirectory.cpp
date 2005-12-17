
#include "../stdafx.h"
#include "virtualdirectory.h"
#include "FactoryDirectory.h"
#include "../util.h"
#include "directorycache.h"


CVirtualDirectory::CVirtualDirectory(void) : m_vecShares(NULL)
{
  m_allowPrompting = true;  // by default, prompting is allowed.
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

void CVirtualDirectory::AddShare(const CShare& share)
{
  if (!m_vecShares)
    return;
  unsigned int i;
  for (i=0;i<m_vecShares->size();++i ) 
    if( (*m_vecShares)[i].strPath == share.strPath) // not added before i presume
      break;
  if (i==m_vecShares->size()) // we're safe, then add
    m_vecShares->push_back(share);
}

bool CVirtualDirectory::RemoveShare(const CStdString& strPath)
{
  if (!m_vecShares)
    return true;
  for (vector<CShare>::iterator it=m_vecShares->begin(); it != m_vecShares->end(); ++it)
    if (it->strPath == strPath) 
    {
      m_vecShares->erase(it,it+1);
      return true;
    }

  return false;
}

bool CVirtualDirectory::RemoveShareName(const CStdString& strName)
{
  if (!m_vecShares)
    return true;
  for (vector<CShare>::iterator it=m_vecShares->begin(); it != m_vecShares->end(); ++it)
    if (it->strName == strName) 
    {
      m_vecShares->erase(it,it+1);
      return true;
    }
  return false;
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
    return true;

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

  if (!strPath.IsEmpty())
  {
    bool bIsBookmarkName = false;
    VECSHARES vecShares = *m_vecShares;
    int iIndex = CUtil::GetMatchingShare(strPath, vecShares, bIsBookmarkName);
    // added exception for various local hd items
    if (iIndex > -1 || strPath.Mid(1, 1) == ":")
    {
      // Only cache directory we are getting now
      if (strPath.Left(7) != "lastfm:")
        g_directoryCache.Clear();
      return CDirectory::GetDirectory(strPath, items, m_strFileMask, bUseFileDirectories, m_allowPrompting);
    }

    // what do with an invalid path?
    // return false so the calling window can deal with the error accordingly
    // otherwise the root bookmark listing is returned which seems incorrect but was the previous behaviour
    CLog::Log(LOGERROR,"CVirtualDirectory::GetDirectory(%s) matches no valid bookmark, getting root bookmark list instead", strPath.c_str());
    return false;
  }

  // if strPath is blank, return the root bookmark listing
  items.Clear();
  for (int i = 0; i < (int)m_vecShares->size(); ++i)
  {
    CShare& share = m_vecShares->at(i);
    if ((share.strPath.substr(0,6) == "rar://") || (share.strPath.substr(0,6) == "zip://")) // do not add the virtual archive shares to list
      continue;

    CFileItem* pItem = new CFileItem(share);
    CStdString strPathUpper = pItem->m_strPath;
    strPathUpper.ToUpper();

    CStdString strIcon;
    // We have the real DVD-ROM, set icon on disktype
    if (share.m_iDriveType == SHARE_TYPE_DVD)
      CUtil::GetDVDDriveIcon( pItem->m_strPath, strIcon );
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

    if (share.m_iLockMode > 0)
      strIcon = "defaultLocked.png";

    pItem->SetIconImage(strIcon);
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
  if (strPath.Left(6) == "zip://" || strPath.Left(6) == "rar://") // fucks up directory navigation otherwise..
    return false; 

  CStdString strPathCpy = strPath;
  strPathCpy.TrimRight("/");
  strPathCpy.TrimRight("\\");

  // just to make sure there's no mixed slashing in share/default defines
  // ie. f:/video and f:\video was not be recognised as the same directory,
  // resulting in navigation to a lower directory then the share.
  strPathCpy.Replace("/", "\\");
  for (int i = 0; i < (int)m_vecShares->size(); ++i)
  {
    const CShare& share = m_vecShares->at(i);
    CStdString strShare = share.strPath;
    strShare.TrimRight("/");
    strShare.TrimRight("\\");
    strShare.Replace("/", "\\");
    if (strShare == strPathCpy) return true;
  }
  return false;
}

/*!
 \brief Retrieve the current share type of the DVD-Drive.
 \return Returns the share type, eg. iso9660://.
 */
CStdString CVirtualDirectory::GetDVDDriveUrl()
{
  for (int i = 0; i < (int)m_vecShares->size(); ++i)
  {
    const CShare& share = m_vecShares->at(i);
    if (share.m_iDriveType == SHARE_TYPE_DVD)
      return share.strPath;
  }

  return "";
}
