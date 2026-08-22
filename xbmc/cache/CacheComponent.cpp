/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CacheComponent.h"

#include "FileSystemCache.h"
#include "ServiceBroker.h"
#include "filesystem/Zip.h"

CCacheComponent::CCacheComponent() : m_zipCache(std::make_unique<CFileSystemCache<SZipEntry>>())
{
}

CCacheComponent::~CCacheComponent()
{
  Deinit();
}

void CCacheComponent::Init()
{
  CServiceBroker::RegisterCacheComponent(this);
}

void CCacheComponent::Deinit()
{
  CServiceBroker::UnregisterCacheComponent();
}

CFileSystemCache<SZipEntry>& CCacheComponent::GetZipCache()
{
  return *m_zipCache;
}
