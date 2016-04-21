/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_GUIListItem.h"
#include "Addon_GUIGeneral.h"

#include "FileItem.h"
#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "guilib/GUIListItem.h"
#include "guilib/GUIWindowManager.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "video/VideoInfoTag.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnListItem::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.ListItem.Create                = V2::KodiAPI::GUI::CAddOnListItem::Create;
  interfaces->GUI.ListItem.Destroy               = V2::KodiAPI::GUI::CAddOnListItem::Destroy;
  interfaces->GUI.ListItem.GetLabel              = V2::KodiAPI::GUI::CAddOnListItem::GetLabel;
  interfaces->GUI.ListItem.SetLabel              = V2::KodiAPI::GUI::CAddOnListItem::SetLabel;
  interfaces->GUI.ListItem.GetLabel2             = V2::KodiAPI::GUI::CAddOnListItem::GetLabel2;
  interfaces->GUI.ListItem.SetLabel2             = V2::KodiAPI::GUI::CAddOnListItem::SetLabel2;
  interfaces->GUI.ListItem.GetIconImage          = V2::KodiAPI::GUI::CAddOnListItem::GetIconImage;
  interfaces->GUI.ListItem.SetIconImage          = V2::KodiAPI::GUI::CAddOnListItem::SetIconImage;
  interfaces->GUI.ListItem.GetOverlayImage       = V2::KodiAPI::GUI::CAddOnListItem::GetOverlayImage;
  interfaces->GUI.ListItem.SetOverlayImage       = V2::KodiAPI::GUI::CAddOnListItem::SetOverlayImage;
  interfaces->GUI.ListItem.SetThumbnailImage     = V2::KodiAPI::GUI::CAddOnListItem::SetThumbnailImage;
  interfaces->GUI.ListItem.SetArt                = V2::KodiAPI::GUI::CAddOnListItem::SetArt;
  interfaces->GUI.ListItem.GetArt                = V2::KodiAPI::GUI::CAddOnListItem::GetArt;
  interfaces->GUI.ListItem.SetArtFallback        = V2::KodiAPI::GUI::CAddOnListItem::SetArtFallback;
  interfaces->GUI.ListItem.HasArt                = V2::KodiAPI::GUI::CAddOnListItem::HasArt;
  interfaces->GUI.ListItem.Select                = V2::KodiAPI::GUI::CAddOnListItem::Select;
  interfaces->GUI.ListItem.IsSelected            = V2::KodiAPI::GUI::CAddOnListItem::IsSelected;
  interfaces->GUI.ListItem.HasIcon               = V2::KodiAPI::GUI::CAddOnListItem::HasIcon;
  interfaces->GUI.ListItem.HasOverlay            = V2::KodiAPI::GUI::CAddOnListItem::HasOverlay;
  interfaces->GUI.ListItem.IsFileItem            = V2::KodiAPI::GUI::CAddOnListItem::IsFileItem;
  interfaces->GUI.ListItem.IsFolder              = V2::KodiAPI::GUI::CAddOnListItem::IsFolder;
  interfaces->GUI.ListItem.SetProperty           = V2::KodiAPI::GUI::CAddOnListItem::SetProperty;
  interfaces->GUI.ListItem.GetProperty           = V2::KodiAPI::GUI::CAddOnListItem::GetProperty;
  interfaces->GUI.ListItem.ClearProperty         = V2::KodiAPI::GUI::CAddOnListItem::ClearProperty;
  interfaces->GUI.ListItem.ClearProperties       = V2::KodiAPI::GUI::CAddOnListItem::ClearProperties;
  interfaces->GUI.ListItem.HasProperties         = V2::KodiAPI::GUI::CAddOnListItem::HasProperties;
  interfaces->GUI.ListItem.HasProperty           = V2::KodiAPI::GUI::CAddOnListItem::HasProperty;
  interfaces->GUI.ListItem.SetPath               = V2::KodiAPI::GUI::CAddOnListItem::SetPath;
  interfaces->GUI.ListItem.GetPath               = V2::KodiAPI::GUI::CAddOnListItem::GetPath;
  interfaces->GUI.ListItem.GetDuration           = V2::KodiAPI::GUI::CAddOnListItem::GetDuration;
  interfaces->GUI.ListItem.SetSubtitles          = V2::KodiAPI::GUI::CAddOnListItem::SetSubtitles;
  interfaces->GUI.ListItem.SetMimeType           = V2::KodiAPI::GUI::CAddOnListItem::SetMimeType;
  interfaces->GUI.ListItem.SetContentLookup      = V2::KodiAPI::GUI::CAddOnListItem::SetContentLookup;
  interfaces->GUI.ListItem.AddContextMenuItems   = V2::KodiAPI::GUI::CAddOnListItem::AddContextMenuItems;
  interfaces->GUI.ListItem.AddStreamInfo         = V2::KodiAPI::GUI::CAddOnListItem::AddStreamInfo;
}

void* CAddOnListItem::Create(void *addonData, const char *label, const char *label2, const char *iconImage, const char *thumbnailImage, const char *path)
{
  try
  {
    // create CFileItem
    CFileItem *pItem = new CFileItem();
    if (!pItem)
      return nullptr;

    if (label)
      pItem->SetLabel(label);
    if (label2)
      pItem->SetLabel2(label2);
    if (iconImage)
      pItem->SetIconImage(iconImage);
    if (thumbnailImage)
      pItem->SetArt("thumb", thumbnailImage);
    if (path)
      pItem->SetPath(path);

    return pItem;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnListItem::Destroy(void *addonData, void* handle)
{
  // @note: Delete of CFileItem brings crash, need to check about the in related
  // window and no memleak present to confirm.
  // In old version was the same way, only the Destroy passed here to allow
  // changes and fixes without API level changes.
}

void CAddOnListItem::GetLabel(void *addonData, void* handle, char &label, unsigned int &iMaxStringSize)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    strncpy(&label, static_cast<CFileItem *>(handle)->GetLabel().c_str(), iMaxStringSize);
    iMaxStringSize = static_cast<CFileItem *>(handle)->GetLabel().length();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetLabel(void *addonData, void* handle, const char *label)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetLabel(label);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::GetLabel2(void *addonData, void* handle, char &label, unsigned int &iMaxStringSize)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    strncpy(&label, static_cast<CFileItem *>(handle)->GetLabel2().c_str(), iMaxStringSize);
    iMaxStringSize = static_cast<CFileItem *>(handle)->GetLabel2().length();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetLabel2(void *addonData, void* handle, const char *label)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetLabel2(label);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::GetIconImage(void *addonData, void* handle, char &image, unsigned int &iMaxStringSize)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    strncpy(&image, static_cast<CFileItem *>(handle)->GetIconImage().c_str(), iMaxStringSize);
    iMaxStringSize = static_cast<CFileItem *>(handle)->GetIconImage().length();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetIconImage(void *addonData, void* handle, const char *image)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetIconImage(image);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::GetOverlayImage(void *addonData, void* handle, char &image, unsigned int &iMaxStringSize)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    strncpy(&image, static_cast<CFileItem *>(handle)->GetOverlayImage().c_str(), iMaxStringSize);
    iMaxStringSize = static_cast<CFileItem *>(handle)->GetOverlayImage().length();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetOverlayImage(void *addonData, void* handle, unsigned int image, bool bOnOff /* = false*/)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetOverlayImage((CGUIListItem::GUIIconOverlay)image, bOnOff);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetThumbnailImage(void *addonData, void* handle, const char *image)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetArt("thumb", image);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetArt(void *addonData, void* handle, const char *type, const char *url)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetArt(type, url);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

char* CAddOnListItem::GetArt(void *addonData, void* handle, const char *type)
{
  char* buffer = nullptr;
  CAddOnGUIGeneral::Lock();
  try
  {
    buffer = strdup(static_cast<CFileItem *>(handle)->GetArt(type).c_str());
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
  return buffer;
}

void CAddOnListItem::SetArtFallback(void *addonData, void* handle, const char *from, const char *to)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetArtFallback(from, to);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

bool CAddOnListItem::HasArt(void *addonData, void* handle, const char *type)
{
  bool ret = false;
  CAddOnGUIGeneral::Lock();
  try
  {
    ret = static_cast<CFileItem *>(handle)->HasArt(type);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
  return ret;
}

void CAddOnListItem::Select(void *addonData, void* handle, bool bOnOff)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->Select(bOnOff);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

bool CAddOnListItem::IsSelected(void *addonData, void* handle)
{
  bool ret = false;
  CAddOnGUIGeneral::Lock();
  try
  {
    ret = static_cast<CFileItem *>(handle)->IsSelected();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();

  return ret;
}

bool CAddOnListItem::HasIcon(void *addonData, void* handle)
{
  bool ret = false;
  CAddOnGUIGeneral::Lock();
  try
  {
    ret = static_cast<CFileItem *>(handle)->HasIcon();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();

  return ret;
}

bool CAddOnListItem::HasOverlay(void *addonData, void* handle)
{
  bool ret = false;
  CAddOnGUIGeneral::Lock();
  try
  {
    ret = static_cast<CFileItem *>(handle)->HasOverlay();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();

  return ret;
}

bool CAddOnListItem::IsFileItem(void *addonData, void* handle)
{
  bool ret = false;
  CAddOnGUIGeneral::Lock();
  try
  {
    ret = static_cast<CFileItem *>(handle)->IsFileItem();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
  return ret;
}

bool CAddOnListItem::IsFolder(void *addonData, void* handle)
{
  bool ret = false;
  CAddOnGUIGeneral::Lock();
  try
  {
    ret = static_cast<CFileItem *>(handle)->m_bIsFolder;
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
  return ret;
}

void CAddOnListItem::SetProperty(void *addonData, void* handle, const char *key, const char *value)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetProperty(key, value);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::GetProperty(void *addonData, void* handle, const char *key, char &property, unsigned int &iMaxStringSize)
{
  try
  {
    std::string lowerKey = key;
    StringUtils::ToLower(lowerKey);

    CAddOnGUIGeneral::Lock();
    std::string value = static_cast<CFileItem *>(handle)->GetProperty(lowerKey).asString();
    CAddOnGUIGeneral::Unlock();

    strncpy(&property, value.c_str(), iMaxStringSize);
    iMaxStringSize = value.length();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnListItem::ClearProperty(void *addonData, void* handle, const char *key)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->ClearProperty(key);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::ClearProperties(void *addonData, void* handle)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->ClearProperties();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

bool CAddOnListItem::HasProperties(void *addonData, void* handle)
{
  bool ret = false;
  CAddOnGUIGeneral::Lock();
  try
  {
    ret = static_cast<CFileItem *>(handle)->HasProperties();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
  return ret;
}

bool CAddOnListItem::HasProperty(void *addonData, void* handle, const char *key)
{
  bool ret = false;
  CAddOnGUIGeneral::Lock();
  try
  {
    ret = static_cast<CFileItem *>(handle)->HasProperty(key);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
  return ret;
}

void CAddOnListItem::SetPath(void *addonData, void* handle, const char *path)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetPath(path);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

char* CAddOnListItem::GetPath(void *addonData, void* handle)
{
  char* buffer = nullptr;
  CAddOnGUIGeneral::Lock();
  try
  {
    buffer = strdup(static_cast<CFileItem *>(handle)->GetPath().c_str());
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
  return buffer;
}

int CAddOnListItem::GetDuration(void *addonData, void* handle)
{
  int duration = 0;
  CAddOnGUIGeneral::Lock();
  try
  {
    if (static_cast<CFileItem *>(handle)->LoadMusicTag())
      duration = static_cast<CFileItem *>(handle)->GetMusicInfoTag()->GetDuration();
    else if (static_cast<CFileItem *>(handle)->HasVideoInfoTag())
      duration = static_cast<CFileItem *>(handle)->GetVideoInfoTag()->GetDuration();
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
  return duration;
}

void CAddOnListItem::SetSubtitles(void *addonData, void* handle, const char** streams, unsigned int entries)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    for (unsigned int i = 0; i < entries; ++i)
    {
      std::string property = StringUtils::Format("subtitle:%u", i+1);
      static_cast<CFileItem *>(handle)->SetProperty(property, streams[i]);
    }
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetMimeType(void* addonData, void* handle, const char* mimetype)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetMimeType(mimetype);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetContentLookup(void* addonData, void* handle, bool enable)
{
  CAddOnGUIGeneral::Lock();
  try
  {
    static_cast<CFileItem *>(handle)->SetContentLookup(enable);
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::AddContextMenuItems(void* addonData, void* handle, const char** menus[2], unsigned int entries, bool replaceItems)
{
  try
  {
    if (menus == nullptr)
      throw ADDON::WrongValueException("Must pass a list of strings. List self is empty.");

    for (unsigned int i = 0; i < entries; ++i)
    {
      if (menus[i][0] == nullptr || menus[i][1] == nullptr)
        throw ADDON::WrongValueException("Must pass in a list of tuples of pairs of strings. One entry in the list is empty.");

      std::string uText   = menus[i][0];
      std::string uAction = menus[i][1];

      CAddOnGUIGeneral::Lock();
      std::string property = StringUtils::Format("contextmenulabel(%i)", i);
      static_cast<CFileItem *>(handle)->SetProperty(property, uText);

      property = StringUtils::Format("contextmenuaction(%i)", i);
      static_cast<CFileItem *>(handle)->SetProperty(property, uAction);
      CAddOnGUIGeneral::Unlock();
    }

    // set our replaceItems status
    if (replaceItems)
      static_cast<CFileItem *>(handle)->SetProperty("pluginreplacecontextitems", replaceItems);
    // end addContextMenuItems
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnListItem::AddStreamInfo(void* addonData, void* handle, const char* cType, const char** dictionary[2], unsigned int entries)
{
  CAddOnGUIGeneral::Lock();

  try
  {
    if (strcmpi(cType, "video") == 0)
    {
      CStreamDetailVideo* video = new CStreamDetailVideo;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string key   = dictionary[i][0];
        const std::string value = dictionary[i][1];

        if (key == "codec")
          video->m_strCodec = value;
        else if (key == "aspect")
          video->m_fAspect = (float)atof(value.c_str());
        else if (key == "width")
          video->m_iWidth = strtol(value.c_str(), NULL, 10);
        else if (key == "height")
          video->m_iHeight = strtol(value.c_str(), NULL, 10);
        else if (key == "duration")
          video->m_iDuration = strtol(value.c_str(), NULL, 10);
        else if (key == "stereomode")
          video->m_strStereoMode = value;
      }
      static_cast<CFileItem *>(handle)->GetVideoInfoTag()->m_streamDetails.AddStream(video);
    }
    else if (strcmpi(cType, "audio") == 0)
    {
      CStreamDetailAudio* audio = new CStreamDetailAudio;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string key   = dictionary[i][0];
        const std::string value = dictionary[i][1];

        if (key == "codec")
          audio->m_strCodec = value;
        else if (key == "language")
          audio->m_strLanguage = value;
        else if (key == "channels")
          audio->m_iChannels = strtol(value.c_str(), NULL, 10);
      }
      static_cast<CFileItem *>(handle)->GetVideoInfoTag()->m_streamDetails.AddStream(audio);
    }
    else if (strcmpi(cType, "subtitle") == 0)
    {
      CStreamDetailSubtitle* subtitle = new CStreamDetailSubtitle;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string key   = dictionary[i][0];
        const std::string value = dictionary[i][1];

        if (key == "language")
          subtitle->m_strLanguage = value;
      }
      static_cast<CFileItem *>(handle)->GetVideoInfoTag()->m_streamDetails.AddStream(subtitle);
    }
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetMusicInfo(void* addonData, void* handle, unsigned int type, void* data, unsigned int entries)
{
  CAddOnGUIGeneral::Lock();

  try
  {
    CFileItem* fileItem = static_cast<CFileItem *>(handle);
    switch (type)
    {
      case ADDON_MusicInfoTag____url_______________________STRING:
        fileItem->GetMusicInfoTag()->SetURL((const char*)data);
        break;
      case ADDON_MusicInfoTag____title_____________________STRING:
        fileItem->GetMusicInfoTag()->SetTitle((const char*)data);
        break;
      case ADDON_MusicInfoTag____artist____________________STRING_LIST:
      {
        std::vector<std::string> artists;
        for (unsigned int i = 0; i < entries; ++i)
          artists.push_back(((const char**)data)[i]);
        fileItem->GetMusicInfoTag()->SetArtist(artists);
        break;
      }
      case ADDON_MusicInfoTag____album_____________________STRING:
        fileItem->GetMusicInfoTag()->SetAlbum((const char*)data);
        break;
      case ADDON_MusicInfoTag____albumartist_______________STRING_LIST:
      {
        std::vector<std::string> artists;
        for (unsigned int i = 0; i < entries; ++i)
          artists.push_back(((const char**)data)[i]);
        fileItem->GetMusicInfoTag()->SetAlbumArtist(artists);
        break;
      }
      case ADDON_MusicInfoTag____genre_____________________STRING_LIST:
      {
        std::vector<std::string> genre;
        for (unsigned int i = 0; i < entries; ++i)
          genre.push_back(((const char**)data)[i]);
        fileItem->GetMusicInfoTag()->SetGenre(genre);
        break;
      }
      case ADDON_MusicInfoTag____duration__________________INT:
        fileItem->GetMusicInfoTag()->SetDuration(*(int*)data);
        break;
      case ADDON_MusicInfoTag____track_number______________INT:
        fileItem->GetMusicInfoTag()->SetTrackNumber(*(int*)data);
        break;
      case ADDON_MusicInfoTag____disc_number_______________INT:
        fileItem->GetMusicInfoTag()->SetDiscNumber(*(int*)data);
        break;
      case ADDON_MusicInfoTag____loaded____________________BOOL:
        fileItem->GetMusicInfoTag()->SetLoaded(*(bool*)data);
        break;
      case ADDON_MusicInfoTag____year______________________UINT:
        fileItem->GetMusicInfoTag()->SetYear(*(unsigned int*)data);
        break;
      case ADDON_MusicInfoTag____musicbrainztrackid________STRING:
        fileItem->GetMusicInfoTag()->SetMusicBrainzTrackID((const char*)data);
        break;
      case ADDON_MusicInfoTag____musicbrainzartistid_______STRING_LIST:
      {
        std::vector<std::string> musicbrainzartistid;
        for (unsigned int i = 0; i < entries; ++i)
          musicbrainzartistid.push_back(((const char**)data)[i]);
        fileItem->GetMusicInfoTag()->SetMusicBrainzArtistID(musicbrainzartistid);
        break;
      }
      case ADDON_MusicInfoTag____musicbrainzalbumid________STRING:
        fileItem->GetMusicInfoTag()->SetMusicBrainzAlbumID((const char*)data);
        break;
      case ADDON_MusicInfoTag____musicbrainzalbumartistid__STRING_LIST:
      {
        std::vector<std::string> musicbrainzalbumartistid;
        for (unsigned int i = 0; i < entries; ++i)
          musicbrainzalbumartistid.push_back(((const char**)data)[i]);
        fileItem->GetMusicInfoTag()->SetMusicBrainzAlbumArtistID(musicbrainzalbumartistid);
        break;
      }
      case ADDON_MusicInfoTag____mediatype_________________STRING:
      {
        if (CMediaTypes::IsValidMediaType((const char*)data))
          fileItem->GetMusicInfoTag()->SetType((const char*)data);
        else
          CLog::Log(LOGWARNING, "Invalid media type \"%s\"", (const char*)data);
        break;
      }
      case ADDON_MusicInfoTag____comment___________________STRING:
        fileItem->GetMusicInfoTag()->SetComment((const char*)data);
        break;
      case ADDON_MusicInfoTag____mood______________________STRING:
        fileItem->GetMusicInfoTag()->SetMood((const char*)data);
        break;
      case ADDON_MusicInfoTag____rating____________________FLOAT:
        fileItem->GetMusicInfoTag()->SetRating(*(float*)data);
        break;
      case ADDON_MusicInfoTag____userrating________________INT:
        fileItem->GetMusicInfoTag()->SetUserrating(*(int*)data);
        break;
      case ADDON_MusicInfoTag____votes_____________________INT:
        fileItem->GetMusicInfoTag()->SetVotes(*(int*)data);
        break;
      case ADDON_MusicInfoTag____playcount_________________INT:
        fileItem->GetMusicInfoTag()->SetPlayCount(*(int*)data);
        break;
      case ADDON_MusicInfoTag____lastplayed________________STRING_DATE_TIME:
        if (strlen((const char*)data) == 10)
        {
          SYSTEMTIME time;
          CDateTime date;
          date.SetFromDBDateTime((const char*)data);
          date.GetAsSystemTime(time);
          fileItem->GetMusicInfoTag()->SetLastPlayed(time);
        }
        break;
      case ADDON_MusicInfoTag____dateadded_________________STRING_DATE_TIME:
        if (strlen((const char*)data) == 10)
        {
          SYSTEMTIME time;
          CDateTime date;
          date.SetFromDBDateTime((const char*)data);
          date.GetAsSystemTime(time);
          fileItem->GetMusicInfoTag()->SetDateAdded(time);
        }
        break;
      case ADDON_MusicInfoTag____lyrics____________________STRING:
        fileItem->GetMusicInfoTag()->SetLyrics((const char*)data);
        break;
      case ADDON_MusicInfoTag____albumid___________________INT:
        fileItem->GetMusicInfoTag()->SetAlbumId(*(int*)data);
        break;
      case ADDON_MusicInfoTag____compilation_______________BOOL:
        fileItem->GetMusicInfoTag()->SetCompilation(*(bool*)data);
        break;
      case ADDON_MusicInfoTag____releasedate_______________STRING_DATE_TIME:
        if (strlen((const char*)data) == 10)
        {
          SYSTEMTIME time;
          CDateTime date;
          date.SetFromDBDateTime((const char*)data);
          date.GetAsSystemTime(time);
          fileItem->GetMusicInfoTag()->SetReleaseDate(time);
        }
        break;
      case ADDON_MusicInfoTag____albumreleasetype__________UINT:
        fileItem->GetMusicInfoTag()->SetAlbumReleaseType(*(CAlbum::ReleaseType*)data);
        break;
      default:
        break;
    }
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetVideoInfo(void* addonData, void* handle, unsigned int type, void*data, unsigned int entries)
{
  CAddOnGUIGeneral::Lock();

  try
  {
    CFileItem* fileItem = static_cast<CFileItem *>(handle);
    switch (type)
    {
    case ADDON_VideoInfoTag____director__________________STRING_LIST:
    {
      std::vector<std::string> list;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string value = ((const char**)data)[i];
        list.push_back(value);
      }
      fileItem->GetVideoInfoTag()->SetDirector(list);
      break;
    }
    case ADDON_VideoInfoTag____writing_credits___________STRING_LIST:
    {
      std::vector<std::string> list;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string value = ((const char**)data)[i];
        list.push_back(value);
      }
      fileItem->GetVideoInfoTag()->SetWritingCredits(list);
      break;
    }
    case ADDON_VideoInfoTag____genre_____________________STRING_LIST:
    {
      std::vector<std::string> list;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string value = ((const char**)data)[i];
        list.push_back(value);
      }
      fileItem->GetVideoInfoTag()->SetGenre(list);
      break;
    }
    case ADDON_VideoInfoTag____country___________________STRING_LIST:
    {
      std::vector<std::string> list;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string value = ((const char**)data)[i];
        list.push_back(value);
      }
      fileItem->GetVideoInfoTag()->SetCountry(list);
      break;
    }
    case ADDON_VideoInfoTag____tagline___________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetTagLine(value);
      break;
    }
    case ADDON_VideoInfoTag____plot_outline______________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetPlotOutline(value);
      break;
    }
    case ADDON_VideoInfoTag____plot______________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetPlot(value);
      break;
    }
    case ADDON_VideoInfoTag____title_____________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetTitle(value);
      break;
    }
    case ADDON_VideoInfoTag____votes_____________________INT:
    {
      fileItem->GetVideoInfoTag()->SetVotes(*(int*)data);
      break;
    }
    case ADDON_VideoInfoTag____studio____________________STRING_LIST:
    {
      std::vector<std::string> list;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string value = ((const char**)data)[i];
        list.push_back(value);
      }
      fileItem->GetVideoInfoTag()->SetStudio(list);
      break;
    }
    case ADDON_VideoInfoTag____trailer___________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetTitle(value);
      break;
    }
    case ADDON_VideoInfoTag____cast______________________DATA_LIST:
    {
      fileItem->GetVideoInfoTag()->m_cast.clear();
      for (unsigned int i = 0; i < entries; ++i)
      {
        SActorInfo info;
        info.strName  = ((ADDON_VideoInfoTag__cast__DATA_STRUCT*)data)[i].name;
        info.strRole  = ((ADDON_VideoInfoTag__cast__DATA_STRUCT*)data)[i].role;
        info.thumb    = ((ADDON_VideoInfoTag__cast__DATA_STRUCT*)data)[i].thumbnail;
        info.order    = ((ADDON_VideoInfoTag__cast__DATA_STRUCT*)data)[i].order;
        fileItem->GetVideoInfoTag()->m_cast.push_back(info);
      }
      break;
    }
    case ADDON_VideoInfoTag____set_______________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetSet(value);
      break;
    }
    case ADDON_VideoInfoTag____setid_____________________INT:
    {
      fileItem->GetVideoInfoTag()->m_iSetId = *(int*)data;
      break;
    }
    case ADDON_VideoInfoTag____setoverview_______________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetSetOverview(value);
      break;
    }
    case ADDON_VideoInfoTag____tag_______________________STRING_LIST:
    {
      std::vector<std::string> list;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string value = ((const char**)data)[i];
        list.push_back(value);
      }
      fileItem->GetVideoInfoTag()->SetTags(list);
      break;
    }
    case ADDON_VideoInfoTag____duration__________________UINT:
      fileItem->GetVideoInfoTag()->m_duration = (*(unsigned int*)data);
      break;
    case ADDON_VideoInfoTag____file______________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetFile(value);
      break;
    }
    case ADDON_VideoInfoTag____path______________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetPath(value);
      break;
    }
    case ADDON_VideoInfoTag____imdbnumber________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetIMDBNumber(value);
      break;
    }
    case ADDON_VideoInfoTag____mpaa_rating_______________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetMPAARating(value);
      break;
    }
    case ADDON_VideoInfoTag____filename_and_path_________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetFileNameAndPath(value);
      break;
    }
    case ADDON_VideoInfoTag____original_title____________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetOriginalTitle(value);
      break;
    }
    case ADDON_VideoInfoTag____sorttitle_________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetSortTitle(value);
      break;
    }
    case ADDON_VideoInfoTag____episode_guide_____________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetEpisodeGuide(value);
      break;
    }
    case ADDON_VideoInfoTag____premiered_________________STRING_DATE:
    {
      CDateTime date;
      date.SetFromDBDate((const char*)data);
      fileItem->GetVideoInfoTag()->m_premiered = date;
      break;
    }
    case ADDON_VideoInfoTag____status____________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetStatus(value);
      break;
    }
    case ADDON_VideoInfoTag____production_code___________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetProductionCode(value);
      break;
    }
    case ADDON_VideoInfoTag____first_aired_______________STRING_DATE:
    {
      CDateTime date;
      date.SetFromDBDate((const char*)data);
      fileItem->GetVideoInfoTag()->m_firstAired = date;
      break;
    }
    case ADDON_VideoInfoTag____show_title________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetShowTitle(value);
      break;
    }
    case ADDON_VideoInfoTag____album_____________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetAlbum(value);
      break;
    }
    case ADDON_VideoInfoTag____artist____________________STRING_LIST:
    {
      std::vector<std::string> list;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string value = ((const char**)data)[i];
        list.push_back(value);
      }
      fileItem->GetVideoInfoTag()->SetArtist(list);
      break;
    }
    case ADDON_VideoInfoTag____playcount_________________INT:
      fileItem->GetVideoInfoTag()->m_playCount = *(int*)data;
      break;
    case ADDON_VideoInfoTag____lastplayed________________STRING_DATE_TIME:
    {
      CDateTime date;
      date.SetFromDBDateTime((const char*)data);
      fileItem->GetVideoInfoTag()->m_lastPlayed = date;
      break;
    }
    case ADDON_VideoInfoTag____top250____________________INT:
      fileItem->GetVideoInfoTag()->m_iTop250 = *(int*)data;
      break;
    case ADDON_VideoInfoTag____dbid______________________INT:
      fileItem->GetVideoInfoTag()->m_iDbId = *(int*)data;
      break;
    case ADDON_VideoInfoTag____year______________________INT:
      fileItem->GetVideoInfoTag()->m_iYear = *(int*)data;
      break;
    case ADDON_VideoInfoTag____season____________________INT:
      fileItem->GetVideoInfoTag()->m_iSeason = *(int*)data;
      break;
    case ADDON_VideoInfoTag____episode___________________INT:
      fileItem->GetVideoInfoTag()->m_iEpisode = *(int*)data;
      break;
    case ADDON_VideoInfoTag____unique_id_________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->SetUniqueId(value);
      break;
    }
    case ADDON_VideoInfoTag____rating____________________FLOAT:
      fileItem->GetVideoInfoTag()->SetRating(*(float*)data);
      break;
    case ADDON_VideoInfoTag____user_rating_______________INT:
      fileItem->GetVideoInfoTag()->SetUserrating(*(int*)data);
      break;
    case ADDON_VideoInfoTag____db_id_____________________INT:
      fileItem->GetVideoInfoTag()->m_iDbId = *(int*)data;
      break;
    case ADDON_VideoInfoTag____file_id___________________INT:
      fileItem->GetVideoInfoTag()->m_iFileId = *(int*)data;
      break;
    case ADDON_VideoInfoTag____track_____________________INT:
      fileItem->GetVideoInfoTag()->m_iTrack = *(int*)data;
      break;
    case ADDON_VideoInfoTag____show_link_________________STRING_LIST:
    {
      std::vector<std::string> list;
      for (unsigned int i = 0; i < entries; ++i)
      {
        const std::string value = ((const char**)data)[i];
        list.push_back(value);
      }
      fileItem->GetVideoInfoTag()->SetShowLink(list);
      break;
    }
    case ADDON_VideoInfoTag____resume____________________DATA :
      fileItem->GetVideoInfoTag()->m_resumePoint.timeInSeconds      = ((ADDON_VideoInfoTag_Resume*)data)->position;
      fileItem->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds = ((ADDON_VideoInfoTag_Resume*)data)->total;
      break;
    case ADDON_VideoInfoTag____tvshow_id_________________INT:
      fileItem->GetVideoInfoTag()->m_iIdShow = *(int*)data;
      break;
    case ADDON_VideoInfoTag____date_added________________STRING_DATE_TIME:
    {
      CDateTime date;
      date.SetFromDBDateTime((const char*)data);
      fileItem->GetVideoInfoTag()->m_dateAdded = date;
      break;
    }
    case ADDON_VideoInfoTag____type______________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->GetVideoInfoTag()->m_type = CMediaTypes::FromString(value);
      break;
    }
    case ADDON_VideoInfoTag____season_id_________________INT:
      fileItem->GetVideoInfoTag()->m_iIdSeason = *(int*)data;
      break;
    case ADDON_VideoInfoTag____special_sort_season_______INT:
      fileItem->GetVideoInfoTag()->m_iSpecialSortSeason = *(int*)data;
      break;
    case ADDON_VideoInfoTag____special_sort_episode______INT:
      fileItem->GetVideoInfoTag()->m_iSpecialSortEpisode = *(int*)data;
      break;
    default:
      break;
    }
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

void CAddOnListItem::SetPictureInfo(void* addonData, void* handle, unsigned int type, void* data, unsigned int entries)
{
  CAddOnGUIGeneral::Lock();

  try
  {
    CFileItem* fileItem = static_cast<CFileItem *>(handle);
    switch (type)
    {
    case ADDON_PictureInfoTag__count_____________________INT:
      fileItem->m_iprogramCount = *(int*)data;
      break;
    case ADDON_PictureInfoTag__size______________________INT:
      fileItem->m_dwSize = *(int*)data;
      break;
    case ADDON_PictureInfoTag__title_____________________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->m_strTitle = value;
      break;
    }
    case ADDON_PictureInfoTag__picturepath_______________STRING:
    {
      const std::string value = (const char*)data;
      fileItem->SetPath(value);
      break;
    }
    case ADDON_PictureInfoTag__date______________________STRING_DATE_TIME:
    {
      const std::string value = (const char*)data;
      if (strlen(value.c_str()) == 10)
      {
        int year = atoi(value.substr(value.size() - 4).c_str());
        int month = atoi(value.substr(3, 4).c_str());
        int day = atoi(value.substr(0, 2).c_str());
        fileItem->m_dateTime.SetDate(year, month, day);
      }
      break;
    }
    case ADDON_PictureInfoTag__datetime__________________STRING_DATE_TIME:
    {
      const std::string value = (const char*)data;
      fileItem->GetPictureInfoTag()->SetInfo(SLIDE_EXIF_DATE_TIME, value);
      break;
    }
    case ADDON_PictureInfoTag__resolution________________DATA:
    {
      std::string dimension = StringUtils::Format("%i,%i", ((int*)data)[0], ((int*)data)[1]);
      fileItem->GetPictureInfoTag()->SetInfo(SLIDE_RESOLUTION, dimension);
      break;
    }
    default:
      break;
    }
  }
  HANDLE_ADDON_EXCEPTION

  CAddOnGUIGeneral::Unlock();
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
