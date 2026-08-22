/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabaseActorCache.h"

#include "dbwrappers/dataset.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

namespace KODI::VIDEO
{

std::string CActorCache::StoredName(const std::string& name)
{
  std::string stored{name};
  StringUtils::Trim(stored);
  return stored.substr(0, 255);
}

std::string CActorCache::Key(const std::string& storedName)
{
  std::string key{storedName};
  StringUtils::ToLower(key);
  return key;
}

void CActorCache::Load(dbiplus::Dataset* dataset)
{
  m_actors.clear();
  m_loaded = false;
  if (!dataset)
  {
    CLog::LogF(LOGERROR, "no dataset - actors will be looked up individually");
    return;
  }

  try
  {
    dataset->query("SELECT actor_id, name, art_urls FROM actor");
    while (!dataset->eof())
    {
      Set(StoredName(dataset->fv(1).get_asString()), dataset->fv(0).get_asInt(),
          dataset->fv(2).get_asString());
      dataset->next();
    }
    dataset->close();
    m_loaded = true;
    CLog::LogF(LOGDEBUG, "holding {} actors", m_actors.size());
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "failed - actors will be looked up individually");
    m_actors.clear();
  }
}

void CActorCache::Release()
{
  m_loaded = false;
  m_actors.clear();
}

const CActorCache::Actor* CActorCache::Find(const std::string& storedName) const
{
  const auto actor{m_actors.find(Key(storedName))};
  return actor != m_actors.cend() ? &actor->second : nullptr;
}

void CActorCache::Set(const std::string& storedName, int id, const std::string& artUrls)
{
  m_actors.insert_or_assign(Key(storedName), Actor{id, artUrls});
}

CActorCacheScope::CActorCacheScope(CVideoDatabase& db) : m_db(db)
{
  m_db.LoadActorCache();
}

CActorCacheScope::~CActorCacheScope()
{
  m_db.ReleaseActorCache();
}

} // namespace KODI::VIDEO
