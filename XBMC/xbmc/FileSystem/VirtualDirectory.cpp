#include "virtualdirectory.h"
#include "FactoryDirectory.h"
#include "file.h"
#include "../util.h"

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
  m_vecShares.erase(m_vecShares.begin(),m_vecShares.end());
  for (int i=0; i < (int)vecShares.size(); ++i)
  {
    CShare share=vecShares[i];
    m_vecShares.push_back( share );
  }
}

bool CVirtualDirectory::GetDirectory(const CStdString& strPath,VECFILEITEMS &items)
{
  CStdString strPath2=strPath;
  CStdString strPath3=strPath;
	strPath2 += "/";
	strPath3 += "\\";
	for (int i=0; i < (int)m_vecShares.size(); ++i)
	{
		CShare& share=m_vecShares[i];
		if ( share.strPath == strPath.Left( share.strPath.size() )  ||
			   share.strPath == strPath2.Left( share.strPath.size() )  ||
				 share.strPath == strPath3.Left( share.strPath.size() ) )
		{
			CFactoryDirectory factory;
			CDirectory *pDirectory=factory.Create(share.strPath);
			if (!pDirectory) return false;
      pDirectory->SetMask(m_strFileMask);
			bool bResult=pDirectory->GetDirectory(strPath,items);
      CacheThumbs(items);
			delete pDirectory;
			return bResult;
		}
	}

 items.erase(items.begin(),items.end());
 for (int i=0; i < (int)m_vecShares.size(); ++i)
	{
		CShare& share=m_vecShares[i];
		CFileItem* pItem = new CFileItem(share.strName);
		pItem->m_bIsFolder=true;
    pItem->m_bIsShareOrDrive=false;
		pItem->m_strPath=share.strPath;
		pItem->SetIconImage(share.strIcon);
		CStdString strBig;
		int iPos=share.strIcon.Find(".");
		strBig=share.strIcon.Left(iPos);
		strBig+="Big";
		strBig+=share.strIcon.Right(share.strIcon.size()-(iPos));
		pItem->SetThumbnailImage(strBig);
		items.push_back(pItem);
	}

  return true;
}


bool CVirtualDirectory::IsShare(const CStdString& strPath) const
{
	for (int i=0; i < (int)m_vecShares.size(); ++i)
	{
		const CShare& share=m_vecShares[i];
		if (share.strPath==strPath) return true;
	}
	return false;
}

void  CVirtualDirectory::CacheThumbs(VECFILEITEMS &items)
{
  CStdString strThumb;
  for (int i=0; i < (int)items.size(); ++i)
  {
    CFileItem* pItem=items[i];

		if (pItem->m_bIsFolder)
		{
			pItem->SetIconImage("icon-folder.png");
		}
		
		if (pItem->HasThumbnail() )
		{
			CUtil::GetThumbnail( pItem->m_strPath,strThumb);
      
			CFile file;
			CStdString strThumbnailFileName;
			CUtil::ReplaceExtension(pItem->m_strPath,".tbn", strThumbnailFileName);
			if ( file.Cache(strThumbnailFileName.c_str(), strThumb.c_str()))
			{
				pItem->SetThumbnailImage(strThumb);
			}
			else
			{
				
				CUtil::GetThumbnail(pItem->m_strPath,strThumb);
				if (CUtil::FileExists(strThumb) )
				{
					pItem->SetThumbnailImage(strThumb);
				}
			}
		}
		else
		{
			CUtil::GetThumbnail(pItem->m_strPath,strThumb);
			if (CUtil::FileExists(strThumb))
			{
				pItem->SetThumbnailImage(strThumb);
			}
		}
		
  }
}