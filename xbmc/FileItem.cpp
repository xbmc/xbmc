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
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/MusicDatabaseDirectory/QueryParams.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "games/GameUtils.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/LocalizeStrings.h"
#include "media/MediaLockState.h"
#include "music/Album.h"
#include "music/Artist.h"
#include "music/MusicDatabase.h"
#include "music/MusicFileItemClassify.h"
#include "music/tags/MusicInfoTag.h"
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "network/NetworkFileItemClassify.h"
#include "pictures/PictureInfoTag.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "playlists/PlayListFileItemClassify.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/epg/EpgSearchFilter.h"
#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsUtils.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/Archive.h"
#include "utils/ArtUtils.h"
#include "utils/FileExtensionProvider.h"
#include "utils/Mime.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/Bookmark.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"
#include "video/VideoUtils.h"

#include <cstdlib>
#include <memory>

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
  ART::FillInDefaultIcon(*this);
  FillInMimeType(false);
}

CFileItem::CFileItem(const CVideoInfoTag& movie)
{
  Initialize();
  SetFromVideoInfoTag(movie);
}

void CFileItem::FillMusicInfoTag(const std::shared_ptr<const CPVREpgInfoTag>& tag)
{
  CMusicInfoTag* musictag = GetMusicInfoTag(); // create (!) the music tag.

  musictag->SetTitle(CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().GetTitleForEpgTag(tag));

  if (tag)
  {
    musictag->SetGenre(tag->Genre());
    musictag->SetDuration(tag->GetDuration());
    musictag->SetURL(tag->Path());
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
  SetLabel(CServiceBroker::GetPVRManager().Get<PVR::GUI::EPG>().GetTitleForEpgTag(tag));
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

  const std::string iconPath = filter->GetIconPath();
  if (!iconPath.empty())
    SetArt("icon", iconPath);
  else
    SetArt("icon", "DefaultPVRSearch.png");

  // Speedup FillInDefaultIcon()
  SetProperty("icon_never_overlay", true);

  FillInMimeType(false);
}

CFileItem::CFileItem(const std::shared_ptr<CPVRChannelGroupMember>& channelGroupMember)
{
  Initialize();

  const std::shared_ptr<const CPVRChannel> channel = channelGroupMember->Channel();

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
    const std::shared_ptr<const CPVREpgInfoTag> epgNow = channel->GetEPGNow();
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
    const std::shared_ptr<const CPVRChannel> channel = record->Channel();
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

CFileItem::CFileItem(const std::string& path, const std::shared_ptr<CPVRProvider>& provider)
{
  Initialize();

  m_strPath = path;
  m_bIsFolder = true;
  m_pvrProviderInfoTag = provider;
  SetLabel(provider->GetName());
  m_bCanQueue = false;

  // Set art
  if (!provider->GetIconPath().empty())
    SetArt("icon", provider->GetIconPath());
  else
    SetArt("icon", "DefaultPVRProvider.png");

  if (!provider->GetThumbPath().empty())
    SetArt("thumb", provider->GetThumbPath());

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
  m_pvrProviderInfoTag = item.m_pvrProviderInfoTag;
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
  m_iDriveType = SourceType::UNKNOWN;
  m_lStartOffset = 0;
  m_lStartPartNumber = 1;
  m_lEndOffset = 0;
  m_iprogramCount = 0;
  m_idepth = 1;
  m_iLockMode = LockMode::EVERYONE;
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
  m_pvrProviderInfoTag.reset();
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

void CFileItem::Archive(CArchive& ar)
{
  CGUIListItem::Archive(ar);

  if (ar.IsStoring())
  {
    ar << m_bIsParentFolder;
    ar << m_bLabelPreformatted;
    ar << m_strPath;
    ar << m_strDynPath;
    ar << m_bIsShareOrDrive;
    ar << static_cast<int>(m_iDriveType);
    ar << m_dateTime;
    ar << m_dwSize;
    ar << m_strDVDLabel;
    ar << m_strTitle;
    ar << m_iprogramCount;
    ar << m_idepth;
    ar << m_lStartOffset;
    ar << m_lStartPartNumber;
    ar << m_lEndOffset;
    ar << static_cast<int>(m_iLockMode);
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
    ar >> m_strDynPath;
    ar >> m_bIsShareOrDrive;
    int dtype;
    ar >> dtype;
    m_iDriveType = static_cast<SourceType>(dtype);
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
    m_iLockMode = static_cast<LockMode>(temp);
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
      sortable[FieldDriveType] = static_cast<int>(m_iDriveType);
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

  if (HasPVRProviderInfoTag())
    GetPVRProviderInfoTag()->ToSortable(sortable, field);

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
  if (m_strPath.empty() || IsPath("add") || NETWORK::IsInternetStream(*this) || IsParentFolder() ||
      IsVirtualDirectoryRoot() || IsPlugin() || IsPVR())
    return true;

  if (VIDEO::IsVideoDb(*this) && HasVideoInfoTag())
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

bool CFileItem::IsPVRProvider() const
{
  return HasPVRProviderInfoTag();
}

bool CFileItem::IsDeleted() const
{
  if (HasPVRRecordingInfoTag())
    return GetPVRRecordingInfoTag()->IsDeleted();

  return false;
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
      HasPVRRecordingInfoTag() || HasEPGInfoTag() || HasEPGSearchFilter() ||
      HasPVRProviderInfoTag())
    return false;

  if (!m_strPath.empty())
    return CUtil::IsPicture(m_strPath);

  return false;
}

bool CFileItem::IsFileFolder(FileFolderType types) const
{
  FileFolderType always_type = FileFolderType::ALWAYS;

  /* internet streams are not directly expanded */
  if (NETWORK::IsInternetStream(*this))
    always_type = FileFolderType::ONCLICK;

  // strm files are not browsable
  if (IsType(".strm") && (static_cast<int>(types) & static_cast<int>(FileFolderType::ONBROWSE)))
    return false;

  if (static_cast<int>(types) & static_cast<int>(always_type))
  {
    if (PLAYLIST::IsSmartPlayList(*this) ||
        (PLAYLIST::IsPlayList(*this) &&
         CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_playlistAsFolders) ||
        IsAPK() || IsZIP() || IsRAR() || IsRSS() || MUSIC::IsAudioBook(*this) ||
        IsType(".ogg|.oga|.xbt")
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

  if (static_cast<int>(types) & static_cast<int>(FileFolderType::ONBROWSE))
  {
    if ((PLAYLIST::IsPlayList(*this) &&
         !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_playlistAsFolders) ||
        IsDiscImage())
      return true;
  }

  return false;
}

bool CFileItem::IsLibraryFolder() const
{
  if (HasProperty("library.filter") && GetProperty("library.filter").asBoolean())
    return true;

  return URIUtils::IsLibraryFolder(m_strPath);
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
  return URIUtils::IsOpticalMediaFile(GetDynPath());
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
  return URIUtils::IsStack(GetDynPath());
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
  return URIUtils::IsBlurayPath(GetDynPath()) ||
         URIUtils::IsBDFile(VIDEO::UTILS::GetOpticalMediaPath(*this));
}

bool CFileItem::IsDVD() const
{
  return URIUtils::IsDVD(m_strPath) || m_iDriveType == SourceType::OPTICAL_DISC;
}

bool CFileItem::IsOnDVD() const
{
  return URIUtils::IsOnDVD(m_strPath) || m_iDriveType == SourceType::OPTICAL_DISC;
}

bool CFileItem::IsNfs() const
{
  return URIUtils::IsNfs(m_strPath);
}

bool CFileItem::IsISO9660() const
{
  return URIUtils::IsISO9660(m_strPath);
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

bool CFileItem::IsVirtualDirectoryRoot() const
{
  return (m_bIsFolder && m_strPath.empty());
}

bool CFileItem::IsRemovable() const
{
  return IsOnDVD() || MUSIC::IsCDDA(*this) || m_iDriveType == SourceType::REMOVABLE;
}

bool CFileItem::IsReadOnly() const
{
  if (IsParentFolder())
    return true;

  if (m_bIsShareOrDrive)
    return true;

  return !CUtil::SupportsWriteFileOperations(m_strPath);
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

void CFileItem::UpdateMimeType(bool lookup /*= true*/)
{
  //! @todo application/octet-stream might actually have been set by a web lookup. Currently we
  //! cannot distinguish between set as fallback only (see FillInMimeType) or as an actual value.
  if (m_mimetype == "application/octet-stream")
    m_mimetype.clear();

  FillInMimeType(lookup);
}

void CFileItem::SetMimeTypeForInternetFile()
{
  if (m_doContentLookup && NETWORK::IsInternetStream(*this))
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
    // See if we have associated a bluray playlist
    if (URIUtils::IsBlurayPath(GetDynPath()) || URIUtils::IsBlurayPath(item->GetDynPath()))
      return (GetDynPath() == item->GetDynPath());
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
    const CVideoInfoTag* myTag{GetVideoInfoTag()};
    const CVideoInfoTag* otherTag{item->GetVideoInfoTag()};
    if (myTag->m_iDbId != -1 && otherTag->m_iDbId != -1)
    {
      if ((myTag->m_iDbId == otherTag->m_iDbId) && (myTag->m_type == otherTag->m_type))
      {
        // for movies with multiple versions, wie need also to check the file id
        if (HasVideoVersions() && item->HasVideoVersions() && myTag->m_iFileId != -1 &&
            otherTag->m_iFileId != -1)
          return myTag->m_iFileId == otherTag->m_iFileId;
        return true;
      }
    }
  }
  if (MUSIC::IsMusicDb(*this) && HasMusicInfoTag())
  {
    CFileItem dbItem(m_musicInfoTag->GetURL(), false);
    if (HasProperty("item_start"))
      dbItem.SetProperty("item_start", GetProperty("item_start"));
    return dbItem.IsSamePath(item);
  }
  if (VIDEO::IsVideoDb(*this) && HasVideoInfoTag())
  {
    CFileItem dbItem(GetVideoInfoTag()->m_strFileNameAndPath, false);
    if (HasProperty("item_start"))
      dbItem.SetProperty("item_start", GetProperty("item_start"));
    return dbItem.IsSamePath(item);
  }
  if (MUSIC::IsMusicDb(*item) && item->HasMusicInfoTag())
  {
    CFileItem dbItem(item->m_musicInfoTag->GetURL(), false);
    if (item->HasProperty("item_start"))
      dbItem.SetProperty("item_start", item->GetProperty("item_start"));
    return IsSamePath(&dbItem);
  }
  if (VIDEO::IsVideoDb(*item) && item->HasVideoInfoTag())
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

    SetOverlayImage(GetVideoInfoTag()->GetPlayCount() > 0 ? CGUIListItem::ICON_OVERLAY_WATCHED
                                                          : CGUIListItem::ICON_OVERLAY_UNWATCHED);
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
  if (item.HasPVRChannelGroupMemberInfoTag())
  {
    m_pvrChannelGroupMemberInfoTag = item.GetPVRChannelGroupMemberInfoTag();
    SetInvalid();
  }
  if (item.HasPVRTimerInfoTag())
  {
    m_pvrTimerInfoTag = item.m_pvrTimerInfoTag;
    SetInvalid();
  }
  if (item.HasPVRProviderInfoTag())
  {
    m_pvrProviderInfoTag = item.m_pvrProviderInfoTag;
    SetInvalid();
  }
  if (item.HasEPGInfoTag())
  {
    m_epgInfoTag = item.m_epgInfoTag;
    SetInvalid();
  }
  if (item.HasEPGSearchFilter())
  {
    m_epgSearchFilter = item.m_epgSearchFilter;
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

  SetContentLookup(item.m_doContentLookup);
  SetMimeType(item.m_mimetype);
  UpdateMimeType(m_doContentLookup);
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

    SetOverlayImage(GetVideoInfoTag()->GetPlayCount() > 0 ? CGUIListItem::ICON_OVERLAY_WATCHED
                                                          : CGUIListItem::ICON_OVERLAY_UNWATCHED);
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
  if (item.HasPVRChannelGroupMemberInfoTag())
  {
    m_pvrChannelGroupMemberInfoTag = item.GetPVRChannelGroupMemberInfoTag();
    SetInvalid();
  }
  if (item.HasPVRTimerInfoTag())
  {
    m_pvrTimerInfoTag = item.m_pvrTimerInfoTag;
    SetInvalid();
  }
  if (item.HasPVRProviderInfoTag())
  {
    m_pvrProviderInfoTag = item.m_pvrProviderInfoTag;
    SetInvalid();
  }
  if (item.HasEPGInfoTag())
  {
    m_epgInfoTag = item.m_epgInfoTag;
    SetInvalid();
  }
  if (item.HasEPGSearchFilter())
  {
    m_epgSearchFilter = item.m_epgSearchFilter;
    SetInvalid();
  }
  SetDynPath(item.GetDynPath());
  if (!item.GetLabel().empty())
    SetLabel(item.GetLabel());
  if (!item.GetLabel2().empty())
    SetLabel2(item.GetLabel2());
  if (!item.GetArt().empty())
  {
    if (VIDEO::IsVideo(item))
      AppendArt(item.GetArt());
    else
      SetArt(item.GetArt());
  }
  AppendProperties(item);

  SetContentLookup(item.m_doContentLookup);
  SetMimeType(item.m_mimetype);
  UpdateMimeType(m_doContentLookup);
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
  ART::FillInDefaultIcon(*this);
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
  ART::FillInDefaultIcon(*this);
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

  bool result = m_cueDocument->LoadTracks(scannedItems, *this);
  m_cueDocument.reset();

  return result;
}

std::string CFileItem::GetUserMusicThumb(bool alwaysCheckRemote /* = false */, bool fallbackToFolder /* = false */) const
{
  if (m_strPath.empty() || StringUtils::StartsWithNoCase(m_strPath, "newsmartplaylist://") ||
      StringUtils::StartsWithNoCase(m_strPath, "newplaylist://") || m_bIsShareOrDrive ||
      NETWORK::IsInternetStream(*this) || URIUtils::IsUPnP(m_strPath) ||
      (URIUtils::IsFTP(m_strPath) &&
       !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bFTPThumbs) ||
      IsPlugin() || IsAddonsPath() || IsLibraryFolder() || IsParentFolder() ||
      MUSIC::IsMusicDb(*this))
    return "";

  // we first check for <filename>.tbn or <foldername>.tbn
  std::string fileThumb(ART::GetTBNFile(*this));
  if (CFile::Exists(fileThumb))
    return fileThumb;

  // Fall back to folder thumb, if requested
  if (!m_bIsFolder && fallbackToFolder)
  {
    CFileItem item(URIUtils::GetDirectory(m_strPath), true);
    return item.GetUserMusicThumb(alwaysCheckRemote);
  }

  // if a folder, check for folder.jpg
  if (m_bIsFolder && !IsFileFolder() &&
      (!NETWORK::IsRemote(*this) || alwaysCheckRemote ||
       CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
           CSettings::SETTING_MUSICFILES_FINDREMOTETHUMBS)))
  {
    std::vector<CVariant> thumbs = CServiceBroker::GetSettingsComponent()->GetSettings()->GetList(
        CSettings::SETTING_MUSICLIBRARY_MUSICTHUMBS);
    for (const auto& i : thumbs)
    {
      std::string strFileName = i.asString();
      std::string folderThumb(ART::GetFolderThumb(*this, strFileName));
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

bool CFileItem::SkipLocalArt() const
{
  return (m_strPath.empty() || StringUtils::StartsWithNoCase(m_strPath, "newsmartplaylist://") ||
          StringUtils::StartsWithNoCase(m_strPath, "newplaylist://") || m_bIsShareOrDrive ||
          NETWORK::IsInternetStream(*this) || URIUtils::IsUPnP(m_strPath) ||
          (URIUtils::IsFTP(m_strPath) &&
           !CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_bFTPThumbs) ||
          IsPlugin() || IsAddonsPath() || IsLibraryFolder() || IsParentFolder() || IsLiveTV() ||
          IsPVRRecording() || IsDVD());
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
    thumb = ART::GetLocalArt(*this, artFile, false);
    if (!thumb.empty() && CFile::Exists(thumb))
      return thumb;
  }
  if ((useFolder || (m_bIsFolder && !IsFileFolder())) && !artFile.empty())
  {
    std::string thumb2 = ART::GetLocalArt(*this, artFile, true);
    if (!thumb2.empty() && thumb2 != thumb && CFile::Exists(thumb2))
      return thumb2;
  }
  return "";
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
      (!m_bIsFolder || URIUtils::IsInArchive(m_strPath) || URIUtils::IsBlurayPath(m_strPath) ||
       (HasVideoInfoTag() && GetVideoInfoTag()->m_iDbId > 0 &&
        !CMediaTypes::IsContainer(GetVideoInfoTag()->m_type))))
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

    // Remove trailing 'Disc n' path segment to get actual movie title
    strMovieName = CUtil::RemoveTrailingDiscNumberSegmentFromPath(strMovieName);
  }

  return strMovieName;
}

std::string CFileItem::GetLocalMetadataPath() const
{
  if (m_bIsFolder && !IsFileFolder())
    return m_strPath;

  if (URIUtils::IsBlurayPath(GetDynPath()) || VIDEO::IsDVDFile(*this) || VIDEO::IsBDFile(*this))
    return URIUtils::GetDiscBasePath(GetDynPath());

  return URIUtils::GetParentPath(m_strPath);
}

bool CFileItem::LoadMusicTag()
{
  // not audio
  if (!MUSIC::IsAudio(*this))
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
  if (MUSIC::IsCDDA(*this))
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
  if (VIDEO::IsVideoDb(*this))
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
      ret = db.GetMovieInfo({}, *tag, static_cast<int>(params.GetMovieId()),
                            static_cast<int>(params.GetVideoVersionId()),
                            static_cast<int>(params.GetVideoAssetId()));
    else if (params.GetMVideoId() >= 0)
      ret = db.GetMusicVideoInfo({}, *tag, static_cast<int>(params.GetMVideoId()));
    else if (params.GetEpisodeId() >= 0)
      ret = db.GetEpisodeInfo({}, *tag, static_cast<int>(params.GetEpisodeId()));
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
        ret = db.GetTvShowInfo({}, *tag, static_cast<int>(params.GetTvShowId()), this);
    }

    if (ret)
    {
      const CFileItem loadedItem{*tag};
      UpdateInfo(loadedItem);
    }
    return ret;
  }

  if (IsPVR())
  {
    const std::shared_ptr<CFileItem> loadedItem{
        CServiceBroker::GetPVRManager().Get<PVR::GUI::Utils>().LoadItem(*this)};
    if (loadedItem)
    {
      UpdateInfo(*loadedItem);
      return true;
    }
    CLog::LogF(LOGERROR, "Error filling PVR item details (path={})", GetPath());
    return false;
  }

  if (!PLAYLIST::IsPlayList(*this) && VIDEO::IsVideo(*this))
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
      const CFileItem loadedItem{*tag};
      UpdateInfo(loadedItem);
      return true;
    }

    CLog::LogF(LOGERROR, "Error filling item details (path={})", GetPath());
    return false;
  }

  if (PLAYLIST::IsPlayList(*this) && IsType(".strm"))
  {
    const std::unique_ptr<PLAYLIST::CPlayList> playlist(PLAYLIST::CPlayListFactory::Create(*this));
    if (playlist)
    {
      if (playlist->Load(GetPath()) && playlist->size() == 1)
      {
        const auto item{(*playlist)[0]};
        if (VIDEO::IsVideo(*item))
        {
          CVideoDatabase db;
          if (!db.Open())
          {
            CLog::LogF(LOGERROR, "Error opening video database");
            return false;
          }

          CVideoInfoTag tag;
          if (db.LoadVideoInfo(GetDynPath(), tag))
          {
            UpdateInfo(*item);
            *GetVideoInfoTag() = tag;
            return true;
          }
        }
        else if (MUSIC::IsAudio(*item))
        {
          if (item->LoadMusicTag())
          {
            UpdateInfo(*item);
            return true;
          }
        }
      }
    }
    CLog::LogF(LOGERROR, "Error loading strm file details (path={})", GetPath());
    return false;
  }

  if (MUSIC::IsAudio(*this))
  {
    return LoadMusicTag();
  }

  if (MUSIC::IsMusicDb(*this))
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

  if (GetProperty("IsVideoFolder").asBoolean(false))
  {
    const std::shared_ptr<CFileItem> loadedItem{VIDEO::UTILS::LoadVideoFilesFolderInfo(*this)};
    if (loadedItem)
    {
      UpdateInfo(*loadedItem);
      return true;
    }
    CLog::LogF(LOGERROR, "Error filling video files folder item details (path={})", GetPath());
    return false;
  }

  //! @todo add support for other types on demand.
  CLog::LogF(LOGDEBUG, "Unsupported item type (path={})", GetPath());
  return false;
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

bool CFileItem::HasVideoVersions() const
{
  if (HasVideoInfoTag())
  {
    return GetVideoInfoTag()->HasVideoVersions();
  }
  return false;
}

bool CFileItem::HasVideoExtras() const
{
  if (HasVideoInfoTag())
  {
    return GetVideoInfoTag()->HasVideoExtras();
  }
  return false;
}
