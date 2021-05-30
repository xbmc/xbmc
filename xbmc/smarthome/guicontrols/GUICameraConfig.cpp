/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUICameraConfig.h"

#include "smarthome/ImageSubscriptionKey.h"
#include "smarthome/guicontrols/GUICameraControl.h"
#include "utils/Geometry.h" //! @todo For IDE

using namespace KODI;
using namespace SMART_HOME;

CGUICameraConfig::CGUICameraConfig() : m_topic(), m_imageTransport(DEFAULT_IMAGE_TRANSPORT)
{
}

CGUICameraConfig::CGUICameraConfig(const CGUICameraConfig& other)
  : m_topic(other.m_topic),
    m_imageTransport(other.m_imageTransport)
{
}

CGUICameraConfig::~CGUICameraConfig() = default;

void CGUICameraConfig::SetImageTransport(const std::string& imageTransport)
{
  m_imageTransport = imageTransport.empty() ? DEFAULT_IMAGE_TRANSPORT : imageTransport;
}

void CGUICameraConfig::Reset()
{
  m_topic.clear();
  m_imageTransport = DEFAULT_IMAGE_TRANSPORT;
}
