/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRThumbLoader.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "imagefiles/ImageFileURL.h"
#include "pvr/PVRChannelGroupImageFileLoader.h"
#include "pvr/PVRManager.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

namespace PVR
{
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
    CServiceBroker::GetTextureCache()->ClearCachedImage(thumb);
    if (m_textureDatabase->Open())
    {
      m_textureDatabase->ClearTextureForPath(item.GetPath(), "thumb");
      m_textureDatabase->Close();
    }
    item.SetArt("thumb", "");
    m_bInvalidated = true;
  }
}

void CPVRThumbLoader::ClearCachedImages(const CFileItemList& items)
{
  for (auto& item : items)
    ClearCachedImage(*item);
}

bool CPVRThumbLoader::FillThumb(CFileItem& item)
{
  if (!item.IsPVRChannelGroup())
  {
    CLog::LogF(LOGERROR, "Unsupported PVR item '{}'", item.GetPath());
    return false;
  }

  const std::string thumb{GetChannelGroupThumbURL(item)};
  const std::string cachedThumb{GetCachedImage(item, "thumb")};
  if (thumb != cachedThumb)
  {
    if (!cachedThumb.empty())
      CServiceBroker::GetTextureCache()->ClearCachedImage(cachedThumb);

    SetCachedImage(item, "thumb", thumb);
    m_bInvalidated = true;
  }

  item.SetArt("thumb", thumb);
  return true;
}

std::string CPVRThumbLoader::GetChannelGroupThumbURL(const CFileItem& channelGroupItem) const
{
  KODI::UTILITY::CDigest digest{KODI::UTILITY::CDigest::Type::MD5};
  for (const auto& icon :
       CPVRChannelGroupImageFileLoader::GetChannelGroupIcons(channelGroupItem.GetPath()))
    digest.Update(icon);

  return StringUtils::Format("{}?icons={}", // URL changes with the tiled icons, enforcing a refresh
                             IMAGE_FILES::URLFromFile(channelGroupItem.GetPath(), "pvr"),
                             digest.Finalize());
}

} // namespace PVR
