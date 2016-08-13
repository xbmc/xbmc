/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <cstdlib>

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Archive.h"
#include "Util.h"
#include "playlists/PlayListFactory.h"
#include "utils/Crc32.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/CurlFile.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "CueDocument.h"
#include "video/VideoDatabase.h"
#include "music/MusicDatabase.h"
#include "epg/Epg.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRRadioRDSInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "video/VideoInfoTag.h"
#include "threads/SingleLock.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "music/Artist.h"
#include "music/Album.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/RegExp.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "utils/Mime.h"
#include "utils/Random.h"

#include <assert.h>
#include <algorithm>

using namespace XFILE;
using namespace PLAYLIST;
using namespace MUSIC_INFO;
using namespace PVR;
using namespace EPG;

CFileItem::CFileItem(const CSong& song)
{
  Initialize();
  SetFromSong(song);
}

CFileItem::CFileItem(const CSong& song, const CMusicInfoTag& music)
{
  Initialize();
  SetFromSong(song);
  *GetMusicInfoTag() = music;
}

CFileItem::CFileItem(const CURL &url, const CAlbum& album)
{
  Initialize();

  m_strPath = url.Get();
  URIUtils::AddSlashAtEnd(m_strPath);
  SetFromAlbum(album);
}

CFileItem::CFileItem(const std::string &path, const CAlbum& album)
{
  Initialize();

  m_strPath = path;
  URIUtils::AddSlashAtEnd(m_strPath);
  SetFromAlbum(album);
}

CFileItem::CFileItem(const CMusicInfoTag& music)
{
  Initialize();
  SetLabel(music.GetTitle());
  m_strPath = music.GetURL();
  m_bIsFolder = URIUtils::HasSlashAtEnd(m_strPath);
  *GetMusicInfoTag() = music;
  FillInDefaultIcon();
  FillInMimeType(false);
}

CFileItem::CFileItem(const CVideoInfoTag& movie)
{
  Initialize();
  SetFromVideoInfoTag(movie);
}

CFileItem::CFileItem(const CEpgInfoTagPtr& tag)
{
  assert(tag.get());

  Initialize();

  m_bIsFolder = false;
  m_epgInfoTag = tag;
  m_strPath = tag->Path();
  SetLabel(tag->Title());
  m_strLabel2 = tag->Plot();
  m_dateTime = tag->StartAsLocalTime();

  if (!tag->Icon().empty())
    SetIconImage(tag->Icon());
  else if (tag->HasPVRChannel() && !tag->ChannelTag()->IconPath().empty())
    SetIconImage(tag->ChannelTag()->IconPath());

  FillInMimeType(false);
}

CFileItem::CFileItem(const CPVRChannelPtr& channel)
{
  assert(channel.get());

  Initialize();

  CEpgInfoTagPtr epgNow(channel->GetEPGNow());

  m_strPath = channel->Path();
  m_bIsFolder = false;
  m_pvrChannelInfoTag = channel;
  SetLabel(channel->ChannelName());
  m_strLabel2 = epgNow ? epgNow->Title() :
      CSettings::GetInstance().GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE) ?
                            "" : g_localizeStrings.Get(19055); // no information available

  if (channel->IsRadio())
  {
    CMusicInfoTag* musictag = GetMusicInfoTag();
    if (musictag)
    {
      musictag->SetURL(channel->Path());
      musictag->SetTitle(m_strLabel2);
      musictag->SetArtist(channel->ChannelName());
      musictag->SetAlbumArtist(channel->ChannelName());
      if (epgNow)
        musictag->SetGenre(epgNow->Genre());
      musictag->SetDuration(epgNow ? epgNow->GetDuration() : 3600);
      musictag->SetLoaded(true);
      musictag->SetComment("");
      musictag->SetLyrics("");
    }
  }

  if (!channel->IconPath().empty())
    SetIconImage(channel->IconPath());

  SetProperty("channelid", channel->ChannelID());
  SetProperty("path", channel->Path());
  SetArt("thumb", channel->IconPath());

  FillInMimeType(false);
}

CFileItem::CFileItem(const CPVRRecordingPtr& record)
{
  assert(record.get());

  Initialize();

  m_bIsFolder = false;
  m_pvrRecordingInfoTag = record;
  m_strPath = record->m_strFileNameAndPath;
  SetLabel(record->m_strTitle);
  m_strLabel2 = record->m_strPlot;
  FillInMimeType(false);
}

CFileItem::CFileItem(const CPVRTimerInfoTagPtr& timer)
{
  assert(timer.get());

  Initialize();

  m_bIsFolder = timer->IsTimerRule();
  m_pvrTimerInfoTag = timer;
  m_strPath = timer->Path();
  SetLabel(timer->Title());
  m_strLabel2 = timer->Summary();
  m_dateTime = timer->StartAsLocalTime();

  if (!timer->ChannelIcon().empty())
    SetIconImage(timer->ChannelIcon());

  FillInMimeType(false);
}

CFileItem::CFileItem(const CArtist& artist)
{
  Initialize();
  SetLabel(artist.strArtist);
  m_strPath = artist.strArtist;
  m_bIsFolder = true;
  URIUtils::AddSlashAtEnd(m_strPath);
  GetMusicInfoTag()->SetArtist(artist);
  FillInMimeType(false);
}

CFileItem::CFileItem(const CGenre& genre)
{
  Initialize();
  SetLabel(genre.strGenre);
  m_strPath = genre.strGenre;
  m_bIsFolder = true;
  URIUtils::AddSlashAtEnd(m_strPath);
  GetMusicInfoTag()->SetGenre(genre.strGenre);
  FillInMimeType(false);
}

CFileItem::CFileItem(const CFileItem& item)
: m_musicInfoTag(NULL),
  m_videoInfoTag(NULL),
  m_pictureInfoTag(NULL)
{
  *this = item;
}

CFileItem::CFileItem(const CGUIListItem& item)
{
  Initialize();
  // not particularly pretty, but it gets around the issue of Initialize() defaulting
  // parameters in the CGUIListItem base class.
  *((CGUIListItem *)this) = item;

  FillInMimeType(false);
}

CFileItem::CFileItem(void)
{
  Initialize();
}

CFileItem::CFileItem(const std::string& strLabel)
{
  Initialize();
  SetLabel(strLabel);
}

CFileItem::CFileItem(const char* strLabel)
{
  Initialize();
  SetLabel(std::string(strLabel));
}

CFileItem::CFileItem(const CURL& path, bool bIsFolder)
{
  Initialize();
  m_strPath = path.Get();
  m_bIsFolder = bIsFolder;
  if (m_bIsFolder && !m_strPath.empty() && !IsFileFolder())
    URIUtils::AddSlashAtEnd(m_strPath);
  FillInMimeType(false);
}

CFileItem::CFileItem(const std::string& strPath, bool bIsFolder)
{
  Initialize();
  m_strPath = strPath;
  m_bIsFolder = bIsFolder;
  if (m_bIsFolder && !m_strPath.empty() && !IsFileFolder())
    URIUtils::AddSlashAtEnd(m_strPath);
  FillInMimeType(false);
}

CFileItem::CFileItem(const CMediaSource& share)
{
  Initialize();
  m_bIsFolder = true;
  m_bIsShareOrDrive = true;
  m_strPath = share.strPath;
  if (!IsRSS()) // no slash at end for rss feeds
    URIUtils::AddSlashAtEnd(m_strPath);
  std::string label = share.strName;
  if (!share.strStatus.empty())
    label = StringUtils::Format("%s (%s)", share.strName.c_str(), share.strStatus.c_str());
  SetLabel(label);
  m_iLockMode = share.m_iLockMode;
  m_strLockCode = share.m_strLockCode;
  m_iHasLock = share.m_iHasLock;
  m_iBadPwdCount = share.m_iBadPwdCount;
  m_iDriveType = share.m_iDriveType;
  SetArt("thumb", share.m_strThumbnailImage);
  SetLabelPreformated(true);
  if (IsDVD())
    GetVideoInfoTag()->m_strFileNameAndPath = share.strDiskUniqueId; // share.strDiskUniqueId contains disc unique id
  FillInMimeType(false);
}

CFileItem::CFileItem(std::shared_ptr<const ADDON::IAddon> addonInfo) : m_addonInfo(std::move(addonInfo))
{
  Initialize();
}

CFileItem::~CFileItem(void)
{
  delete m_musicInfoTag;
  delete m_videoInfoTag;
  delete m_pictureInfoTag;

  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
}

const CFileItem& CFileItem::operator=(const CFileItem& item)
{
  if (this == &item)
    return *this;

  CGUIListItem::operator=(item);
  m_bLabelPreformated=item.m_bLabelPreformated;
  FreeMemory();
  m_strPath = item.GetPath();
  m_bIsParentFolder = item.m_bIsParentFolder;
  m_iDriveType = item.m_iDriveType;
  m_bIsShareOrDrive = item.m_bIsShareOrDrive;
  m_dateTime = item.m_dateTime;
  m_dwSize = item.m_dwSize;

  if (item.m_musicInfoTag)
  {
    if (m_musicInfoTag)
      *m_musicInfoTag = *item.m_musicInfoTag;
    else
      m_musicInfoTag = new MUSIC_INFO::CMusicInfoTag(*item.m_musicInfoTag);
  }
  else
  {
    delete m_musicInfoTag;
    m_musicInfoTag = NULL;
  }

  if (item.m_videoInfoTag)
  {
    if (m_videoInfoTag)
      *m_videoInfoTag = *item.m_videoInfoTag;
    else
      m_videoInfoTag = new CVideoInfoTag(*item.m_videoInfoTag);
  }
  else
  {
    delete m_videoInfoTag;
    m_videoInfoTag = NULL;
  }

  if (item.m_pictureInfoTag)
  {
    if (m_pictureInfoTag)
      *m_pictureInfoTag = *item.m_pictureInfoTag;
    else
      m_pictureInfoTag = new CPictureInfoTag(*item.m_pictureInfoTag);
  }
  else
  {
    delete m_pictureInfoTag;
    m_pictureInfoTag = NULL;
  }

  m_epgInfoTag = item.m_epgInfoTag;
  m_pvrChannelInfoTag = item.m_pvrChannelInfoTag;
  m_pvrRecordingInfoTag = item.m_pvrRecordingInfoTag;
  m_pvrTimerInfoTag = item.m_pvrTimerInfoTag;
  m_pvrRadioRDSInfoTag = item.m_pvrRadioRDSInfoTag;
  m_addonInfo = item.m_addonInfo;

  m_lStartOffset = item.m_lStartOffset;
  m_lStartPartNumber = item.m_lStartPartNumber;
  m_lEndOffset = item.m_lEndOffset;
  m_strDVDLabel = item.m_strDVDLabel;
  m_strTitle = item.m_strTitle;
  m_iprogramCount = item.m_iprogramCount;
  m_idepth = item.m_idepth;
  m_iLockMode = item.m_iLockMode;
  m_strLockCode = item.m_strLockCode;
  m_iHasLock = item.m_iHasLock;
  m_iBadPwdCount = item.m_iBadPwdCount;
  m_bCanQueue=item.m_bCanQueue;
  m_mimetype = item.m_mimetype;
  m_extrainfo = item.m_extrainfo;
  m_specialSort = item.m_specialSort;
  m_bIsAlbum = item.m_bIsAlbum;
  m_doContentLookup = item.m_doContentLookup;
  return *this;
}

void CFileItem::Initialize()
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  m_bLabelPreformated = false;
  m_bIsAlbum = false;
  m_dwSize = 0;
  m_bIsParentFolder = false;
  m_bIsShareOrDrive = false;
  m_iDriveType = CMediaSource::SOURCE_TYPE_UNKNOWN;
  m_lStartOffset = 0;
  m_lStartPartNumber = 1;
  m_lEndOffset = 0;
  m_iprogramCount = 0;
  m_idepth = 1;
  m_iLockMode = LOCK_MODE_EVERYONE;
  m_iBadPwdCount = 0;
  m_iHasLock = 0;
  m_bCanQueue = true;
  m_specialSort = SortSpecialNone;
  m_doContentLookup = true;
}

void CFileItem::Reset()
{
  // CGUIListItem members...
  m_strLabel2.clear();
  SetLabel("");
  FreeIcons();
  m_overlayIcon = ICON_OVERLAY_NONE;
  m_bSelected = false;
  m_bIsFolder = false;

  m_strDVDLabel.clear();
  m_strTitle.clear();
  m_strPath.clear();
  m_dateTime.Reset();
  m_strLockCode.clear();
  m_mimetype.clear();
  delete m_musicInfoTag;
  m_musicInfoTag=NULL;
  delete m_videoInfoTag;
  m_videoInfoTag=NULL;
  m_epgInfoTag.reset();
  m_pvrChannelInfoTag.reset();
  m_pvrRecordingInfoTag.reset();
  m_pvrTimerInfoTag.reset();
  m_pvrRadioRDSInfoTag.reset();
  delete m_pictureInfoTag;
  m_pictureInfoTag=NULL;
  m_extrainfo.clear();
  ClearProperties();

  Initialize();
  SetInvalid();
}

void CFileItem::Archive(CArchive& ar)
{
  CGUIListItem::Archive(ar);

  if (ar.IsStoring())
  {
    ar << m_bIsParentFolder;
    ar << m_bLabelPreformated;
    ar << m_strPath;
    ar << m_bIsShareOrDrive;
    ar << m_iDriveType;
    ar << m_dateTime;
    ar << m_dwSize;
    ar << m_strDVDLabel;
    ar << m_strTitle;
    ar << m_iprogramCount;
    ar << m_idepth;
    ar << m_lStartOffset;
    ar << m_lStartPartNumber;
    ar << m_lEndOffset;
    ar << m_iLockMode;
    ar << m_strLockCode;
    ar << m_iBadPwdCount;

    ar << m_bCanQueue;
    ar << m_mimetype;
    ar << m_extrainfo;
    ar << m_specialSort;
    ar << m_doContentLookup;

    if (m_musicInfoTag)
    {
      ar << 1;
      ar << *m_musicInfoTag;
    }
    else
      ar << 0;
    if (m_videoInfoTag)
    {
      ar << 1;
      ar << *m_videoInfoTag;
    }
    else
      ar << 0;
    if (m_pvrRadioRDSInfoTag)
    {
      ar << 1;
      ar << *m_pvrRadioRDSInfoTag;
    }
    else
      ar << 0;
    if (m_pictureInfoTag)
    {
      ar << 1;
      ar << *m_pictureInfoTag;
    }
    else
      ar << 0;
  }
  else
  {
    ar >> m_bIsParentFolder;
    ar >> m_bLabelPreformated;
    ar >> m_strPath;
    ar >> m_bIsShareOrDrive;
    ar >> m_iDriveType;
    ar >> m_dateTime;
    ar >> m_dwSize;
    ar >> m_strDVDLabel;
    ar >> m_strTitle;
    ar >> m_iprogramCount;
    ar >> m_idepth;
    ar >> m_lStartOffset;
    ar >> m_lStartPartNumber;
    ar >> m_lEndOffset;
    int temp;
    ar >> temp;
    m_iLockMode = (LockType)temp;
    ar >> m_strLockCode;
    ar >> m_iBadPwdCount;

    ar >> m_bCanQueue;
    ar >> m_mimetype;
    ar >> m_extrainfo;
    ar >> temp;
    m_specialSort = (SortSpecial)temp;
    ar >> m_doContentLookup;

    int iType;
    ar >> iType;
    if (iType == 1)
      ar >> *GetMusicInfoTag();
    ar >> iType;
    if (iType == 1)
      ar >> *GetVideoInfoTag();
    ar >> iType;
    if (iType == 1)
      ar >> *m_pvrRadioRDSInfoTag;
    ar >> iType;
    if (iType == 1)
      ar >> *GetPictureInfoTag();

    SetInvalid();
  }
}

void CFileItem::Serialize(CVariant& value) const
{
  //CGUIListItem::Serialize(value["CGUIListItem"]);

  value["strPath"] = m_strPath;
  value["dateTime"] = (m_dateTime.IsValid()) ? m_dateTime.GetAsRFC1123DateTime() : "";
  value["lastmodified"] = m_dateTime.IsValid() ? m_dateTime.GetAsDBDateTime() : "";
  value["size"] = m_dwSize;
  value["DVDLabel"] = m_strDVDLabel;
  value["title"] = m_strTitle;
  value["mimetype"] = m_mimetype;
  value["extrainfo"] = m_extrainfo;

  if (m_musicInfoTag)
    (*m_musicInfoTag).Serialize(value["musicInfoTag"]);

  if (m_videoInfoTag)
    (*m_videoInfoTag).Serialize(value["videoInfoTag"]);

  if (m_pvrRadioRDSInfoTag)
    m_pvrRadioRDSInfoTag->Serialize(value["rdsInfoTag"]);

  if (m_pictureInfoTag)
    (*m_pictureInfoTag).Serialize(value["pictureInfoTag"]);
}

void CFileItem::ToSortable(SortItem &sortable, Field field) const
{
  switch (field)
  {
    case FieldPath:
      sortable[FieldPath] = m_strPath;
      break;
    case FieldDate:
      sortable[FieldDate] = (m_dateTime.IsValid()) ? m_dateTime.GetAsDBDateTime() : "";
      break;
    case FieldSize:
      sortable[FieldSize] = m_dwSize;
      break;
    case FieldDriveType:
      sortable[FieldDriveType] = m_iDriveType;
      break;
    case FieldStartOffset:
      sortable[FieldStartOffset] = m_lStartOffset;
      break;
    case FieldEndOffset:
      sortable[FieldEndOffset] = m_lEndOffset;
      break;
    case FieldProgramCount:
      sortable[FieldProgramCount] = m_iprogramCount;
      break;
    case FieldBitrate:
      sortable[FieldBitrate] = m_dwSize;
      break;
    case FieldTitle:
      sortable[FieldTitle] = m_strTitle;
      break;

    // If there's ever a need to convert more properties from CGUIListItem it might be
    // worth to make CGUIListItem implement ISortable as well and call it from here

    default:
      break;
  }

  if (HasMusicInfoTag())
    GetMusicInfoTag()->ToSortable(sortable, field);

  if (HasVideoInfoTag())
    GetVideoInfoTag()->ToSortable(sortable, field);

  if (HasPictureInfoTag())
    GetPictureInfoTag()->ToSortable(sortable, field);

  if (HasPVRChannelInfoTag())
    GetPVRChannelInfoTag()->ToSortable(sortable, field);

  if (HasAddonInfo())
  {
    switch (field)
    {
      case FieldInstallDate:
        sortable[FieldInstallDate] = GetAddonInfo()->InstallDate().GetAsDBDateTime();
        break;
      case FieldLastUpdated:
        sortable[FieldLastUpdated] = GetAddonInfo()->LastUpdated().GetAsDBDateTime();
        break;
      case FieldLastUsed:
        sortable[FieldLastUsed] = GetAddonInfo()->LastUsed().GetAsDBDateTime();
        break;
      default:
        break;
    }
  }
}

void CFileItem::ToSortable(SortItem &sortable, const Fields &fields) const
{
  Fields::const_iterator it;
  for (it = fields.begin(); it != fields.end(); it++)
    ToSortable(sortable, *it);

  /* FieldLabel is used as a fallback by all sorters and therefore has to be present as well */
  sortable[FieldLabel] = GetLabel();
  /* FieldSortSpecial and FieldFolder are required in conjunction with all other sorters as well */
  sortable[FieldSortSpecial] = m_specialSort;
  sortable[FieldFolder] = m_bIsFolder;
}

bool CFileItem::Exists(bool bUseCache /* = true */) const
{
  if (m_strPath.empty()
   || IsPath("add")
   || IsInternetStream()
   || IsParentFolder()
   || IsVirtualDirectoryRoot()
   || IsPlugin())
    return true;

  if (IsVideoDb() && HasVideoInfoTag())
  {
    CFileItem dbItem(m_bIsFolder ? GetVideoInfoTag()->m_strPath : GetVideoInfoTag()->m_strFileNameAndPath, m_bIsFolder);
    return dbItem.Exists();
  }

  std::string strPath = m_strPath;

  if (URIUtils::IsMultiPath(strPath))
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  if (URIUtils::IsStack(strPath))
    strPath = CStackDirectory::GetFirstStackedFile(strPath);

  if (m_bIsFolder)
    return CDirectory::Exists(strPath, bUseCache);
  else
    return CFile::Exists(strPath, bUseCache);

  return false;
}

bool CFileItem::IsVideo() const
{
  /* check preset mime type */
  if(StringUtils::StartsWithNoCase(m_mimetype, "video/"))
    return true;

  if (HasVideoInfoTag())
    return true;

  if (HasMusicInfoTag())
    return false;

  if (HasPictureInfoTag())
    return false;

  // only tv recordings are videos...
  if (IsPVRRecording())
    return !GetPVRRecordingInfoTag()->IsRadio();

  // ... all other PVR items are not.
  if (IsPVR())
    return false;

  if (URIUtils::IsDVD(m_strPath))
    return true;

  std::string extension;
  if(StringUtils::StartsWithNoCase(m_mimetype, "application/"))
  { /* check for some standard types */
    extension = m_mimetype.substr(12);
    if( StringUtils::EqualsNoCase(extension, "ogg")
     || StringUtils::EqualsNoCase(extension, "mp4")
     || StringUtils::EqualsNoCase(extension, "mxf") )
     return true;
  }

  return URIUtils::HasExtension(m_strPath, g_advancedSettings.m_videoExtensions);
}

bool CFileItem::IsEPG() const
{
  return HasEPGInfoTag();
}

bool CFileItem::IsPVRChannel() const
{
  return HasPVRChannelInfoTag();
}

bool CFileItem::IsPVRRecording() const
{
  return HasPVRRecordingInfoTag();
}

bool CFileItem::IsUsablePVRRecording() const
{
  return (m_pvrRecordingInfoTag && !m_pvrRecordingInfoTag->IsDeleted());
}

bool CFileItem::IsDeletedPVRRecording() const
{
  return (m_pvrRecordingInfoTag && m_pvrRecordingInfoTag->IsDeleted());
}

bool CFileItem::IsPVRTimer() const
{
  return HasPVRTimerInfoTag();
}

bool CFileItem::IsPVRRadioRDS() const
{
  return HasPVRRadioRDSInfoTag();
}

bool CFileItem::IsDiscStub() const
{
  if (IsVideoDb() && HasVideoInfoTag())
  {
    CFileItem dbItem(m_bIsFolder ? GetVideoInfoTag()->m_strPath : GetVideoInfoTag()->m_strFileNameAndPath, m_bIsFolder);
    return dbItem.IsDiscStub();
  }

  return URIUtils::HasExtension(m_strPath, g_advancedSettings.m_discStubExtensions);
}

bool CFileItem::IsAudio() const
{
  /* check preset mime type */
  if(StringUtils::StartsWithNoCase(m_mimetype, "audio/"))
    return true;

  if (HasMusicInfoTag())
    return true;

  if (HasVideoInfoTag())
    return false;

  if (HasPictureInfoTag())
    return false;

  if (IsCDDA())
    return true;

  if(StringUtils::StartsWithNoCase(m_mimetype, "application/"))
  { /* check for some standard types */
    std::string extension = m_mimetype.substr(12);
    if( StringUtils::EqualsNoCase(extension, "ogg")
     || StringUtils::EqualsNoCase(extension, "mp4")
     || StringUtils::EqualsNoCase(extension, "mxf") )
     return true;
  }

  return URIUtils::HasExtension(m_strPath, g_advancedSettings.GetMusicExtensions());
}

bool CFileItem::IsPicture() const
{
  if(StringUtils::StartsWithNoCase(m_mimetype, "image/"))
    return true;

  if (HasPictureInfoTag())
    return true;

  if (HasMusicInfoTag())
    return false;

  if (HasVideoInfoTag())
    return false;

  return CUtil::IsPicture(m_strPath);
}

bool CFileItem::IsLyrics() const
{
  return URIUtils::HasExtension(m_strPath, ".cdg|.lrc");
}

bool CFileItem::IsSubtitle() const
{
  return URIUtils::HasExtension(m_strPath, g_advancedSettings.m_subtitlesExtensions);
}

bool CFileItem::IsCUESheet() const
{
  return URIUtils::HasExtension(m_strPath, ".cue");
}

bool CFileItem::IsInternetStream(const bool bStrictCheck /* = false */) const
{
  if (HasProperty("IsHTTPDirectory"))
    return false;

  return URIUtils::IsInternetStream(m_strPath, bStrictCheck);
}

bool CFileItem::IsFileFolder(EFileFolderType types) const
{
  EFileFolderType always_type = EFILEFOLDER_TYPE_ALWAYS;

  /* internet streams are not directly expanded */
  if(IsInternetStream())
    always_type = EFILEFOLDER_TYPE_ONCLICK;

  if(types & always_type)
  {
    if(IsSmartPlayList()
    || (IsPlayList() && g_advancedSettings.m_playlistAsFolders)
    || IsAPK()
    || IsZIP()
    || IsRAR()
    || IsRSS()
    || IsType(".ogg|.oga|.nsf|.sid|.sap|.xbt|.xsp")
#if defined(TARGET_ANDROID)
    || IsType(".apk")
#endif
    )
    return true;
  }

  if(types & EFILEFOLDER_TYPE_ONBROWSE)
  {
    if((IsPlayList() && !g_advancedSettings.m_playlistAsFolders)
    || IsDiscImage())
      return true;
  }

  return false;
}

bool CFileItem::IsSmartPlayList() const
{
  if (HasProperty("library.smartplaylist") && GetProperty("library.smartplaylist").asBoolean())
    return true;

  return URIUtils::HasExtension(m_strPath, ".xsp");
}

bool CFileItem::IsLibraryFolder() const
{
  if (HasProperty("library.filter") && GetProperty("library.filter").asBoolean())
    return true;

  return URIUtils::IsLibraryFolder(m_strPath);
}

bool CFileItem::IsPlayList() const
{
  return CPlayListFactory::IsPlaylist(*this);
}

bool CFileItem::IsPythonScript() const
{
  return URIUtils::HasExtension(m_strPath, ".py");
}

bool CFileItem::IsType(const char *ext) const
{
  return URIUtils::HasExtension(m_strPath, ext);
}

bool CFileItem::IsNFO() const
{
  return URIUtils::HasExtension(m_strPath, ".nfo");
}

bool CFileItem::IsDiscImage() const
{
  return URIUtils::HasExtension(m_strPath, ".img|.iso|.nrg");
}

bool CFileItem::IsOpticalMediaFile() const
{
  if (IsDVDFile(false, true))
    return true;

  return IsBDFile();
}

bool CFileItem::IsDVDFile(bool bVobs /*= true*/, bool bIfos /*= true*/) const
{
  std::string strFileName = URIUtils::GetFileName(m_strPath);
  if (bIfos)
  {
    if (StringUtils::EqualsNoCase(strFileName, "video_ts.ifo"))
      return true;
    if (StringUtils::StartsWithNoCase(strFileName, "vts_") && StringUtils::EndsWithNoCase(strFileName, "_0.ifo") && strFileName.length() == 12)
      return true;
  }
  if (bVobs)
  {
    if (StringUtils::EqualsNoCase(strFileName, "video_ts.vob"))
      return true;
    if (StringUtils::StartsWithNoCase(strFileName, "vts_") && StringUtils::EndsWithNoCase(strFileName, ".vob"))
      return true;
  }

  return false;
}

bool CFileItem::IsBDFile() const
{
  std::string strFileName = URIUtils::GetFileName(m_strPath);
  return (StringUtils::EqualsNoCase(strFileName, "index.bdmv") || StringUtils::EqualsNoCase(strFileName, "MovieObject.bdmv"));
}

bool CFileItem::IsRAR() const
{
  return URIUtils::IsRAR(m_strPath);
}

bool CFileItem::IsAPK() const
{
  return URIUtils::IsAPK(m_strPath);
}

bool CFileItem::IsZIP() const
{
  return URIUtils::IsZIP(m_strPath);
}

bool CFileItem::IsCBZ() const
{
  return URIUtils::HasExtension(m_strPath, ".cbz");
}

bool CFileItem::IsCBR() const
{
  return URIUtils::HasExtension(m_strPath, ".cbr");
}

bool CFileItem::IsRSS() const
{
  return StringUtils::StartsWithNoCase(m_strPath, "rss://") || URIUtils::HasExtension(m_strPath, ".rss")
      || m_mimetype == "application/rss+xml";
}

bool CFileItem::IsAndroidApp() const
{
  return URIUtils::IsAndroidApp(m_strPath);
}

bool CFileItem::IsStack() const
{
  return URIUtils::IsStack(m_strPath);
}

bool CFileItem::IsPlugin() const
{
  return URIUtils::IsPlugin(m_strPath);
}

bool CFileItem::IsScript() const
{
  return URIUtils::IsScript(m_strPath);
}

bool CFileItem::IsAddonsPath() const
{
  return URIUtils::IsAddonsPath(m_strPath);
}

bool CFileItem::IsSourcesPath() const
{
  return URIUtils::IsSourcesPath(m_strPath);
}

bool CFileItem::IsMultiPath() const
{
  return URIUtils::IsMultiPath(m_strPath);
}

bool CFileItem::IsCDDA() const
{
  return URIUtils::IsCDDA(m_strPath);
}

bool CFileItem::IsDVD() const
{
  return URIUtils::IsDVD(m_strPath) || m_iDriveType == CMediaSource::SOURCE_TYPE_DVD;
}

bool CFileItem::IsOnDVD() const
{
  return URIUtils::IsOnDVD(m_strPath) || m_iDriveType == CMediaSource::SOURCE_TYPE_DVD;
}

bool CFileItem::IsNfs() const
{
  return URIUtils::IsNfs(m_strPath);
}

bool CFileItem::IsOnLAN() const
{
  return URIUtils::IsOnLAN(m_strPath);
}

bool CFileItem::IsISO9660() const
{
  return URIUtils::IsISO9660(m_strPath);
}

bool CFileItem::IsRemote() const
{
  return URIUtils::IsRemote(m_strPath);
}

bool CFileItem::IsSmb() const
{
  return URIUtils::IsSmb(m_strPath);
}

bool CFileItem::IsURL() const
{
  return URIUtils::IsURL(m_strPath);
}

bool CFileItem::IsPVR() const
{
  return CUtil::IsPVR(m_strPath);
}

bool CFileItem::IsLiveTV() const
{
  return URIUtils::IsLiveTV(m_strPath);
}

bool CFileItem::IsHD() const
{
  return URIUtils::IsHD(m_strPath);
}

bool CFileItem::IsMusicDb() const
{
  return URIUtils::IsMusicDb(m_strPath);
}

bool CFileItem::IsVideoDb() const
{
  return URIUtils::IsVideoDb(m_strPath);
}

bool CFileItem::IsVirtualDirectoryRoot() const
{
  return (m_bIsFolder && m_strPath.empty());
}

bool CFileItem::IsRemovable() const
{
  return IsOnDVD() || IsCDDA() || m_iDriveType == CMediaSource::SOURCE_TYPE_REMOVABLE;
}

bool CFileItem::IsReadOnly() const
{
  if (IsParentFolder())
    return true;

  if (m_bIsShareOrDrive)
    return true;

  return !CUtil::SupportsWriteFileOperations(m_strPath);
}

void CFileItem::FillInDefaultIcon()
{
  if (URIUtils::IsPVRGuideItem(m_strPath))
  {
    // epg items never have a default icon. no need to execute this expensive method.
    // when filling epg grid window, easily tens of thousands of epg items are processed.
    return;
  }

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

  if (GetIconImage().empty())
  {
    if (!m_bIsFolder)
    {
      /* To reduce the average runtime of this code, this list should
       * be ordered with most frequently seen types first.  Also bear
       * in mind the complexity of the code behind the check in the
       * case of IsWhatater() returns false.
       */
      if (IsPVRChannel())
      {
        if (GetPVRChannelInfoTag()->IsRadio())
          SetIconImage("DefaultAudio.png");
        else
          SetIconImage("DefaultVideo.png");
      }
      else if ( IsLiveTV() )
      {
        // Live TV Channel
        SetIconImage("DefaultVideo.png");
      }
      else if ( URIUtils::IsArchive(m_strPath) )
      { // archive
        SetIconImage("DefaultFile.png");
      }
      else if ( IsUsablePVRRecording() )
      {
        // PVR recording
        SetIconImage("DefaultVideo.png");
      }
      else if ( IsDeletedPVRRecording() )
      {
        // PVR deleted recording
        SetIconImage("DefaultVideoDeleted.png");
      }
      else if ( IsAudio() )
      {
        // audio
        SetIconImage("DefaultAudio.png");
      }
      else if ( IsVideo() )
      {
        // video
        SetIconImage("DefaultVideo.png");
      }
      else if (IsPVRTimer())
      {
        SetIconImage("DefaultVideo.png");
      }
      else if ( IsPicture() )
      {
        // picture
        SetIconImage("DefaultPicture.png");
      }
      else if ( IsPlayList() )
      {
        SetIconImage("DefaultPlaylist.png");
      }
      else if ( IsPythonScript() )
      {
        SetIconImage("DefaultScript.png");
      }
      else
      {
        // default icon for unknown file type
        SetIconImage("DefaultFile.png");
      }
    }
    else
    {
      if ( IsPlayList() )
      {
        SetIconImage("DefaultPlaylist.png");
      }
      else if (IsParentFolder())
      {
        SetIconImage("DefaultFolderBack.png");
      }
      else
      {
        SetIconImage("DefaultFolder.png");
      }
    }
  }
  // Set the icon overlays (if applicable)
  if (!HasOverlay())
  {
    if (URIUtils::IsInRAR(m_strPath))
      SetOverlayImage(CGUIListItem::ICON_OVERLAY_RAR);
    else if (URIUtils::IsInZIP(m_strPath))
      SetOverlayImage(CGUIListItem::ICON_OVERLAY_ZIP);
  }
}

void CFileItem::RemoveExtension()
{
  if (m_bIsFolder)
    return;

  std::string strLabel = GetLabel();
  URIUtils::RemoveExtension(strLabel);
  SetLabel(strLabel);
}

void CFileItem::CleanString()
{
  if (IsLiveTV())
    return;

  std::string strLabel = GetLabel();
  std::string strTitle, strTitleAndYear, strYear;
  CUtil::CleanString(strLabel, strTitle, strTitleAndYear, strYear, true);
  SetLabel(strTitleAndYear);
}

void CFileItem::SetLabel(const std::string &strLabel)
{
  if (strLabel == "..")
  {
    m_bIsParentFolder = true;
    m_bIsFolder = true;
    m_specialSort = SortSpecialOnTop;
    SetLabelPreformated(true);
  }
  CGUIListItem::SetLabel(strLabel);
}

void CFileItem::SetFileSizeLabel()
{
  if(m_bIsFolder && m_dwSize == 0)
    SetLabel2("");
  else
    SetLabel2(StringUtils::SizeToString(m_dwSize));
}

bool CFileItem::CanQueue() const
{
  return m_bCanQueue;
}

void CFileItem::SetCanQueue(bool bYesNo)
{
  m_bCanQueue = bYesNo;
}

bool CFileItem::IsParentFolder() const
{
  return m_bIsParentFolder;
}

void CFileItem::FillInMimeType(bool lookup /*= true*/)
{
  //! @todo adapt this to use CMime::GetMimeType()
  if (m_mimetype.empty())
  {
    if( m_bIsFolder )
      m_mimetype = "x-directory/normal";
    else if( m_pvrChannelInfoTag )
      m_mimetype = m_pvrChannelInfoTag->InputFormat();
    else if( StringUtils::StartsWithNoCase(m_strPath, "shout://")
          || StringUtils::StartsWithNoCase(m_strPath, "http://")
          || StringUtils::StartsWithNoCase(m_strPath, "https://"))
    {
      // If lookup is false, bail out early to leave mime type empty
      if (!lookup)
        return;

      CCurlFile::GetMimeType(GetURL(), m_mimetype);

      // try to get mime-type again but with an NSPlayer User-Agent
      // in order for server to provide correct mime-type.  Allows us
      // to properly detect an MMS stream
      if (StringUtils::StartsWithNoCase(m_mimetype, "video/x-ms-"))
        CCurlFile::GetMimeType(GetURL(), m_mimetype, "NSPlayer/11.00.6001.7000");

      // make sure there are no options set in mime-type
      // mime-type can look like "video/x-ms-asf ; charset=utf8"
      size_t i = m_mimetype.find(';');
      if(i != std::string::npos)
        m_mimetype.erase(i, m_mimetype.length() - i);
      StringUtils::Trim(m_mimetype);
    }
    else
      m_mimetype = CMime::GetMimeType(*this);

    // if it's still empty set to an unknown type
    if (m_mimetype.empty())
      m_mimetype = "application/octet-stream";
  }

  // change protocol to mms for the following mime-type.  Allows us to create proper FileMMS.
  if( StringUtils::StartsWithNoCase(m_mimetype, "application/vnd.ms.wms-hdr.asfv1") || StringUtils::StartsWithNoCase(m_mimetype, "application/x-mms-framed") )
    StringUtils::Replace(m_strPath, "http:", "mms:");
}

void CFileItem::SetMimeTypeForInternetFile()
{
  if (m_doContentLookup && IsInternetStream())
  {
    SetMimeType("");
    FillInMimeType(true);
  }
}

bool CFileItem::IsSamePath(const CFileItem *item) const
{
  if (!item)
    return false;

  if (item->GetPath() == m_strPath)
  {
    if (item->HasProperty("item_start") || HasProperty("item_start"))
      return (item->GetProperty("item_start") == GetProperty("item_start"));
    return true;
  }
  if (HasVideoInfoTag() && item->HasVideoInfoTag())
  {
    if (m_videoInfoTag->m_iDbId != -1 && item->m_videoInfoTag->m_iDbId != -1)
      return ((m_videoInfoTag->m_iDbId == item->m_videoInfoTag->m_iDbId) &&
        (m_videoInfoTag->m_type == item->m_videoInfoTag->m_type));
  }
  if (IsMusicDb() && HasMusicInfoTag())
  {
    CFileItem dbItem(m_musicInfoTag->GetURL(), false);
    if (HasProperty("item_start"))
      dbItem.SetProperty("item_start", GetProperty("item_start"));
    return dbItem.IsSamePath(item);
  }
  if (IsVideoDb() && HasVideoInfoTag())
  {
    CFileItem dbItem(m_videoInfoTag->m_strFileNameAndPath, false);
    if (HasProperty("item_start"))
      dbItem.SetProperty("item_start", GetProperty("item_start"));
    return dbItem.IsSamePath(item);
  }
  if (item->IsMusicDb() && item->HasMusicInfoTag())
  {
    CFileItem dbItem(item->m_musicInfoTag->GetURL(), false);
    if (item->HasProperty("item_start"))
      dbItem.SetProperty("item_start", item->GetProperty("item_start"));
    return IsSamePath(&dbItem);
  }
  if (item->IsVideoDb() && item->HasVideoInfoTag())
  {
    CFileItem dbItem(item->m_videoInfoTag->m_strFileNameAndPath, false);
    if (item->HasProperty("item_start"))
      dbItem.SetProperty("item_start", item->GetProperty("item_start"));
    return IsSamePath(&dbItem);
  }
  if (HasProperty("original_listitem_url"))
    return (GetProperty("original_listitem_url") == item->GetPath());
  return false;
}

bool CFileItem::IsAlbum() const
{
  return m_bIsAlbum;
}

void CFileItem::UpdateInfo(const CFileItem &item, bool replaceLabels /*=true*/)
{
  if (item.HasVideoInfoTag())
  { // copy info across
    //! @todo premiered info is normally stored in m_dateTime by the db
    *GetVideoInfoTag() = *item.GetVideoInfoTag();
    // preferably use some information from PVR info tag if available
    if (m_pvrRecordingInfoTag)
      m_pvrRecordingInfoTag->CopyClientInfo(GetVideoInfoTag());
    SetOverlayImage(ICON_OVERLAY_UNWATCHED, GetVideoInfoTag()->m_playCount > 0);
    SetInvalid();
  }
  if (item.HasMusicInfoTag())
  {
    *GetMusicInfoTag() = *item.GetMusicInfoTag();
    SetInvalid();
  }
  if (item.HasPVRRadioRDSInfoTag())
  {
    m_pvrRadioRDSInfoTag = item.m_pvrRadioRDSInfoTag;
    SetInvalid();
  }
  if (item.HasPictureInfoTag())
  {
    *GetPictureInfoTag() = *item.GetPictureInfoTag();
    SetInvalid();
  }
  if (replaceLabels && !item.GetLabel().empty())
    SetLabel(item.GetLabel());
  if (replaceLabels && !item.GetLabel2().empty())
    SetLabel2(item.GetLabel2());
  if (!item.GetArt("thumb").empty())
    SetArt("thumb", item.GetArt("thumb"));
  if (!item.GetIconImage().empty())
    SetIconImage(item.GetIconImage());
  AppendProperties(item);
}

void CFileItem::SetFromVideoInfoTag(const CVideoInfoTag &video)
{
  if (!video.m_strTitle.empty())
    SetLabel(video.m_strTitle);
  if (video.m_strFileNameAndPath.empty())
  {
    m_strPath = video.m_strPath;
    URIUtils::AddSlashAtEnd(m_strPath);
    m_bIsFolder = true;
  }
  else
  {
    m_strPath = video.m_strFileNameAndPath;
    m_bIsFolder = false;
  }

  *GetVideoInfoTag() = video;
  if (video.m_iSeason == 0)
    SetProperty("isspecial", "true");
  FillInDefaultIcon();
  FillInMimeType(false);
}

void CFileItem::SetFromMusicInfoTag(const MUSIC_INFO::CMusicInfoTag &music)
{
  if (!music.GetTitle().empty())
    SetLabel(music.GetTitle());
  if (!music.GetURL().empty())
    m_strPath = music.GetURL();
  m_bIsFolder = URIUtils::HasSlashAtEnd(m_strPath);

  *GetMusicInfoTag() = music;
  FillInDefaultIcon();
  FillInMimeType(false);
}

void CFileItem::SetFromAlbum(const CAlbum &album)
{
  if (!album.strAlbum.empty())
    SetLabel(album.strAlbum);
  m_bIsFolder = true;
  m_strLabel2 = album.GetAlbumArtistString();
  GetMusicInfoTag()->SetAlbum(album);
  SetArt(album.art);
  m_bIsAlbum = true;
  CMusicDatabase::SetPropertiesFromAlbum(*this,album);
  FillInMimeType(false);
}

void CFileItem::SetFromSong(const CSong &song)
{
  if (!song.strTitle.empty())
    SetLabel(song.strTitle);
  if (song.idSong > 0)
  {
    std::string strExt = URIUtils::GetExtension(song.strFileName);
    m_strPath = StringUtils::Format("musicdb://songs/%li%s", song.idSong, strExt.c_str());
  }
  else if (!song.strFileName.empty())
    m_strPath = song.strFileName;
  GetMusicInfoTag()->SetSong(song);
  m_lStartOffset = song.iStartOffset;
  m_lStartPartNumber = 1;
  SetProperty("item_start", song.iStartOffset);
  m_lEndOffset = song.iEndOffset;
  if (!song.strThumb.empty())
    SetArt("thumb", song.strThumb);
  FillInMimeType(false);
}

std::string CFileItem::GetOpticalMediaPath() const
{
  std::string path;
  path = URIUtils::AddFileToFolder(GetPath(), "VIDEO_TS.IFO");
  if (CFile::Exists(path))
    return path;
  
  path = URIUtils::AddFileToFolder(GetPath(), "VIDEO_TS", "VIDEO_TS.IFO");
  if (CFile::Exists(path))
    return path;

#ifdef HAVE_LIBBLURAY
  path = URIUtils::AddFileToFolder(GetPath(), "index.bdmv");
  if (CFile::Exists(path))
    return path;

  path = URIUtils::AddFileToFolder(GetPath(), "BDMV", "index.bdmv");
  if (CFile::Exists(path))
    return path;
#endif
  return std::string();
}

/**
* @todo Ideally this (and SetPath) would not be available outside of construction
* for CFileItem objects, or at least restricted to essentially be equivalent
* to construction. This would require re-formulating a bunch of CFileItem
* construction, and also allowing CFileItemList to have it's own (public)
* SetURL() function, so for now we give direct access.
*/
void CFileItem::SetURL(const CURL& url)
{
  m_strPath = url.Get();
}

const CURL CFileItem::GetURL() const
{
  CURL url(m_strPath);
  return url;
}

bool CFileItem::IsURL(const CURL& url) const
{
  return IsPath(url.Get());
}

bool CFileItem::IsPath(const std::string& path, bool ignoreURLOptions /* = false */) const
{
  return URIUtils::PathEquals(m_strPath, path, false, ignoreURLOptions);
}

void CFileItem::SetCueDocument(const CCueDocumentPtr& cuePtr)
{
  m_cueDocument = cuePtr;
}

void CFileItem::LoadEmbeddedCue()
{
  CMusicInfoTag& tag = *GetMusicInfoTag();
  if (!tag.Loaded())
    return;

  const std::string embeddedCue = tag.GetCueSheet();
  if (!embeddedCue.empty())
  {
    CCueDocumentPtr cuesheet(new CCueDocument);
    if (cuesheet->ParseTag(embeddedCue))
    {
      std::vector<std::string> MediaFileVec;
      cuesheet->GetMediaFiles(MediaFileVec);
      for (std::vector<std::string>::iterator itMedia = MediaFileVec.begin(); itMedia != MediaFileVec.end(); itMedia++)
        cuesheet->UpdateMediaFile(*itMedia, GetPath());
      SetCueDocument(cuesheet);
    }
  }
}

bool CFileItem::HasCueDocument() const
{
  return (m_cueDocument.get() != nullptr);
}

bool CFileItem::LoadTracksFromCueDocument(CFileItemList& scannedItems)
{
  if (!m_cueDocument)
    return false;

  CMusicInfoTag& tag = *GetMusicInfoTag();

  VECSONGS tracks;
  m_cueDocument->GetSongs(tracks);

  bool oneFilePerTrack = m_cueDocument->IsOneFilePerTrack();
  m_cueDocument.reset();

  int tracksFound = 0;
  for (VECSONGS::iterator it = tracks.begin(); it != tracks.end(); ++it)
  {
    CSong& song = *it;
    if (song.strFileName == GetPath())
    {
      if (tag.Loaded())
      {
        if (song.strAlbum.empty() && !tag.GetAlbum().empty())
          song.strAlbum = tag.GetAlbum();
        //Pass album artist to final MusicInfoTag object via setting song album artist vector.
        if (song.GetAlbumArtist().empty() && !tag.GetAlbumArtist().empty())
          song.SetAlbumArtist(tag.GetAlbumArtist());
        if (song.genre.empty() && !tag.GetGenre().empty())
          song.genre = tag.GetGenre();
        //Pass artist to final MusicInfoTag object via setting song artist description string only.
        //Artist credits not used during loading from cue sheet.
        if (song.strArtistDesc.empty() && !tag.GetArtistString().empty())
          song.strArtistDesc = tag.GetArtistString();
        if (tag.GetDiscNumber())
          song.iTrack |= (tag.GetDiscNumber() << 16); // see CMusicInfoTag::GetDiscNumber()
        if (!tag.GetCueSheet().empty())
          song.strCueSheet = tag.GetCueSheet();

        SYSTEMTIME dateTime;
        tag.GetReleaseDate(dateTime);
        if (dateTime.wYear)
          song.iYear = dateTime.wYear;
        if (song.embeddedArt.empty() && !tag.GetCoverArtInfo().empty())
          song.embeddedArt = tag.GetCoverArtInfo();
      }

      if (!song.iDuration && tag.GetDuration() > 0)
      { // must be the last song
        song.iDuration = (tag.GetDuration() * 75 - song.iStartOffset + 37) / 75;
      }
      if ( tag.Loaded() && oneFilePerTrack && ! ( tag.GetAlbum().empty() || tag.GetArtist().empty() || tag.GetTitle().empty() ) )
      {
        // If there are multiple files in a cue file, the tags from the files should be prefered if they exist.
        scannedItems.Add(CFileItemPtr(new CFileItem(song, tag)));
      }
      else
      {
        scannedItems.Add(CFileItemPtr(new CFileItem(song)));
      }
      ++tracksFound;
    }
  }
  return tracksFound != 0;
}

/////////////////////////////////////////////////////////////////////////////////
/////
///// CFileItemList
/////
//////////////////////////////////////////////////////////////////////////////////

CFileItemList::CFileItemList()
: CFileItem("", true),
  m_ignoreURLOptions(false),
  m_fastLookup(false),
  m_sortIgnoreFolders(false),
  m_cacheToDisc(CACHE_IF_SLOW),
  m_replaceListing(false)
{
}

CFileItemList::CFileItemList(const std::string& strPath)
: CFileItem(strPath, true),
  m_ignoreURLOptions(false),
  m_fastLookup(false),
  m_sortIgnoreFolders(false),
  m_cacheToDisc(CACHE_IF_SLOW),
  m_replaceListing(false)
{
}

CFileItemList::~CFileItemList()
{
  Clear();
}

CFileItemPtr CFileItemList::operator[] (int iItem)
{
  return Get(iItem);
}

const CFileItemPtr CFileItemList::operator[] (int iItem) const
{
  return Get(iItem);
}

CFileItemPtr CFileItemList::operator[] (const std::string& strPath)
{
  return Get(strPath);
}

const CFileItemPtr CFileItemList::operator[] (const std::string& strPath) const
{
  return Get(strPath);
}

void CFileItemList::SetIgnoreURLOptions(bool ignoreURLOptions)
{
  m_ignoreURLOptions = ignoreURLOptions;

  if (m_fastLookup)
  {
    m_fastLookup = false; // Force SetFastlookup to clear map
    SetFastLookup(true);  // and regenerate map
  }
}

void CFileItemList::SetFastLookup(bool fastLookup)
{
  CSingleLock lock(m_lock);

  if (fastLookup && !m_fastLookup)
  { // generate the map
    m_map.clear();
    for (unsigned int i=0; i < m_items.size(); i++)
    {
      CFileItemPtr pItem = m_items[i];
      m_map.insert(MAPFILEITEMSPAIR(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions() : pItem->GetPath(), pItem));
    }
  }
  if (!fastLookup && m_fastLookup)
    m_map.clear();
  m_fastLookup = fastLookup;
}

bool CFileItemList::Contains(const std::string& fileName) const
{
  CSingleLock lock(m_lock);

  if (m_fastLookup)
    return m_map.find(m_ignoreURLOptions ? CURL(fileName).GetWithoutOptions() : fileName) != m_map.end();

  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    const CFileItemPtr pItem = m_items[i];
    if (pItem->IsPath(m_ignoreURLOptions ? CURL(fileName).GetWithoutOptions() : fileName))
      return true;
  }
  return false;
}

void CFileItemList::Clear()
{
  CSingleLock lock(m_lock);

  ClearItems();
  m_sortDescription.sortBy = SortByNone;
  m_sortDescription.sortOrder = SortOrderNone;
  m_sortDescription.sortAttributes = SortAttributeNone;
  m_sortIgnoreFolders = false;
  m_cacheToDisc = CACHE_IF_SLOW;
  m_sortDetails.clear();
  m_replaceListing = false;
  m_content.clear();
}

void CFileItemList::ClearItems()
{
  CSingleLock lock(m_lock);
  // make sure we free the memory of the items (these are GUIControls which may have allocated resources)
  FreeMemory();
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr item = m_items[i];
    item->FreeMemory();
  }
  m_items.clear();
  m_map.clear();
}

void CFileItemList::Add(CFileItemPtr pItem)
{
  CSingleLock lock(m_lock);
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions() : pItem->GetPath(), pItem));
  m_items.emplace_back(std::move(pItem));
}

void CFileItemList::Add(CFileItem&& item)
{
  CSingleLock lock(m_lock);
  auto ptr = std::make_shared<CFileItem>(std::move(item));
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(m_ignoreURLOptions ? CURL(ptr->GetPath()).GetWithoutOptions() : ptr->GetPath(), ptr));
  m_items.emplace_back(std::move(ptr));
}

void CFileItemList::AddFront(const CFileItemPtr &pItem, int itemPosition)
{
  CSingleLock lock(m_lock);

  if (itemPosition >= 0)
  {
    m_items.insert(m_items.begin()+itemPosition, pItem);
  }
  else
  {
    m_items.insert(m_items.begin()+(m_items.size()+itemPosition), pItem);
  }
  if (m_fastLookup)
  {
    m_map.insert(MAPFILEITEMSPAIR(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions() : pItem->GetPath(), pItem));
  }
}

void CFileItemList::Remove(CFileItem* pItem)
{
  CSingleLock lock(m_lock);

  for (IVECFILEITEMS it = m_items.begin(); it != m_items.end(); ++it)
  {
    if (pItem == it->get())
    {
      m_items.erase(it);
      if (m_fastLookup)
      {
        m_map.erase(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions() : pItem->GetPath());
      }
      break;
    }
  }
}

void CFileItemList::Remove(int iItem)
{
  CSingleLock lock(m_lock);

  if (iItem >= 0 && iItem < (int)Size())
  {
    CFileItemPtr pItem = *(m_items.begin() + iItem);
    if (m_fastLookup)
    {
      m_map.erase(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions() : pItem->GetPath());
    }
    m_items.erase(m_items.begin() + iItem);
  }
}

void CFileItemList::Append(const CFileItemList& itemlist)
{
  CSingleLock lock(m_lock);

  for (int i = 0; i < itemlist.Size(); ++i)
    Add(itemlist[i]);
}

void CFileItemList::Assign(const CFileItemList& itemlist, bool append)
{
  CSingleLock lock(m_lock);
  if (!append)
    Clear();
  Append(itemlist);
  SetPath(itemlist.GetPath());
  SetLabel(itemlist.GetLabel());
  m_sortDetails = itemlist.m_sortDetails;
  m_sortDescription = itemlist.m_sortDescription;
  m_replaceListing = itemlist.m_replaceListing;
  m_content = itemlist.m_content;
  m_mapProperties = itemlist.m_mapProperties;
  m_cacheToDisc = itemlist.m_cacheToDisc;
}

bool CFileItemList::Copy(const CFileItemList& items, bool copyItems /* = true */)
{
  // assign all CFileItem parts
  *(CFileItem*)this = *(CFileItem*)&items;

  // assign the rest of the CFileItemList properties
  m_replaceListing  = items.m_replaceListing;
  m_content         = items.m_content;
  m_mapProperties   = items.m_mapProperties;
  m_cacheToDisc     = items.m_cacheToDisc;
  m_sortDetails     = items.m_sortDetails;
  m_sortDescription = items.m_sortDescription;
  m_sortIgnoreFolders = items.m_sortIgnoreFolders;

  if (copyItems)
  {
    // make a copy of each item
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr newItem(new CFileItem(*items[i]));
      Add(newItem);
    }
  }

  return true;
}

CFileItemPtr CFileItemList::Get(int iItem)
{
  CSingleLock lock(m_lock);

  if (iItem > -1 && iItem < (int)m_items.size())
    return m_items[iItem];

  return CFileItemPtr();
}

const CFileItemPtr CFileItemList::Get(int iItem) const
{
  CSingleLock lock(m_lock);

  if (iItem > -1 && iItem < (int)m_items.size())
    return m_items[iItem];

  return CFileItemPtr();
}

CFileItemPtr CFileItemList::Get(const std::string& strPath)
{
  CSingleLock lock(m_lock);

  if (m_fastLookup)
  {
    IMAPFILEITEMS it = m_map.find(m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath);
    if (it != m_map.end())
      return it->second;

    return CFileItemPtr();
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsPath(m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath))
      return pItem;
  }

  return CFileItemPtr();
}

const CFileItemPtr CFileItemList::Get(const std::string& strPath) const
{
  CSingleLock lock(m_lock);

  if (m_fastLookup)
  {
    std::map<std::string, CFileItemPtr>::const_iterator it = m_map.find(m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath);
    if (it != m_map.end())
      return it->second;

    return CFileItemPtr();
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsPath(m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath))
      return pItem;
  }

  return CFileItemPtr();
}

int CFileItemList::Size() const
{
  CSingleLock lock(m_lock);
  return (int)m_items.size();
}

bool CFileItemList::IsEmpty() const
{
  CSingleLock lock(m_lock);
  return m_items.empty();
}

void CFileItemList::Reserve(int iCount)
{
  CSingleLock lock(m_lock);
  m_items.reserve(iCount);
}

void CFileItemList::Sort(FILEITEMLISTCOMPARISONFUNC func)
{
  CSingleLock lock(m_lock);
  std::stable_sort(m_items.begin(), m_items.end(), func);
}

void CFileItemList::FillSortFields(FILEITEMFILLFUNC func)
{
  CSingleLock lock(m_lock);
  std::for_each(m_items.begin(), m_items.end(), func);
}

void CFileItemList::Sort(SortBy sortBy, SortOrder sortOrder, SortAttribute sortAttributes /* = SortAttributeNone */)
{
  if (sortBy == SortByNone ||
     (m_sortDescription.sortBy == sortBy && m_sortDescription.sortOrder == sortOrder &&
      m_sortDescription.sortAttributes == sortAttributes))
    return;

  SortDescription sorting;
  sorting.sortBy = sortBy;
  sorting.sortOrder = sortOrder;
  sorting.sortAttributes = sortAttributes;

  Sort(sorting);
  m_sortDescription = sorting;
}

void CFileItemList::Sort(SortDescription sortDescription)
{
  if (sortDescription.sortBy == SortByFile ||
      sortDescription.sortBy == SortBySortTitle ||
      sortDescription.sortBy == SortByDateAdded ||
      sortDescription.sortBy == SortByRating ||
      sortDescription.sortBy == SortByYear ||
      sortDescription.sortBy == SortByPlaylistOrder ||
      sortDescription.sortBy == SortByLastPlayed ||
      sortDescription.sortBy == SortByPlaycount)
    sortDescription.sortAttributes = (SortAttribute)((int)sortDescription.sortAttributes | SortAttributeIgnoreFolders);

  if (sortDescription.sortBy == SortByNone ||
     (m_sortDescription.sortBy == sortDescription.sortBy && m_sortDescription.sortOrder == sortDescription.sortOrder &&
      m_sortDescription.sortAttributes == sortDescription.sortAttributes))
    return;

  if (m_sortIgnoreFolders)
    sortDescription.sortAttributes = (SortAttribute)((int)sortDescription.sortAttributes | SortAttributeIgnoreFolders);

  const Fields fields = SortUtils::GetFieldsForSorting(sortDescription.sortBy);
  SortItems sortItems((size_t)Size());
  for (int index = 0; index < Size(); index++)
  {
    sortItems[index] = std::shared_ptr<SortItem>(new SortItem);
    m_items[index]->ToSortable(*sortItems[index], fields);
    (*sortItems[index])[FieldId] = index;
  }

  // do the sorting
  SortUtils::Sort(sortDescription, sortItems);

  // apply the new order to the existing CFileItems
  VECFILEITEMS sortedFileItems;
  sortedFileItems.reserve(Size());
  for (SortItems::const_iterator it = sortItems.begin(); it != sortItems.end(); it++)
  {
    CFileItemPtr item = m_items[(int)(*it)->at(FieldId).asInteger()];
    // Set the sort label in the CFileItem
    item->SetSortLabel((*it)->at(FieldSort).asWideString());

    sortedFileItems.push_back(item);
  }

  // replace the current list with the re-ordered one
  m_items = std::move(sortedFileItems);
}

void CFileItemList::Randomize()
{
  CSingleLock lock(m_lock);
  KODI::UTILS::RandomShuffle(m_items.begin(), m_items.end());
}

void CFileItemList::Archive(CArchive& ar)
{
  CSingleLock lock(m_lock);
  if (ar.IsStoring())
  {
    CFileItem::Archive(ar);

    int i = 0;
    if (!m_items.empty() && m_items[0]->IsParentFolder())
      i = 1;

    ar << (int)(m_items.size() - i);

    ar << m_ignoreURLOptions;

    ar << m_fastLookup;

    ar << (int)m_sortDescription.sortBy;
    ar << (int)m_sortDescription.sortOrder;
    ar << (int)m_sortDescription.sortAttributes;
    ar << m_sortIgnoreFolders;
    ar << (int)m_cacheToDisc;

    ar << (int)m_sortDetails.size();
    for (unsigned int j = 0; j < m_sortDetails.size(); ++j)
    {
      const GUIViewSortDetails &details = m_sortDetails[j];
      ar << (int)details.m_sortDescription.sortBy;
      ar << (int)details.m_sortDescription.sortOrder;
      ar << (int)details.m_sortDescription.sortAttributes;
      ar << details.m_buttonLabel;
      ar << details.m_labelMasks.m_strLabelFile;
      ar << details.m_labelMasks.m_strLabelFolder;
      ar << details.m_labelMasks.m_strLabel2File;
      ar << details.m_labelMasks.m_strLabel2Folder;
    }

    ar << m_content;

    for (; i < (int)m_items.size(); ++i)
    {
      CFileItemPtr pItem = m_items[i];
      ar << *pItem;
    }
  }
  else
  {
    CFileItemPtr pParent;
    if (!IsEmpty())
    {
      CFileItemPtr pItem=m_items[0];
      if (pItem->IsParentFolder())
        pParent.reset(new CFileItem(*pItem));
    }

    SetIgnoreURLOptions(false);
    SetFastLookup(false);
    Clear();

    CFileItem::Archive(ar);

    int iSize = 0;
    ar >> iSize;
    if (iSize <= 0)
      return ;

    if (pParent)
    {
      m_items.reserve(iSize + 1);
      m_items.push_back(pParent);
    }
    else
      m_items.reserve(iSize);

    bool ignoreURLOptions = false;
    ar >> ignoreURLOptions;

    bool fastLookup = false;
    ar >> fastLookup;

    int tempint;
    ar >> (int&)tempint;
    m_sortDescription.sortBy = (SortBy)tempint;
    ar >> (int&)tempint;
    m_sortDescription.sortOrder = (SortOrder)tempint;
    ar >> (int&)tempint;
    m_sortDescription.sortAttributes = (SortAttribute)tempint;
    ar >> m_sortIgnoreFolders;
    ar >> (int&)tempint;
    m_cacheToDisc = CACHE_TYPE(tempint);

    unsigned int detailSize = 0;
    ar >> detailSize;
    for (unsigned int j = 0; j < detailSize; ++j)
    {
      GUIViewSortDetails details;
      ar >> (int&)tempint;
      details.m_sortDescription.sortBy = (SortBy)tempint;
      ar >> (int&)tempint;
      details.m_sortDescription.sortOrder = (SortOrder)tempint;
      ar >> (int&)tempint;
      details.m_sortDescription.sortAttributes = (SortAttribute)tempint;
      ar >> details.m_buttonLabel;
      ar >> details.m_labelMasks.m_strLabelFile;
      ar >> details.m_labelMasks.m_strLabelFolder;
      ar >> details.m_labelMasks.m_strLabel2File;
      ar >> details.m_labelMasks.m_strLabel2Folder;
      m_sortDetails.push_back(details);
    }

    ar >> m_content;

    for (int i = 0; i < iSize; ++i)
    {
      CFileItemPtr pItem(new CFileItem);
      ar >> *pItem;
      Add(pItem);
    }

    SetIgnoreURLOptions(ignoreURLOptions);
    SetFastLookup(fastLookup);
  }
}

void CFileItemList::FillInDefaultIcons()
{
  CSingleLock lock(m_lock);
  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->FillInDefaultIcon();
  }
}

int CFileItemList::GetFolderCount() const
{
  CSingleLock lock(m_lock);
  int nFolderCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->m_bIsFolder)
      nFolderCount++;
  }

  return nFolderCount;
}

int CFileItemList::GetObjectCount() const
{
  CSingleLock lock(m_lock);

  int numObjects = (int)m_items.size();
  if (numObjects && m_items[0]->IsParentFolder())
    numObjects--;

  return numObjects;
}

int CFileItemList::GetFileCount() const
{
  CSingleLock lock(m_lock);
  int nFileCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (!pItem->m_bIsFolder)
      nFileCount++;
  }

  return nFileCount;
}

int CFileItemList::GetSelectedCount() const
{
  CSingleLock lock(m_lock);
  int count = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsSelected())
      count++;
  }

  return count;
}

void CFileItemList::FilterCueItems()
{
  CSingleLock lock(m_lock);
  // Handle .CUE sheet files...
  std::vector<std::string> itemstodelete;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (!pItem->m_bIsFolder)
    { // see if it's a .CUE sheet
      if (pItem->IsCUESheet())
      {
        CCueDocumentPtr cuesheet(new CCueDocument);
        if (cuesheet->ParseFile(pItem->GetPath()))
        {
          std::vector<std::string> MediaFileVec;
          cuesheet->GetMediaFiles(MediaFileVec);

          // queue the cue sheet and the underlying media file for deletion
          for(std::vector<std::string>::iterator itMedia = MediaFileVec.begin(); itMedia != MediaFileVec.end(); itMedia++)
          {
            std::string strMediaFile = *itMedia;
            std::string fileFromCue = strMediaFile; // save the file from the cue we're matching against,
                                                   // as we're going to search for others here...
            bool bFoundMediaFile = CFile::Exists(strMediaFile);
            if (!bFoundMediaFile)
            {
              // try file in same dir, not matching case...
              if (Contains(strMediaFile))
              {
                bFoundMediaFile = true;
              }
              else
              {
                // try removing the .cue extension...
                strMediaFile = pItem->GetPath();
                URIUtils::RemoveExtension(strMediaFile);
                CFileItem item(strMediaFile, false);
                if (item.IsAudio() && Contains(strMediaFile))
                {
                  bFoundMediaFile = true;
                }
                else
                { // try replacing the extension with one of our allowed ones.
                  std::vector<std::string> extensions = StringUtils::Split(g_advancedSettings.GetMusicExtensions(), "|");
                  for (std::vector<std::string>::const_iterator i = extensions.begin(); i != extensions.end(); ++i)
                  {
                    strMediaFile = URIUtils::ReplaceExtension(pItem->GetPath(), *i);
                    CFileItem item(strMediaFile, false);
                    if (!item.IsCUESheet() && !item.IsPlayList() && Contains(strMediaFile))
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
              cuesheet->UpdateMediaFile(fileFromCue, strMediaFile);
              // apply CUE for later processing
              for (int j = 0; j < (int)m_items.size(); j++)
              {
                CFileItemPtr pItem = m_items[j];
                if (stricmp(pItem->GetPath().c_str(), strMediaFile.c_str()) == 0)
                  pItem->SetCueDocument(cuesheet);
              }
            }
          }
        }
        itemstodelete.push_back(pItem->GetPath());
      }
    }
  }
  // now delete the .CUE files.
  for (int i = 0; i < (int)itemstodelete.size(); i++)
  {
    for (int j = 0; j < (int)m_items.size(); j++)
    {
      CFileItemPtr pItem = m_items[j];
      if (stricmp(pItem->GetPath().c_str(), itemstodelete[i].c_str()) == 0)
      { // delete this item
        m_items.erase(m_items.begin() + j);
        break;
      }
    }
  }
}

// Remove the extensions from the filenames
void CFileItemList::RemoveExtensions()
{
  CSingleLock lock(m_lock);
  for (int i = 0; i < Size(); ++i)
    m_items[i]->RemoveExtension();
}

void CFileItemList::Stack(bool stackFiles /* = true */)
{
  CSingleLock lock(m_lock);

  // not allowed here
  if (IsVirtualDirectoryRoot() ||
      IsLiveTV() ||
      IsSourcesPath() ||
      IsLibraryFolder())
    return;

  SetProperty("isstacked", true);

  // items needs to be sorted for stuff below to work properly
  Sort(SortByLabel, SortOrderAscending);

  StackFolders();

  if (stackFiles)
    StackFiles();
}

void CFileItemList::StackFolders()
{
  // Precompile our REs
  VECCREGEXP folderRegExps;
  CRegExp folderRegExp(true, CRegExp::autoUtf8);
  const std::vector<std::string>& strFolderRegExps = g_advancedSettings.m_folderStackRegExps;

  std::vector<std::string>::const_iterator strExpression = strFolderRegExps.begin();
  while (strExpression != strFolderRegExps.end())
  {
    if (!folderRegExp.RegComp(*strExpression))
      CLog::Log(LOGERROR, "%s: Invalid folder stack RegExp:'%s'", __FUNCTION__, strExpression->c_str());
    else
      folderRegExps.push_back(folderRegExp);

    strExpression++;
  }

  if (!folderRegExp.IsCompiled())
  {
    CLog::Log(LOGDEBUG, "%s: No stack expressions available. Skipping folder stacking", __FUNCTION__);
    return;
  }

  // stack folders
  for (int i = 0; i < Size(); i++)
  {
    CFileItemPtr item = Get(i);
    // combined the folder checks
    if (item->m_bIsFolder)
    {
      // only check known fast sources?
      // NOTES:
      // 1. rars and zips may be on slow sources? is this supposed to be allowed?
      if( !item->IsRemote()
        || item->IsSmb()
        || item->IsNfs()
        || URIUtils::IsInRAR(item->GetPath())
        || URIUtils::IsInZIP(item->GetPath())
        || URIUtils::IsOnLAN(item->GetPath())
        )
      {
        // stack cd# folders if contains only a single video file

        bool bMatch(false);

        VECCREGEXP::iterator expr = folderRegExps.begin();
        while (!bMatch && expr != folderRegExps.end())
        {
          //CLog::Log(LOGDEBUG,"%s: Running expression %s on %s", __FUNCTION__, expr->GetPattern().c_str(), item->GetLabel().c_str());
          bMatch = (expr->RegFind(item->GetLabel().c_str()) != -1);
          if (bMatch)
          {
            CFileItemList items;
            CDirectory::GetDirectory(item->GetPath(),items,g_advancedSettings.m_videoExtensions);
            // optimized to only traverse listing once by checking for filecount
            // and recording last file item for later use
            int nFiles = 0;
            int index = -1;
            for (int j = 0; j < items.Size(); j++)
            {
              if (!items[j]->m_bIsFolder)
              {
                nFiles++;
                index = j;
              }

              if (nFiles > 1)
                break;
            }

            if (nFiles == 1)
              *item = *items[index];
          }
          expr++;
        }

        // check for dvd folders
        if (!bMatch)
        {
          std::string dvdPath = item->GetOpticalMediaPath();

          if (!dvdPath.empty())
          {
            // NOTE: should this be done for the CD# folders too?
            item->m_bIsFolder = false;
            item->SetPath(dvdPath);
            item->SetLabel2("");
            item->SetLabelPreformated(true);
            m_sortDescription.sortBy = SortByNone; /* sorting is now broken */
          }
        }
      }
    }
  }
}

void CFileItemList::StackFiles()
{
  // Precompile our REs
  VECCREGEXP stackRegExps;
  CRegExp tmpRegExp(true, CRegExp::autoUtf8);
  const std::vector<std::string>& strStackRegExps = g_advancedSettings.m_videoStackRegExps;
  std::vector<std::string>::const_iterator strRegExp = strStackRegExps.begin();
  while (strRegExp != strStackRegExps.end())
  {
    if (tmpRegExp.RegComp(*strRegExp))
    {
      if (tmpRegExp.GetCaptureTotal() == 4)
        stackRegExps.push_back(tmpRegExp);
      else
        CLog::Log(LOGERROR, "Invalid video stack RE (%s). Must have 4 captures.", strRegExp->c_str());
    }
    strRegExp++;
  }

  // now stack the files, some of which may be from the previous stack iteration
  int i = 0;
  while (i < Size())
  {
    CFileItemPtr item1 = Get(i);

    // skip folders, nfo files, playlists
    if (item1->m_bIsFolder
      || item1->IsParentFolder()
      || item1->IsNFO()
      || item1->IsPlayList()
      )
    {
      // increment index
      i++;
      continue;
    }

    int64_t               size        = 0;
    size_t                offset      = 0;
    std::string           stackName;
    std::string           file1;
    std::string           filePath;
    std::vector<int>      stack;
    VECCREGEXP::iterator  expr        = stackRegExps.begin();

    URIUtils::Split(item1->GetPath(), filePath, file1);
    if (URIUtils::HasEncodedFilename(CURL(filePath)))
      file1 = CURL::Decode(file1);

    int j;
    while (expr != stackRegExps.end())
    {
      if (expr->RegFind(file1, offset) != -1)
      {
        std::string Title1      = expr->GetMatch(1),
                    Volume1     = expr->GetMatch(2),
                    Ignore1     = expr->GetMatch(3),
                    Extension1  = expr->GetMatch(4);
        if (offset)
          Title1 = file1.substr(0, expr->GetSubStart(2));
        j = i + 1;
        while (j < Size())
        {
          CFileItemPtr item2 = Get(j);

          // skip folders, nfo files, playlists
          if (item2->m_bIsFolder
            || item2->IsParentFolder()
            || item2->IsNFO()
            || item2->IsPlayList()
            )
          {
            // increment index
            j++;
            continue;
          }

          std::string file2, filePath2;
          URIUtils::Split(item2->GetPath(), filePath2, file2);
          if (URIUtils::HasEncodedFilename(CURL(filePath2)) )
            file2 = CURL::Decode(file2);

          if (expr->RegFind(file2, offset) != -1)
          {
            std::string  Title2      = expr->GetMatch(1),
                        Volume2     = expr->GetMatch(2),
                        Ignore2     = expr->GetMatch(3),
                        Extension2  = expr->GetMatch(4);
            if (offset)
              Title2 = file2.substr(0, expr->GetSubStart(2));
            if (StringUtils::EqualsNoCase(Title1, Title2))
            {
              if (!StringUtils::EqualsNoCase(Volume1, Volume2))
              {
                if (StringUtils::EqualsNoCase(Ignore1, Ignore2) &&
                    StringUtils::EqualsNoCase(Extension1, Extension2))
                {
                  if (stack.empty())
                  {
                    stackName = Title1 + Ignore1 + Extension1;
                    stack.push_back(i);
                    size += item1->m_dwSize;
                  }
                  stack.push_back(j);
                  size += item2->m_dwSize;
                }
                else // Sequel
                {
                  offset = 0;
                  expr++;
                  break;
                }
              }
              else if (!StringUtils::EqualsNoCase(Ignore1, Ignore2)) // False positive, try again with offset
              {
                offset = expr->GetSubStart(3);
                break;
              }
              else // Extension mismatch
              {
                offset = 0;
                expr++;
                break;
              }
            }
            else // Title mismatch
            {
              offset = 0;
              expr++;
              break;
            }
          }
          else // No match 2, next expression
          {
            offset = 0;
            expr++;
            break;
          }
          j++;
        }
        if (j == Size())
          expr = stackRegExps.end();
      }
      else // No match 1
      {
        offset = 0;
        expr++;
      }
      if (stack.size() > 1)
      {
        // have a stack, remove the items and add the stacked item
        // dont actually stack a multipart rar set, just remove all items but the first
        std::string stackPath;
        if (Get(stack[0])->IsRAR())
          stackPath = Get(stack[0])->GetPath();
        else
        {
          CStackDirectory dir;
          stackPath = dir.ConstructStackPath(*this, stack);
        }
        item1->SetPath(stackPath);
        // clean up list
        for (unsigned k = 1; k < stack.size(); k++)
          Remove(i+1);
        // item->m_bIsFolder = true;  // don't treat stacked files as folders
        // the label may be in a different char set from the filename (eg over smb
        // the label is converted from utf8, but the filename is not)
        if (!CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS))
          URIUtils::RemoveExtension(stackName);

        item1->SetLabel(stackName);
        item1->m_dwSize = size;
        break;
      }
    }
    i++;
  }
}

bool CFileItemList::Load(int windowID)
{
  CFile file;
  auto path = GetDiscFileCache(windowID);
  try
  {
    if (file.Open(path))
    {
      CArchive ar(&file, CArchive::load);
      ar >> *this;
      CLog::Log(LOGDEBUG,"Loading items: %i, directory: %s sort method: %i, ascending: %s", Size(), CURL::GetRedacted(GetPath()).c_str(), m_sortDescription.sortBy,
        m_sortDescription.sortOrder == SortOrderAscending ? "true" : "false");
      ar.Close();
      file.Close();
      return true;
    }
  }
  catch(std::out_of_range ex)
  {
    CLog::Log(LOGERROR, "Corrupt archive: %s", CURL::GetRedacted(path).c_str());
  }

  return false;
}

bool CFileItemList::Save(int windowID)
{
  int iSize = Size();
  if (iSize <= 0)
    return false;

  CLog::Log(LOGDEBUG,"Saving fileitems [%s]", CURL::GetRedacted(GetPath()).c_str());

  CFile file;
  if (file.OpenForWrite(GetDiscFileCache(windowID), true)) // overwrite always
  {
    CArchive ar(&file, CArchive::store);
    ar << *this;
    CLog::Log(LOGDEBUG,"  -- items: %i, sort method: %i, ascending: %s", iSize, m_sortDescription.sortBy, m_sortDescription.sortOrder == SortOrderAscending ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

void CFileItemList::RemoveDiscCache(int windowID) const
{
  std::string cacheFile(GetDiscFileCache(windowID));
  if (CFile::Exists(cacheFile))
  {
    CLog::Log(LOGDEBUG,"Clearing cached fileitems [%s]", CURL::GetRedacted(GetPath()).c_str());
    CFile::Delete(cacheFile);
  }
}

std::string CFileItemList::GetDiscFileCache(int windowID) const
{
  std::string strPath(GetPath());
  URIUtils::RemoveSlashAtEnd(strPath);

  uint32_t crc = Crc32::ComputeFromLowerCase(strPath);

  std::string cacheFile;
  if (IsCDDA() || IsOnDVD())
    return StringUtils::Format("special://temp/archive_cache/r-%08x.fi", crc);

  if (IsMusicDb())
    return StringUtils::Format("special://temp/archive_cache/mdb-%08x.fi", crc);

  if (IsVideoDb())
    return StringUtils::Format("special://temp/archive_cache/vdb-%08x.fi", crc);

  if (IsSmartPlayList())
    return StringUtils::Format("special://temp/archive_cache/sp-%08x.fi", crc);

  if (windowID)
    return StringUtils::Format("special://temp/archive_cache/%i-%08x.fi", windowID, crc);

  return StringUtils::Format("special://temp/archive_cache/%08x.fi", crc);
}

bool CFileItemList::AlwaysCache() const
{
  // some database folders are always cached
  if (IsMusicDb())
    return CMusicDatabaseDirectory::CanCache(GetPath());
  if (IsVideoDb())
    return CVideoDatabaseDirectory::CanCache(GetPath());
  if (IsEPG())
    return true; // always cache
  return false;
}

std::string CFileItem::GetUserMusicThumb(bool alwaysCheckRemote /* = false */, bool fallbackToFolder /* = false */) const
{
  if (m_strPath.empty()
   || StringUtils::StartsWithNoCase(m_strPath, "newsmartplaylist://")
   || StringUtils::StartsWithNoCase(m_strPath, "newplaylist://")
   || m_bIsShareOrDrive
   || IsInternetStream()
   || URIUtils::IsUPnP(m_strPath)
   || (URIUtils::IsFTP(m_strPath) && !g_advancedSettings.m_bFTPThumbs)
   || IsPlugin()
   || IsAddonsPath()
   || IsLibraryFolder()
   || IsParentFolder()
   || IsMusicDb())
    return "";

  // we first check for <filename>.tbn or <foldername>.tbn
  std::string fileThumb(GetTBNFile());
  if (CFile::Exists(fileThumb))
    return fileThumb;

  // Fall back to folder thumb, if requested
  if (!m_bIsFolder && fallbackToFolder)
  {
    CFileItem item(URIUtils::GetDirectory(m_strPath), true);
    return item.GetUserMusicThumb(alwaysCheckRemote);
  }

  // if a folder, check for folder.jpg
  if (m_bIsFolder && !IsFileFolder() && (!IsRemote() || alwaysCheckRemote || CSettings::GetInstance().GetBool(CSettings::SETTING_MUSICFILES_FINDREMOTETHUMBS)))
  {
    std::vector<std::string> thumbs = StringUtils::Split(g_advancedSettings.m_musicThumbs, "|");
    for (std::vector<std::string>::const_iterator i = thumbs.begin(); i != thumbs.end(); ++i)
    {
      std::string folderThumb(GetFolderThumb(*i));
      if (CFile::Exists(folderThumb))
      {
        return folderThumb;
      }
    }
  }
  // No thumb found
  return "";
}

// Gets the .tbn filename from a file or folder name.
// <filename>.ext -> <filename>.tbn
// <foldername>/ -> <foldername>.tbn
std::string CFileItem::GetTBNFile() const
{
  std::string thumbFile;
  std::string strFile = m_strPath;

  if (IsStack())
  {
    std::string strPath, strReturn;
    URIUtils::GetParentPath(m_strPath,strPath);
    CFileItem item(CStackDirectory::GetFirstStackedFile(strFile),false);
    std::string strTBNFile = item.GetTBNFile();
    strReturn = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(strTBNFile));
    if (CFile::Exists(strReturn))
      return strReturn;

    strFile = URIUtils::AddFileToFolder(strPath,URIUtils::GetFileName(CStackDirectory::GetStackedTitlePath(strFile)));
  }

  if (URIUtils::IsInRAR(strFile) || URIUtils::IsInZIP(strFile))
  {
    std::string strPath = URIUtils::GetDirectory(strFile);
    std::string strParent;
    URIUtils::GetParentPath(strPath,strParent);
    strFile = URIUtils::AddFileToFolder(strParent, URIUtils::GetFileName(m_strPath));
  }

  CURL url(strFile);
  strFile = url.GetFileName();

  if (m_bIsFolder && !IsFileFolder())
    URIUtils::RemoveSlashAtEnd(strFile);

  if (!strFile.empty())
  {
    if (m_bIsFolder && !IsFileFolder())
      thumbFile = strFile + ".tbn"; // folder, so just add ".tbn"
    else
      thumbFile = URIUtils::ReplaceExtension(strFile, ".tbn");
    url.SetFileName(thumbFile);
    thumbFile = url.Get();
  }
  return thumbFile;
}

bool CFileItem::SkipLocalArt() const
{
  return (m_strPath.empty()
       || StringUtils::StartsWithNoCase(m_strPath, "newsmartplaylist://")
       || StringUtils::StartsWithNoCase(m_strPath, "newplaylist://")
       || m_bIsShareOrDrive
       || IsInternetStream()
       || URIUtils::IsUPnP(m_strPath)
       || (URIUtils::IsFTP(m_strPath) && !g_advancedSettings.m_bFTPThumbs)
       || IsPlugin()
       || IsAddonsPath()
       || IsLibraryFolder()
       || IsParentFolder()
       || IsLiveTV()
       || IsDVD());
}

std::string CFileItem::FindLocalArt(const std::string &artFile, bool useFolder) const
{
  if (SkipLocalArt())
    return "";

  std::string thumb;
  if (!m_bIsFolder)
  {
    thumb = GetLocalArt(artFile, false);
    if (!thumb.empty() && CFile::Exists(thumb))
      return thumb;
  }
  if ((useFolder || (m_bIsFolder && !IsFileFolder())) && !artFile.empty())
  {
    std::string thumb2 = GetLocalArt(artFile, true);
    if (!thumb2.empty() && thumb2 != thumb && CFile::Exists(thumb2))
      return thumb2;
  }
  return "";
}

std::string CFileItem::GetLocalArt(const std::string &artFile, bool useFolder) const
{
  // no retrieving of empty art files from folders
  if (useFolder && artFile.empty())
    return "";

  std::string strFile = m_strPath;
  if (IsStack())
  {
/*    CFileItem item(CStackDirectory::GetFirstStackedFile(strFile),false);
    std::string localArt = item.GetLocalArt(artFile);
    return localArt;
    */
    std::string strPath;
    URIUtils::GetParentPath(m_strPath,strPath);
    strFile = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(CStackDirectory::GetStackedTitlePath(strFile)));
  }

  if (URIUtils::IsInRAR(strFile) || URIUtils::IsInZIP(strFile))
  {
    std::string strPath = URIUtils::GetDirectory(strFile);
    std::string strParent;
    URIUtils::GetParentPath(strPath,strParent);
    strFile = URIUtils::AddFileToFolder(strParent, URIUtils::GetFileName(strFile));
  }

  if (IsMultiPath())
    strFile = CMultiPathDirectory::GetFirstPath(m_strPath);

  if (IsOpticalMediaFile())
  { // optical media files should be treated like folders
    useFolder = true;
    strFile = GetLocalMetadataPath();
  }
  else if (useFolder && !(m_bIsFolder && !IsFileFolder()))
    strFile = URIUtils::GetDirectory(strFile);

  if (strFile.empty()) // empty filepath -> nothing to find
    return "";

  if (useFolder)
  {
    if (!artFile.empty())
      return URIUtils::AddFileToFolder(strFile, artFile);
  }
  else
  {
    if (artFile.empty()) // old thumbnail matching
      return URIUtils::ReplaceExtension(strFile, ".tbn");
    else
      return URIUtils::ReplaceExtension(strFile, "-" + artFile);
  }
  return "";
}

std::string CFileItem::GetFolderThumb(const std::string &folderJPG /* = "folder.jpg" */) const
{
  std::string strFolder = m_strPath;

  if (IsStack() ||
      URIUtils::IsInRAR(strFolder) ||
      URIUtils::IsInZIP(strFolder))
  {
    URIUtils::GetParentPath(m_strPath,strFolder);
  }

  if (IsMultiPath())
    strFolder = CMultiPathDirectory::GetFirstPath(m_strPath);

  return URIUtils::AddFileToFolder(strFolder, folderJPG);
}

std::string CFileItem::GetMovieName(bool bUseFolderNames /* = false */) const
{
  if (IsLabelPreformated())
    return GetLabel();

  if (m_pvrRecordingInfoTag)
    return m_pvrRecordingInfoTag->m_strTitle;
  else if (CUtil::IsTVRecording(m_strPath))
  {
    std::string title = CPVRRecording::GetTitleFromURL(m_strPath);
    if (!title.empty())
      return title;
  }

  std::string strMovieName = GetBaseMoviePath(bUseFolderNames);

  if (URIUtils::IsStack(strMovieName))
    strMovieName = CStackDirectory::GetStackedTitlePath(strMovieName);

  URIUtils::RemoveSlashAtEnd(strMovieName);

  return CURL::Decode(URIUtils::GetFileName(strMovieName));
}

std::string CFileItem::GetBaseMoviePath(bool bUseFolderNames) const
{
  std::string strMovieName = m_strPath;

  if (IsMultiPath())
    strMovieName = CMultiPathDirectory::GetFirstPath(m_strPath);

  if (IsOpticalMediaFile())
    return GetLocalMetadataPath();

  if (bUseFolderNames &&
     (!m_bIsFolder || URIUtils::IsInArchive(m_strPath) ||
     (HasVideoInfoTag() && GetVideoInfoTag()->m_iDbId > 0 && !CMediaTypes::IsContainer(GetVideoInfoTag()->m_type))))
  {
    std::string name2(strMovieName);
    URIUtils::GetParentPath(name2,strMovieName);
    if (URIUtils::IsInArchive(m_strPath))
    {
      std::string strArchivePath;
      URIUtils::GetParentPath(strMovieName, strArchivePath);
      strMovieName = strArchivePath;
    }
  }

  return strMovieName;
}

std::string CFileItem::GetLocalFanart() const
{
  if (IsVideoDb())
  {
    if (!HasVideoInfoTag())
      return ""; // nothing can be done
    CFileItem dbItem(m_bIsFolder ? GetVideoInfoTag()->m_strPath : GetVideoInfoTag()->m_strFileNameAndPath, m_bIsFolder);
    return dbItem.GetLocalFanart();
  }

  std::string strFile2;
  std::string strFile = m_strPath;
  if (IsStack())
  {
    std::string strPath;
    URIUtils::GetParentPath(m_strPath,strPath);
    CStackDirectory dir;
    std::string strPath2;
    strPath2 = dir.GetStackedTitlePath(strFile);
    strFile = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(strPath2));
    CFileItem item(dir.GetFirstStackedFile(m_strPath),false);
    std::string strTBNFile(URIUtils::ReplaceExtension(item.GetTBNFile(), "-fanart"));
    strFile2 = URIUtils::AddFileToFolder(strPath, URIUtils::GetFileName(strTBNFile));
  }
  if (URIUtils::IsInRAR(strFile) || URIUtils::IsInZIP(strFile))
  {
    std::string strPath = URIUtils::GetDirectory(strFile);
    std::string strParent;
    URIUtils::GetParentPath(strPath,strParent);
    strFile = URIUtils::AddFileToFolder(strParent, URIUtils::GetFileName(m_strPath));
  }

  // no local fanart available for these
  if (IsInternetStream()
   || URIUtils::IsUPnP(strFile)
   || URIUtils::IsBluray(strFile)
   || IsLiveTV()
   || IsPlugin()
   || IsAddonsPath()
   || IsDVD()
   || (URIUtils::IsFTP(strFile) && !g_advancedSettings.m_bFTPThumbs)
   || m_strPath.empty())
    return "";

  std::string strDir = URIUtils::GetDirectory(strFile);

  if (strDir.empty())
    return "";

  CFileItemList items;
  CDirectory::GetDirectory(strDir, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  if (IsOpticalMediaFile())
  { // grab from the optical media parent folder as well
    CFileItemList moreItems;
    CDirectory::GetDirectory(GetLocalMetadataPath(), moreItems, g_advancedSettings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
    items.Append(moreItems);
  }

  std::vector<std::string> fanarts = StringUtils::Split(g_advancedSettings.m_fanartImages, "|");

  strFile = URIUtils::ReplaceExtension(strFile, "-fanart");
  fanarts.insert(m_bIsFolder ? fanarts.end() : fanarts.begin(), URIUtils::GetFileName(strFile));

  if (!strFile2.empty())
    fanarts.insert(m_bIsFolder ? fanarts.end() : fanarts.begin(), URIUtils::GetFileName(strFile2));

  for (std::vector<std::string>::const_iterator i = fanarts.begin(); i != fanarts.end(); ++i)
  {
    for (int j = 0; j < items.Size(); j++)
    {
      std::string strCandidate = URIUtils::GetFileName(items[j]->m_strPath);
      URIUtils::RemoveExtension(strCandidate);
      std::string strFanart = *i;
      URIUtils::RemoveExtension(strFanart);
      if (StringUtils::EqualsNoCase(strCandidate, strFanart))
        return items[j]->m_strPath;
    }
  }

  return "";
}

std::string CFileItem::GetLocalMetadataPath() const
{
  if (m_bIsFolder && !IsFileFolder())
    return m_strPath;

  std::string parent(URIUtils::GetParentPath(m_strPath));
  std::string parentFolder(parent);
  URIUtils::RemoveSlashAtEnd(parentFolder);
  parentFolder = URIUtils::GetFileName(parentFolder);
  if (StringUtils::EqualsNoCase(parentFolder, "VIDEO_TS") || StringUtils::EqualsNoCase(parentFolder, "BDMV"))
  { // go back up another one
    parent = URIUtils::GetParentPath(parent);
  }
  return parent;
}

bool CFileItem::LoadMusicTag()
{
  // not audio
  if (!IsAudio())
    return false;
  // already loaded?
  if (HasMusicInfoTag() && m_musicInfoTag->Loaded())
    return true;
  // check db
  CMusicDatabase musicDatabase;
  if (musicDatabase.Open())
  {
    CSong song;
    if (musicDatabase.GetSongByFileName(m_strPath, song))
    {
      GetMusicInfoTag()->SetSong(song);
      SetArt("thumb", song.strThumb);
      return true;
    }
    musicDatabase.Close();
  }
  // load tag from file
  CLog::Log(LOGDEBUG, "%s: loading tag information for file: %s", __FUNCTION__, m_strPath.c_str());
  CMusicInfoTagLoaderFactory factory;
  std::unique_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(*this));
  if (pLoader.get() != NULL)
  {
    if (pLoader->Load(m_strPath, *GetMusicInfoTag()))
      return true;
  }
  // no tag - try some other things
  if (IsCDDA())
  {
    // we have the tracknumber...
    int iTrack = GetMusicInfoTag()->GetTrackNumber();
    if (iTrack >= 1)
    {
      std::string strText = g_localizeStrings.Get(554); // "Track"
      if (!strText.empty() && strText[strText.size() - 1] != ' ')
        strText += " ";
      std::string strTrack = StringUtils::Format((strText + "%i").c_str(), iTrack);
      GetMusicInfoTag()->SetTitle(strTrack);
      GetMusicInfoTag()->SetLoaded(true);
      return true;
    }
  }
  else
  {
    std::string fileName = URIUtils::GetFileName(m_strPath);
    URIUtils::RemoveExtension(fileName);
    for (unsigned int i = 0; i < g_advancedSettings.m_musicTagsFromFileFilters.size(); i++)
    {
      CLabelFormatter formatter(g_advancedSettings.m_musicTagsFromFileFilters[i], "");
      if (formatter.FillMusicTag(fileName, GetMusicInfoTag()))
      {
        GetMusicInfoTag()->SetLoaded(true);
        return true;
      }
    }
  }
  return false;
}

void CFileItemList::Swap(unsigned int item1, unsigned int item2)
{
  if (item1 != item2 && item1 < m_items.size() && item2 < m_items.size())
    std::swap(m_items[item1], m_items[item2]);
}

bool CFileItemList::UpdateItem(const CFileItem *item)
{
  if (!item)
    return false;

  CSingleLock lock(m_lock);
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsSamePath(item))
    {
      pItem->UpdateInfo(*item);
      return true;
    }
  }
  return false;
}

void CFileItemList::AddSortMethod(SortBy sortBy, int buttonLabel, const LABEL_MASKS &labelMasks, SortAttribute sortAttributes /* = SortAttributeNone */)
{
  AddSortMethod(sortBy, sortAttributes, buttonLabel, labelMasks);
}

void CFileItemList::AddSortMethod(SortBy sortBy, SortAttribute sortAttributes, int buttonLabel, const LABEL_MASKS &labelMasks)
{
  SortDescription sorting;
  sorting.sortBy = sortBy;
  sorting.sortAttributes = sortAttributes;

  AddSortMethod(sorting, buttonLabel, labelMasks);
}

void CFileItemList::AddSortMethod(SortDescription sortDescription, int buttonLabel, const LABEL_MASKS &labelMasks)
{
  GUIViewSortDetails sort;
  sort.m_sortDescription = sortDescription;
  sort.m_buttonLabel = buttonLabel;
  sort.m_labelMasks = labelMasks;

  m_sortDetails.push_back(sort);
}

void CFileItemList::SetReplaceListing(bool replace)
{
  m_replaceListing = replace;
}

void CFileItemList::ClearSortState()
{
  m_sortDescription.sortBy = SortByNone;
  m_sortDescription.sortOrder = SortOrderNone;
  m_sortDescription.sortAttributes = SortAttributeNone;
}

CVideoInfoTag* CFileItem::GetVideoInfoTag()
{
  if (!m_videoInfoTag)
    m_videoInfoTag = new CVideoInfoTag;

  return m_videoInfoTag;
}

CPictureInfoTag* CFileItem::GetPictureInfoTag()
{
  if (!m_pictureInfoTag)
    m_pictureInfoTag = new CPictureInfoTag;

  return m_pictureInfoTag;
}

MUSIC_INFO::CMusicInfoTag* CFileItem::GetMusicInfoTag()
{
  if (!m_musicInfoTag)
    m_musicInfoTag = new MUSIC_INFO::CMusicInfoTag;

  return m_musicInfoTag;
}

std::string CFileItem::FindTrailer() const
{
  std::string strFile2;
  std::string strFile = m_strPath;
  if (IsStack())
  {
    std::string strPath;
    URIUtils::GetParentPath(m_strPath,strPath);
    CStackDirectory dir;
    std::string strPath2;
    strPath2 = dir.GetStackedTitlePath(strFile);
    strFile = URIUtils::AddFileToFolder(strPath,URIUtils::GetFileName(strPath2));
    CFileItem item(dir.GetFirstStackedFile(m_strPath),false);
    std::string strTBNFile(URIUtils::ReplaceExtension(item.GetTBNFile(), "-trailer"));
    strFile2 = URIUtils::AddFileToFolder(strPath,URIUtils::GetFileName(strTBNFile));
  }
  if (URIUtils::IsInRAR(strFile) || URIUtils::IsInZIP(strFile))
  {
    std::string strPath = URIUtils::GetDirectory(strFile);
    std::string strParent;
    URIUtils::GetParentPath(strPath,strParent);
    strFile = URIUtils::AddFileToFolder(strParent,URIUtils::GetFileName(m_strPath));
  }

  // no local trailer available for these
  if (IsInternetStream()
   || URIUtils::IsUPnP(strFile)
   || URIUtils::IsBluray(strFile)
   || IsLiveTV()
   || IsPlugin()
   || IsDVD())
    return "";

  std::string strDir = URIUtils::GetDirectory(strFile);
  CFileItemList items;
  CDirectory::GetDirectory(strDir, items, g_advancedSettings.m_videoExtensions, DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO | DIR_FLAG_NO_FILE_DIRS);
  URIUtils::RemoveExtension(strFile);
  strFile += "-trailer";
  std::string strFile3 = URIUtils::AddFileToFolder(strDir, "movie-trailer");

  // Precompile our REs
  VECCREGEXP matchRegExps;
  CRegExp tmpRegExp(true, CRegExp::autoUtf8);
  const std::vector<std::string>& strMatchRegExps = g_advancedSettings.m_trailerMatchRegExps;

  std::vector<std::string>::const_iterator strRegExp = strMatchRegExps.begin();
  while (strRegExp != strMatchRegExps.end())
  {
    if (tmpRegExp.RegComp(*strRegExp))
    {
      matchRegExps.push_back(tmpRegExp);
    }
    strRegExp++;
  }

  std::string strTrailer;
  for (int i = 0; i < items.Size(); i++)
  {
    std::string strCandidate = items[i]->m_strPath;
    URIUtils::RemoveExtension(strCandidate);
    if (StringUtils::EqualsNoCase(strCandidate, strFile) ||
        StringUtils::EqualsNoCase(strCandidate, strFile2) ||
        StringUtils::EqualsNoCase(strCandidate, strFile3))
    {
      strTrailer = items[i]->m_strPath;
      break;
    }
    else
    {
      VECCREGEXP::iterator expr = matchRegExps.begin();

      while (expr != matchRegExps.end())
      {
        if (expr->RegFind(strCandidate) != -1)
        {
          strTrailer = items[i]->m_strPath;
          i = items.Size();
          break;
        }
        expr++;
      }
    }
  }

  return strTrailer;
}

int CFileItem::GetVideoContentType() const
{
  VIDEODB_CONTENT_TYPE type = VIDEODB_CONTENT_MOVIES;
  if (HasVideoInfoTag() && GetVideoInfoTag()->m_type == MediaTypeTvShow)
    type = VIDEODB_CONTENT_TVSHOWS;
  if (HasVideoInfoTag() && GetVideoInfoTag()->m_type == MediaTypeEpisode)
    return VIDEODB_CONTENT_EPISODES;
  if (HasVideoInfoTag() && GetVideoInfoTag()->m_type == MediaTypeMusicVideo)
    return VIDEODB_CONTENT_MUSICVIDEOS;

  CVideoDatabaseDirectory dir;
  VIDEODATABASEDIRECTORY::CQueryParams params;
  dir.GetQueryParams(m_strPath, params);
  if (params.GetSetId() != -1 && params.GetMovieId() == -1) // movie set
    return VIDEODB_CONTENT_MOVIE_SETS;

  return type;
}

bool CFileItem::IsResumePointSet() const
{
  return (HasVideoInfoTag() && GetVideoInfoTag()->m_resumePoint.IsSet()) ||
      (m_pvrRecordingInfoTag && m_pvrRecordingInfoTag->GetLastPlayedPosition() > 0);
}

double CFileItem::GetCurrentResumeTime() const
{
  if (m_pvrRecordingInfoTag)
  {
    // This will retrieve 'fresh' resume information from the PVR server
    int rc = m_pvrRecordingInfoTag->GetLastPlayedPosition();
    if (rc > 0)
      return rc;
    // Fall through to default value
  }
  if (HasVideoInfoTag() && GetVideoInfoTag()->m_resumePoint.IsSet())
  {
    return GetVideoInfoTag()->m_resumePoint.timeInSeconds;
  }
  // Resume from start when resume points are invalid or the PVR server returns an error
  return 0;
}
