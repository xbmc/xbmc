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
  std::vector<std::string> channelIcons{GetChannelGroupIcons(imageFile.GetTargetFile())};
  for (auto& icon : channelIcons)
    icon = IMAGE_FILES::CImageFileURL(icon).GetTargetFile();

  return CPicture::CreateTiledThumb(channelIcons);
}

std::vector<std::string> PVR::CPVRChannelGroupImageFileLoader::GetChannelGroupIcons(
    const std::string& groupPath)
{
  std::vector<std::string> icons;
  const CPVRGUIDirectory channelGroupDir(groupPath);
  CFileItemList channels;
  if (!channelGroupDir.GetChannelsDirectory(channels))
    return icons;

  for (const auto& channel : channels)
  {
    const std::string& icon{channel->GetArt("thumb")};
    if (!icon.empty())
      icons.emplace_back(icon);

    if (icons.size() == 9) // limit number of tiles
      break;
  }
  return icons;
}
