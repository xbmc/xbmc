/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRThumbLoader.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "TextureCacheJob.h"
#include "pictures/Picture.h"
#include "pvr/PVRManager.h"
#include "pvr/filesystem/PVRGUIDirectory.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <ctime>

using namespace PVR;

bool CPVRThumbLoader::LoadItem(CFileItem* item)
{
  bool result = LoadItemCached(item);
  result |= LoadItemLookup(item);
  return result;
}

bool CPVRThumbLoader::LoadItemCached(CFileItem* item)
{
  return FillThumb(*item);
}

bool CPVRThumbLoader::LoadItemLookup(CFileItem* item)
{
  return false;
}

void CPVRThumbLoader::OnLoaderFinish()
{
  if (m_bInvalidated)
  {
    m_bInvalidated = false;
    CServiceBroker::GetPVRManager().PublishEvent(PVREvent::ChannelGroupsInvalidated);
  }
  CThumbLoader::OnLoaderFinish();
}

void CPVRThumbLoader::ClearCachedImage(CFileItem& item)
{
  const std::string thumb = item.GetArt("thumb");
  if (!thumb.empty())
  {
    CTextureCache::GetInstance().ClearCachedImage(thumb);
    if (m_textureDatabase->Open())
    {
      m_textureDatabase->ClearTextureForPath(item.GetPath(), "thumb");
      m_textureDatabase->Close();
    }
    item.SetArt("thumb", "");
    m_bInvalidated = true;
  }
}

void CPVRThumbLoader::ClearCachedImages(CFileItemList& items)
{
  for (auto& item : items)
    ClearCachedImage(*item);
}

bool CPVRThumbLoader::FillThumb(CFileItem& item)
{
  // see whether we have a cached image for this item
  std::string thumb = GetCachedImage(item, "thumb");
  if (thumb.empty())
  {
    if (item.IsPVRChannelGroup())
      thumb = CreateChannelGroupThumb(item);
    else
      CLog::LogF(LOGERROR, "Unsupported PVR item '{}'", item.GetPath());

    if (!thumb.empty())
    {
      SetCachedImage(item, "thumb", thumb);
      m_bInvalidated = true;
    }
  }

  if (thumb.empty())
    return false;

  item.SetArt("thumb", thumb);
  return true;
}

std::string CPVRThumbLoader::CreateChannelGroupThumb(const CFileItem& channelGroupItem)
{
  const CPVRGUIDirectory channelGroupDir(channelGroupItem.GetPath());
  CFileItemList channels;
  if (channelGroupDir.GetChannelsDirectory(channels))
  {
    std::vector<std::string> channelIcons;
    for (const auto& channel : channels)
    {
      const std::string& icon = channel->GetArt("icon");
      if (!icon.empty())
        channelIcons.emplace_back(icon);

      if (channelIcons.size() == 9) // limit number of tiles
        break;
    }

    std::string thumb = StringUtils::Format(
        "%s?ts=%d", // append timestamp to Thumb URL to enforce texture refresh
        CTextureUtils::GetWrappedImageURL(channelGroupItem.GetPath(), "pvr"), std::time(nullptr));
    const std::string relativeCacheFile = CTextureCache::GetCacheFile(thumb) + ".png";
    if (CPicture::CreateTiledThumb(channelIcons, CTextureCache::GetCachedPath(relativeCacheFile)))
    {
      CTextureDetails details;
      details.file = relativeCacheFile;
      details.width = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_imageRes;
      details.height = details.width;
      CTextureCache::GetInstance().AddCachedTexture(thumb, details);
      return thumb;
    }
  }

  return {};
}
