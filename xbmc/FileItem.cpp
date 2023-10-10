/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"

#include "CueDocument.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "events/IEvent.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "games/GameUtils.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/LocalizeStrings.h"
#include "media/MediaLockState.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "pictures/PictureInfoTag.h"
#include "playlists/PlayListFactory.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/epg/EpgSearchFilter.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/Archive.h"
#include "utils/Crc32.h"
#include "utils/FileExtensionProvider.h"
#include "utils/Mime.h"
#include "utils/Random.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"

#include <algorithm>
#include <cstdlib>
#include <mutex>

using namespace KODI;
using namespace XFILE;
using namespace PLAYLIST;
using namespace MUSIC_INFO;
using namespace PVR;
using namespace GAME;

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

namespace
{
  std::string GetEpgTagTitle(const std::shared_ptr<CPVREpgInfoTag>& epgTag)
  {
    if (CServiceBroker::GetPVRManager().IsParentalLocked(epgTag))
      return g_localizeStrings.Get(19266); // Parental locked
    else if (epgTag->Title().empty() &&
             !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_EPG_HIDENOINFOAVAILABLE))
      return g_localizeStrings.Get(19055); // no information available
    else
      return epgTag->Title();
  }
} // unnamed namespace

void CFileItem::FillMusicInfoTag(const std::shared_ptr<CPVREpgInfoTag>& tag)
{
  CMusicInfoTag* musictag = GetMusicInfoTag(); // create (!) the music tag.

  if (tag)
  {
    musictag->SetTitle(GetEpgTagTitle(tag));
    musictag->SetGenre(tag->Genre());
    musictag->SetDuration(tag->GetDuration());
    musictag->SetURL(tag->Path());
  }
  else if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
               CSettings::SETTING_EPG_HIDENOINFOAVAILABLE))
  {
    musictag->SetTitle(g_localizeStrings.Get(19055)); // no information available
  }

  musictag->SetLoaded(true);
}

CFileItem::CFileItem(const std::shared_ptr<CPVREpgInfoTag>& tag)
{
  Initialize();

  m_bIsFolder = false;
  m_epgInfoTag = tag;
  m_strPath = tag->Path();
  m_bCanQueue = false;
  SetLabel(GetEpgTagTitle(tag));
  m_dateTime = tag->StartAsLocalTime();

  if (!tag->IconPath().empty())
  {
    SetArt("icon", tag->IconPath());
  }
  else
  {
    const std::string iconPath = tag->ChannelIconPath();
    if (!iconPath.empty())
      SetArt("icon", iconPath);
    else if (tag->IsRadio())
      SetArt("icon", "DefaultMusicSongs.png");
    else
      SetArt("icon", "DefaultTVShows.png");
  }

  // Speedup FillInDefaultIcon()
  SetProperty("icon_never_overlay", true);

  if (tag->IsRadio() && !HasMusicInfoTag())
    FillMusicInfoTag(tag);

  FillInMimeType(false);
}

CFileItem::CFileItem(const std::shared_ptr<PVR::CPVREpgSearchFilter>& filter)
{
  Initialize();

  m_bIsFolder = true;
  m_epgSearchFilter = filter;
  m_strPath = filter->GetPath();
  m_bCanQueue = false;
  SetLabel(filter->GetTitle());

  const CDateTime lastExec = filter->GetLastExecutedDateTime();
  if (lastExec.IsValid())
    m_dateTime.SetFromUTCDateTime(lastExec);

  SetArt("icon", "DefaultPVRSearch.png");

  // Speedup FillInDefaultIcon()
  SetProperty("icon_never_overlay", true);

  FillInMimeType(false);
}

CFileItem::CFileItem(const std::shared_ptr<CPVRChannelGroupMember>& channelGroupMember)
{
  Initialize();

  const std::shared_ptr<CPVRChannel> channel = channelGroupMember->Channel();

  m_pvrChannelGroupMemberInfoTag = channelGroupMember;

  m_strPath = channelGroupMember->Path();
  m_bIsFolder = false;
  m_bCanQueue = false;
  SetLabel(channel->ChannelName());

  if (!channel->IconPath().empty())
    SetArt("icon", channel->IconPath());
  else if (channel->IsRadio())
    SetArt("icon", "DefaultMusicSongs.png");
  else
    SetArt("icon", "DefaultTVShows.png");

  SetProperty("channelid", channel->ChannelID());
  SetProperty("path", channelGroupMember->Path());
  SetArt("thumb", channel->IconPath());

  // Speedup FillInDefaultIcon()
  SetProperty("icon_never_overlay", true);

  if (channel->IsRadio() && !HasMusicInfoTag())
  {
    const std::shared_ptr<CPVREpgInfoTag> epgNow = channel->GetEPGNow();
    FillMusicInfoTag(epgNow);
  }
  FillInMimeType(false);
}

CFileItem::CFileItem(const std::shared_ptr<CPVRRecording>& record)
{
  Initialize();

  m_bIsFolder = false;
  m_pvrRecordingInfoTag = record;
  m_strPath = record->m_strFileNameAndPath;
  SetLabel(record->m_strTitle);
  m_dateTime = record->RecordingTimeAsLocalTime();
  m_dwSize = record->GetSizeInBytes();
  m_bCanQueue = true;

  // Set art
  if (!record->IconPath().empty())
    SetArt("icon", record->IconPath());
  else
  {
    const std::shared_ptr<CPVRChannel> channel = record->Channel();
    if (channel && !channel->IconPath().empty())
      SetArt("icon", channel->IconPath());
    else if (record->IsRadio())
      SetArt("icon", "DefaultMusicSongs.png");
    else
      SetArt("icon", "DefaultTVShows.png");
  }

  if (!record->ThumbnailPath().empty())
    SetArt("thumb", record->ThumbnailPath());

  if (!record->FanartPath().empty())
    SetArt("fanart", record->FanartPath());

  // Speedup FillInDefaultIcon()
  SetProperty("icon_never_overlay", true);

  FillInMimeType(false);
}

CFileItem::CFileItem(const std::shared_ptr<CPVRTimerInfoTag>& timer)
{
  Initialize();

  m_bIsFolder = timer->IsTimerRule();
  m_pvrTimerInfoTag = timer;
  m_strPath = timer->Path();
  SetLabel(timer->Title());
  m_dateTime = timer->StartAsLocalTime();
  m_bCanQueue = false;

  if (!timer->ChannelIcon().empty())
    SetArt("icon", timer->ChannelIcon());
  else if (timer->IsRadio())
    SetArt("icon", "DefaultMusicSongs.png");
  else
    SetArt("icon", "DefaultTVShows.png");

  // Speedup FillInDefaultIcon()
  SetProperty("icon_never_overlay", true);

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
  : CGUIListItem(item),
    m_musicInfoTag(NULL),
    m_videoInfoTag(NULL),
    m_pictureInfoTag(NULL),
    m_gameInfoTag(NULL)
{
  *this = item;
}

CFileItem::CFileItem(const CGUIListItem& item)
{
  Initialize();
  // not particularly pretty, but it gets around the issue of Initialize() defaulting
  // parameters in the CGUIListItem base class.
  *static_cast<CGUIListItem*>(this) = item;

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
    label = StringUtils::Format("{} ({})", share.strName, share.strStatus);
  SetLabel(label);
  m_iLockMode = share.m_iLockMode;
  m_strLockCode = share.m_strLockCode;
  m_iHasLock = share.m_iHasLock;
  m_iBadPwdCount = share.m_iBadPwdCount;
  m_iDriveType = share.m_iDriveType;
  SetArt("thumb", share.m_strThumbnailImage);
  SetLabelPreformatted(true);
  if (IsDVD())
    GetVideoInfoTag()->m_strFileNameAndPath = share.strDiskUniqueId; // share.strDiskUniqueId contains disc unique id
  FillInMimeType(false);
}

CFileItem::CFileItem(std::shared_ptr<const ADDON::IAddon> addonInfo) : m_addonInfo(std::move(addonInfo))
{
  Initialize();
}

CFileItem::CFileItem(const EventPtr& eventLogEntry)
{
  Initialize();

  m_eventLogEntry = eventLogEntry;
  SetLabel(eventLogEntry->GetLabel());
  m_dateTime = eventLogEntry->GetDateTime();
  if (!eventLogEntry->GetIcon().empty())
    SetArt("icon", eventLogEntry->GetIcon());
}

CFileItem::~CFileItem(void)
{
  delete m_musicInfoTag;
  delete m_videoInfoTag;
  delete m_pictureInfoTag;
  delete m_gameInfoTag;

  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  m_gameInfoTag = NULL;
}

CFileItem& CFileItem::operator=(const CFileItem& item)
{
  if (this == &item)
    return *this;

  CGUIListItem::operator=(item);
  m_bLabelPreformatted=item.m_bLabelPreformatted;
  FreeMemory();
  m_strPath = item.m_strPath;
  m_strDynPath = item.m_strDynPath;
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

  if (item.m_gameInfoTag)
  {
    if (m_gameInfoTag)
      *m_gameInfoTag = *item.m_gameInfoTag;
    else
      m_gameInfoTag = new CGameInfoTag(*item.m_gameInfoTag);
  }
  else
  {
    delete m_gameInfoTag;
    m_gameInfoTag = NULL;
  }

  m_epgInfoTag = item.m_epgInfoTag;
  m_epgSearchFilter = item.m_epgSearchFilter;
  m_pvrChannelGroupMemberInfoTag = item.m_pvrChannelGroupMemberInfoTag;
  m_pvrRecordingInfoTag = item.m_pvrRecordingInfoTag;
  m_pvrTimerInfoTag = item.m_pvrTimerInfoTag;
  m_addonInfo = item.m_addonInfo;
  m_eventLogEntry = item.m_eventLogEntry;

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
  m_gameInfoTag = NULL;
  m_bLabelPreformatted = false;
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
  m_iHasLock = LOCK_STATE_NO_LOCK;
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
  m_strDynPath.clear();
  m_dateTime.Reset();
  m_strLockCode.clear();
  m_mimetype.clear();
  delete m_musicInfoTag;
  m_musicInfoTag=NULL;
  delete m_videoInfoTag;
  m_videoInfoTag=NULL;
  m_epgInfoTag.reset();
  m_epgSearchFilter.reset();
  m_pvrChannelGroupMemberInfoTag.reset();
  m_pvrRecordingInfoTag.reset();
  m_pvrTimerInfoTag.reset();
  delete m_pictureInfoTag;
  m_pictureInfoTag=NULL;
  delete m_gameInfoTag;
  m_gameInfoTag = NULL;
  m_extrainfo.clear();
  ClearProperties();
  m_eventLogEntry.reset();

  Initialize();
  SetInvalid();
}

// do not archive dynamic path
void CFileItem::Archive(CArchive& ar)
{
  CGUIListItem::Archive(ar);

  if (ar.IsStoring())
  {
    ar << m_bIsParentFolder;
    ar << m_bLabelPreformatted;
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
    if (m_pictureInfoTag)
    {
      ar << 1;
      ar << *m_pictureInfoTag;
    }
    else
      ar << 0;
    if (m_gameInfoTag)
    {
      ar << 1;
      ar << *m_gameInfoTag;
    }
    else
      ar << 0;
  }
  else
  {
    ar >> m_bIsParentFolder;
    ar >> m_bLabelPreformatted;
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
      ar >> *GetPictureInfoTag();
    ar >> iType;
    if (iType == 1)
      ar >> *GetGameInfoTag();

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

  if (m_pictureInfoTag)
    (*m_pictureInfoTag).Serialize(value["pictureInfoTag"]);

  if (m_gameInfoTag)
    (*m_gameInfoTag).Serialize(value["gameInfoTag"]);

  if (!m_mapProperties.empty())
  {
    auto& customProperties = value["customproperties"];
    for (const auto& prop : m_mapProperties)
      customProperties[prop.first] = prop.second;
  }
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

  if (HasPVRChannelGroupMemberInfoTag())
    GetPVRChannelGroupMemberInfoTag()->ToSortable(sortable, field);

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

  if (HasGameInfoTag())
    GetGameInfoTag()->ToSortable(sortable, field);

  if (m_eventLogEntry)
    m_eventLogEntry->ToSortable(sortable, field);

  if (IsFavourite())
  {
    if (field == FieldUserPreference)
      sortable[FieldUserPreference] = GetProperty("favourite.index").asString();
  }
}

void CFileItem::ToSortable(SortItem &sortable, const Fields &fields) const
{
  Fields::const_iterator it;
  for (it = fields.begin(); it != fields.end(); ++it)
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
   || IsPlugin()
   || IsPVR())
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

  if (HasGameInfoTag())
    return false;

  if (HasMusicInfoTag())
    return false;

  if (HasPictureInfoTag())
    return false;

  // TV recordings are videos...
  if (!m_bIsFolder && URIUtils::IsPVRTVRecordingFileOrFolder(GetPath()))
    return true;

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

  //! @todo If the file is a zip file, ask the game clients if any support this
  // file before assuming it is video.

  return URIUtils::HasExtension(m_strPath, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions());
}

bool CFileItem::IsEPG() const
{
  return HasEPGInfoTag();
}

bool CFileItem::IsPVRChannel() const
{
  return HasPVRChannelInfoTag();
}

bool CFileItem::IsPVRChannelGroup() const
{
  return URIUtils::IsPVRChannelGroup(m_strPath);
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

bool CFileItem::IsInProgressPVRRecording() const
{
  return (m_pvrRecordingInfoTag && m_pvrRecordingInfoTag->IsInProgress());
}

bool CFileItem::IsPVRTimer() const
{
  return HasPVRTimerInfoTag();
}

bool CFileItem::IsDiscStub() const
{
  if (IsVideoDb() && HasVideoInfoTag())
  {
    CFileItem dbItem(m_bIsFolder ? GetVideoInfoTag()->m_strPath : GetVideoInfoTag()->m_strFileNameAndPath, m_bIsFolder);
    return dbItem.IsDiscStub();
  }

  return URIUtils::HasExtension(m_strPath, CServiceBroker::GetFileExtensionProvider().GetDiscStubExtensions());
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

  if (HasGameInfoTag())
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

  //! @todo If the file is a zip file, ask the game clients if any support this
  // file before assuming it is audio

  return URIUtils::HasExtension(m_strPath, CServiceBroker::GetFileExtensionProvider().GetMusicExtensions());
}

bool CFileItem::IsDeleted() const
{
  if (HasPVRRecordingInfoTag())
    return GetPVRRecordingInfoTag()->IsDeleted();

  return false;
}

bool CFileItem::IsAudioBook() const
{
  return IsType(".m4b") || IsType(".mka");
}

bool CFileItem::IsGame() const
{
  if (HasGameInfoTag())
    return true;

  if (HasVideoInfoTag())
    return false;

  if (HasMusicInfoTag())
    return false;

  if (HasPictureInfoTag())
    return false;

  if (IsPVR())
    return false;

  if (HasAddonInfo())
    return CGameUtils::IsStandaloneGame(std::const_pointer_cast<ADDON::IAddon>(GetAddonInfo()));

  return CGameUtils::HasGameExtension(m_strPath);
}

bool CFileItem::IsPicture() const
{
  if (StringUtils::StartsWithNoCase(m_mimetype, "image/"))
    return true;

  if (HasPictureInfoTag())
    return true;

  if (HasGameInfoTag())
    return false;

  if (HasMusicInfoTag())
    return false;

  if (HasVideoInfoTag())
    return false;

  if (HasPVRTimerInfoTag() || HasPVRChannelInfoTag() || HasPVRChannelGroupMemberInfoTag() ||
      HasPVRRecordingInfoTag() || HasEPGInfoTag() || HasEPGSearchFilter())
    return false;

  if (!m_strPath.empty())
    return CUtil::IsPicture(m_strPath);

  return false;
}

bool CFileItem::IsLyrics() const
{
  return URIUtils::HasExtension(m_strPath, ".cdg|.lrc");
}

bool CFileItem::IsSubtitle() const
{
  return URIUtils::HasExtension(m_strPath, CServiceBroker::GetFileExtensionProvider().GetSubtitleExtensions());
}

bool CFileItem::IsCUESheet() const
{
  return URIUtils::HasExtension(m_strPath, ".cue");
}

bool CFileItem::IsInternetStream(const bool bStrictCheck /* = false */) const
{
  if (HasProperty("IsHTTPDirectory"))
    return bStrictCheck;

  if (!m_strDynPath.empty())
    return URIUtils::IsInternetStream(m_strDynPath, bStrictCheck);

  return URIUtils::IsInternetStream(m_strPath, bStrictCheck);
}

bool CFileItem::IsStreamedFilesystem() const
{
  if (!m_strDynPath.empty())
    return URIUtils::IsStreamedFilesystem(m_strDynPath);

  return URIUtils::IsStreamedFilesystem(m_strPath);
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
    || (IsPlayList() && CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_playlistAsFolders)
    || IsAPK()
    || IsZIP()
    || IsRAR()
    || IsRSS()
    || IsAudioBook()
    || IsType(".ogg|.oga|.xbt")
#if defined(TARGET_ANDROID)
    || IsType(".apk")
#endif
    )
    return true;
  }

  if (CServiceBroker::IsAddonInterfaceUp() &&
      IsType(CServiceBroker::GetFileExtensionProvider().GetFileFolderExtensions().c_str()) &&
      CServiceBroker::GetFileExtensionProvider().CanOperateExtension(m_strPath))
    return true;

  if(types & EFILEFOLDER_TYPE_ONBROWSE)
  {
    if((IsPlayList() && !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_playlistAsFolders)
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
  if (!m_strDynPath.empty())
    return URIUtils::HasExtension(m_strDynPath, ext);

  return URIUtils::HasExtension(m_strPath, ext);
}

bool CFileItem::IsNFO() const
{
  return URIUtils::HasExtension(m_strPath, ".nfo");
}

bool CFileItem::IsDiscImage() const
{
  return URIUtils::IsDiscImage(GetDynPath());
}

bool CFileItem::IsOpticalMediaFile() const
{
  if (IsDVDFile(false, true))
    return true;

  return IsBDFile();
}

bool CFileItem::IsDVDFile(bool bVobs /*= true*/, bool bIfos /*= true*/) const
{
  std::string strFileName = URIUtils::GetFileName(GetDynPath());
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
  std::string strFileName = URIUtils::GetFileName(GetDynPath());
  return (StringUtils::EqualsNoCase(strFileName, "index.bdmv") || StringUtils::EqualsNoCase(strFileName, "MovieObject.bdmv")
          || StringUtils::EqualsNoCase(strFileName, "INDEX.BDM") || StringUtils::EqualsNoCase(strFileName, "MOVIEOBJ.BDM"));
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
      || StringUtils::StartsWithNoCase(m_strPath, "rsss://")
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

bool CFileItem::IsFavourite() const
{
  return URIUtils::IsFavourite(m_strPath);
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

bool CFileItem::IsBluray() const
{
  if (URIUtils::IsBluray(m_strPath))
    return true;

  CFileItem item = CFileItem(GetOpticalMediaPath(), false);

  return item.IsBDFile();
}

bool CFileItem::IsProtectedBlurayDisc() const
{
  std::string path;
  path = URIUtils::AddFileToFolder(GetPath(), "AACS", "Unit_Key_RO.inf");
  if (CFile::Exists(path))
    return true;

  return false;
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
  return URIUtils::IsPVR(m_strPath);
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

  //CLog::Log(LOGINFO, "FillInDefaultIcon({})", pItem->GetLabel());
  // find the default icon for a file or folder item
  // for files this can be the (depending on the file type)
  //   default picture for photo's
  //   default picture for songs
  //   default picture for videos
  //   default picture for shortcuts
  //   default picture for playlists
  //
  // for folders
  //   for .. folders the default picture for parent folder
  //   for other folders the defaultFolder.png

  if (GetArt("icon").empty())
  {
    if (!m_bIsFolder)
    {
      /* To reduce the average runtime of this code, this list should
       * be ordered with most frequently seen types first.  Also bear
       * in mind the complexity of the code behind the check in the
       * case of IsWhatever() returns false.
       */
      if (IsPVRChannel())
      {
        if (GetPVRChannelInfoTag()->IsRadio())
          SetArt("icon", "DefaultMusicSongs.png");
        else
          SetArt("icon", "DefaultTVShows.png");
      }
      else if ( IsLiveTV() )
      {
        // Live TV Channel
        SetArt("icon", "DefaultTVShows.png");
      }
      else if ( URIUtils::IsArchive(m_strPath) )
      { // archive
        SetArt("icon", "DefaultFile.png");
      }
      else if ( IsUsablePVRRecording() )
      {
        // PVR recording
        SetArt("icon", "DefaultVideo.png");
      }
      else if ( IsDeletedPVRRecording() )
      {
        // PVR deleted recording
        SetArt("icon", "DefaultVideoDeleted.png");
      }
      else if ( IsAudio() )
      {
        // audio
        SetArt("icon", "DefaultAudio.png");
      }
      else if ( IsVideo() )
      {
        // video
        SetArt("icon", "DefaultVideo.png");
      }
      else if (IsPVRTimer())
      {
        SetArt("icon", "DefaultVideo.png");
      }
      else if ( IsPicture() )
      {
        // picture
        SetArt("icon", "DefaultPicture.png");
      }
      else if ( IsPlayList() || IsSmartPlayList())
      {
        SetArt("icon", "DefaultPlaylist.png");
      }
      else if ( IsPythonScript() )
      {
        SetArt("icon", "DefaultScript.png");
      }
      else if (IsFavourite())
      {
        SetArt("icon", "DefaultFavourites.png");
      }
      else
      {
        // default icon for unknown file type
        SetArt("icon", "DefaultFile.png");
      }
    }
    else
    {
      if ( IsPlayList() || IsSmartPlayList())
      {
        SetArt("icon", "DefaultPlaylist.png");
      }
      else if (IsParentFolder())
      {
        SetArt("icon", "DefaultFolderBack.png");
      }
      else
      {
        SetArt("icon", "DefaultFolder.png");
      }
    }
  }
  // Set the icon overlays (if applicable)
  if (!HasOverlay() && !HasProperty("icon_never_overlay"))
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
    SetLabelPreformatted(true);
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
    if (m_bIsFolder)
      m_mimetype = "x-directory/normal";
    else if (HasPVRChannelInfoTag())
      m_mimetype = GetPVRChannelInfoTag()->MimeType();
    else if (StringUtils::StartsWithNoCase(GetDynPath(), "shout://") ||
             StringUtils::StartsWithNoCase(GetDynPath(), "http://") ||
             StringUtils::StartsWithNoCase(GetDynPath(), "https://"))
    {
      // If lookup is false, bail out early to leave mime type empty
      if (!lookup)
        return;

      CCurlFile::GetMimeType(GetDynURL(), m_mimetype);

      // try to get mime-type again but with an NSPlayer User-Agent
      // in order for server to provide correct mime-type.  Allows us
      // to properly detect an MMS stream
      if (StringUtils::StartsWithNoCase(m_mimetype, "video/x-ms-"))
        CCurlFile::GetMimeType(GetDynURL(), m_mimetype, "NSPlayer/11.00.6001.7000");

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
  if(StringUtils::StartsWithNoCase(m_mimetype, "application/vnd.ms.wms-hdr.asfv1") ||
     StringUtils::StartsWithNoCase(m_mimetype, "application/x-mms-framed"))
  {
    if (m_strDynPath.empty())
      m_strDynPath = m_strPath;

    StringUtils::Replace(m_strDynPath, "http:", "mms:");
  }
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

  if (!m_strPath.empty() && item->GetPath() == m_strPath)
  {
    if (item->HasProperty("item_start") || HasProperty("item_start"))
      return (item->GetProperty("item_start") == GetProperty("item_start"));
    return true;
  }
  if (HasMusicInfoTag() && item->HasMusicInfoTag())
  {
    if (GetMusicInfoTag()->GetDatabaseId() != -1 && item->GetMusicInfoTag()->GetDatabaseId() != -1)
      return ((GetMusicInfoTag()->GetDatabaseId() == item->GetMusicInfoTag()->GetDatabaseId()) &&
        (GetMusicInfoTag()->GetType() == item->GetMusicInfoTag()->GetType()));
  }
  if (HasVideoInfoTag() && item->HasVideoInfoTag())
  {
    if (GetVideoInfoTag()->m_iDbId != -1 && item->GetVideoInfoTag()->m_iDbId != -1)
      return ((GetVideoInfoTag()->m_iDbId == item->GetVideoInfoTag()->m_iDbId) &&
        (GetVideoInfoTag()->m_type == item->GetVideoInfoTag()->m_type));
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
    CFileItem dbItem(GetVideoInfoTag()->m_strFileNameAndPath, false);
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
    CFileItem dbItem(item->GetVideoInfoTag()->m_strFileNameAndPath, false);
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

    if (item.m_videoInfoTag)
    {
      if (m_videoInfoTag)
        *m_videoInfoTag = *item.m_videoInfoTag;
      else
        m_videoInfoTag = new CVideoInfoTag(*item.m_videoInfoTag);
    }
    else
    {
      if (m_videoInfoTag)
        delete m_videoInfoTag;

      m_videoInfoTag = new CVideoInfoTag;
    }

    m_pvrRecordingInfoTag = item.m_pvrRecordingInfoTag;

    SetOverlayImage(ICON_OVERLAY_UNWATCHED, GetVideoInfoTag()->GetPlayCount() > 0);
    SetInvalid();
  }
  if (item.HasMusicInfoTag())
  {
    *GetMusicInfoTag() = *item.GetMusicInfoTag();
    SetInvalid();
  }
  if (item.HasPictureInfoTag())
  {
    *GetPictureInfoTag() = *item.GetPictureInfoTag();
    SetInvalid();
  }
  if (item.HasGameInfoTag())
  {
    *GetGameInfoTag() = *item.GetGameInfoTag();
    SetInvalid();
  }
  SetDynPath(item.GetDynPath());
  if (replaceLabels && !item.GetLabel().empty())
    SetLabel(item.GetLabel());
  if (replaceLabels && !item.GetLabel2().empty())
    SetLabel2(item.GetLabel2());
  if (!item.GetArt().empty())
    SetArt(item.GetArt());
  AppendProperties(item);
}

void CFileItem::MergeInfo(const CFileItem& item)
{
  // TODO: Currently merge the metadata/art info is implemented for video case only
  if (item.HasVideoInfoTag())
  {
    if (item.m_videoInfoTag)
    {
      if (m_videoInfoTag)
        m_videoInfoTag->Merge(*item.m_videoInfoTag);
      else
        m_videoInfoTag = new CVideoInfoTag(*item.m_videoInfoTag);
    }

    m_pvrRecordingInfoTag = item.m_pvrRecordingInfoTag;

    SetOverlayImage(ICON_OVERLAY_UNWATCHED, GetVideoInfoTag()->GetPlayCount() > 0);
    SetInvalid();
  }
  if (item.HasMusicInfoTag())
  {
    *GetMusicInfoTag() = *item.GetMusicInfoTag();
    SetInvalid();
  }
  if (item.HasPictureInfoTag())
  {
    *GetPictureInfoTag() = *item.GetPictureInfoTag();
    SetInvalid();
  }
  if (item.HasGameInfoTag())
  {
    *GetGameInfoTag() = *item.GetGameInfoTag();
    SetInvalid();
  }
  SetDynPath(item.GetDynPath());
  if (!item.GetLabel().empty())
    SetLabel(item.GetLabel());
  if (!item.GetLabel2().empty())
    SetLabel2(item.GetLabel2());
  if (!item.GetArt().empty())
  {
    if (item.IsVideo())
      AppendArt(item.GetArt());
    else
      SetArt(item.GetArt());
  }
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

  if (m_videoInfoTag)
    *m_videoInfoTag = video;
  else
    m_videoInfoTag = new CVideoInfoTag(video);

  if (video.m_iSeason == 0)
    SetProperty("isspecial", "true");
  FillInDefaultIcon();
  FillInMimeType(false);
}

namespace
{
class CPropertySaveHelper
{
public:
  CPropertySaveHelper(CFileItem& item, const std::string& property, const std::string& value)
    : m_item(item), m_property(property), m_value(value)
  {
  }

  bool NeedsSave() const { return !m_value.empty() || m_item.HasProperty(m_property); }

  std::string GetValueToSave(const std::string& currentValue) const
  {
    std::string value;

    if (!m_value.empty())
    {
      // Overwrite whatever we have; remember what we had originally.
      if (!m_item.HasProperty(m_property))
        m_item.SetProperty(m_property, currentValue);

      value = m_value;
    }
    else if (m_item.HasProperty(m_property))
    {
      // Restore original value
      value = m_item.GetProperty(m_property).asString();
      m_item.ClearProperty(m_property);
    }

    return value;
  }

private:
  CFileItem& m_item;
  const std::string m_property;
  const std::string m_value;
};
} // unnamed namespace

void CFileItem::SetFromMusicInfoTag(const MUSIC_INFO::CMusicInfoTag& music)
{
  const std::string path = GetPath();
  if (path.empty())
  {
    SetPath(music.GetURL());
  }
  else
  {
    const CPropertySaveHelper dynpath(*this, "OriginalDynPath", music.GetURL());
    if (dynpath.NeedsSave())
      SetDynPath(dynpath.GetValueToSave(m_strDynPath));
  }

  const CPropertySaveHelper label(*this, "OriginalLabel", music.GetTitle());
  if (label.NeedsSave())
    SetLabel(label.GetValueToSave(GetLabel()));

  const CPropertySaveHelper thumb(*this, "OriginalThumb", music.GetStationArt());
  if (thumb.NeedsSave())
    SetArt("thumb", thumb.GetValueToSave(GetArt("thumb")));

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

  if (album.art.empty())
    SetArt("icon", "DefaultAlbumCover.png");
  else
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
    m_strPath = StringUtils::Format("musicdb://songs/{}{}", song.idSong, strExt);
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

  path = URIUtils::AddFileToFolder(GetPath(), "INDEX.BDM");
  if (CFile::Exists(path))
    return path;

  path = URIUtils::AddFileToFolder(GetPath(), "BDMV", "INDEX.BDM");
  if (CFile::Exists(path))
    return path;
#endif
  return std::string();
}

/**
* @todo Ideally this (and SetPath) would not be available outside of construction
* for CFileItem objects, or at least restricted to essentially be equivalent
* to construction. This would require re-formulating a bunch of CFileItem
* construction, and also allowing CFileItemList to have its own (public)
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

void CFileItem::SetDynURL(const CURL& url)
{
  m_strDynPath = url.Get();
}

const CURL CFileItem::GetDynURL() const
{
  if (!m_strDynPath.empty())
  {
    CURL url(m_strDynPath);
    return url;
  }
  else
  {
    CURL url(m_strPath);
    return url;
  }
}

const std::string &CFileItem::GetDynPath() const
{
  if (!m_strDynPath.empty())
    return m_strDynPath;
  else
    return m_strPath;
}

void CFileItem::SetDynPath(const std::string &path)
{
  m_strDynPath = path;
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
      for (std::vector<std::string>::iterator itMedia = MediaFileVec.begin();
           itMedia != MediaFileVec.end(); ++itMedia)
        cuesheet->UpdateMediaFile(*itMedia, GetPath());
      SetCueDocument(cuesheet);
    }
    // Clear cuesheet tag having added it to item
    tag.SetCueSheet("");
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

  const CMusicInfoTag& tag = *GetMusicInfoTag();

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

        if (tag.GetYear())
          song.strReleaseDate = tag.GetReleaseDate();
        if (song.embeddedArt.Empty() && !tag.GetCoverArtInfo().Empty())
          song.embeddedArt = tag.GetCoverArtInfo();
      }

      if (!song.iDuration && tag.GetDuration() > 0)
      { // must be the last song
        song.iDuration = CUtil::ConvertMilliSecsToSecsIntRounded(CUtil::ConvertSecsToMilliSecs(tag.GetDuration()) - song.iStartOffset);
      }
      if ( tag.Loaded() && oneFilePerTrack && ! ( tag.GetAlbum().empty() || tag.GetArtist().empty() || tag.GetTitle().empty() ) )
      {
        // If there are multiple files in a cue file, the tags from the files should be preferred if they exist.
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
: CFileItem("", true)
{
}

CFileItemList::CFileItemList(const std::string& strPath)
: CFileItem(strPath, true)
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
  std::unique_lock<CCriticalSection> lock(m_lock);

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
  std::unique_lock<CCriticalSection> lock(m_lock);

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
  std::unique_lock<CCriticalSection> lock(m_lock);

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
  std::unique_lock<CCriticalSection> lock(m_lock);
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
  std::unique_lock<CCriticalSection> lock(m_lock);
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(m_ignoreURLOptions ? CURL(pItem->GetPath()).GetWithoutOptions() : pItem->GetPath(), pItem));
  m_items.emplace_back(std::move(pItem));
}

void CFileItemList::Add(CFileItem&& item)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  auto ptr = std::make_shared<CFileItem>(std::move(item));
  if (m_fastLookup)
    m_map.insert(MAPFILEITEMSPAIR(m_ignoreURLOptions ? CURL(ptr->GetPath()).GetWithoutOptions() : ptr->GetPath(), ptr));
  m_items.emplace_back(std::move(ptr));
}

void CFileItemList::AddFront(const CFileItemPtr &pItem, int itemPosition)
{
  std::unique_lock<CCriticalSection> lock(m_lock);

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
  std::unique_lock<CCriticalSection> lock(m_lock);

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

VECFILEITEMS::iterator CFileItemList::erase(VECFILEITEMS::iterator first,
                                            VECFILEITEMS::iterator last)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return m_items.erase(first, last);
}

void CFileItemList::Remove(int iItem)
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (iItem >= 0 && iItem < Size())
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
  std::unique_lock<CCriticalSection> lock(m_lock);

  for (int i = 0; i < itemlist.Size(); ++i)
    Add(itemlist[i]);
}

void CFileItemList::Assign(const CFileItemList& itemlist, bool append)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
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
  *static_cast<CFileItem*>(this) = static_cast<const CFileItem&>(items);

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

CFileItemPtr CFileItemList::Get(int iItem) const
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (iItem > -1 && iItem < (int)m_items.size())
    return m_items[iItem];

  return CFileItemPtr();
}

CFileItemPtr CFileItemList::Get(const std::string& strPath) const
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (m_fastLookup)
  {
    MAPFILEITEMS::const_iterator it =
        m_map.find(m_ignoreURLOptions ? CURL(strPath).GetWithoutOptions() : strPath);
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
  std::unique_lock<CCriticalSection> lock(m_lock);
  return (int)m_items.size();
}

bool CFileItemList::IsEmpty() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return m_items.empty();
}

void CFileItemList::Reserve(size_t iCount)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  m_items.reserve(iCount);
}

void CFileItemList::Sort(FILEITEMLISTCOMPARISONFUNC func)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  std::stable_sort(m_items.begin(), m_items.end(), func);
}

void CFileItemList::FillSortFields(FILEITEMFILLFUNC func)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
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
  if (sortDescription.sortBy == SortByFile || sortDescription.sortBy == SortBySortTitle ||
      sortDescription.sortBy == SortByOriginalTitle || sortDescription.sortBy == SortByDateAdded ||
      sortDescription.sortBy == SortByRating || sortDescription.sortBy == SortByYear ||
      sortDescription.sortBy == SortByPlaylistOrder || sortDescription.sortBy == SortByLastPlayed ||
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
  for (SortItems::const_iterator it = sortItems.begin(); it != sortItems.end(); ++it)
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
  std::unique_lock<CCriticalSection> lock(m_lock);
  KODI::UTILS::RandomShuffle(m_items.begin(), m_items.end());
}

void CFileItemList::Archive(CArchive& ar)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
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
    ar >> tempint;
    m_sortDescription.sortBy = (SortBy)tempint;
    ar >> tempint;
    m_sortDescription.sortOrder = (SortOrder)tempint;
    ar >> tempint;
    m_sortDescription.sortAttributes = (SortAttribute)tempint;
    ar >> m_sortIgnoreFolders;
    ar >> tempint;
    m_cacheToDisc = CACHE_TYPE(tempint);

    unsigned int detailSize = 0;
    ar >> detailSize;
    for (unsigned int j = 0; j < detailSize; ++j)
    {
      GUIViewSortDetails details;
      ar >> tempint;
      details.m_sortDescription.sortBy = (SortBy)tempint;
      ar >> tempint;
      details.m_sortDescription.sortOrder = (SortOrder)tempint;
      ar >> tempint;
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
  std::unique_lock<CCriticalSection> lock(m_lock);
  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->FillInDefaultIcon();
  }
}

int CFileItemList::GetFolderCount() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);
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
  std::unique_lock<CCriticalSection> lock(m_lock);

  int numObjects = (int)m_items.size();
  if (numObjects && m_items[0]->IsParentFolder())
    numObjects--;

  return numObjects;
}

int CFileItemList::GetFileCount() const
{
  std::unique_lock<CCriticalSection> lock(m_lock);
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
  std::unique_lock<CCriticalSection> lock(m_lock);
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
  std::unique_lock<CCriticalSection> lock(m_lock);
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
          for (std::vector<std::string>::iterator itMedia = MediaFileVec.begin();
               itMedia != MediaFileVec.end(); ++itMedia)
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
                  std::vector<std::string> extensions = StringUtils::Split(CServiceBroker::GetFileExtensionProvider().GetMusicExtensions(), "|");
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
                if (StringUtils::CompareNoCase(pItem->GetPath(), strMediaFile) == 0)
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
      if (StringUtils::CompareNoCase(pItem->GetPath(), itemstodelete[i]) == 0)
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
  std::unique_lock<CCriticalSection> lock(m_lock);
  for (int i = 0; i < Size(); ++i)
    m_items[i]->RemoveExtension();
}

void CFileItemList::Stack(bool stackFiles /* = true */)
{
  std::unique_lock<CCriticalSection> lock(m_lock);

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
  const std::vector<std::string>& strFolderRegExps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_folderStackRegExps;

  std::vector<std::string>::const_iterator strExpression = strFolderRegExps.begin();
  while (strExpression != strFolderRegExps.end())
  {
    if (!folderRegExp.RegComp(*strExpression))
      CLog::Log(LOGERROR, "{}: Invalid folder stack RegExp:'{}'", __FUNCTION__,
                strExpression->c_str());
    else
      folderRegExps.push_back(folderRegExp);

    ++strExpression;
  }

  if (!folderRegExp.IsCompiled())
  {
    CLog::Log(LOGDEBUG, "{}: No stack expressions available. Skipping folder stacking",
              __FUNCTION__);
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
          //CLog::Log(LOGDEBUG,"{}: Running expression {} on {}", __FUNCTION__, expr->GetPattern(), item->GetLabel());
          bMatch = (expr->RegFind(item->GetLabel().c_str()) != -1);
          if (bMatch)
          {
            CFileItemList items;
            CDirectory::GetDirectory(item->GetPath(), items,
                                     CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(),
                                     DIR_FLAG_DEFAULTS);
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
          ++expr;
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
            item->SetLabelPreformatted(true);
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
  const std::vector<std::string>& strStackRegExps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoStackRegExps;
  std::vector<std::string>::const_iterator strRegExp = strStackRegExps.begin();
  while (strRegExp != strStackRegExps.end())
  {
    if (tmpRegExp.RegComp(*strRegExp))
    {
      if (tmpRegExp.GetCaptureTotal() == 4)
        stackRegExps.push_back(tmpRegExp);
      else
        CLog::Log(LOGERROR, "Invalid video stack RE ({}). Must have 4 captures.",
                  strRegExp->c_str());
    }
    ++strRegExp;
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
                  ++expr;
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
                ++expr;
                break;
              }
            }
            else // Title mismatch
            {
              offset = 0;
              ++expr;
              break;
            }
          }
          else // No match 2, next expression
          {
            offset = 0;
            ++expr;
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
        ++expr;
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
        if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_SHOWEXTENSIONS))
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
      CLog::Log(LOGDEBUG, "Loading items: {}, directory: {} sort method: {}, ascending: {}", Size(),
                CURL::GetRedacted(GetPath()), m_sortDescription.sortBy,
                m_sortDescription.sortOrder == SortOrderAscending ? "true" : "false");
      ar.Close();
      file.Close();
      return true;
    }
  }
  catch(const std::out_of_range&)
  {
    CLog::Log(LOGERROR, "Corrupt archive: {}", CURL::GetRedacted(path));
  }

  return false;
}

bool CFileItemList::Save(int windowID)
{
  int iSize = Size();
  if (iSize <= 0)
    return false;

  CLog::Log(LOGDEBUG, "Saving fileitems [{}]", CURL::GetRedacted(GetPath()));

  CFile file;
  std::string cachefile = GetDiscFileCache(windowID);
  if (file.OpenForWrite(cachefile, true)) // overwrite always
  {
    // Before caching save simplified cache file name in every item so the cache file can be
    // identifed and removed if the item is updated. List path and options (used for file
    // name when list cached) can not be accurately derived from item path.
    StringUtils::Replace(cachefile, "special://temp/archive_cache/", "");
    StringUtils::Replace(cachefile, ".fi", "");
    for (const auto& item : m_items)
      item->SetProperty("cachefilename", cachefile);

    CArchive ar(&file, CArchive::store);
    ar << *this;
    CLog::Log(LOGDEBUG, "  -- items: {}, sort method: {}, ascending: {}", iSize,
              m_sortDescription.sortBy,
              m_sortDescription.sortOrder == SortOrderAscending ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

void CFileItemList::RemoveDiscCache(int windowID) const
{
  RemoveDiscCache(GetDiscFileCache(windowID));
}

void CFileItemList::RemoveDiscCache(const std::string& cacheFile) const
{
  if (CFile::Exists(cacheFile))
  {
    CLog::Log(LOGDEBUG, "Clearing cached fileitems [{}]", CURL::GetRedacted(GetPath()));
    CFile::Delete(cacheFile);
  }
}

void CFileItemList::RemoveDiscCacheCRC(const std::string& crc) const
{
  std::string cachefile = StringUtils::Format("special://temp/archive_cache/{}.fi", crc);
  RemoveDiscCache(cachefile);
}

std::string CFileItemList::GetDiscFileCache(int windowID) const
{
  std::string strPath(GetPath());
  URIUtils::RemoveSlashAtEnd(strPath);

  uint32_t crc = Crc32::ComputeFromLowerCase(strPath);

  if (IsCDDA() || IsOnDVD())
    return StringUtils::Format("special://temp/archive_cache/r-{:08x}.fi", crc);

  if (IsMusicDb())
    return StringUtils::Format("special://temp/archive_cache/mdb-{:08x}.fi", crc);

  if (IsVideoDb())
    return StringUtils::Format("special://temp/archive_cache/vdb-{:08x}.fi", crc);

  if (IsSmartPlayList())
    return StringUtils::Format("special://temp/archive_cache/sp-{:08x}.fi", crc);

  if (windowID)
    return StringUtils::Format("special://temp/archive_cache/{}-{:08x}.fi", windowID, crc);

  return StringUtils::Format("special://temp/archive_cache/{:08x}.fi", crc);
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
   || (URIUtils::IsFTP(m_strPath) && !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bFTPThumbs)
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
  if (m_bIsFolder && !IsFileFolder() && (!IsRemote() || alwaysCheckRemote || CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICFILES_FINDREMOTETHUMBS)))
  {
    std::vector<CVariant> thumbs = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
        CSettings::SETTING_MUSICLIBRARY_MUSICTHUMBS);
    for (const auto& i : thumbs)
    {
      std::string strFileName = i.asString();
      std::string folderThumb(GetFolderThumb(strFileName));
      if (CFile::Exists(folderThumb))   // folder.jpg
        return folderThumb;
      size_t period = strFileName.find_last_of('.');
      if (period != std::string::npos)
      {
        std::string ext;
        std::string name = strFileName;
        std::string folderThumb1 = folderThumb;
        name.erase(period);
        ext = strFileName.substr(period);
        StringUtils::ToUpper(ext);
        StringUtils::Replace(folderThumb1, strFileName, name + ext);
        if (CFile::Exists(folderThumb1)) // folder.JPG
          return folderThumb1;

        folderThumb1 = folderThumb;
        std::string firstletter = name.substr(0, 1);
        StringUtils::ToUpper(firstletter);
        name.replace(0, 1, firstletter);
        StringUtils::Replace(folderThumb1, strFileName, name + ext);
        if (CFile::Exists(folderThumb1)) // Folder.JPG
          return folderThumb1;

        folderThumb1 = folderThumb;
        StringUtils::ToLower(ext);
        StringUtils::Replace(folderThumb1, strFileName, name + ext);
        if (CFile::Exists(folderThumb1)) // Folder.jpg
          return folderThumb1;
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
       || (URIUtils::IsFTP(m_strPath) && !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bFTPThumbs)
       || IsPlugin()
       || IsAddonsPath()
       || IsLibraryFolder()
       || IsParentFolder()
       || IsLiveTV()
       || IsPVRRecording()
       || IsDVD());
}

std::string CFileItem::GetThumbHideIfUnwatched(const CFileItem* item) const
{
  const std::shared_ptr<CSettingList> setting(std::dynamic_pointer_cast<CSettingList>(
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(
          CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS)));
  if (setting && item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_type == MediaTypeEpisode &&
      item->GetVideoInfoTag()->GetPlayCount() == 0 &&
      !CSettingUtils::FindIntInList(setting,
                                    CSettings::VIDEOLIBRARY_THUMB_SHOW_UNWATCHED_EPISODE) &&
      item->HasArt("thumb"))
  {
    std::string fanArt = item->GetArt("fanart");
    if (fanArt.empty())
      return "OverlaySpoiler.png";
    else
      return fanArt;
  }

  return item->GetArt("thumb");
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

std::string CFileItem::GetLocalArtBaseFilename() const
{
  bool useFolder = false;
  return GetLocalArtBaseFilename(useFolder);
}

std::string CFileItem::GetLocalArtBaseFilename(bool& useFolder) const
{
  std::string strFile;
  if (IsStack())
  {
    std::string strPath;
    URIUtils::GetParentPath(m_strPath,strPath);
    strFile = URIUtils::AddFileToFolder(
        strPath, URIUtils::GetFileName(CStackDirectory::GetStackedTitlePath(m_strPath)));
  }

  std::string file = strFile.empty() ? m_strPath : strFile;
  if (URIUtils::IsInRAR(file) || URIUtils::IsInZIP(file))
  {
    std::string strPath = URIUtils::GetDirectory(file);
    std::string strParent;
    URIUtils::GetParentPath(strPath,strParent);
    strFile = URIUtils::AddFileToFolder(strParent, URIUtils::GetFileName(file));
  }

  if (IsMultiPath())
    strFile = CMultiPathDirectory::GetFirstPath(m_strPath);

  if (IsOpticalMediaFile())
  { // optical media files should be treated like folders
    useFolder = true;
    strFile = GetLocalMetadataPath();
  }
  else if (useFolder && !(m_bIsFolder && !IsFileFolder()))
  {
    file = strFile.empty() ? m_strPath : strFile;
    strFile = URIUtils::GetDirectory(file);
  }

  if (strFile.empty())
    strFile = GetDynPath();

  return strFile;
}

std::string CFileItem::GetLocalArt(const std::string& artFile, bool useFolder) const
{
  // no retrieving of empty art files from folders
  if (useFolder && artFile.empty())
    return "";

  std::string strFile = GetLocalArtBaseFilename(useFolder);
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

  if (IsPlugin())
    return "";

  return URIUtils::AddFileToFolder(strFolder, folderJPG);
}

std::string CFileItem::GetMovieName(bool bUseFolderNames /* = false */) const
{
  if (IsPlugin() && HasVideoInfoTag() && !GetVideoInfoTag()->m_strTitle.empty())
    return GetVideoInfoTag()->m_strTitle;

  if (IsLabelPreformatted())
    return GetLabel();

  if (m_pvrRecordingInfoTag)
    return m_pvrRecordingInfoTag->m_strTitle;
  else if (URIUtils::IsPVRRecording(m_strPath))
  {
    std::string title = CPVRRecording::GetTitleFromURL(m_strPath);
    if (!title.empty())
      return title;
  }

  std::string strMovieName;
  if (URIUtils::IsStack(m_strPath))
    strMovieName = CStackDirectory::GetStackedTitlePath(m_strPath);
  else
    strMovieName = GetBaseMoviePath(bUseFolderNames);

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
      // Try to get archive itself, if empty take path before
      name2 = CURL(m_strPath).GetHostName();
      if (name2.empty())
        name2 = strMovieName;

      URIUtils::GetParentPath(name2, strMovieName);
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
   || (URIUtils::IsFTP(strFile) && !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bFTPThumbs)
   || m_strPath.empty())
    return "";

  std::string strDir = URIUtils::GetDirectory(strFile);

  if (strDir.empty())
    return "";

  CFileItemList items;
  CDirectory::GetDirectory(strDir, items, CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(), DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  if (IsOpticalMediaFile())
  { // grab from the optical media parent folder as well
    CFileItemList moreItems;
    CDirectory::GetDirectory(GetLocalMetadataPath(), moreItems, CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(), DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
    items.Append(moreItems);
  }

  std::vector<std::string> fanarts = { "fanart" };

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
      return true;
    }
    musicDatabase.Close();
  }
  // load tag from file
  CLog::Log(LOGDEBUG, "{}: loading tag information for file: {}", __FUNCTION__, m_strPath);
  CMusicInfoTagLoaderFactory factory;
  std::unique_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(*this));
  if (pLoader)
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
      std::string strTrack = StringUtils::Format((strText + "{}"), iTrack);
      GetMusicInfoTag()->SetTitle(strTrack);
      GetMusicInfoTag()->SetLoaded(true);
      return true;
    }
  }
  else
  {
    std::string fileName = URIUtils::GetFileName(m_strPath);
    URIUtils::RemoveExtension(fileName);
    for (const std::string& fileFilter : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicTagsFromFileFilters)
    {
      CLabelFormatter formatter(fileFilter, "");
      if (formatter.FillMusicTag(fileName, GetMusicInfoTag()))
      {
        GetMusicInfoTag()->SetLoaded(true);
        return true;
      }
    }
  }
  return false;
}

bool CFileItem::LoadGameTag()
{
  // Already loaded?
  if (HasGameInfoTag() && m_gameInfoTag->IsLoaded())
    return true;

  //! @todo
  GetGameInfoTag();

  m_gameInfoTag->SetLoaded(true);

  return false;
}

bool CFileItem::LoadDetails()
{
  if (IsVideoDb())
  {
    if (HasVideoInfoTag())
      return true;

    CVideoDatabase db;
    if (!db.Open())
    {
      CLog::LogF(LOGERROR, "Error opening video database");
      return false;
    }

    VIDEODATABASEDIRECTORY::CQueryParams params;
    VIDEODATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(GetPath(), params);

    bool ret{false};
    auto tag{std::make_unique<CVideoInfoTag>()};
    if (params.GetMovieId() >= 0)
      ret = db.GetMovieInfo(GetPath(), *tag, static_cast<int>(params.GetMovieId()));
    else if (params.GetMVideoId() >= 0)
      ret = db.GetMusicVideoInfo(GetPath(), *tag, static_cast<int>(params.GetMVideoId()));
    else if (params.GetEpisodeId() >= 0)
      ret = db.GetEpisodeInfo(GetPath(), *tag, static_cast<int>(params.GetEpisodeId()));
    else if (params.GetSetId() >= 0) // movie set
      ret = db.GetSetInfo(static_cast<int>(params.GetSetId()), *tag, this);
    else if (params.GetTvShowId() >= 0)
    {
      if (params.GetSeason() >= 0)
      {
        const int idSeason = db.GetSeasonId(static_cast<int>(params.GetTvShowId()),
                                            static_cast<int>(params.GetSeason()));
        if (idSeason >= 0)
          ret = db.GetSeasonInfo(idSeason, *tag, this);
      }
      else
        ret = db.GetTvShowInfo(GetPath(), *tag, static_cast<int>(params.GetTvShowId()), this);
    }

    if (ret)
    {
      m_videoInfoTag = tag.release();
      m_strDynPath = m_videoInfoTag->m_strFileNameAndPath;
    }

    return ret;
  }

  if (URIUtils::IsPVRRecordingFileOrFolder(GetPath()))
  {
    if (HasProperty("watchedepisodes") || HasProperty("watched"))
      return true;

    const std::string parentPath = URIUtils::GetParentPath(GetPath());

    //! @todo optimize, find a way to set the details of the item without loading parent directory.
    CFileItemList items;
    if (CDirectory::GetDirectory(parentPath, items, "", XFILE::DIR_FLAG_DEFAULTS))
    {
      const std::string path = GetPath();
      const auto it = std::find_if(items.cbegin(), items.cend(),
                                   [path](const auto& entry) { return entry->GetPath() == path; });
      if (it != items.cend())
      {
        *this = *(*it);
        return true;
      }
    }

    CLog::LogF(LOGERROR, "Error filling item details (path={})", GetPath());
    return false;
  }

  if (!IsPlayList() && IsVideo())
  {
    if (HasVideoInfoTag())
      return true;

    CVideoDatabase db;
    if (!db.Open())
    {
      CLog::LogF(LOGERROR, "Error opening video database");
      return false;
    }

    auto tag{std::make_unique<CVideoInfoTag>()};
    if (db.LoadVideoInfo(GetDynPath(), *tag))
    {
      m_videoInfoTag = tag.release();
      m_strDynPath = m_videoInfoTag->m_strFileNameAndPath;
      return true;
    }

    CLog::LogF(LOGERROR, "Error filling item details (path={})", GetPath());
    return false;
  }

  if (IsAudio())
  {
    return LoadMusicTag();
  }

  if (IsMusicDb())
  {
    if (HasMusicInfoTag())
      return true;

    CMusicDatabase db;
    if (!db.Open())
    {
      CLog::LogF(LOGERROR, "Error opening music database");
      return false;
    }

    MUSICDATABASEDIRECTORY::CQueryParams params;
    MUSICDATABASEDIRECTORY::CDirectoryNode::GetDatabaseInfo(GetPath(), params);

    if (params.GetSongId() >= 0)
    {
      CSong song;
      if (db.GetSong(params.GetSongId(), song))
      {
        GetMusicInfoTag()->SetSong(song);
        return true;
      }
    }
    else if (params.GetAlbumId() >= 0)
    {
      m_bIsFolder = true;
      CAlbum album;
      if (db.GetAlbum(params.GetAlbumId(), album, false))
      {
        GetMusicInfoTag()->SetAlbum(album);
        return true;
      }
    }
    else if (params.GetArtistId() >= 0)
    {
      m_bIsFolder = true;
      CArtist artist;
      if (db.GetArtist(params.GetArtistId(), artist, false))
      {
        GetMusicInfoTag()->SetArtist(artist);
        return true;
      }
    }
    return false;
  }

  //! @todo add support for other types on demand.
  CLog::LogF(LOGDEBUG, "Unsupported item type (path={})", GetPath());
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

  std::unique_lock<CCriticalSection> lock(m_lock);
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

bool CFileItem::HasVideoInfoTag() const
{
  // Note: CPVRRecording is derived from CVideoInfoTag
  return m_pvrRecordingInfoTag.get() != nullptr || m_videoInfoTag != nullptr;
}

CVideoInfoTag* CFileItem::GetVideoInfoTag()
{
  // Note: CPVRRecording is derived from CVideoInfoTag
  if (m_pvrRecordingInfoTag)
    return m_pvrRecordingInfoTag.get();
  else if (!m_videoInfoTag)
    m_videoInfoTag = new CVideoInfoTag;

  return m_videoInfoTag;
}

const CVideoInfoTag* CFileItem::GetVideoInfoTag() const
{
  // Note: CPVRRecording is derived from CVideoInfoTag
  return m_pvrRecordingInfoTag ? m_pvrRecordingInfoTag.get() : m_videoInfoTag;
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

CGameInfoTag* CFileItem::GetGameInfoTag()
{
  if (!m_gameInfoTag)
    m_gameInfoTag = new CGameInfoTag;

  return m_gameInfoTag;
}

bool CFileItem::HasPVRChannelInfoTag() const
{
  return m_pvrChannelGroupMemberInfoTag && m_pvrChannelGroupMemberInfoTag->Channel() != nullptr;
}

const std::shared_ptr<PVR::CPVRChannel> CFileItem::GetPVRChannelInfoTag() const
{
  return m_pvrChannelGroupMemberInfoTag ? m_pvrChannelGroupMemberInfoTag->Channel()
                                        : std::shared_ptr<CPVRChannel>();
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
  CDirectory::GetDirectory(strDir, items, CServiceBroker::GetFileExtensionProvider().GetVideoExtensions(), DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO | DIR_FLAG_NO_FILE_DIRS);
  URIUtils::RemoveExtension(strFile);
  strFile += "-trailer";
  std::string strFile3 = URIUtils::AddFileToFolder(strDir, "movie-trailer");

  // Precompile our REs
  VECCREGEXP matchRegExps;
  CRegExp tmpRegExp(true, CRegExp::autoUtf8);
  const std::vector<std::string>& strMatchRegExps = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_trailerMatchRegExps;

  std::vector<std::string>::const_iterator strRegExp = strMatchRegExps.begin();
  while (strRegExp != strMatchRegExps.end())
  {
    if (tmpRegExp.RegComp(*strRegExp))
    {
      matchRegExps.push_back(tmpRegExp);
    }
    ++strRegExp;
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
        ++expr;
      }
    }
  }

  return strTrailer;
}

VideoDbContentType CFileItem::GetVideoContentType() const
{
  VideoDbContentType type = VideoDbContentType::MOVIES;
  if (HasVideoInfoTag() && GetVideoInfoTag()->m_type == MediaTypeTvShow)
    type = VideoDbContentType::TVSHOWS;
  if (HasVideoInfoTag() && GetVideoInfoTag()->m_type == MediaTypeEpisode)
    return VideoDbContentType::EPISODES;
  if (HasVideoInfoTag() && GetVideoInfoTag()->m_type == MediaTypeMusicVideo)
    return VideoDbContentType::MUSICVIDEOS;
  if (HasVideoInfoTag() && GetVideoInfoTag()->m_type == MediaTypeAlbum)
    return VideoDbContentType::MUSICALBUMS;

  CVideoDatabaseDirectory dir;
  VIDEODATABASEDIRECTORY::CQueryParams params;
  dir.GetQueryParams(m_strPath, params);
  if (params.GetSetId() != -1 && params.GetMovieId() == -1) // movie set
    return VideoDbContentType::MOVIE_SETS;

  return type;
}

CFileItem CFileItem::GetItemToPlay() const
{
  if (HasEPGInfoTag())
  {
    const std::shared_ptr<CPVRChannelGroupMember> groupMember =
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Channels>().GetChannelGroupMember(*this);
    if (groupMember)
      return CFileItem(groupMember);
  }
  return *this;
}

CBookmark CFileItem::GetResumePoint() const
{
  if (HasVideoInfoTag())
    return GetVideoInfoTag()->GetResumePoint();
  return CBookmark();
}

bool CFileItem::IsResumePointSet() const
{
  return GetResumePoint().IsSet();
}

double CFileItem::GetCurrentResumeTime() const
{
  return lrint(GetResumePoint().timeInSeconds);
}

bool CFileItem::GetCurrentResumeTimeAndPartNumber(int64_t& startOffset, int& partNumber) const
{
  CBookmark resumePoint(GetResumePoint());
  if (resumePoint.IsSet())
  {
    startOffset = llrint(resumePoint.timeInSeconds);
    partNumber = resumePoint.partNumber;
    return true;
  }
  return false;
}

bool CFileItem::IsResumable() const
{
  if (m_bIsFolder)
  {
    int64_t watched = 0;
    int64_t inprogress = 0;
    int64_t total = 0;
    if (HasProperty("inprogressepisodes"))
    {
      // show/season
      watched = GetProperty("watchedepisodes").asInteger();
      inprogress = GetProperty("inprogressepisodes").asInteger();
      total = GetProperty("totalepisodes").asInteger();
    }
    else if (HasProperty("inprogress"))
    {
      // movie set
      watched = GetProperty("watched").asInteger();
      inprogress = GetProperty("inprogress").asInteger();
      total = GetProperty("total").asInteger();
    }

    return ((total != watched) && (inprogress > 0 || watched != 0));
  }
  else
  {
    return HasVideoInfoTag() && GetVideoInfoTag()->GetResumePoint().IsPartWay();
  }
}
