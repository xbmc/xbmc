
#include "stdafx.h"
#include "virtualdirectory.h"
#include "FactoryDirectory.h"
#include "../settings.h"
#include "file.h"
#include "../util.h"
#include "../DetectDVDType.h"
#include "../GUIDialogOK.h"
#include "guiwindowmanager.h"
#include "directorycache.h"

using namespace DIRECTORY;
using namespace XFILE;

CVirtualDirectory::CVirtualDirectory(void)
{
}

CVirtualDirectory::~CVirtualDirectory(void)
{
}

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
bool CVirtualDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
  CStdString strPath2=strPath;
  CStdString strPath3=strPath;
	strPath2 += "/";
	strPath3 += "\\";
	for (int i=0; i < (int)m_vecShares->size(); ++i)
	{
		CShare& share=m_vecShares->at(i);
		if ( share.strPath == strPath.Left( share.strPath.size() )  ||
			share.strPath == strPath2.Left( share.strPath.size() )  ||
			share.strPath == strPath3.Left( share.strPath.size() ) ||
			strPath.Mid(1,1) == ":" )
		{
/*
			//	Check if cd is detected already before
			//	reading the directory.
			if ( share.m_iDriveType == SHARE_TYPE_DVD )
				CDetectDVDMedia::WaitMediaReady();
*/
			CFactoryDirectory factory;
			CStdString path;

			// get local path
			if (strPath.Mid(1,1) == ":")
			{
				path = strPath;
			}
			else
			{
				path = share.strPath;
			}
			IDirectory *pDirectory = factory.Create(path);

			if (!pDirectory) return false;

			//	Only cache directory we are getting now
			g_directoryCache.Clear();
			pDirectory->SetMask(m_strFileMask);
			bool bResult=pDirectory->GetDirectory(strPath,items);
			//CUtil::SetThumbs(items);
			//CUtil::FillInDefaultIcons(items);
			delete pDirectory;
			return bResult;
		}
	}

 items.erase(items.begin(),items.end());
 for (int i=0; i < (int)m_vecShares->size(); ++i)
	{
		CShare& share=m_vecShares->at(i);
		CFileItem* pItem = new CFileItem(share.strName);
		pItem->m_bIsFolder=true;
		pItem->m_strPath=share.strPath;
		CStdString strPathUpper=share.strPath;
		strPathUpper.ToUpper();

		CStdString strIcon;
			//	We have the real DVD-ROM, set icon on disktype
		if (share.m_iDriveType==SHARE_TYPE_DVD)
			CUtil::GetDVDDriveIcon( pItem->m_strPath, strIcon );
		else if (strPathUpper.Left(11) =="SOUNDTRACK:")
			strIcon="defaultHardDisk.png";
		else if (CUtil::IsRemote(pItem->m_strPath))
			strIcon="defaultNetwork.png";	
		else if (CUtil::IsISO9660(pItem->m_strPath))
			strIcon="defaultDVDRom.png";
		else if (CUtil::IsDVD(pItem->m_strPath))
			strIcon="defaultDVDRom.png";
		else if (CUtil::IsCDDA(pItem->m_strPath))
				strIcon="defaultCDDA.png";
		else 
			strIcon="defaultHardDisk.png";

		pItem->SetIconImage(strIcon);
		pItem->m_bIsShareOrDrive=true;
		pItem->m_iDriveType=share.m_iDriveType;
		CStdString strBig;
		int iPos=strIcon.Find(".");
		strBig=strIcon.Left(iPos);
		strBig+="Big";
		strBig+=strIcon.Right(strIcon.size()-(iPos));
		pItem->SetThumbnailImage(strBig);
		items.push_back(pItem);
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
	for (int i=0; i < (int)m_vecShares->size(); ++i)
	{
		const CShare& share=m_vecShares->at(i);
		CStdString strShare = share.strPath;
		strShare.TrimRight("/");
		strShare.TrimRight("\\");
		if (strShare==strPathCpy) return true;
	}
	return false;
}

/*!
	\brief Retrieve the current share type of the DVD-Drive.
	\return Returns the share type, eg. iso9660://.
	*/
CStdString CVirtualDirectory::GetDVDDriveUrl()
{
	for (int i=0; i < (int)m_vecShares->size(); ++i)
	{
		const CShare& share=m_vecShares->at(i);
		if (share.m_iDriveType==SHARE_TYPE_DVD) 
			return share.strPath;
	}

	return "";
}
