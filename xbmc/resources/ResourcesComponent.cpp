/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ResourcesComponent.h"

#include "LocalizeStrings.h"
#include "language/LangInfo.h"

#include <memory>

CResourcesComponent::CResourcesComponent()
  : m_localizeStrings(std::make_unique<CLocalizeStrings>()),
    m_langInfo(std::make_unique<KODI::LANGUAGE::CLangInfo>())
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

KODI::LANGUAGE::CLangInfo& CResourcesComponent::GetLangInfo()
{
  return *m_langInfo;
}
