
#include "stdafx.h"
#include "musicinfotagloadermp4.h"
#include "sectionloader.h"
#include "Util.h"
#include "picture.h"
#include "utils/log.h"
#include "autoptrhandle.h"
#include "XMP4File.h"

using namespace AUTOPTR;
using namespace MUSIC_INFO;

CMusicInfoTagLoaderMP4::CMusicInfoTagLoaderMP4(void)
{
	CSectionLoader::Load("LIBMP4");
}

CMusicInfoTagLoaderMP4::~CMusicInfoTagLoaderMP4()
{
	CSectionLoader::Unload("LIBMP4");
}

bool CMusicInfoTagLoaderMP4::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	try
	{
		tag.SetURL(strFileName);
		// retrieve the mp4 metadata info from strFileName
		// and put it in tag
		bool bResult=false;
		XMP4File file;

		try
		{
			file.Read(strFileName.c_str());
		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
			return false;
		}
		
		char* pBuffer=NULL;
		u_int16_t nTrackNum=0;
		u_int16_t nTrackCount=0;

		try
		{
			if (file.GetMetadataGenre(&pBuffer))
			{
				tag.SetGenre(pBuffer);
				delete pBuffer;
			}
		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
		}

		try
		{
			if (file.GetMetadataName(&pBuffer))
			{
				bResult = true;
				tag.SetLoaded(true);
				tag.SetTitle(pBuffer);
				delete pBuffer;
			}
		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
			tag.SetLoaded(false);
			return false;
		}

		try
		{
			if (file.GetMetadataArtist(&pBuffer))
			{
				tag.SetArtist(pBuffer);
				delete pBuffer;
			}
		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
		}

		try
		{
			if (file.GetMetadataAlbum(&pBuffer))
			{
				tag.SetAlbum(pBuffer);
				delete pBuffer;
			}
		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
		}

		try
		{
			if (file.GetMetadataYear(&pBuffer))
			{
				SYSTEMTIME dateTime;
				dateTime.wYear=atoi(pBuffer);
				tag.SetReleaseDate(dateTime);
				delete pBuffer;
			}
		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
		}

		try
		{
			if (file.GetMetadataTrack(&nTrackNum, &nTrackCount))
			{
				tag.SetTrackNumber(nTrackNum);
			}
		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
		}

		u_int8_t* pCover=NULL;
		u_int32_t nSize=0;
		try
		{
			file.GetMetadataCoverArt(&pCover, &nSize);

			if (pCover && nSize>0)
			{
				CStdString strCoverArt, strPath, strFileName;
				CUtil::Split(tag.GetURL(), strPath, strFileName);
				CUtil::GetAlbumThumb(tag.GetAlbum()+strPath, strCoverArt,true);

				CPicture pic;
				if (pic.CreateAlbumThumbnailFromMemory(pCover, nSize, "", strCoverArt))
				{
					CUtil::ThumbCacheAdd(strCoverArt, true);
				}
				else
				{
					CUtil::ThumbCacheAdd(strCoverArt, false);
					CLog::Log("Tag loader mp4: Unable to create album art for %s (size=%d)", tag.GetURL().c_str(), nSize);
				}

				delete pCover;
			}
		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
			if (pCover)
				delete pCover;
		}

		try
		{
			MP4Duration duration=file.GetDuration();
			if (duration>0)
			{
				tag.SetDuration((int)(duration/file.GetTimeScale()));
			}

		}
		catch(MP4Error* e)
		{
			CLog::Log("Tag loader mp4: %s, errno=%i in file %s", e->m_errstring, e->m_errno, strFileName.c_str());
			delete e;
		}

		file.Close();
		return bResult;
	}
	catch(...)
	{
		CLog::Log("Tag loader mp4: exception in file %s", strFileName.c_str());
	}

	tag.SetLoaded(false);
	return false;
}
