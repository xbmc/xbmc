/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ResourcesComponent.h"

#include "LocalizeStrings.h"

#include <memory>

CResourcesComponent::CResourcesComponent() : m_localizeStrings(std::make_unique<CLocalizeStrings>())
{
}

CResourcesComponent::~CResourcesComponent()
{
  Deinit();
}

void CResourcesComponent::Init()
{
}

void CResourcesComponent::Deinit()
{
  m_localizeStrings->Clear();
}

CLocalizeStrings& CResourcesComponent::GetLocalizeStrings()
{
  return *m_localizeStrings;
}
