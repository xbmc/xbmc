#include "virtualdirectory.h"
#include "FactoryDirectory.h"
#include "../settings.h"
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
		pItem->m_strPath=share.strPath;

		CStdString strIcon;
		if (CUtil::IsRemote(pItem->m_strPath) )
			strIcon="defaultNetwork.png";	
		else if (CUtil::IsDVD(pItem->m_strPath) )
			strIcon="defaultDVDRom.png";
		else 
			strIcon="defaultHardDisk.png";

		pItem->SetIconImage(strIcon);
		pItem->m_bIsShareOrDrive=true;
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
		pItem->m_bIsShareOrDrive=false;
		if (pItem->m_bIsFolder)
		{
		}
		else
		{
			if (CUtil::IsPicture(pItem->m_strPath) )
			{
				pItem->SetIconImage("defaultPicture.png");
			}
			if (CUtil::IsAudio(pItem->m_strPath) )
			{
				pItem->SetIconImage("defaultAudio.png");
			}
			if (CUtil::IsXBE(pItem->m_strPath) )
			{
				pItem->SetIconImage("defaultProgram.png");
			}

			if (CUtil::IsVideo(pItem->m_strPath) )
			{
				pItem->SetIconImage("defaultVideo.png");
			}
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

		
		if (pItem->GetThumbnailImage()=="")
		{
			if (pItem->GetIconImage()=="")
			{
				if (pItem->m_bIsFolder)
				{
					pItem->SetIconImage("defaultFolder.png");
				}
				else
				{
					CStdString strExtension;
					CUtil::GetExtension(pItem->m_strPath,strExtension);
					
					for (int i=0; i < (int)g_settings.m_vecIcons.size(); ++i)
					{
						CFileTypeIcon& icon=g_settings.m_vecIcons[i];

						if (CUtil::cmpnocase(strExtension.c_str(), icon.m_strName)==0)
						{
							pItem->SetIconImage(icon.m_strIcon);
							break;
						}
					}
				}
			}

			if (pItem->GetIconImage()!="")
			{
				CStdString strBig;
				int iPos=pItem->GetIconImage().Find(".");
				strBig=pItem->GetIconImage().Left(iPos);
				strBig+="Big";
				strBig+=pItem->GetIconImage().Right(pItem->GetIconImage().size()-(iPos));
				pItem->SetThumbnailImage(strBig);
			}
		}
  }
}