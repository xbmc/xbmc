
#include "stdafx.h"
#include "musicinfotagloaderWMA.h"
#include "stdstring.h"
#include "sectionloader.h"
#include "utils/log.h"
#include "autoptrhandle.h"
#include "util.h"
#include "picture.h"

using namespace MUSIC_INFO;
using namespace XFILE;
using namespace AUTOPTR;

//	WMA metadata attribut types
//	http://msdn.microsoft.com/library/en-us/wmform/htm/attributelist.asp
typedef enum WMT_ATTR_DATATYPE
{
	WMT_TYPE_STRING  = 0,
	WMT_TYPE_BINARY  = 1,
	WMT_TYPE_BOOL    = 2,
	WMT_TYPE_DWORD   = 3,
	WMT_TYPE_QWORD   = 4,
	WMT_TYPE_WORD    = 5,
} WMT_ATTR_DATATYPE;

//	Data item for the WM/Picture metadata attribute
//	http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wmform/htm/wm_picture.asp
typedef struct _WMPicture
{
	LPWSTR	pwszMIMEType;
	BYTE		bPictureType;
	LPWSTR	pwszDescription;
	DWORD		dwDataLen;
	BYTE*		pbData;
} WM_PICTURE;

CMusicInfoTagLoaderWMA::CMusicInfoTagLoaderWMA(void)
{
}

CMusicInfoTagLoaderWMA::~CMusicInfoTagLoaderWMA()
{
}

// Based on MediaInfo
// by Jérôme Martinez, Zen@MediaArea.net
// http://sourceforge.net/projects/mediainfo/
bool CMusicInfoTagLoaderWMA::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	try
	{
		tag.SetLoaded(false);
		CFile file;
		if (!file.Open(strFileName.c_str())) return false;

		tag.SetURL(strFileName);

		auto_aptr<unsigned char> pData(new unsigned char[65536]);
		file.Read(pData.get(), 65536);
		file.Close();

		int iOffset=0;
		unsigned int* pDataI;

		//Play time
		iOffset=0;
		pDataI=(unsigned int*)pData.get();
		while (!(pDataI[0]==0x75B22630 && pDataI[1]==0x11CF668E && pDataI[2]==0xAA00D9A6 && pDataI[3]==0x6CCE6200) && iOffset<=65536-4)
		{
			iOffset++;
			pDataI=(unsigned int*)(pData.get()+iOffset);
		}
		if (iOffset>65536-4)
			return false;

		//Play time
		iOffset=0;
		pDataI=(unsigned int*)pData.get();
		while (!(pDataI[0]==0x8CABDCA1 && pDataI[1]==0x11CFA947 && pDataI[2]==0xC000E48E && pDataI[3]==0x6553200C) && iOffset<=65536-4)
		{
			iOffset++;
			pDataI=(unsigned int*)(pData.get()+iOffset);
		}
		if (iOffset<=65536-4)
		{
			iOffset+=64;
			pDataI=(unsigned int*)(pData.get()+iOffset);
			float F1=(float)pDataI[1];
			F1=F1*0x10000*0x10000+pDataI[0];
			tag.SetDuration((long)((F1/10000)/1000));	//	from milliseconds to seconds
		}

		//Description  Title
		iOffset=0;
		pDataI=(unsigned int*)pData.get();
		while (!(pDataI[0]==0x75B22633 && pDataI[1]==0x11CF668E && pDataI[2]==0xAA00D9A6 && pDataI[3]==0x6CCE6200) && iOffset<=65536-4)
		{
			iOffset++;
			pDataI=(unsigned int*)(pData.get()+iOffset);
		}
		if (iOffset<=65536-4)
		{
			iOffset+=24;
			int nTitleSize			= pData[iOffset+0]+pData[iOffset+1]*0x100;
			int nAuthorSize			= pData[iOffset+2]+pData[iOffset+3]*0x100;
			int nCopyrightSize	= pData[iOffset+4]+pData[iOffset+5]*0x100;
			int nCommentsSize		= pData[iOffset+6]+pData[iOffset+7]*0x100;

			iOffset+=10;

			tag.SetTitle((LPWSTR)(pData.get()+iOffset));	// titel
			tag.SetArtist((LPWSTR)(pData.get()+iOffset+nTitleSize)); //	author
			//General(ZT("Copyright"))=(LPWSTR)(pData.get()+iOffset+(nTitleSize+nAuthorSize));
			//General(ZT("Comments"))=(LPWSTR)(pData.get()+iOffset+(nTitleSize+nAuthorSize+nCopyrightSize));
		}

		//	Maybe these information can be usefull in the future

		//Info audio
		//iOffset=0;
		//pDataI=(unsigned int*)pData;
		//while (!(pDataI[0]==0xF8699E40 && pDataI[1]==0x11CF5B4D && pDataI[2]==0x8000FDA8 && pDataI[3]==0x2B445C5F) && iOffset<=65536-4)
		//{
			//iOffset++;
			//pDataI=(unsigned int*)(pData+iOffset);
		//}
		//if (iOffset<=65536-4)
		//{
			//iOffset+=54;
			////Codec
			//TCHAR C1[30]; 
		  //_itoa(pData[iOffset]+pData[iOffset+1]*0x100, C1, 16);
			//CStdString Codec=C1;
			//while (Codec.size()<4)
			//		Codec='0'+Codec;
			//Audio[0](ZT("Codec"))=Codec;
			//Audio[0](ZT("Channels"))=pData[iOffset+2]; //2 octets
			//pDataI=(unsigned int*)(pData+iOffset);
			//Audio[0](ZT("SamplingRate"))=pDataI[1];
			//Audio[0](ZT("BitRate"))=pDataI[2]*8;
		//}

		//Info video
		//iOffset=0;
		//pDataI=(unsigned int*)pData;
		//while (!(pDataI[0]==0xBC19EFC0 && pDataI[1]==0x11CF5B4D && pDataI[2]==0x8000FDA8 && pDataI[3]==0x2B445C5F) && iOffset<=65536-4)
		//{
			//iOffset++;
			//pDataI=(unsigned int*)(pData+iOffset);
		//}
		//if (iOffset<=65536-4)
		//{
			//iOffset+=54;
			//iOffset+=15;
			//pDataI=(unsigned int*)(pData+iOffset);
			//Video[0](ZT("Width"))=pDataI[0];
			//Video[0](ZT("Height"))=pDataI[1];
			//Codec
			//unsigned char C1[5]; C1[4]='\0';
			//C1[0]=pData[iOffset+12+0]; C1[1]=pData[iOffset+12+1]; C1[2]=pData[iOffset+12+2]; C1[3]=pData[iOffset+12+3];
			//Video[0](ZT("Codec"))=wxString((char*)C1,wxConvUTF8).c_str();
		//}


		//Read extended metadata
		iOffset=0;
		pDataI=(unsigned int*)pData.get();
		while (!(pDataI[0]==0xD2D0A440 && pDataI[1]==0x11D2E307 && pDataI[2]==0xA000F097 && pDataI[3]==0x50A85EC9) && iOffset<=65536-4)
		{
			iOffset++;
			pDataI=(unsigned int*)(pData.get()+iOffset);
		}

		if (iOffset<=65536-4)
		{
			iOffset+=24;

			//	Walk through all frames in the file
			int iNumOfFrames=pData[iOffset]+pData[iOffset+1]*0x100;
			iOffset+=2;
			for (int Pos=0; Pos<iNumOfFrames; Pos++)
			{
				int iFrameNameSize=pData[iOffset]+(pData[iOffset+1]*0x100);
				iOffset+=2;

				//	Get frame name
				CStdString strFrameName((LPWSTR)(pData.get()+iOffset));
				iOffset+=iFrameNameSize;

				//	Get datatype of frame
				int iFrameType=pData[iOffset]+pData[iOffset+1];
				iOffset+=2;

				//	Size of frame value
				int iValueSize=pData[iOffset]+(pData[iOffset+1]*0x100);
				iOffset+=2;

				//	Parse frame value
				LPWSTR	pwszValue;
				BOOL		bValue;
				DWORD		dwValue;
				DWORD		qwValue;
				WORD		wValue;
				BYTE*		pValue;
				if (iFrameType==WMT_TYPE_STRING)
				{
					pwszValue=(LPWSTR)(pData.get()+iOffset);
				}
				else if (iFrameType==WMT_TYPE_BINARY)
					pValue=(BYTE*)(pData.get()+iOffset);	//	Raw data
				else if (iFrameType==WMT_TYPE_BOOL)
					bValue=(BOOL)pData[iOffset];
				else if (iFrameType==WMT_TYPE_DWORD)
					dwValue=pData[iOffset]+pData[iOffset+1]*0x100+pData[iOffset+2]*0x10000+pData[iOffset+3]*0x1000000;
				else if (iFrameType==WMT_TYPE_QWORD)
					qwValue=pData[iOffset]+pData[iOffset+1]*0x100+pData[iOffset+2]*0x10000+pData[iOffset+3]*0x1000000;
				else if (iFrameType==WMT_TYPE_WORD)
					wValue=pData[iOffset]+pData[iOffset+1]*0x100;

				//	Fill tag with extended metadata
				if (strFrameName=="WM/AlbumTitle" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				{
					tag.SetAlbum(pwszValue);
				}
				else if (strFrameName=="WM/AlbumArtist" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				{
					if (tag.GetArtist().IsEmpty()) tag.SetArtist(pwszValue);
				}
				else if (strFrameName=="WM/TrackNumber" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				{
					if (tag.GetTrackNumber()<=0) tag.SetTrackNumber(_wtoi(pwszValue));
				}
				else if (strFrameName=="WM/TrackNumber" && iFrameType==WMT_TYPE_DWORD && iValueSize>0)
				{
					if (tag.GetTrackNumber()<=0) tag.SetTrackNumber(dwValue);
				}
				//else if (Nom=="WM/Track" && iAttrDataType==WMT_TYPE_STRING && iValueSize>0)	//	Old Tracknumber, should not be used anymore
				else if (strFrameName=="WM/Year" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				{
					SYSTEMTIME dateTime;
					dateTime.wYear=_wtoi(pwszValue);
					tag.SetReleaseDate(dateTime);
				}
				else if (strFrameName=="WM/Genre" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				{
					tag.SetGenre(pwszValue);
				}
				else if (strFrameName=="WM/Picture" && iFrameType==WMT_TYPE_BINARY && iValueSize>0)
				{
					WM_PICTURE picture;
					int iPicOffset=0;

					//	Picture types: http://msdn.microsoft.com/library/en-us/wmform/htm/wm_picture.asp
					picture.bPictureType=(BYTE)pValue[iPicOffset];
					iPicOffset+=1;

					picture.dwDataLen=(DWORD)pValue[iPicOffset]+(pValue[iPicOffset+1]*0x100);
					iPicOffset+=4;

					picture.pwszMIMEType=(LPWSTR)(pValue+iPicOffset);
					iPicOffset+=(wcslen(picture.pwszMIMEType)*2);
					iPicOffset+=2;

					picture.pwszDescription=(LPWSTR)(pValue+iPicOffset);
					iPicOffset+=(wcslen(picture.pwszDescription)*2);
					iPicOffset+=2;
					
					picture.pbData=(pValue+iPicOffset);

					if (picture.bPictureType==3) //	Cover Front
					{
						CStdString strExtension(picture.pwszMIMEType);
						CStdString strCoverArt, strPath, strFileName;
						CUtil::Split(tag.GetURL(), strPath, strFileName);
						CUtil::GetAlbumThumb(tag.GetAlbum()+strPath, strCoverArt,true);
						if (!CUtil::ThumbExists(strCoverArt))
						{
							int nPos=strExtension.Find('/');
							if (nPos>-1)
								strExtension.Delete(0, nPos+1);

							if (picture.pbData!=NULL && picture.dwDataLen>0)
							{
								CPicture pic;
								if (pic.CreateAlbumThumbnailFromMemory(picture.pbData, picture.dwDataLen, strExtension, strCoverArt))
								{
									CUtil::ThumbCacheAdd(strCoverArt, true);
								}
								else
								{
									CUtil::ThumbCacheAdd(strCoverArt, false);
									CLog::Log("Tag loader wma: Unable to create album art for %s (extension=%s, size=%d)", tag.GetURL().c_str(), strExtension.c_str(), picture.dwDataLen);
								}
							}
						}
					}
				}
				//else if (strFrameName=="isVBR" && iFrameType==WMT_TYPE_BOOL && iValueSize>0 && bValue)
				//{
				//}
				//else if (strFrameName=="WM/DRM" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				//{
				//	//	File is DRM protected
				//	pwszValue;
				//}
				//else if (strFrameName=="WM/Codec" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				//{
				//	pwszValue;
				//}
				//else if (strFrameName=="WM/BeatsPerMinute" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				//{
				//	pwszValue;
				//}
				//else if (strFrameName=="WM/Mood" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				//{
				//	pwszValue;
				//}
				//else if (strFrameName=="WM/RadioStationName" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				//{
				//	pwszValue;
				//}
				//else if (strFrameName=="WM/RadioStationOwner" && iFrameType==WMT_TYPE_STRING && iValueSize>0)
				//{
				//	pwszValue;
				//}

				//	parse next frame
				iOffset+=iValueSize;
			}
		}

		tag.SetLoaded(true);
		return true;
	}
	catch(...)
	{
		CLog::Log("Tag loader wma: exception in file %s", strFileName.c_str());
	}

	tag.SetLoaded(false);
	return false;
}
