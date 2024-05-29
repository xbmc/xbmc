/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupImageFileLoader.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "filesystem/PVRGUIDirectory.h"
#include "guilib/Texture.h"
#include "imagefiles/ImageFileURL.h"
#include "pictures/Picture.h"
#include "utils/log.h"

bool PVR::CPVRChannelGroupImageFileLoader::CanLoad(const std::string& specialType) const
{
  return specialType == "pvr";
}

std::unique_ptr<CTexture> PVR::CPVRChannelGroupImageFileLoader::Load(
    const IMAGE_FILES::CImageFileURL& imageFile) const
{
  const CPVRGUIDirectory channelGroupDir(imageFile.GetTargetFile());
  CFileItemList channels;
  if (!channelGroupDir.GetChannelsDirectory(channels))
  {
    return {};
  }

  std::vector<std::string> channelIcons;
  for (const auto& channel : channels)
  {
    const std::string& icon = channel->GetArt("icon");
    if (!icon.empty())
      channelIcons.emplace_back(IMAGE_FILES::CImageFileURL(icon).GetTargetFile());

    if (channelIcons.size() == 9) // limit number of tiles
      break;
  }

  return CPicture::CreateTiledThumb(channelIcons);
}
