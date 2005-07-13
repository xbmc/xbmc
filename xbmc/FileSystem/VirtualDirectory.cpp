
#include "../stdafx.h"
#include "virtualdirectory.h"
#include "FactoryDirectory.h"
#include "../util.h"
#include "directorycache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CVirtualDirectory::CVirtualDirectory(void)
{}

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
  unsigned int i;
  for (i=0;i<m_vecShares->size();++i ) 
    if( (*m_vecShares)[i].strPath == share.strPath) // not added before i presume
      break;
  if (i==m_vecShares->size()) // we're safe, then add
    m_vecShares->push_back(share);
}

bool CVirtualDirectory::RemoveShare(const CStdString& strPath)
{
  for (vector<CShare>::iterator it=m_vecShares->begin(); it != m_vecShares->end(); ++it)
    if ((*it).strPath == strPath) 
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
  CStdString strPath2 = strPath;
  CStdString strPath3 = strPath;
  strPath2 += "/";
  strPath3 += "\\";
  for (int i = 0; i < (int)m_vecShares->size(); ++i)
  {
    CShare& share = m_vecShares->at(i);
    if ( share.strPath == strPath.Left( share.strPath.size() ) ||
         share.strPath == strPath2.Left( share.strPath.size() ) ||
         share.strPath == strPath3.Left( share.strPath.size() ) ||
         strPath.Mid(1, 1) == ":" )
    {
      // Only cache directory we are getting now
      g_directoryCache.Clear();

      return CDirectory::GetDirectory(strPath, items, m_strFileMask);
    }
  }

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
  if (strPath.Left(6) == "zip://") // fucks up directory navigation otherwise..
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
