/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUICameraConfig.h"

#include "smarthome/guicontrols/GUICameraControl.h"
#include "utils/Geometry.h" //! @todo For IDE

using namespace KODI;
using namespace SMART_HOME;

CGUICameraConfig::CGUICameraConfig() = default;

CGUICameraConfig::CGUICameraConfig(const CGUICameraConfig& other) : m_topic(other.m_topic)
{
}

CGUICameraConfig::~CGUICameraConfig() = default;

void CGUICameraConfig::Reset()
{
  m_topic.clear();
}
