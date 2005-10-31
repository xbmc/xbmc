#include "stdafx.h"
#include "fileitem.h"
#include "Util.h"
#include "picture.h"
#include "playlistfactory.h"
#include "shortcut.h"
#include "crc32.h"
#include "filesystem/DirectoryCache.h"
#include "musicInfoTagLoaderFactory.h"
#include "cuedocument.h"
#include "Utils/fstrcmp.h"
#include "videodatabase.h"

CFileItem::CFileItem(const CSong& song)
{
  Clear();
  m_strLabel = song.strTitle;
  m_strPath = song.strFileName;
  m_musicInfoTag.SetSong(song);
  m_lStartOffset = song.iStartOffset;
  m_lEndOffset = song.iEndOffset;
  m_strThumbnailImage = song.strThumb;
}

CFileItem::CFileItem(const CAlbum& album)
{
  Clear();
  m_strLabel = album.strAlbum;
  m_strPath = album.strPath;
  m_bIsFolder = true;
  m_strLabel2 = album.strArtist;
  m_musicInfoTag.SetAlbum(album);
}

CFileItem::CFileItem(const CFileItem& item)
{
  *this = item;
}

CFileItem::CFileItem(void)
{
  Clear();
}

CFileItem::CFileItem(const CStdString& strLabel)
    : CGUIListItem()
{
  Clear();
  m_strLabel = strLabel;
}

CFileItem::CFileItem(const CStdString& strPath, bool bIsFolder)
{
  Clear();
  m_strPath = strPath;
  m_bIsFolder = bIsFolder;
}

CFileItem::CFileItem(const CShare& share)
{
  Clear();
  m_bIsFolder = true;
  m_bIsShareOrDrive = true;
  m_strPath = share.strPath;
  m_strLabel = share.strName;
  m_iLockMode = share.m_iLockMode;
  m_strLockCode = share.m_strLockCode;
  m_iBadPwdCount = share.m_iBadPwdCount;
  m_iDriveType = share.m_iDriveType;
  m_idepth = share.m_iDepthSize;
  m_strThumbnailImage = share.m_strThumbnailImage;
}

CFileItem::~CFileItem(void)
{
}

const CFileItem& CFileItem::operator=(const CFileItem& item)
{
  if (this == &item) return * this;
  m_strLabel2 = item.m_strLabel2;
  m_strLabel = item.m_strLabel;
  FreeMemory();
  m_bSelected = item.m_bSelected;
  m_strIcon = item.m_strIcon;
  m_strThumbnailImage = item.m_strThumbnailImage;
  m_strPath = item.m_strPath;
  m_bIsFolder = item.m_bIsFolder;
  m_iDriveType = item.m_iDriveType;
  m_bIsShareOrDrive = item.m_bIsShareOrDrive;
  memcpy(&m_stTime, &item.m_stTime, sizeof(SYSTEMTIME));
  m_dwSize = item.m_dwSize;
  m_musicInfoTag = item.m_musicInfoTag;
  m_lStartOffset = item.m_lStartOffset;
  m_lEndOffset = item.m_lEndOffset;
  m_fRating = item.m_fRating;
  m_strDVDLabel = item.m_strDVDLabel;
  m_iprogramCount = item.m_iprogramCount;
  m_iLockMode = item.m_iLockMode;
  m_strLockCode = item.m_strLockCode;
  m_iBadPwdCount = item.m_iBadPwdCount;
  return *this;
}

void CFileItem::Clear()
{
  m_strLabel2 = "";
  m_strLabel = "";
  FreeIcons();
  m_musicInfoTag.Clear();
  m_bSelected = false;
  m_fRating = 0.0f;
  m_strDVDLabel = "";
  m_strPath = "";
  m_fRating = 0.0f;
  m_dwSize = 0;
  m_bIsFolder = false;
  m_bIsShareOrDrive = false;
  memset(&m_stTime, 0, sizeof(m_stTime));
  m_iDriveType = SHARE_TYPE_UNKNOWN;
  m_lStartOffset = 0;
  m_lEndOffset = 0;
  m_iprogramCount = 0;
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
  CUtil::GetExtension(m_strPath, strExtension);
  if (strExtension.size() < 2) return false;
  CUtil::Lower(strExtension);
  if ( strstr( g_stSettings.m_szMyVideoExtensions, strExtension.c_str() ) )
    return true;

  return false;
}

bool CFileItem::IsAudio() const
{
  if (strstr(m_strPath.c_str(), ".cdda") ) return true;
  if (IsShoutCast() ) return true;
  if (strstr(m_strPath.c_str(), "lastfm:") ) return true;

  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  if (strExtension.size() < 2) return false;
  CUtil::Lower(strExtension);

  if ( strstr( g_stSettings.m_szMyMusicExtensions, strExtension.c_str() ) )
    return true;

  return false;
}

bool CFileItem::IsPicture() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
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

bool CFileItem::IsLastFM() const
{
  if (strstr(m_strPath.c_str(), "lastfm:") ) return true;
  return false;
}

bool CFileItem::IsInternetStream() const
{
  CURL url(m_strPath);
  CStdString strProtocol = url.GetProtocol();
  strProtocol.ToLower();

  if (strProtocol.size() == 0)
    return false;

  if (strProtocol == "shout" || strProtocol == "mms" ||
      strProtocol == "http" || strProtocol == "ftp" ||
      strProtocol == "rtsp" || strProtocol == "rtp" ||
      strProtocol == "udp"  || strProtocol == "lastfm")
    return true;

  return false;
}

bool CFileItem::IsPlayList() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  strExtension.ToLower();
  if (strExtension == ".m3u") return true;
  if (strExtension == ".b4s") return true;
  if (strExtension == ".pls") return true;
  if (strExtension == ".strm") return true;
  if (strExtension == ".wpl") return true;
  return false;
}

bool CFileItem::IsPythonScript() const
{
  char* pExtension = CUtil::GetExtension(m_strPath);
  if (!pExtension) return false;
  if (strcmpi(pExtension, ".py") == 0) return true;
  return false;
}

bool CFileItem::IsXBE() const
{
  char* pExtension = CUtil::GetExtension(m_strPath);
  if (!pExtension) return false;
  if (strcmpi(pExtension, ".xbe") == 0) return true;
  return false;
}

bool CFileItem::IsType(const char *ext) const
{
  char* pExtension = CUtil::GetExtension(m_strPath);
  if (!pExtension) return false;
  return (strcmpi(pExtension, ext) == 0);
}

bool CFileItem::IsDefaultXBE() const
{
  char* pFileName = CUtil::GetFileName(m_strPath);
  if (!pFileName) return false;
  if (strcmpi(pFileName, "default.xbe") == 0) return true;
  return false;
}

bool CFileItem::IsShortCut() const
{
  char* pExtension = CUtil::GetExtension(m_strPath);
  if (!pExtension) return false;
  if (strcmpi(pExtension, ".cut") == 0) return true;
  return false;
}

bool CFileItem::IsNFO() const
{
  char *pExtension = CUtil::GetExtension(m_strPath);
  if (!pExtension) return false;
  if (strcmpi(pExtension, ".nfo") == 0) return true;
  return false;
}

bool CFileItem::IsDVDImage() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  if (strExtension.Equals(".img") || strExtension.Equals(".iso")) return true;
  return false;
}

bool CFileItem::IsDVDFile(bool bVobs /*= true*/, bool bIfos /*= true*/) const
{
  CStdString strFileName = CUtil::GetFileName(m_strPath);
  if (bIfos)
  {
    if (strFileName.Equals("video_ts.ifo")) return true;
    if (strFileName.Left(4).Equals("vts_") && strFileName.Right(6).Equals("_0.ifo") && strFileName.length() == 12) return true;
  }
  if (bVobs)
  {
    if (strFileName.Equals("video_ts.vob")) return true;
    if (strFileName.Left(4).Equals("vts_") && strFileName.Right(4).Equals(".vob")) return true;
  }

  return false;
}

bool CFileItem::IsRAR() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  if ( (strExtension.CompareNoCase(".rar") == 0) || ((strExtension.Equals(".001") && m_strPath.Mid(m_strPath.length()-7,7).CompareNoCase(".ts.001"))) ) return true; // sometimes the first rar is named .001
  return false;
}

bool CFileItem::IsZIP() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  if (strExtension.CompareNoCase(".zip") == 0) return true;
  return false;
}

bool CFileItem::IsCBZ() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  if (strExtension.CompareNoCase(".cbz") == 0) return true;
  return false;
}

bool CFileItem::IsCBR() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  if (strExtension.CompareNoCase(".cbr") == 0) return true;
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

bool CFileItem::IsOnDVD() const
{
  return CUtil::IsOnDVD(m_strPath);
}

bool CFileItem::IsISO9660() const
{
  return CUtil::IsISO9660(m_strPath);
}

bool CFileItem::IsRemote() const
{
  return CUtil::IsRemote(m_strPath);
}

bool CFileItem::IsSmb() const
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
  if (IsOnDVD() || IsCDDA()) return true;
  // now check for remote shares (smb is writeable??)
  if (IsSmb()) return false;
  // no other protocols are writeable??
  if (IsRemote()) return true;
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
  if (!m_bIsFolder)
  {
    CStdString strExtension;
    CUtil::GetExtension(m_strPath, strExtension);

    for (int i = 0; i < (int)g_settings.m_vecIcons.size(); ++i)
    {
      CFileTypeIcon& icon = g_settings.m_vecIcons[i];

      if (strcmpi(strExtension.c_str(), icon.m_strName) == 0)
      {
        SetIconImage(icon.m_strIcon);
        break;
      }
    }
  }
  if (GetIconImage() == "")
  {
    if (!m_bIsFolder)
    {
      if ( IsPlayList() )
      {
        // playlist
        SetIconImage("defaultPlaylist.png");

        CUtil::GetExtension(m_strPath, strExtension);
        if ( strcmpi(strExtension.c_str(), ".strm") != 0)
        {
          //  Save playlists to playlist directroy
          CStdString strDir;
          CStdString strFileName;
          strFileName = CUtil::GetFileName(m_strPath);
          strDir.Format("%s\\playlists\\%s", g_stSettings.m_szAlbumDirectory, strFileName.c_str());
          if (strDir != m_strPath)
          {
            CPlayListFactory factory;
            auto_ptr<CPlayList> pPlayList (factory.Create(m_strPath));
            if (pPlayList.get() != NULL)
            {
              if (pPlayList->Load(m_strPath) && pPlayList->size() > 0)
              {
                const CPlayList::CPlayListItem& item = (*pPlayList.get())[0];
                if (!item.IsInternetStream())
                {
                  pPlayList->Save(strDir);
                }
              }
            }
          }
        }
      }
      else if ( IsPicture() )
      {
        // picture
        SetIconImage("defaultPicture.png");
      }
      else if ( IsXBE() )
      {
        // xbe
        SetIconImage("defaultProgram.png");
      }
      else if ( IsAudio() )
      {
        // audio
        SetIconImage("defaultAudio.png");
      }
      else if ( IsVideo() )
      {
        // video
        SetIconImage("defaultVideo.png");
      }
      else if ( IsShortCut() )
      {
        // shortcut
        CStdString strDescription;
        CStdString strFName;
        strFName = CUtil::GetFileName(m_strPath);

        int iPos = strFName.ReverseFind(".");
        strDescription = strFName.Left(iPos);
        SetLabel(strDescription);
        SetIconImage("defaultShortcut.png");
      }
      else if ( IsPythonScript() )
      {
        SetIconImage("DefaultScript.png");
      }
      //else
      //{
      //  // default icon for unknown file type
      //  SetIconImage("defaultUnknown.png");
      //}
    }
    else
    {
      if (GetLabel() == "..")
      {
        SetIconImage("defaultFolderBack.png");
      }
      else
      {
        SetIconImage("defaultFolder.png");
      }
    }
  }
}

// set the thumbnail for an file item
void CFileItem::SetThumb()
{
  CStdString strThumb;

  // if it already has a thumbnail, then return
  if (HasThumbnail() || m_bIsShareOrDrive)
    return;

  //  No thumb for parent folder items
  if (GetLabel() == "..") return ;

  if (IsXBE() && m_bIsFolder) return ;  // case where we have multiple paths with XBE

  /*
  does anyone know what is this section of code is for?

  it doesnt work when the path is an iso9660:// path
  you get an error that the texture manager cannot open the file
  somewhere m_strPath gets concatenated to the skin media directory
  
  example:
  ERROR Texture manager unable to load file: Q:\skin\Project Mayhem\media\iso9660://somedir/somefile.tbn
  */
  if (!IsRemote() && !IsISO9660())
  {
    CStdString strFile;
    CUtil::ReplaceExtension(m_strPath, ".tbn", strFile);
    if (CFile::Exists(strFile))
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
      item.m_strPath = shortcut.m_strPath;
    }
  }
  else
    item.m_strPath = m_strPath;

  // get filename of cached thumbnail like Q:\thumbs\00aed638.tbn
  CStdString strCachedThumbnail;
  CUtil::GetCachedThumbnail(item.m_strPath,strCachedThumbnail);

  bool bGotIcon(false);

  // does a cached thumbnail exists?
  // If it is on the DVD and is an XBE, let's grab get the thumbnail again
  if (!CFile::Exists(strCachedThumbnail) || (item.IsXBE() && item.IsOnDVD()) )
  {
    if (IsRemote() && !IsOnDVD() && !g_guiSettings.GetBool("VideoFiles.FindRemoteThumbs")) return;
    // get the path for the  thumbnail
    CUtil::GetThumbnail( item.m_strPath, strThumb);
    // local cached thumb does not exists
    // check if strThumb exists
    if (CFile::Exists(strThumb))
    {
      // yes, is it a local or remote file
      if (CUtil::IsRemote(strThumb) || CUtil::IsOnDVD(strThumb) || CUtil::IsISO9660(strThumb) )
      {
        // remote file, then cache it...
        if ( CFile::Cache(strThumb.c_str(), strCachedThumbnail.c_str(), NULL, NULL))
        {
          CLog::Log(LOGDEBUG,"  Cached thumb: %s",strCachedThumbnail.c_str());
          // cache ok, then use it
          SetThumbnailImage(strCachedThumbnail);
          bGotIcon = true;
        }
      }
      else
      {
        // local file, then use it
        SetThumbnailImage(strThumb);
        bGotIcon = true;
      }
    }
    else
    {
      // strThumb doesnt exists either
      // now check for filename.tbn or foldername.tbn
      CStdString strThumbnailFileName;
      if (m_bIsFolder)
      {
        strThumbnailFileName = m_strPath;
        if (CUtil::HasSlashAtEnd(strThumbnailFileName))
          strThumbnailFileName.Delete(strThumbnailFileName.size() - 1);
        strThumbnailFileName += ".tbn";
      }
      else
        CUtil::ReplaceExtension(item.m_strPath, ".tbn", strThumbnailFileName);

      if (CFile::Exists(strThumbnailFileName))
      {
        //  local or remote ?
        if (item.IsRemote() || item.IsOnDVD() || item.IsISO9660())
        {
          //  remote, cache thumb to hdd
          if ( CFile::Cache(strThumbnailFileName.c_str(), strCachedThumbnail.c_str(), NULL, NULL))
          {
            SetThumbnailImage(strCachedThumbnail);
            bGotIcon = true;
          }
        }
        else
        {
          //  local, just use it
          SetThumbnailImage(strThumbnailFileName);
          bGotIcon = true;
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

void CFileItem::SetArtistThumb()
{
  CStdString strArtist = "artist" + m_strPath;
  CStdString strThumb = "";
  CUtil::GetCachedThumbnail(strArtist, strThumb);

  if (CFile::Exists(strThumb))
  {
    // found it, we are finished.
    SetIconImage(strThumb);
    SetThumbnailImage(strThumb);
    //CLog::Log(LOGDEBUG,"  Found cached artist [%s] thumb: %s",m_strPath.c_str(),strThumb.c_str());
  }

  return;
}

// set the album thumb for a file or folder
void CFileItem::SetMusicThumb()
{ 
  // if it already has a thumbnail, then return
  if (HasThumbnail()) return;

  // streams do not have thumbnails
  if (IsInternetStream()) return;

  // ignore the parent dir items
  if (GetLabel() == "..") return;

  // if item is not a folder, extract its path
  //CLog::Log(LOGDEBUG,"Looking for thumb for: %s",m_strPath.c_str());
  CStdString strPath;
  if (!m_bIsFolder)
    CUtil::GetDirectory(m_strPath, strPath);
  else
  {
    strPath = m_strPath;
    if (CUtil::HasSlashAtEnd(strPath))
      strPath.Delete(strPath.size() - 1);
  }

  // look if an album thumb is available,
  // could be any file with tags loaded or
  // a directory in album window
  CStdString strAlbum;
  if (m_musicInfoTag.Loaded())
    strAlbum = m_musicInfoTag.GetAlbum();

  CStdString strThumb;

  // try permanent album thumb (Q:\albums\thumbs)
  // using "album name + path"
  CUtil::GetAlbumThumb(strAlbum, strPath, strThumb);
  if (CFile::Exists(strThumb))
  {
    // found it, we are finished.
    SetIconImage(strThumb);
    SetThumbnailImage(strThumb);
    //CLog::Log(LOGDEBUG,"  Found cached permanent album thumb: %s",strThumb.c_str());
    return;
  }
  
  // try temporary album thumb (Q:\albums\thumbs\temp)
  // using "album name + path"
  CUtil::GetAlbumThumb(strAlbum, strPath, strThumb, true);
  if (CFile::Exists(strThumb))
  {
    // found it, we are finished.
    SetIconImage(strThumb);
    SetThumbnailImage(strThumb);
    //CLog::Log(LOGDEBUG,"  Found cached temporary album thumb: %s",strThumb.c_str());
    return;
  }

  // if a file, try to find a file.tbn
  if (!m_bIsFolder)
  {
    // make .tbn filename
    CUtil::ReplaceExtension(m_strPath, ".tbn", strThumb);

    // look for locally cached tbn
    CStdString strCached;
    CUtil::GetAlbumFolderThumb(strThumb, strCached, true);
    if (CFile::Exists(strCached))
    {
      // found it, we are finished.
      SetIconImage(strCached);
      SetThumbnailImage(strCached);
      //CLog::Log(LOGDEBUG,"  Found cached permanent file.tbn thumb: %s",strCached.c_str());
      return;
    }

    // no pre-cached thumbs so check for remote thumbs
    bool bTryRemote = true;
    if (IsRemote() && !IsOnDVD() && !g_guiSettings.GetBool("MusicFiles.FindRemoteThumbs"))
    {
      bTryRemote = false;
    }

    //  create cached thumb, if a .tbn file is found
    //  on a remote share
    if (bTryRemote)
    {
      if (CFile::Exists(strThumb))
      {
        // found, save a thumb
        // to the temp thumb dir.
        CPicture pic;
        if (pic.CreateAlbumThumbnail(strThumb, strThumb))
        {
          SetIconImage(strCached);
          SetThumbnailImage(strCached);
          //CLog::Log(LOGDEBUG,"  Cached file.tbn thumb: %s",strThumb.c_str());
          //CLog::Log(LOGDEBUG,"  => to: %s",strCached.c_str());
          return;
        }
      }
    }
  }

  // try finding cached folder thumb
  // from either folder.jpg or folder.tbn
  // try permanent folder thumb (Q:\albums\thumbs)
  CStdString strFolderThumb;
  CUtil::GetAlbumFolderThumb(strPath, strFolderThumb);
  if (CFile::Exists(strFolderThumb))
  {
    // found it, we are finished.
    SetIconImage(strFolderThumb);
    SetThumbnailImage(strFolderThumb);
    //CLog::Log(LOGDEBUG,"  Found cached permanent folder.jpg thumb: %s",strFolderThumb.c_str());
    return;
  }
  
  // try temporary folder thumb (Q:\albums\thumbs\temp)
  CUtil::GetAlbumFolderThumb(strPath, strFolderThumb, true);
  if (CFile::Exists(strFolderThumb))
  {
    // found it, we are finished.
    SetIconImage(strFolderThumb);
    SetThumbnailImage(strFolderThumb);
    //CLog::Log(LOGDEBUG,"  Found cached temporary folder.jpg thumb: %s",strFolderThumb.c_str());
    return;
  }

  // try finding a remote folder thumbnail
  if (m_bIsFolder)
  {
    bool bTryRemote = true;
    if (IsRemote() && !IsOnDVD() && !g_guiSettings.GetBool("MusicFiles.FindRemoteThumbs"))
    {
      bTryRemote = false;
    }

    if (bTryRemote)
    {
      // folder.jpg file
      CUtil::AddFileToFolder(strPath, "folder.jpg", strThumb);
      if (CUtil::ThumbExists(strThumb, true))
      {
        // found it, save a thumb for this folder
        //  to the temp thumb dir.
        CPicture pic;
        if (pic.CreateAlbumThumbnail(strThumb, strPath))
        {
          SetIconImage(strFolderThumb);
          SetThumbnailImage(strFolderThumb);
          //CLog::Log(LOGDEBUG,"  Cached folder.jpg thumb: %s",strThumb.c_str());
          //CLog::Log(LOGDEBUG,"  => to: %s",strFolderThumb.c_str());
          return;
        }
      }

      // folder.tbn file
      CStdString strFolderTbn = strPath + ".tbn";
      if (CUtil::ThumbExists(strFolderTbn, true))
      {
        // found, save a thumb for this folder
        // to the temp thumb dir.
        CPicture pic;
        if (pic.CreateAlbumThumbnail(strFolderTbn, strPath))
        {
          SetIconImage(strFolderThumb);
          SetThumbnailImage(strFolderThumb);
          //CLog::Log(LOGDEBUG,"  Created cached folder.tbn thumb: %s",strFolderTbn.c_str());
          //CLog::Log(LOGDEBUG,"  => to: %s",strFolderThumb.c_str());
          return;
        }
      }

      //CLog::Log(LOGDEBUG,"  No remote thumbs found or cached");
    }

    // no remote thumb exists or remote thumbs are disabled
    // if we have a directory from album window, use music.jpg as icon
    if (!strAlbum.IsEmpty())
    {
      SetIconImage("Music.jpg");
      SetThumbnailImage("Music.jpg");
      //CLog::Log(LOGDEBUG,"  Using default music.jpg thumb for album");
      return;
    }
  }

  // no thumb found
  return;
}

void CFileItem::RemoveExtension()
{
  if (m_bIsFolder)
    return ;
  CStdString strLabel = GetLabel();
  CUtil::RemoveExtension(strLabel);
  SetLabel(strLabel);
}

void CFileItem::CleanFileName()
{
  if (m_bIsFolder)
    return ;
  CStdString strLabel = GetLabel();
  CUtil::CleanFileName(strLabel);
  SetLabel(strLabel);
}

void CFileItem::SetFileSizeLabel()
{
  // file < 1 kbyte?
  if (m_dwSize < 1024)
  {
    //  substract the integer part of the float value
    float fRemainder = (((float)m_dwSize) / 1024.0f) - floor(((float)m_dwSize) / 1024.0f);
    float fToAdd = 0.0f;
    if (fRemainder < 0.01f)
      fToAdd = 0.1f;
    m_strLabel2.Format("%2.1f KB", (((float)m_dwSize) / 1024.0f) + fToAdd);
    return;
  }
  const __int64 iOneMeg = 1024 * 1024;

  // file < 1 megabyte?
  if (m_dwSize < iOneMeg)
  {
    m_strLabel2.Format("%02.1f KB", ((float)m_dwSize) / 1024.0f);
    return;
  }

  // file < 1 GByte?
  __int64 iOneGigabyte = iOneMeg;
  iOneGigabyte *= (__int64)1000;
  if (m_dwSize < iOneGigabyte)
  {
    m_strLabel2.Format("%02.1f MB", ((float)m_dwSize) / ((float)iOneMeg));
    return;
  }
  //file > 1 GByte
  int iGigs = 0;
  __int64 dwFileSize = m_dwSize;
  while (dwFileSize >= iOneGigabyte)
  {
    dwFileSize -= iOneGigabyte;
    iGigs++;
  }
  float fMegs = ((float)dwFileSize) / ((float)iOneMeg);
  fMegs /= 1000.0f;
  fMegs += iGigs;
  m_strLabel2.Format("%02.1f GB", fMegs);
}


/////////////////////////////////////////////////////////////////////////////////
/////
///// CFileItemList
/////
//////////////////////////////////////////////////////////////////////////////////

CFileItemList::CFileItemList()
{
  m_fastLookup = false;
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

void CFileItemList::SetFastLookup(bool fastLookup)
{
  if (fastLookup && !m_fastLookup)
  { // generate the map
    m_map.clear();
    for (unsigned int i=0; i < m_items.size(); i++)
    {
      CFileItem *pItem = m_items[i];
      m_map.insert(MAPFILEITEMSPAIR(pItem->m_strPath, pItem));
    }
  }
  if (!fastLookup && m_fastLookup)
    m_map.clear();
  m_fastLookup = fastLookup;
}

bool CFileItemList::Contains(CStdString& fileName)
{
  if (m_fastLookup)
    return m_map.find(fileName) != m_map.end();
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItem *pItem = m_items[i];
    if (pItem->m_strPath == fileName)
      return true;
  }
  return false;
}

void CFileItemList::Clear()
{
  if (m_items.size())
  {
    IVECFILEITEMS i;
    i = m_items.begin();
    while (i != m_items.end())
    {
      CFileItem* pItem = *i;
      delete pItem;
      i = m_items.erase(i);
    }
    m_map.clear();
  }
}

void CFileItemList::ClearKeepPointer()
{
  if (m_items.size())
  {
    m_items.clear();
    m_map.clear();
  }
}

void CFileItemList::Add(CFileItem* pItem)
{
  m_items.push_back(pItem);
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(pItem->m_strPath, pItem));
}

void CFileItemList::Remove(CFileItem* pItem)
{
  for (IVECFILEITEMS it = m_items.begin(); it != m_items.end(); ++it)
  {
    if (pItem == *it)
    {
      m_items.erase(it);
      if (m_fastLookup)
        m_map.erase(pItem->m_strPath);
      break;
    }
  }
}

void CFileItemList::Remove(int iItem)
{
  if (iItem >= 0 && iItem < (int)Size())
  {
    CFileItem* pItem = *(m_items.begin() + iItem);
    if (m_fastLookup)
      m_map.erase(pItem->m_strPath);
    delete pItem;
    m_items.erase(m_items.begin() + iItem);
  }
}

void CFileItemList::Append(const CFileItemList& itemlist)
{
  for (int i = 0; i < itemlist.Size(); ++i)
  {
    const CFileItem* pItem = itemlist[i];
    CFileItem* pNewItem = new CFileItem(*pItem);
    Add(pNewItem);
  }
}

void CFileItemList::AppendPointer(const CFileItemList& itemlist)
{
  for (int i = 0; i < itemlist.Size(); ++i)
  {
    CFileItem* pItem = const_cast<CFileItem*>(itemlist[i]);
    Add(pItem);
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
  return (m_items.size() <= 0);
}

void CFileItemList::Reserve(int iCount)
{
  m_items.reserve(iCount);
}

void CFileItemList::Sort(FILEITEMLISTCOMPARISONFUNC func)
{
  sort(m_items.begin(), m_items.end(), func);
}

void CFileItemList::Serialize(CArchive& ar)
{
  if (ar.IsStoring())
  {
    int i = 0;
    if (m_items.size() > 0 && m_items[0]->GetLabel() == "..")
      i = 1;

    ar << (int)(m_items.size() - i);

    for (i; i < (int)m_items.size(); ++i)
    {
      CFileItem* pItem = m_items[i];
      ar << *pItem;
    }
  }
  else
  {
    int iSize = 0;
    ar >> iSize;
    if (iSize <= 0)
      return ;

    Clear();

    m_items.reserve(iSize);

    for (int i = 0; i < iSize; ++i)
    {
      CFileItem* pItem = new CFileItem;
      ar >> *pItem;
      m_items.push_back(pItem);
    }
  }
}

void CFileItemList::FillInDefaultIcons()
{
  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->FillInDefaultIcon();
  }
}

void CFileItemList::SetThumbs()
{
  //cache thumbnails directory
  g_directoryCache.InitThumbCache();

  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->SetThumb();
  }

  g_directoryCache.ClearThumbCache();
}

void CFileItemList::SetMusicThumbs()
{
  //cache thumbnails directory
  g_directoryCache.InitMusicThumbCache();

  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItem* pItem = m_items[i];
    pItem->SetMusicThumb();
  }

  g_directoryCache.ClearMusicThumbCache();
}

int CFileItemList::GetFolderCount() const
{
  int nFolderCount = 0;
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
  int nFileCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItem* pItem = m_items[i];
    if (!pItem->m_bIsFolder)
      nFileCount++;
  }

  return nFileCount;
}

// Checks through our file list for the path specified in path.
// Check is done case-insensitive
bool CFileItemList::HasFileNoCase(CStdString& path)
{
  bool bFound = false;
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    if (stricmp(m_items[i]->m_strPath.c_str(), path) == 0)
    {
      bFound = true;
      path = m_items[i]->m_strPath;
      break;
    }
  }
  return bFound;
}

void CFileItemList::FilterCueItems()
{
  // Handle .CUE sheet files...
  VECSONGS itemstoadd;
  VECARTISTS itemstodelete;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItem *pItem = m_items[i];
    if (!pItem->m_bIsFolder)
    { // see if it's a .CUE sheet
      if (pItem->IsCUESheet())
      {
        CCueDocument cuesheet;
        if (cuesheet.Parse(pItem->m_strPath))
        {
          VECSONGS newitems;
          cuesheet.GetSongs(newitems);
          // queue the cue sheet and the underlying media file for deletion
          CStdString strMediaFile = cuesheet.GetMediaPath();
          bool bFoundMediaFile = CFile::Exists(strMediaFile);
          if (!bFoundMediaFile)
          {
            // try file in same dir, not matching case...
            if (HasFileNoCase(strMediaFile))
            {
              bFoundMediaFile = true;
            }
            else
            {
              // try removing the .cue extension...
              strMediaFile = pItem->m_strPath;
              CUtil::RemoveExtension(strMediaFile);
              CFileItem item(strMediaFile, false);
              if (item.IsAudio() && HasFileNoCase(strMediaFile))
              {
                bFoundMediaFile = true;
              }
              else
              { // try replacing the extension with one of our allowed ones.
                CStdStringArray extensions;
                StringUtils::SplitString(g_stSettings.m_szMyMusicExtensions, "|", extensions);
                for (unsigned int i = 0; i < extensions.size(); i++)
                {
                  CUtil::ReplaceExtension(pItem->m_strPath, extensions[i], strMediaFile);
                  CFileItem item(strMediaFile, false);
                  if (!item.IsCUESheet() && !item.IsPlayList() && HasFileNoCase(strMediaFile))
                  {
                    bFoundMediaFile = true;
                    break;
                  }
                }
              }
            }
          }
          if (bFoundMediaFile)
          {
            itemstodelete.push_back(pItem->m_strPath);
            itemstodelete.push_back(strMediaFile);
            // get the additional stuff (year, genre etc.) from the underlying media files tag.
            CMusicInfoTagLoaderFactory factory;
            CMusicInfoTag tag;
            auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(cuesheet.GetMediaPath()));
            if (NULL != pLoader.get())
            {
              // get id3tag
              pLoader->Load(cuesheet.GetMediaPath(), tag);
            }
            // fill in any missing entries from underlying media file
            for (int j = 0; j < (int)newitems.size(); j++)
            {
              CSong song = newitems[j];
              song.strFileName = strMediaFile;
              if (tag.Loaded())
              {
                if (song.strAlbum.empty() && !tag.GetAlbum().empty()) song.strAlbum = tag.GetAlbum();
                if (song.strGenre.empty() && !tag.GetGenre().empty()) song.strGenre = tag.GetGenre();
                if (song.strArtist.empty() && !tag.GetArtist().empty()) song.strArtist = tag.GetArtist();
                SYSTEMTIME dateTime;
                tag.GetReleaseDate(dateTime);
                if (dateTime.wYear > 1900) song.iYear = dateTime.wYear;
              }
              if (!song.iDuration && tag.GetDuration() > 0)
              { // must be the last song
                song.iDuration = (tag.GetDuration() * 75 - song.iStartOffset + 37) / 75;
              }
              // add this item to the list
              itemstoadd.push_back(song);
            }
          }
          else
          { // remove the .cue sheet from the directory
            itemstodelete.push_back(pItem->m_strPath);
          }
        }
        else
        { // remove the .cue sheet from the directory (can't parse it - no point listing it)
          itemstodelete.push_back(pItem->m_strPath);
        }
      }
    }
  }
  // now delete the .CUE files and underlying media files.
  for (int i = 0; i < (int)itemstodelete.size(); i++)
  {
    for (int j = 0; j < (int)m_items.size(); j++)
    {
      CFileItem *pItem = m_items[j];
      if (stricmp(pItem->m_strPath.c_str(), itemstodelete[i].c_str()) == 0)
      { // delete this item
        delete pItem;
        m_items.erase(m_items.begin() + j);
        break;
      }
    }
  }
  // and add the files from the .CUE sheet
  for (int i = 0; i < (int)itemstoadd.size(); i++)
  {
    // now create the file item, and add to the item list.
    CFileItem *pItem = new CFileItem(itemstoadd[i]);
    m_items.push_back(pItem);
  }
}

// Remove the extensions from the filenames
void CFileItemList::RemoveExtensions()
{
  for (int i = 0; i < Size(); ++i)
    m_items[i]->RemoveExtension();
}

void CFileItemList::CleanFileNames()
{
  for (int i = 0; i < Size(); ++i)
    m_items[i]->CleanFileName();
}

void CFileItemList::Stack()
{
  if (g_stSettings.m_iMyVideoVideoStack != STACK_NONE)
  {
    CFileItemList items;
    bool bDVDFolder(false);
    //Figure out first if we are in a folder that contains a dvd
    for (int i = 0; i < (int)Size(); ++i) //Do it this way to avoid an extra roundtrip to files
    {
      CFileItem* pItem1 = Get(i);
      if (CStdString(CUtil::GetFileName(pItem1->m_strPath)).Equals("VIDEO_TS.IFO"))
      {
        bDVDFolder = true;
        items.Add(new CFileItem(*pItem1));
        Remove(i); //Make sure this is not included in the comeing search as it would have been deleted.
        break;
      }
    }

    for (int i = 0; i < Size(); ++i)
    {
      bool bAdd(true);
      CFileItem* pItem1 = Get(i);
      if (pItem1->IsNFO())
      {
        bAdd = false;
      }
      else if (bDVDFolder && pItem1->IsDVDFile(true, true)) //Hide all dvdfiles
      {
        bAdd = false;
      }
      else
      {
        //don't stack folders and playlists
        if ((!pItem1->m_bIsFolder) && !pItem1->IsPlayList())
        {
          CStdString fileName1 = CUtil::GetFileName(pItem1->m_strPath);

          CStdString fileTitle;
          CStdString volumeNumber;

          bool searchForStackedFiles = false;
          if (g_stSettings.m_iMyVideoVideoStack == STACK_FUZZY)
          {
            searchForStackedFiles = true;
          }
          else
          {
            searchForStackedFiles = CUtil::GetVolumeFromFileName(fileName1, fileTitle, volumeNumber);
          }

          if (searchForStackedFiles)
          {
            for (int x = 0; x < (int)items.Size(); ++x)
            {
              CFileItem* pItem2 = items[x];
              if (pItem1 != pItem2)
              {
                if ((!pItem2->m_bIsFolder) && !pItem2->IsPlayList())
                {
                  CStdString fileName2 = CUtil::GetFileName(pItem2->m_strPath);

                  if (g_stSettings.m_iMyVideoVideoStack == STACK_FUZZY)
                  {
                    // use "fuzzy" stacking

                    double fPercentage = fstrcmp(fileName1, fileName2, COMPARE_PERCENTAGE_MIN);
                    if (fPercentage >= COMPARE_PERCENTAGE)
                    {
                      int iGreater = strcmp(fileName1, fileName2);
                      if (iGreater > 0)
                      {
                        bAdd = false;
                        break;
                      }
                    }
                  }
                  else
                  {
                    // use traditional "simple" stacking (like XBMP)
                    // file name must end in -CD[n], where only the first
                    // one (-CD1) will be added to the display list

                    CStdString fileTitle2;
                    CStdString volumeNumber2;
                    if (CUtil::GetVolumeFromFileName(fileName2, fileTitle2, volumeNumber2))
                    {
                      // TODO: check volumePrefix - they should be in the
                      // same category, but not necessarily equal!

                      if (fileTitle.Equals(fileTitle2) && strcmp(volumeNumber.c_str(), volumeNumber2.c_str()) > 0)
                      {
                        bAdd = false;
                        break;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      if (bAdd)
      {
        items.Add(new CFileItem(*pItem1));
      }
    }

    Clear();

    for (int i=0; i<items.Size(); ++i)
    {
      CFileItem* pItem=items[i];
      Add(pItem);
    }

    items.ClearKeepPointer();
  }
}