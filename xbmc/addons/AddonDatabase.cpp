/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonDatabase.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "addons/AddonBuilder.h"
#include "addons/AddonManager.h"
#include "dbwrappers/dataset.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "DllLibCPluff.h"
#include "XBDateTime.h"

using namespace ADDON;

static std::string SerializeMetadata(const IAddon& addon)
{
  CVariant variant;
  variant["author"] = addon.Author();
  variant["disclaimer"] = addon.Disclaimer();
  variant["broken"] = addon.Broken();
  variant["size"] = addon.PackageSize();

  variant["path"] = addon.Path();
  variant["fanart"] = addon.FanArt();
  variant["icon"] = addon.Icon();

  variant["screenshots"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& item : addon.Screenshots())
    variant["screenshots"].push_back(item);

  variant["extensions"] = CVariant(CVariant::VariantTypeArray);
  variant["extensions"].push_back(ADDON::TranslateType(addon.Type(), false));

  variant["dependencies"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& kv : addon.GetDeps())
  {
    CVariant dep(CVariant::VariantTypeObject);
    dep["addonId"] = kv.first;
    dep["version"] = kv.second.first.asString();
    dep["optional"] = kv.second.second;
    variant["dependencies"].push_back(std::move(dep));
  }

  variant["extrainfo"] = CVariant(CVariant::VariantTypeArray);
  for (const auto& kv : addon.ExtraInfo())
  {
    CVariant info(CVariant::VariantTypeObject);
    info["key"] = kv.first;
    info["value"] = kv.second;
    variant["extrainfo"].push_back(std::move(info));
  }

  return CJSONVariantWriter::Write(variant, true);
}

static void DeserializeMetadata(const std::string& document, CAddonBuilder& builder)
{
  CVariant variant = CJSONVariantParser::Parse(document);

  builder.SetAuthor(variant["author"].asString());
  builder.SetDisclaimer(variant["disclaimer"].asString());
  builder.SetBroken(variant["broken"].asString());
  builder.SetPackageSize(variant["size"].asUnsignedInteger());

  builder.SetPath(variant["path"].asString());
  builder.SetFanart(variant["fanart"].asString());
  builder.SetIcon(variant["icon"].asString());

  std::vector<std::string> screenshots;
  for (auto it = variant["screenshots"].begin_array(); it != variant["screenshots"].end_array(); ++it)
    screenshots.push_back(it->asString());
  builder.SetScreenshots(std::move(screenshots));

  builder.SetType(TranslateType(variant["extensions"][0].asString()));

  ADDONDEPS deps;
  for (auto it = variant["dependencies"].begin_array(); it != variant["dependencies"].end_array(); ++it)
  {
    AddonVersion version((*it)["version"].asString());
    deps.emplace((*it)["addonId"].asString(), std::make_pair(std::move(version), (*it)["optional"].asBoolean()));
  }
  builder.SetDependencies(std::move(deps));

  InfoMap extraInfo;
  for (auto it = variant["extrainfo"].begin_array(); it != variant["extrainfo"].end_array(); ++it)
    extraInfo.emplace((*it)["key"].asString(), (*it)["value"].asString());
  builder.SetExtrainfo(std::move(extraInfo));
}

CAddonDatabase::CAddonDatabase()
{
}

CAddonDatabase::~CAddonDatabase()
{
}

bool CAddonDatabase::Open()
{
  return CDatabase::Open();
}

int CAddonDatabase::GetMinSchemaVersion() const
{
  return 15;
}

int CAddonDatabase::GetSchemaVersion() const
{
  return 26;
}

void CAddonDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create addons table");
  m_pDS->exec("CREATE TABLE addons ("
      "id INTEGER PRIMARY KEY,"
      "metadata BLOB,"
      "addonID TEXT NOT NULL,"
      "version TEXT NOT NULL,"
      "name TEXT NOT NULL,"
      "summary TEXT NOT NULL,"
      "description TEXT NOT NULL)");

  CLog::Log(LOGINFO, "create repo table");
  m_pDS->exec("CREATE TABLE repo (id integer primary key, addonID text,"
              "checksum text, lastcheck text, version text)\n");

  CLog::Log(LOGINFO, "create addonlinkrepo table");
  m_pDS->exec("CREATE TABLE addonlinkrepo (idRepo integer, idAddon integer)\n");

  CLog::Log(LOGINFO, "create broken table");
  m_pDS->exec("CREATE TABLE broken (id integer primary key, addonID text, reason text)\n");

  CLog::Log(LOGINFO, "create blacklist table");
  m_pDS->exec("CREATE TABLE blacklist (id integer primary key, addonID text)\n");

  CLog::Log(LOGINFO, "create package table");
  m_pDS->exec("CREATE TABLE package (id integer primary key, addonID text, filename text, hash text)\n");

  CLog::Log(LOGINFO, "create installed table");
  m_pDS->exec("CREATE TABLE installed (id INTEGER PRIMARY KEY, addonID TEXT UNIQUE, "
      "enabled BOOLEAN, installDate TEXT, lastUpdated TEXT, lastUsed TEXT, "
      "origin TEXT NOT NULL DEFAULT '') \n");
}

void CAddonDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxAddons ON addons(addonID)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_addonlinkrepo_1 ON addonlinkrepo ( idAddon, idRepo )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_addonlinkrepo_2 ON addonlinkrepo ( idRepo, idAddon )\n");
  m_pDS->exec("CREATE UNIQUE INDEX idxBroken ON broken(addonID)");
  m_pDS->exec("CREATE UNIQUE INDEX idxBlack ON blacklist(addonID)");
  m_pDS->exec("CREATE UNIQUE INDEX idxPackage ON package(filename)");
}

void CAddonDatabase::UpdateTables(int version)
{
  if (version < 16)
  {
    m_pDS->exec("CREATE TABLE package (id integer primary key, addonID text, filename text, hash text)\n");
  }
  if (version < 17)
  {
    m_pDS->exec("ALTER TABLE repo ADD version text DEFAULT '0.0.0'");
  }
  if (version == 17)
  {
    /** remove all add-ons because the previous upgrade created dupes in it's first version */
    m_pDS->exec("DELETE FROM addon");
  }
  if (version < 19)
  {
    m_pDS->exec("CREATE TABLE system (id integer primary key, addonID text)\n");
  }
  if (version < 20)
  {
    m_pDS->exec("CREATE TABLE tmp (id INTEGER PRIMARY KEY, addonID TEXT)");
    m_pDS->exec("INSERT INTO tmp (addonID) SELECT addonID FROM blacklist GROUP BY addonID");
    m_pDS->exec("DROP TABLE blacklist");
    m_pDS->exec("ALTER TABLE tmp RENAME TO blacklist");
  }
  if (version < 21)
  {
    m_pDS->exec("CREATE TABLE installed (id INTEGER PRIMARY KEY, addonID TEXT UNIQUE, "
        "enabled BOOLEAN, installDate TEXT, lastUpdated TEXT, lastUsed TEXT) \n");

    //Ugly hack incoming! As the addon manager isnt created yet, we need to start up our own copy
    //cpluff to find the currently enabled addons.
    auto cpluff = std::unique_ptr<DllLibCPluff>(new DllLibCPluff());
    cpluff->Load();
    cp_status_t status;
    status = cpluff->init();
    if (status != CP_OK)
    {
      CLog::Log(LOGERROR, "AddonDatabase: Upgrade failed. cp_init() returned status: %i", status);
      return;
    }

    cp_context_t* cp_context = cpluff->create_context(&status);

    status = cpluff->register_pcollection(cp_context, CSpecialProtocol::TranslatePath("special://home/addons").c_str());
    if (status != CP_OK)
    {
      CLog::Log(LOGERROR, "AddonDatabase: Upgrade failed. cp_register_pcollection() returned status: %i", status);
      return;
    }

    status = cpluff->register_pcollection(cp_context, CSpecialProtocol::TranslatePath("special://xbmc/addons").c_str());
    if (status != CP_OK)
    {
      CLog::Log(LOGERROR, "AddonDatabase: Upgrade failed. cp_register_pcollection() returned status: %i", status);
      return;
    }

    status = cpluff->register_pcollection(cp_context, CSpecialProtocol::TranslatePath("special://xbmcbin/addons").c_str());
    if (status != CP_OK)
    {
      CLog::Log(LOGERROR, "AddonDatabase: Upgrade failed. cp_register_pcollection() returned status: %i", status);
      return;
    }

    cpluff->scan_plugins(cp_context, CP_SP_UPGRADE);

    std::string systemPath = CSpecialProtocol::TranslatePath("special://xbmc/addons");
    std::string now = CDateTime::GetCurrentDateTime().GetAsDBDateTime();
    BeginTransaction();
    int n;
    cp_plugin_info_t** cp_addons = cpluff->get_plugins_info(cp_context, &status, &n);
    for (int i = 0; i < n; ++i)
    {
      const char* id = cp_addons[i]->identifier;
      // To not risk enabling something user didn't enable, assume that everything from the systems
      // directory is new for this release and set them to disabled.
      bool inSystem = StringUtils::StartsWith(cp_addons[i]->plugin_path, systemPath);
      m_pDS->exec(PrepareSQL("INSERT INTO installed(addonID, enabled, installDate) VALUES "
          "('%s', NOT %d AND NOT EXISTS (SELECT * FROM disabled WHERE addonID='%s'), '%s')",
          id, inSystem, id, now.c_str()));
    }
    cpluff->release_info(cp_context, cp_addons);
    CommitTransaction();

    m_pDS->exec("DROP TABLE disabled");
  }
  if (version < 22)
  {
    m_pDS->exec("DROP TABLE system");
  }
  if (version < 24)
  {
    m_pDS->exec("DELETE FROM addon");
    m_pDS->exec("DELETE FROM addonextra");
    m_pDS->exec("DELETE FROM dependencies");
    m_pDS->exec("DELETE FROM addonlinkrepo");
    m_pDS->exec("DELETE FROM repo");
  }
  if (version < 25)
  {
    m_pDS->exec("ALTER TABLE installed ADD origin TEXT NOT NULL DEFAULT ''");
  }
  if (version < 26)
  {
    m_pDS->exec("DROP TABLE addon");
    m_pDS->exec("DROP TABLE addonextra");
    m_pDS->exec("DROP TABLE dependencies");
    m_pDS->exec("DELETE FROM addonlinkrepo");
    m_pDS->exec("DELETE FROM repo");
    m_pDS->exec("CREATE TABLE addons ("
        "id INTEGER PRIMARY KEY,"
        "metadata BLOB,"
        "addonID TEXT NOT NULL,"
        "version TEXT NOT NULL,"
        "name TEXT NOT NULL,"
        "summary TEXT NOT NULL,"
        "description TEXT NOT NULL)");
  }
}

void CAddonDatabase::SyncInstalled(const std::set<std::string>& ids,
                                   const std::set<std::string>& system,
                                   const std::set<std::string>& optional)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    std::set<std::string> db;
    m_pDS->query(PrepareSQL("SELECT addonID FROM installed"));
    while (!m_pDS->eof())
    {
      db.insert(m_pDS->fv(0).get_asString());
      m_pDS->next();
    }
    m_pDS->close();

    std::set<std::string> added;
    std::set<std::string> removed;
    std::set_difference(ids.begin(), ids.end(), db.begin(), db.end(), std::inserter(added, added.end()));
    std::set_difference(db.begin(), db.end(), ids.begin(), ids.end(), std::inserter(removed, removed.end()));

    for (const auto& id : added)
      CLog::Log(LOGDEBUG, "CAddonDatabase: %s has been installed.", id.c_str());

    for (const auto& id : removed)
      CLog::Log(LOGDEBUG, "CAddonDatabase: %s has been uninstalled.", id.c_str());

    std::string now = CDateTime::GetCurrentDateTime().GetAsDBDateTime();
    BeginTransaction();
    for (const auto& id : added)
    {
      int enable = 0;

      if (system.find(id) != system.end() || optional.find(id) != optional.end())
        enable = 1;

      m_pDS->exec(PrepareSQL("INSERT INTO installed(addonID, enabled, installDate) "
        "VALUES('%s', %d, '%s')", id.c_str(), enable, now.c_str()));
    }

    for (const auto& id : removed)
    {
      m_pDS->exec(PrepareSQL("DELETE FROM installed WHERE addonID='%s'", id.c_str()));
      RemoveAddonFromBlacklist(id);
      DeleteRepository(id);
    }

    for (const auto& id : system)
    {
      m_pDS->exec(PrepareSQL("UPDATE installed SET enabled=1 WHERE addonID='%s'", id.c_str()));
      // Set origin *only* for addons that do not have one yet as it may have been changed by an update.
      m_pDS->exec(PrepareSQL("UPDATE installed SET origin='%s' WHERE addonID='%s' AND origin=''",
          ORIGIN_SYSTEM, id.c_str()));
    }

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
}

void CAddonDatabase::GetInstalled(std::vector<CAddonBuilder>& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    m_pDS->query(PrepareSQL("SELECT * FROM installed"));
    while (!m_pDS->eof())
    {
      auto it = addons.emplace(addons.end());
      it->SetId(m_pDS->fv(1).get_asString());
      it->SetInstallDate(CDateTime::FromDBDateTime(m_pDS->fv(3).get_asString()));
      it->SetLastUpdated(CDateTime::FromDBDateTime(m_pDS->fv(4).get_asString()));
      it->SetLastUsed(CDateTime::FromDBDateTime(m_pDS->fv(5).get_asString()));
      it->SetOrigin(m_pDS->fv(6).get_asString());
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

bool CAddonDatabase::SetLastUpdated(const std::string& addonId, const CDateTime& dateTime)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    m_pDS->exec(PrepareSQL("UPDATE installed SET lastUpdated='%s' WHERE addonID='%s'",
        dateTime.GetAsDBDateTime().c_str(), addonId.c_str()));
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addonId.c_str());
  }
  return false;
}

bool CAddonDatabase::SetOrigin(const std::string& addonId, const std::string& origin)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    m_pDS->exec(PrepareSQL("UPDATE installed SET origin='%s' WHERE addonID='%s'", origin.c_str(), addonId.c_str()));
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addonId.c_str());
  }
  return false;
}

bool CAddonDatabase::SetLastUsed(const std::string& addonId, const CDateTime& dateTime)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    auto start = XbmcThreads::SystemClockMillis();
    m_pDS->exec(PrepareSQL("UPDATE installed SET lastUsed='%s' WHERE addonID='%s'",
        dateTime.GetAsDBDateTime().c_str(), addonId.c_str()));

    CLog::Log(LOGDEBUG, "CAddonDatabase::SetLastUsed[%s] took %i ms", addonId.c_str(), XbmcThreads::SystemClockMillis() - start);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addonId.c_str());
  }
  return false;
}

std::pair<AddonVersion, std::string> CAddonDatabase::GetAddonVersion(const std::string &id)
{
  auto empty = std::make_pair(AddonVersion("0.0.0"), "");
  try
  {
    if (NULL == m_pDB.get()) return empty;
    if (NULL == m_pDS2.get()) return empty;

    std::vector<std::pair<ADDON::AddonVersion, std::string>> versions;
    if (GetAvailableVersions(id, versions) && versions.size() > 0)
      return *std::max_element(versions.begin(), versions.end());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, id.c_str());
  }
  return empty;
}

bool CAddonDatabase::GetAvailableVersions(const std::string& addonId,
    std::vector<std::pair<ADDON::AddonVersion, std::string>>& versionsInfo)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL(
        "SELECT addons.version, repo.addonID AS repoID FROM addons "
        "JOIN addonlinkrepo ON addonlinkrepo.idAddon=addons.id "
        "JOIN repo ON repo.id=addonlinkrepo.idRepo "
        "WHERE "
        "repo.checksum IS NOT NULL AND repo.checksum != '' "
        "AND EXISTS (SELECT * FROM installed WHERE installed.addonID=repoID AND installed.enabled=1) "
        "AND NOT EXISTS (SELECT * FROM  broken WHERE broken.addonID=addons.addonID) "
        "AND addons.addonID='%s'", addonId.c_str());

    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      AddonVersion version(m_pDS->fv(0).get_asString());
      std::string repo = m_pDS->fv(1).get_asString();
      versionsInfo.push_back(std::make_pair(version, repo));
      m_pDS->next();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, addonId.c_str());
  }
  return false;
}

bool CAddonDatabase::GetAddon(const std::string& addonID, const AddonVersion& version, const std::string& repoId, AddonPtr& addon)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

  std::string sql = PrepareSQL(
      "SELECT addons.id FROM addons "
      "JOIN addonlinkrepo ON addonlinkrepo.idAddon=addons.id "
      "JOIN repo ON repo.id=addonlinkrepo.idRepo "
      "WHERE addons.addonID='%s' AND addons.version='%s' AND repo.addonID='%s'",
      addonID.c_str(), version.asString().c_str(), repoId.c_str());

    m_pDS->query(sql.c_str());
    if (m_pDS->eof())
      return false;

    return GetAddon(m_pDS->fv(0).get_asInt(), addon);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, addonID.c_str());
  }
  return false;

}

bool CAddonDatabase::GetAddon(const std::string& id, AddonPtr& addon)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // there may be multiple addons with this id (eg from different repositories) in the database,
    // so we want to retrieve the latest version.  Order by version won't work as the database
    // won't know that 1.10 > 1.2, so grab them all and order outside
    std::string sql = PrepareSQL("select id,version from addons where addonID='%s'",id.c_str());
    m_pDS2->query(sql);

    if (m_pDS2->eof())
      return false;

    AddonVersion maxversion("0.0.0");
    int maxid = 0;
    while (!m_pDS2->eof())
    {
      AddonVersion version(m_pDS2->fv(1).get_asString());
      if (version > maxversion)
      {
        maxid = m_pDS2->fv(0).get_asInt();
        maxversion = version;
      }
      m_pDS2->next();
    }
    return GetAddon(maxid,addon);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, id.c_str());
  }
  addon.reset();
  return false;
}

bool CAddonDatabase::GetAddon(int id, AddonPtr &addon)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    m_pDS2->query(PrepareSQL("SELECT * FROM addons WHERE id=%i", id));
    if (m_pDS2->eof())
      return false;

    CAddonBuilder builder;
    builder.SetId(m_pDS2->fv(2).get_asString());
    builder.SetVersion(AddonVersion(m_pDS2->fv(3).get_asString()));
    builder.SetName(m_pDS2->fv(4).get_asString());
    builder.SetSummary(m_pDS2->fv(5).get_asString());
    builder.SetDescription(m_pDS2->fv(6).get_asString());
    DeserializeMetadata(m_pDS2->fv(1).get_asString(), builder);
    addon = builder.Build();
    return addon != nullptr;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %i", __FUNCTION__, id);
  }
  return false;
}

bool CAddonDatabase::GetRepositoryContent(VECADDONS& addons)
{
  return GetRepositoryContent("", addons);
}

bool CAddonDatabase::GetRepositoryContent(const std::string& id, VECADDONS& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    auto start = XbmcThreads::SystemClockMillis();

    // Ensure that the repositories we fetch from are enabled and valid.
    std::vector<std::string> repoIds;
    {
      std::string sql = PrepareSQL(
          " SELECT repo.id FROM repo"
          " WHERE repo.checksum IS NOT NULL AND repo.checksum != ''"
          " AND EXISTS (SELECT * FROM installed WHERE installed.addonID=repo.addonID AND"
          " installed.enabled=1)");

      if (!id.empty())
        sql += PrepareSQL(" AND repo.addonId='%s'", id.c_str());

      m_pDS->query(sql);
      while (!m_pDS->eof())
      {
        repoIds.emplace_back(m_pDS->fv(0).get_asString());
        m_pDS->next();
      }
    }

    CLog::Log(LOGDEBUG, "CAddonDatabase: SELECT repo.id FROM repo .. took %d ms", XbmcThreads::SystemClockMillis() - start);

    if (repoIds.empty())
    {
      CLog::Log(LOGDEBUG, "CAddonDatabase: no valid repository matching '%s'", id.c_str());
      return false;
    }

    {
      std::string sql = PrepareSQL(
          " SELECT * FROM addons"
          " JOIN addonlinkrepo ON addons.id=addonlinkrepo.idAddon"
          " WHERE addonlinkrepo.idRepo IN (%s)"
          " ORDER BY addons.addonID", StringUtils::Join(repoIds, ",").c_str());

      auto start = XbmcThreads::SystemClockMillis();
      m_pDS->query(sql);
      CLog::Log(LOGDEBUG, "CAddonDatabase: query %s returned %d rows in %d ms", sql.c_str(),
          m_pDS->num_rows(), XbmcThreads::SystemClockMillis() - start);
    }

    VECADDONS result;
    while (!m_pDS->eof())
    {
      std::string addonId = m_pDS->fv(2).get_asString();
      AddonVersion version(m_pDS->fv(3).get_asString());

      if (!result.empty() && result.back()->ID() == addonId && result.back()->Version() >= version)
      {
        // We already have a version of this addon in our list which is newer.
        m_pDS->next();
        continue;
      }

      CAddonBuilder builder;
      builder.SetId(addonId);
      builder.SetVersion(version);
      builder.SetName(m_pDS->fv(4).get_asString());
      builder.SetSummary(m_pDS->fv(5).get_asString());
      builder.SetDescription(m_pDS->fv(6).get_asString());
      DeserializeMetadata(m_pDS->fv(1).get_asString(), builder);

      auto addon = builder.Build();
      if (addon)
      {
        if (!result.empty() && result.back()->ID() == addonId)
          result.back() = std::move(addon);
        else
          result.push_back(std::move(addon));
      }
      else
        CLog::Log(LOGWARNING, "CAddonDatabase: failed to build %s", addonId.c_str());
      m_pDS->next();
    }
    m_pDS->close();
    addons = std::move(result);

    CLog::Log(LOGDEBUG, "CAddonDatabase::GetAddons took %i ms", XbmcThreads::SystemClockMillis() - start);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

void CAddonDatabase::DeleteRepository(const std::string& id)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    m_pDS->query(PrepareSQL("SELECT id FROM repo WHERE addonID='%s'", id.c_str()));
    if (m_pDS->eof())
      return;

    int idRepo = m_pDS->fv(0).get_asInt();

    m_pDS->exec(PrepareSQL("DELETE FROM repo WHERE id=%i", idRepo));
    m_pDS->exec(PrepareSQL("DELETE FROM addons WHERE id IN (SELECT idAddon FROM addonlinkrepo WHERE idRepo=%i)", idRepo));
    m_pDS->exec(PrepareSQL("DELETE FROM addonlinkrepo WHERE idRepo=%i", idRepo));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
}

bool CAddonDatabase::UpdateRepositoryContent(const std::string& repository, const AddonVersion& version,
    const std::string& checksum, const std::vector<AddonPtr>& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    DeleteRepository(repository);

    if (!SetLastChecked(repository, version, CDateTime::GetCurrentDateTime().GetAsDBDateTime()))
      return false;

    int idRepo = static_cast<int>(m_pDS->lastinsertid());
    assert(idRepo > 0);

    m_pDB->start_transaction();
    m_pDS->exec(PrepareSQL("UPDATE repo SET checksum='%s' WHERE id='%d'", checksum.c_str(), idRepo));
    for (const auto& addon : addons)
    {
      m_pDS->exec(PrepareSQL(
          "INSERT INTO addons (id, metadata, addonID, version, name, summary, description) "
          "VALUES (NULL, '%s', '%s', '%s', '%s','%s', '%s')",
          SerializeMetadata(*addon).c_str(),
          addon->ID().c_str(),
          addon->Version().asString().c_str(),
          addon->Name().c_str(),
          addon->Summary().c_str(),
          addon->Description().c_str()));

      auto idAddon = m_pDS->lastinsertid();
      if (idAddon <= 0)
      {
        CLog::Log(LOGERROR, "%s insert failed on addon '%s'", __FUNCTION__, addon->ID().c_str());
        RollbackTransaction();
        return false;
      }

      m_pDS->exec(PrepareSQL("INSERT INTO addonlinkrepo (idRepo, idAddon) VALUES (%i, %i)", idRepo, idAddon));
    }

    m_pDB->commit_transaction();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, repository.c_str());
    RollbackTransaction();
  }
  return false;
}

int CAddonDatabase::GetRepoChecksum(const std::string& id, std::string& checksum)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
    {
      checksum = m_pDS->fv("checksum").get_asString();
      return m_pDS->fv("id").get_asInt();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
  checksum.clear();
  return -1;
}

std::pair<CDateTime, ADDON::AddonVersion> CAddonDatabase::LastChecked(const std::string& id)
{
  CDateTime date;
  AddonVersion version("0.0.0");
  try
  {
    if (m_pDB.get() != nullptr && m_pDS.get() != nullptr)
    {
      std::string strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
      m_pDS->query(strSQL);
      if (!m_pDS->eof())
      {
        date.SetFromDBDateTime(m_pDS->fv("lastcheck").get_asString());
        version = AddonVersion(m_pDS->fv("version").get_asString());
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
  return std::make_pair(date, version);
}

bool CAddonDatabase::SetLastChecked(const std::string& id,
    const ADDON::AddonVersion& version, const std::string& time)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("SELECT * FROM repo WHERE addonID='%s'", id.c_str());
    m_pDS->query(sql);

    if (m_pDS->eof())
      sql = PrepareSQL("INSERT INTO repo (id, addonID, lastcheck, version) "
          "VALUES (NULL, '%s', '%s', '%s')", id.c_str(), time.c_str(), version.asString().c_str());
    else
      sql = PrepareSQL("UPDATE repo SET lastcheck='%s', version='%s' WHERE addonID='%s'",
          time.c_str(), version.asString().c_str(), id.c_str());

    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
  return false;
}

bool CAddonDatabase::Search(const std::string& search, VECADDONS& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL;
    strSQL=PrepareSQL("SELECT addonID FROM addons WHERE name LIKE '%%%s%%' OR summary LIKE '%%%s%%' OR description LIKE '%%%s%%'", search.c_str(), search.c_str(), search.c_str());
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());

    if (!m_pDS->query(strSQL)) return false;
    if (m_pDS->num_rows() == 0) return false;

    while (!m_pDS->eof())
    {
      AddonPtr addon;
      GetAddon(m_pDS->fv(0).get_asString(),addon);
      if (addon->Type() >= ADDON_UNKNOWN+1 && addon->Type() < ADDON_SCRAPER_LIBRARY)
        addons.push_back(addon);
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CAddonDatabase::DisableAddon(const std::string &addonID, bool disable /* = true */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("UPDATE installed SET enabled=%d WHERE addonID='%s'", disable ? 0 : 1, addonID.c_str());
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addonID.c_str());
  }
  return false;
}

bool CAddonDatabase::BreakAddon(const std::string &addonID, const std::string& reason)
{
  if (reason.empty())
    return ExecuteQuery(PrepareSQL("DELETE FROM broken WHERE addonID='%s'", addonID.c_str()));
  else
    return ExecuteQuery(PrepareSQL("REPLACE INTO broken(addonID, reason) VALUES('%s', '%s')",
                                   addonID.c_str(), reason.c_str()));
}

bool CAddonDatabase::GetDisabled(std::set<std::string>& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("SELECT addonID FROM installed WHERE enabled=0");
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      addons.insert(m_pDS->fv(0).get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CAddonDatabase::GetBlacklisted(std::set<std::string>& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("SELECT addonID FROM blacklist");
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      addons.insert(m_pDS->fv(0).get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CAddonDatabase::IsAddonBroken(const std::string &addonID)
{
  return !GetSingleValue(PrepareSQL("SELECT reason FROM broken WHERE addonID='%s'", addonID.c_str())).empty();
}

bool CAddonDatabase::BlacklistAddon(const std::string& addonID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("INSERT INTO blacklist(id, addonID) VALUES(NULL, '%s')", addonID.c_str());
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addonID.c_str());
  }
  return false;
}

bool CAddonDatabase::RemoveAddonFromBlacklist(const std::string& addonID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("DELETE FROM blacklist WHERE addonID='%s'", addonID.c_str());
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addonID.c_str());
  }
  return false;
}

bool CAddonDatabase::AddPackage(const std::string& addonID,
                                const std::string& packageFileName,
                                const std::string& hash)
{
  std::string sql = PrepareSQL("insert into package(id, addonID, filename, hash)"
                              "values(NULL, '%s', '%s', '%s')",
                              addonID.c_str(), packageFileName.c_str(), hash.c_str());
  return ExecuteQuery(sql);
}

bool CAddonDatabase::GetPackageHash(const std::string& addonID,
                                    const std::string& packageFileName,
                                    std::string&       hash)
{
  std::string where = PrepareSQL("addonID='%s' and filename='%s'",
                                addonID.c_str(), packageFileName.c_str());
  hash = GetSingleValue("package", "hash", where);
  return !hash.empty();
}

bool CAddonDatabase::RemovePackage(const std::string& packageFileName)
{
  std::string sql = PrepareSQL("delete from package where filename='%s'", packageFileName.c_str());
  return ExecuteQuery(sql);
}

void CAddonDatabase::OnPostUnInstall(const std::string& addonId)
{
  RemoveAddonFromBlacklist(addonId);
  DeleteRepository(addonId);

  //! @todo should be done before uninstall to avoid any race conditions
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    m_pDS->exec(PrepareSQL("DELETE FROM installed WHERE addonID='%s'", addonId.c_str()));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, addonId.c_str());
  }
}
