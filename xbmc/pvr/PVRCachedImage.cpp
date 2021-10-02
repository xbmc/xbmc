/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRCachedImage.h"

#include "TextureDatabase.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace PVR;

CPVRCachedImage::CPVRCachedImage(const std::string& owner) : m_owner(owner)
{
}

CPVRCachedImage::CPVRCachedImage(const std::string& clientImage, const std::string& owner)
  : m_clientImage(clientImage), m_owner(owner)
{
  UpdateLocalImage();
}

bool CPVRCachedImage::operator==(const CPVRCachedImage& right) const
{
  return (this == &right) || (m_clientImage == right.m_clientImage &&
                              m_localImage == right.m_localImage && m_owner == right.m_owner);
}

bool CPVRCachedImage::operator!=(const CPVRCachedImage& right) const
{
  return !(*this == right);
}

void CPVRCachedImage::SetClientImage(const std::string& image)
{
  if (StringUtils::StartsWith(image, "image://"))
  {
    CLog::LogF(LOGERROR, "Not allowed to call this method with an image URL");
    return;
  }

  if (m_owner.empty())
  {
    CLog::LogF(LOGERROR, "Empty owner");
    return;
  }

  m_clientImage = image;
  UpdateLocalImage();
}

void CPVRCachedImage::SetOwner(const std::string& owner)
{
  if (m_owner != owner)
  {
    m_owner = owner;
    UpdateLocalImage();
  }
}

void CPVRCachedImage::UpdateLocalImage()
{
  if (m_clientImage.empty())
    m_localImage.clear();
  else
    m_localImage = CTextureUtils::GetWrappedImageURL(m_clientImage, m_owner);
}
