#include "virtualdirectory.h"
#include "FactoryDirectory.h"
#include "../settings.h"
#include "file.h"
#include "../util.h"
#include "../DetectDVDType.h"
#include "../GUIDialogOK.h"
#include "guiwindowmanager.h"


using namespace DIRECTORY;
using namespace XFILE;

CVirtualDirectory::CVirtualDirectory(void)
{
}

CVirtualDirectory::~CVirtualDirectory(void)
{
}

void CVirtualDirectory::SetShares(VECSHARES& vecShares)
{
	m_vecShares = &vecShares;
}

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
				 share.strPath == strPath3.Left( share.strPath.size() ) )
		{
#if 0
			//	Check if cd is detected already before
			//	reading the directory.
			if ( share.m_iDriveType == SHARE_TYPE_DVD )
				CDetectDVDMedia::WaitMediaReady();
#endif
			CFactoryDirectory factory;
			CDirectory *pDirectory = factory.Create(share.strPath);

			if (!pDirectory) return false;
			pDirectory->SetMask(m_strFileMask);
			bool bResult=pDirectory->GetDirectory(strPath,items);
			CUtil::SetThumbs(items);
			CUtil::FillInDefaultIcons(items);
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

		CStdString strIcon;
			//	We have the real DVD-ROM, set icon on disktype
		if (share.m_iDriveType==SHARE_TYPE_DVD)
			CUtil::GetDVDDriveIcon( pItem->m_strPath, strIcon );
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

bool CVirtualDirectory::IsShare(const CStdString& strPath) const
{
	for (int i=0; i < (int)m_vecShares->size(); ++i)
	{
		const CShare& share=m_vecShares->at(i);
		if (share.strPath==strPath) return true;
	}
	return false;
}

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
