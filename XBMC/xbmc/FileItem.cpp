#include "stdafx.h"
#include "fileitem.h"
#include "Util.h"
#include "picture.h"
#include "playlistfactory.h"
#include "shortcut.h"
#include "crc32.h"
#include "filesystem/DirectoryCache.h"
#include "musicInfoTagLoaderFactory.h"

CFileItem::CFileItem(const CSong& song)
{
	Clear();
	m_strLabel=song.strTitle;
	m_strPath=song.strFileName;
	m_musicInfoTag.SetSong(song);
	m_lStartOffset = song.iStartOffset;
	m_lEndOffset = song.iEndOffset;
  m_strThumbnailImage = song.strThumb;
}

CFileItem::CFileItem(const CAlbum& album)
{
	Clear();
	m_strLabel=album.strAlbum;
	m_strPath=album.strPath;
	m_bIsFolder=true;
	m_strLabel2=album.strArtist;
	m_musicInfoTag.SetAlbum(album);
}

CFileItem::CFileItem(const CFileItem& item)
{
	*this=item;
}

CFileItem::CFileItem(void)
{
	Clear();
}

CFileItem::CFileItem(const CStdString& strLabel)
:CGUIListItem()
{
	Clear();
	m_strLabel=strLabel;
}

CFileItem::CFileItem(const CStdString& strPath, bool bIsFolder)
{
	Clear();
	m_strPath=strPath;
	m_bIsFolder=bIsFolder;
}

CFileItem::CFileItem(const CShare& share)
{
	Clear();
	m_bIsFolder=true;
	m_bIsShareOrDrive=true;
	m_strPath=share.strPath;
	m_strLabel=share.strName;
	m_iLockMode=share.m_iLockMode;
	m_strLockCode=share.m_strLockCode;
	m_iBadPwdCount=share.m_iBadPwdCount;
	m_iDriveType=share.m_iDriveType;
	m_idepth=share.m_iDepthSize;
}

CFileItem::~CFileItem(void)
{

}

const CFileItem& CFileItem::operator=(const CFileItem& item)
{
	if (this==&item) return *this;
	m_strLabel2=item.m_strLabel2;
	m_strLabel=item.m_strLabel;
	FreeMemory();
	m_bSelected=item.m_bSelected;
	m_strIcon=item.m_strIcon;
	m_strThumbnailImage=item.m_strThumbnailImage;
	m_strPath=item.m_strPath;
	m_bIsFolder=item.m_bIsFolder;
	m_iDriveType=item.m_iDriveType;
	m_bIsShareOrDrive=item.m_bIsShareOrDrive;
	memcpy(&m_stTime,&item.m_stTime,sizeof(SYSTEMTIME));
	m_dwSize=item.m_dwSize;
	m_musicInfoTag=item.m_musicInfoTag;
	m_lStartOffset = item.m_lStartOffset;
	m_lEndOffset = item.m_lEndOffset;
	m_fRating=item.m_fRating;
	m_strDVDLabel=item.m_strDVDLabel;
	m_iprogramCount=item.m_iprogramCount;
  m_iLockMode=item.m_iLockMode;
  m_strLockCode=item.m_strLockCode;
  m_iBadPwdCount=item.m_iBadPwdCount;
	return *this;
}

void CFileItem::Clear()
{
	m_strLabel2="";
	m_strLabel="";
	FreeIcons();
	m_musicInfoTag.Clear();
	m_bSelected=false;
	m_fRating=0.0f;
	m_strDVDLabel="";
	m_strPath = "";
	m_fRating=0.0f;
	m_dwSize=0;
	m_bIsFolder=false;
	m_bIsShareOrDrive=false;
	memset(&m_stTime,0,sizeof(m_stTime));
	m_iDriveType = SHARE_TYPE_UNKNOWN;
	m_lStartOffset = 0;
	m_lEndOffset = 0;
	m_iprogramCount=0;
	m_idepth = 1;
  m_iLockMode = LOCK_MODE_EVERYONE;
  m_strLockCode = "";
  m_iBadPwdCount = 0;
}

void CFileItem::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_bIsFolder;
		ar << m_strLabel;
		ar << m_strLabel2;
		ar << m_strThumbnailImage;
		ar << m_strIcon;
		ar << m_bSelected;
		
		ar << m_strPath;
		ar << m_bIsShareOrDrive;
		ar << m_iDriveType;
		ar << m_stTime;
		ar << m_dwSize;
		ar << m_fRating;
		ar << m_strDVDLabel;
		ar << m_iprogramCount;
		ar << m_idepth;
		ar << m_lStartOffset;
		ar << m_lEndOffset;
		ar << m_iLockMode;
		ar << m_strLockCode;
		ar << m_iBadPwdCount;

		ar << m_musicInfoTag;
	}
	else
	{
		ar >> m_bIsFolder;
		ar >> m_strLabel;
		ar >> m_strLabel2;
		ar >> m_strThumbnailImage;
		ar >> m_strIcon;
		ar >> m_bSelected;
		
		ar >> m_strPath;
		ar >> m_bIsShareOrDrive;
		ar >> m_iDriveType;
		ar >> m_stTime;
		ar >> m_dwSize;
		ar >> m_fRating;
		ar >> m_strDVDLabel;
		ar >> m_iprogramCount;
		ar >> m_idepth;
		ar >> m_lStartOffset;
		ar >> m_lEndOffset;
		ar >> m_iLockMode;
		ar >> m_strLockCode;
		ar >> m_iBadPwdCount;

		ar >> m_musicInfoTag;
	}
}

bool CFileItem::IsVideo() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath,strExtension);
  if (strExtension.size() < 2) return  false;
  CUtil::Lower(strExtension);
  if ( strstr( g_stSettings.m_szMyVideoExtensions, strExtension.c_str() ) )
    return true;

  return false;
}

bool CFileItem::IsAudio() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath,strExtension);
  if (strExtension.size() < 2) return  false;
  CUtil::Lower(strExtension);

  if ( strstr( g_stSettings.m_szMyMusicExtensions, strExtension.c_str() ) )
    return true;

  if (strstr(m_strPath.c_str(), ".cdda") ) return true;
  if (IsShoutCast() ) return true;

  return false;
}

bool CFileItem::IsPicture() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath,strExtension);
  if (strExtension.size() < 2) return false;
  CUtil::Lower(strExtension);

  if ( strstr( g_stSettings.m_szMyPicturesExtensions, strExtension.c_str() ) )
    return true;

	if (strExtension == ".tbn") return true;

  return false;
}

bool CFileItem::IsCUESheet() const
{
	CStdString strExtension;
	CUtil::GetExtension(m_strPath, strExtension);
	return (strExtension.CompareNoCase(".cue") == 0);
}

bool CFileItem::IsShoutCast() const
{
  if (strstr(m_strPath.c_str(), "shout:") ) return true;
  return false;
}

bool CFileItem::IsInternetStream() const
{
	CURL url(m_strPath);
	CStdString strProtocol=url.GetProtocol();
	strProtocol.ToLower();

	if (strProtocol.size()==0)
		return false;

  if (strProtocol=="shout" || strProtocol=="mms" || 
			strProtocol=="http"  || strProtocol=="ftp" || 
			strProtocol=="rtsp"  || strProtocol=="rtp" || 
			strProtocol=="udp") 
			return true;

	return false;
}

bool CFileItem::IsPlayList() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath,strExtension);
  strExtension.ToLower();
  if (strExtension==".m3u") return true;
  if (strExtension==".b4s") return true;
  if (strExtension==".pls") return true;
  if (strExtension==".strm") return true;
  if (strExtension==".wpl") return true;
  return false;
}

bool CFileItem::IsPythonScript() const
{
   char* pExtension=CUtil::GetExtension(m_strPath);
   if (!pExtension) return false;
   if (CUtil::cmpnocase(pExtension,".py")==0) return true;
   return false;
}

bool CFileItem::IsXBE() const
{
   char* pExtension=CUtil::GetExtension(m_strPath);
   if (!pExtension) return false;
   if (CUtil::cmpnocase(pExtension,".xbe")==0) return true;
   return false;
}

bool CFileItem::IsDefaultXBE() const
{
  char* pFileName=CUtil::GetFileName(m_strPath);
  if (!pFileName) return false;
  if (CUtil::cmpnocase(pFileName, "default.xbe")==0) return true;
  return false;
}

bool CFileItem::IsShortCut() const
{
   char* pExtension=CUtil::GetExtension(m_strPath);
   if (!pExtension) return false;
   if (CUtil::cmpnocase(pExtension,".cut")==0) return true;
   return false;
}

bool CFileItem::IsNFO() const
{
  char *pExtension=CUtil::GetExtension(m_strPath);
  if (!pExtension) return false;
  if (CUtil::cmpnocase(pExtension,".nfo")==0) return true;
  return false;
}

bool CFileItem::IsDVDImage() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath,strExtension);
  if (strExtension.Equals(".img")) return true;
  return false;
}

bool CFileItem::IsDVDFile(bool bVobs /*= true*/, bool bIfos /*= true*/) const
{
	CStdString strFileName = CUtil::GetFileName(m_strPath);
	if(bIfos)
	{
		if(strFileName.Equals("video_ts.ifo")) return true;
		if(strFileName.Left(4).Equals("vts_") && strFileName.Right(6).Equals("_0.ifo") && strFileName.length() == 12) return true;
	}
	if(bVobs)
	{
		if(strFileName.Equals("video_ts.vob")) return true;
		if(strFileName.Left(4).Equals("vts_") && strFileName.Right(4).Equals(".vob")) return true;
	}

	return false;
}

bool CFileItem::IsRAR() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath,strExtension);
  if ( (strExtension.CompareNoCase(".rar") == 0) || (strExtension.Equals(".001")) ) return true; // sometimes the first rar is named .001
  return false;
}

bool CFileItem::IsCDDA() const
{
	return CUtil::IsCDDA(m_strPath);
}

bool CFileItem::IsDVD() const
{
	return CUtil::IsDVD(m_strPath);
}

bool CFileItem::IsISO9660() const
{
	return CUtil::IsISO9660(m_strPath);
}

bool CFileItem::IsRemote() const
{
	return CUtil::IsRemote(m_strPath);
}

bool  CFileItem::IsSmb() const
{
	return CUtil::IsSmb(m_strPath);
}

bool CFileItem::IsHD() const
{
	return CUtil::IsHD(m_strPath);
}

bool CFileItem::IsVirtualDirectoryRoot() const
{
	return (m_bIsFolder && m_strPath.IsEmpty());
}

bool CFileItem::IsReadOnly() const
{
	// check for dvd/cd media
	if (IsDVD() || IsCDDA() || IsISO9660()) return true;
	// now check for remote shares (smb is writeable??)
	if (IsSmb()) return false;
	// no other protocols are writeable??
	if (IsRemote()) return true;
	return false;
}

bool CFileItem::HasDefaultThumb() const
{
	CStdString strThumb=GetThumbnailImage();

	if (strThumb.Equals("DefaultPlaylistBig.png")) return true;
	if (strThumb.Equals("DefaultProgramBig.png")) return true;
	if (strThumb.Equals("DefaultShortcutBig.png")) return true;
	if (strThumb.Equals("defaultAudioBig.png")) return true;
	if (strThumb.Equals("defaultCddaBig.png")) return true;
	if (strThumb.Equals("defaultDVDEmptyBig.png")) return true;
	if (strThumb.Equals("defaultDVDRomBig.png")) return true;
	if (strThumb.Equals("defaultFolderBackBig.png")) return true;
	if (strThumb.Equals("defaultFolderBig.png")) return true;
	if (strThumb.Equals("defaultHardDiskBig.png")) return true;
	if (strThumb.Equals("defaultNetworkBig.png")) return true;
	if (strThumb.Equals("defaultPictureBig.png")) return true;
	if (strThumb.Equals("defaultVCDBig.png")) return true;
	if (strThumb.Equals("defaultVideoBig.png")) return true;
	if (strThumb.Equals("defaultXBOXDVDBig.png")) return true;

	// check the default icons
	for (unsigned int i=0; i < g_settings.m_vecIcons.size(); ++i)
  {
		if (strThumb.Equals(g_settings.m_vecIcons[i].m_strIcon)) return true;
  }

	return false;
}

bool CFileItem::HasDefaultIcon() const
{
	CStdString strThumb=GetThumbnailImage();

	if (strThumb.Equals("DefaultPlaylist.png")) return true;
	if (strThumb.Equals("DefaultProgram.png")) return true;
	if (strThumb.Equals("DefaultShortcut.png")) return true;
	if (strThumb.Equals("defaultAudio.png")) return true;
	if (strThumb.Equals("defaultCdda.png")) return true;
	if (strThumb.Equals("defaultDVDEmpty.png")) return true;
	if (strThumb.Equals("defaultDVDRom.png")) return true;
	if (strThumb.Equals("defaultFolder.png")) return true;
	if (strThumb.Equals("defaultFolderBack.png")) return true;
	if (strThumb.Equals("defaultHardDisk.png")) return true;
	if (strThumb.Equals("defaultNetwork.png")) return true;
	if (strThumb.Equals("defaultPicture.png")) return true;
	if (strThumb.Equals("defaultVCD.png")) return true;
	if (strThumb.Equals("defaultVideo.png")) return true;
	if (strThumb.Equals("defaultXBOXDVD.png")) return true;

	// check the default icons
	for (unsigned int i=0; i < g_settings.m_vecIcons.size(); ++i)
  {
		if (strThumb.Equals(g_settings.m_vecIcons[i].m_strIcon)) return true;
  }

	return false;
}

void CFileItem::FillInDefaultIcon()
{
	//CLog::Log(LOGINFO, "FillInDefaultIcon(%s)", pItem->GetLabel().c_str());
  // find the default icon for a file or folder item
  // for files this can be the (depending on the file type)
  //   default picture for photo's
  //   default picture for songs
  //   default picture for videos
  //   default picture for shortcuts
  //   default picture for playlists
  //   or the icon embedded in an .xbe
  //
  // for folders
  //   for .. folders the default picture for parent folder
  //   for other folders the defaultFolder.png

  CStdString strThumb;
  CStdString strExtension;
  bool bOnlyDefaultXBE=g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
  if (!m_bIsFolder)
  {
    CStdString strExtension;
    CUtil::GetExtension(m_strPath,strExtension);

    for (int i=0; i < (int)g_settings.m_vecIcons.size(); ++i)
    {
      CFileTypeIcon& icon=g_settings.m_vecIcons[i];

      if (CUtil::cmpnocase(strExtension.c_str(), icon.m_strName)==0)
      {
        SetIconImage(icon.m_strIcon);
        break;
      }
    }
  }
  if (GetIconImage()=="")
  {
		if (!m_bIsFolder)
		{
			if (IsPlayList())
			{
				// playlist
				SetIconImage("defaultPlaylist.png");

				CUtil::GetExtension(m_strPath, strExtension);
				if ( CUtil::cmpnocase(strExtension.c_str(),".strm") !=0) 
				{
					//	Save playlists to playlist directroy
					CStdString strDir;
					CStdString strFileName;
					strFileName=CUtil::GetFileName(m_strPath);
					strDir.Format("%s\\playlists\\%s",g_stSettings.m_szAlbumDirectory,strFileName.c_str());
					if (strDir!=m_strPath)
					{
						CPlayListFactory factory;
						auto_ptr<CPlayList> pPlayList (factory.Create(m_strPath));
						if (pPlayList.get()!=NULL)
						{
							if (pPlayList->Load(m_strPath) && pPlayList->size()>0)
							{
								const CPlayList::CPlayListItem& item=(*pPlayList.get())[0];
								if (!item.IsInternetStream())
								{
									pPlayList->Save(strDir);
								}
							}
						}
					}
				}
			}
			else if (IsPicture() )
			{
				// picture
				SetIconImage("defaultPicture.png");
			}
			else if ( bOnlyDefaultXBE ? IsDefaultXBE() : IsXBE() )
			{
				// xbe
				SetIconImage("defaultProgram.png");
			}
			else if ( IsAudio() )
			{
				// audio
				SetIconImage("defaultAudio.png");
			}
			else if (IsVideo() )
			{
				// video
				SetIconImage("defaultVideo.png");
			}
			else if (IsShortCut() )
			{
				// shortcut
				CStdString strDescription;
				CStdString strFName;
				strFName=CUtil::GetFileName(m_strPath);

				int iPos=strFName.ReverseFind(".");
				strDescription=strFName.Left(iPos);
				SetLabel(strDescription);
				SetIconImage("defaultShortcut.png");
			}
			//else
			//{
			//	// default icon for unknown file type
			//	SetIconImage("defaultUnknown.png");
			//}
		}
		else
		{
			if (GetLabel()=="..")
			{
				SetIconImage("defaultFolderBack.png");
			}
			else
			{
				SetIconImage("defaultFolder.png");
			}
		}
	}

	if (GetThumbnailImage()=="")
  {
    if (GetIconImage()!="")
    {
      CStdString strBig;
      int iPos=GetIconImage().Find(".");
      strBig=GetIconImage().Left(iPos);
      strBig+="Big";
      strBig+=GetIconImage().Right(GetIconImage().size()-(iPos));
      SetThumbnailImage(strBig);
    }
  }
}

void CFileItem::SetThumb()
{
  CStdString strThumb;
  // set the thumbnail for an file item

  // if it already has a thumbnail, then return
  if (HasThumbnail()) return;

  //  No thumb for parent folder items
  if (GetLabel()=="..") return;


  if (IsXBE() && m_bIsFolder) return;  // case where we have multiple paths with XBE
  if (!IsRemote())
	{
		CStdString strFile;
		CUtil::ReplaceExtension(m_strPath, ".tbn", strFile);
		if (CUtil::FileExists(strFile))
		{
			SetThumbnailImage(strFile);
			return;
		}
	}

  // if this is a shortcut, then get the real filename
	CFileItem item;
  if (IsShortCut())
  {
    CShortcut shortcut;
    if ( shortcut.Create( m_strPath ) )
    {
      item.m_strPath=shortcut.m_strPath;
    }
  }
	else
		item.m_strPath=m_strPath;

	// get filename of cached thumbnail like Q:\thumbs\aed638.tbn
  CStdString strCachedThumbnail;
  Crc32 crc;
  crc.ComputeFromLowerCase(item.m_strPath);
  strCachedThumbnail.Format("%s\\%x.tbn",g_stSettings.szThumbnailsDirectory,crc);

  bool bGotIcon(false);

  // does a cached thumbnail exists?
	// If it is on the DVD and is an XBE, let's grab get the thumbnail again
	if (!CUtil::FileExists(strCachedThumbnail) || (item.IsXBE() && item.IsDVD()) )
  {
    if (IsRemote() && !g_guiSettings.GetBool("VideoLibrary.FindRemoteThumbs")) return;
		// get the path for the  thumbnail
		CUtil::GetThumbnail( item.m_strPath,strThumb);
    // local cached thumb does not exists
    // check if strThumb exists
    if (CUtil::FileExists(strThumb))
    {
      // yes, is it a local or remote file
      if (CUtil::IsRemote(strThumb) || CUtil::IsDVD(strThumb) || CUtil::IsISO9660(strThumb) )
      {
        // remote file, then cache it...
        CFile file;
        if ( file.Cache(strThumb.c_str(), strCachedThumbnail.c_str(),NULL,NULL))
        {
          // cache ok, then use it
          SetThumbnailImage(strCachedThumbnail);
          bGotIcon=true;
        }
      }
      else
      {
        // local file, then use it
        SetThumbnailImage(strThumb);
        bGotIcon=true;
      }
    }
    else
    {
      // strThumb doesnt exists either
      // now check for filename.tbn or foldername.tbn
      CFile file;
      CStdString strThumbnailFileName;
			if (m_bIsFolder)
			{
				strThumbnailFileName=m_strPath;
				if (CUtil::HasSlashAtEnd(strThumbnailFileName))
					strThumbnailFileName.Delete(strThumbnailFileName.size()-1);
				strThumbnailFileName+=".tbn";
			}
			else
				CUtil::ReplaceExtension(item.m_strPath,".tbn", strThumbnailFileName);

			if (file.Exists(strThumbnailFileName))
			{
				//	local or remote ?
				if (item.IsRemote() || item.IsDVD() || item.IsISO9660())
				{
					//	remote, cache thumb to hdd
					if ( file.Cache(strThumbnailFileName.c_str(), strCachedThumbnail.c_str(),NULL,NULL))
					{

						SetThumbnailImage(strCachedThumbnail);
						bGotIcon=true;
					}
				}
				else
				{
					//	local, just use it
					SetThumbnailImage(strThumbnailFileName);
					bGotIcon=true;
				}
      }
    }
		// fill in the folder thumbs
		if (!bGotIcon && GetLabel() != "..")
		{
			// this is a folder ?
			if (m_bIsFolder)
			{
				// yes, then get the folder thumbnail
				if (CUtil::GetFolderThumb(m_strPath, strThumb))
				{
					SetThumbnailImage(strThumb);
				}
			}
		}
  }
  else
  {
    // yes local cached thumbnail exists, use it
    SetThumbnailImage(strCachedThumbnail);
  }
}

void CFileItem::SetMusicThumb()
{
	//	Set the album thumb for a file or folder.

	//	Sets thumb by album title or uses files in
	//	folder like folder.jpg or .tbn files.

  // if it already has a thumbnail, then return
  if ( HasThumbnail() ) return;

  if (IsInternetStream()) return;

  CStdString strThumb, strPath;

	//	If item is not a folder, extract its path
  if (!m_bIsFolder)
    CUtil::GetDirectory(m_strPath, strPath);
  else
	{
    strPath=m_strPath;
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
	}

	//	Look if an album thumb is available,
	//	could be any file with tags loaded or
	//	a directory in album window
  CStdString strAlbum;
	if (m_musicInfoTag.Loaded())
		strAlbum=m_musicInfoTag.GetAlbum();

	if (!m_bIsFolder)
  {
    // look for a permanent thumb (Q:\albums\thumbs)
    CUtil::GetAlbumThumb(strAlbum, strPath, strThumb);
    if (CUtil::FileExists(strThumb))
    {
			//	found it, we are finished.
      SetIconImage(strThumb);
      SetThumbnailImage(strThumb);
    }
    else
    {
      // look for a temporary thumb (Q:\albums\thumbs\temp)
      CUtil::GetAlbumThumb(strAlbum, strPath,strThumb, true);
      if (CUtil::FileExists(strThumb) )
      {
				//	found it
        SetIconImage(strThumb);
        SetThumbnailImage(strThumb);
      }
			else
			{
				//	no thumb found
				strThumb.Empty();
			}
		}
	}

	//	If we have not found a thumb before, look for a .tbn if its a file
	if (strThumb.IsEmpty() && !m_bIsFolder)
	{
		CUtil::ReplaceExtension(m_strPath, ".tbn", strThumb);
		if( IsRemote() || IsDVD() || IsISO9660())
		{
			//	Query local cache
			CStdString strCached;
			CUtil::GetAlbumFolderThumb(strThumb, strCached, true);
			if (CUtil::FileExists(strCached))
			{
				//	Remote thumb found in local cache
				SetIconImage(strCached);
				SetThumbnailImage(strCached);
			}
			else
			{
        if (IsRemote() && !g_guiSettings.GetBool("MusicLibrary.FindRemoteThumbs")) return;
				//	create cached thumb, if a .tbn file is found
				//	on a remote share
				if (CUtil::FileExists(strThumb))
				{
					//	found, save a thumb
					//	to the temp thumb dir.
					CPicture pic;
					if (pic.CreateAlbumThumbnail(strThumb, strThumb))
					{
						SetIconImage(strCached);
						SetThumbnailImage(strCached);
					}
					else
					{
						//	save temp thumb failed,
						//	no thumb available
						strThumb.Empty();
					}
				}
				else
				{
					//	no thumb available
					strThumb.Empty();
				}
			}
		}
		else
		{
			if (CUtil::FileExists(strThumb))
			{
				//	use local .tbn file as thumb
				SetIconImage(strThumb);
				SetThumbnailImage(strThumb);
			}
			else
			{
				//	No thumb found
				strThumb.Empty();
			}
		}
	}

	//	If we have not found a thumb before, look for a folder thumb
  if (strThumb.IsEmpty() && GetLabel()!="..")
  {
		CStdString strFolderThumb;

		//	Lookup permanent thumbs on HD, if a
		//	thumb for this folder exists
    CUtil::GetAlbumFolderThumb(strPath,strFolderThumb);
		if (!CUtil::FileExists(strFolderThumb))
		{
			//	No, lookup saved temp thumbs on HD, if a previously
			//	cached thumb for this folder exists...
			CUtil::GetAlbumFolderThumb(strPath,strFolderThumb, true);
			if (!CUtil::FileExists(strFolderThumb))
			{
        if (IsRemote() && !g_guiSettings.GetBool("MusicLibrary.FindRemoteThumbs")) return;
				if (m_bIsFolder)
				{
					CStdString strFolderTbn=strPath;
					strFolderTbn+=".tbn";
					CUtil::AddFileToFolder(m_strPath, "folder.jpg", strThumb);

					//	...no, check for a folder.jpg
					if (CUtil::ThumbExists(strThumb, true))
					{
						//	found, save a thumb for this folder
						//	to the temp thumb dir.
						CPicture pic;
						if (!pic.CreateAlbumThumbnail(strThumb, strPath))
						{
							//	save temp thumb failed,
							//	no thumb available
							strFolderThumb.Empty();
						}
					}	//	...or maybe we have a "foldername".tbn
					else if (CUtil::ThumbExists(strFolderTbn, true))
					{
						//	found, save a thumb for this folder
						//	to the temp thumb dir.
						CPicture pic;
						if (!pic.CreateAlbumThumbnail(strFolderTbn, strPath))
						{
							//	save temp thumb failed,
							//	no thumb available
							strFolderThumb.Empty();
						}
					}
					else
					{
						//	no thumb exists, do we have a directory
						//	from album window, use music.jpg as icon
						if (!strAlbum.IsEmpty())
						{
							SetIconImage("Music.jpg");
							SetThumbnailImage("Music.jpg");
						}

						strFolderThumb.Empty();
					}
				}
				else
				{
					//	No thumb found for file
					strFolderThumb.Empty();
				}
			}
		}	//	if (pItem->m_bIsFolder && strThumb.IsEmpty() && pItem->GetLabel()!="..")


		//	Have we found a folder thumb
		if (!strFolderThumb.IsEmpty())
		{
				//	if we have a directory from album
				//	window, set the icon too.
			if (m_bIsFolder && !strAlbum.IsEmpty())
				SetIconImage(strFolderThumb);

			SetThumbnailImage(strFolderThumb);
		}
  }
}

/////////////////////////////////////////////////////////////////////////////////
/////
///// CFileItemList
/////
//////////////////////////////////////////////////////////////////////////////////

CFileItemList::CFileItemList(VECFILEITEMS& items)
:m_items(items)
{
}

CFileItemList::~CFileItemList()
{
	Clear();
}

CFileItem* CFileItemList::operator[] (int iItem)
{
	return Get(iItem);
}

const CFileItem* CFileItemList::operator[] (int iItem) const
{
	return Get(iItem);
}

void CFileItemList::Clear()
{
	if (m_items.size())
	{
		IVECFILEITEMS i;
		i=m_items.begin(); 
		while (i != m_items.end())
		{
			CFileItem* pItem = *i;
			delete pItem;
			i=m_items.erase(i);
		}
	}
}

void CFileItemList::Add(CFileItem* pItem)
{
	m_items.push_back(pItem);
}

void CFileItemList::Remove(CFileItem* pItem)
{
	for (IVECFILEITEMS it=m_items.begin(); it!=m_items.end(); ++it)
	{
		if (pItem==*it)
		{
			m_items.erase(it);
			break;
		}
	}
}

void CFileItemList::Remove(int iItem)
{
	if (iItem>=0 && iItem < (int)Size())
		m_items.erase(m_items.begin() + iItem);
}

void CFileItemList::Append(const CFileItemList& itemlist)
{
	for (int i=0; i<itemlist.Size(); ++i)
	{
		const CFileItem* pItem=itemlist[i];
		CFileItem* pNewItem=new CFileItem(*pItem);
		Add(pNewItem);
	}
}

CFileItem* CFileItemList::Get(int iItem)
{
	return m_items[iItem];
}

const CFileItem* CFileItemList::Get(int iItem) const
{
	return m_items[iItem];
}

int CFileItemList::Size() const
{
	return (int)m_items.size();
}

bool CFileItemList::IsEmpty() const
{
	return (m_items.size()<=0);
}

void CFileItemList::Reserve(int iCount)
{
	m_items.reserve(iCount);
}

void CFileItemList::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		int i=0;
		if (m_items.size()>0 && m_items[0]->GetLabel()=="..")
			i=1;

		ar << (int)(m_items.size()-i);

		for (i; i<(int)m_items.size(); ++i)
		{
			CFileItem* pItem=m_items[i];
			ar << *pItem;
		}
	}
	else
	{
		int iSize=0;
		ar >> iSize;
		if (iSize<=0)
			return;

		Clear();

		m_items.reserve(iSize);

		for (int i=0; i<iSize; ++i)
		{
			CFileItem* pItem=new CFileItem;
			ar >> *pItem;
			m_items.push_back(pItem);
		}
	}
}

void CFileItemList::FillInDefaultIcons()
{
  for (int i=0; i < (int)m_items.size(); ++i)
  {
    CFileItem* pItem=m_items[i];
    pItem->FillInDefaultIcon();
  }
}

void CFileItemList::SetThumbs()
{
  //cache thumbnails directory
	g_directoryCache.InitThumbCache();

  for (int i=0; i < (int)m_items.size(); ++i)
  {
    CFileItem* pItem=m_items[i];
    pItem->SetThumb();
  }
	
  g_directoryCache.ClearThumbCache();
}

void CFileItemList::SetMusicThumbs()
{
  //cache thumbnails directory
	g_directoryCache.InitMusicThumbCache();

  for (int i=0; i < (int)m_items.size(); ++i)
  {
    CFileItem* pItem=m_items[i];
    pItem->SetMusicThumb();
  }

  g_directoryCache.ClearMusicThumbCache();
}

int CFileItemList::GetFolderCount() const
{
  int nFolderCount=0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItem* pItem = m_items[i];
    if (pItem->m_bIsFolder)
      nFolderCount++;
  }

  return nFolderCount;
}

int CFileItemList::GetFileCount() const
{
  int nFileCount=0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItem* pItem = m_items[i];
    if (!pItem->m_bIsFolder)
      nFileCount++;
  }

  return nFileCount;
}

void CFileItemList::FilterCueItems()
{
	// Handle .CUE sheet files...
	VECSONGS itemstoadd;
	VECARTISTS itemstodelete;
	for (int i=0; i<(int)m_items.size(); i++)
	{
		CFileItem *pItem = m_items[i];
		if (!pItem->m_bIsFolder)
		{	// see if it's a .CUE sheet
			if (pItem->IsCUESheet())
			{
				CCueDocument cuesheet;
				if (cuesheet.Parse(pItem->m_strPath))
				{
					VECSONGS newitems;
					cuesheet.GetSongs(newitems);
					// queue the cue sheet and the underlying media file for deletion
					if (CUtil::FileExists(cuesheet.GetMediaPath()))
					{
						itemstodelete.push_back(pItem->m_strPath);
						itemstodelete.push_back(cuesheet.GetMediaPath());
						// get the additional stuff (year, genre etc.) from the underlying media files tag.
						CMusicInfoTagLoaderFactory factory;
						CMusicInfoTag tag;
						auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(cuesheet.GetMediaPath()));
						if (NULL != pLoader.get())
						{						
							// get id3tag
							pLoader->Load(cuesheet.GetMediaPath(),tag);
						}
						// fill in any missing entries from underlying media file
						for (int j=0; j<(int)newitems.size(); j++)
						{
							CSong song = newitems[j];
							if (tag.Loaded())
							{
								if (song.strAlbum.empty() && !tag.GetAlbum().empty()) song.strAlbum = tag.GetAlbum();
								if (song.strGenre.empty() && !tag.GetGenre().empty()) song.strGenre = tag.GetGenre();
								if (song.strArtist.empty() && !tag.GetArtist().empty()) song.strArtist = tag.GetArtist();
								SYSTEMTIME dateTime;
								tag.GetReleaseDate(dateTime);
								if (dateTime.wYear > 1900) song.iYear = dateTime.wYear;
							}
							if (!song.iDuration && tag.GetDuration()>0)
							{	// must be the last song
								song.iDuration = (tag.GetDuration()*75 - song.iStartOffset+37)/75;
							}
							// add this item to the list
							itemstoadd.push_back(song);
						}
					}
					else
					{	// remove the .cue sheet from the directory
						itemstodelete.push_back(pItem->m_strPath);
					}
				}
			}
		}
	}
	// now delete the .CUE files and underlying media files.
	for (int i=0; i<(int)itemstodelete.size(); i++)
	{
		for (int j=0; j<(int)m_items.size(); j++)
		{
			CFileItem *pItem = m_items[j];
			if (pItem->m_strPath == itemstodelete[i])
			{	// delete this item
				delete pItem;
				m_items.erase(m_items.begin()+j);
				break;
			}
		}
	}
	// and add the files from the .CUE sheet
	for (int i=0; i<(int)itemstoadd.size(); i++)
	{
		// now create the file item, and add to the item list.
		CFileItem *pItem = new CFileItem(itemstoadd[i]);
		m_items.push_back(pItem);
	}
}
