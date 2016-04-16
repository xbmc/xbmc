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
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "DllLibCPluff.h"
#include "XBDateTime.h"

using namespace ADDON;

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
  return 23;
}

void CAddonDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create addon table");
  m_pDS->exec("CREATE TABLE addon (id integer primary key, type text,"
              "name text, summary text, description text, stars integer,"
              "path text, addonID text, icon text, version text, "
              "changelog text, fanart text, author text, disclaimer text,"
              "minversion text)\n");

  CLog::Log(LOGINFO, "create addonextra table");
  m_pDS->exec("CREATE TABLE addonextra (id integer, key text, value text)\n");

  CLog::Log(LOGINFO, "create dependencies table");
  m_pDS->exec("CREATE TABLE dependencies (id integer, addon text, version text, optional boolean)\n");

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
      "enabled BOOLEAN, installDate TEXT, lastUpdated TEXT, lastUsed TEXT) \n");
}

void CAddonDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxAddon ON addon(addonID)");
  m_pDS->exec("CREATE INDEX idxAddonExtra ON addonextra(id)");
  m_pDS->exec("CREATE INDEX idxDependencies ON dependencies(id)");
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
  if (version < 23)
  {
    m_pDS->exec("DELETE FROM addon");
    m_pDS->exec("DELETE FROM addonextra");
    m_pDS->exec("DELETE FROM dependencies");
    m_pDS->exec("DELETE FROM addonlinkrepo");
    m_pDS->exec("DELETE FROM repo");
  }
}

void CAddonDatabase::SyncInstalled(const std::set<std::string>& ids, const std::set<std::string>& enabled)
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
      m_pDS->exec(PrepareSQL("INSERT INTO installed(addonID, enabled, installDate) "
          "VALUES('%s', 0, '%s')", id.c_str(), now.c_str()));

    for (const auto& id : removed)
    {
      m_pDS->exec(PrepareSQL("DELETE FROM installed WHERE addonID='%s'", id.c_str()));
      RemoveAddonFromBlacklist(id);
      DeleteRepository(id);
    }

    for (const auto& id : enabled)
      m_pDS->exec(PrepareSQL("UPDATE installed SET enabled=1 WHERE addonID='%s'", id.c_str()));

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

int CAddonDatabase::AddAddon(const AddonPtr& addon, int idRepo)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string sql = PrepareSQL("insert into addon (id, type, name, summary,"
                               "description, path, icon, changelog, "
                               "fanart, addonID, version, author, disclaimer, minversion)"
                               " values(NULL, '%s', '%s', '%s', '%s',"
                               "'%s', '%s', '%s', '%s', '%s','%s','%s','%s','%s')",
                               TranslateType(addon->Type(),false).c_str(),
                               addon->Name().c_str(), addon->Summary().c_str(),
                               addon->Description().c_str(),
                               addon->Path().c_str(), addon->Icon().c_str(),
                               addon->ChangeLog().c_str(),addon->FanArt().c_str(),
                               addon->ID().c_str(), addon->Version().asString().c_str(),
                               addon->Author().c_str(),addon->Disclaimer().c_str(),
                               addon->MinVersion().asString().c_str());
    m_pDS->exec(sql);
    int idAddon = (int)m_pDS->lastinsertid();

    sql = PrepareSQL("insert into addonlinkrepo (idRepo, idAddon) values (%i,%i)",idRepo,idAddon);
    m_pDS->exec(sql);

    const InfoMap &info = addon->ExtraInfo();
    for (InfoMap::const_iterator i = info.begin(); i != info.end(); ++i)
    {
      sql = PrepareSQL("insert into addonextra(id, key, value) values (%i, '%s', '%s')", idAddon, i->first.c_str(), i->second.c_str());
      m_pDS->exec(sql);
    }
    const ADDONDEPS &deps = addon->GetDeps();
    for (ADDONDEPS::const_iterator i = deps.begin(); i != deps.end(); ++i)
    {
      sql = PrepareSQL("insert into dependencies(id, addon, version, optional) values (%i, '%s', '%s', %i)", idAddon, i->first.c_str(), i->second.first.asString().c_str(), i->second.second ? 1 : 0);
      m_pDS->exec(sql);
    }
    return idAddon;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addon->Name().c_str());
  }
  return -1;
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
        "SELECT addon.version, repo.addonID AS repoID FROM addon "
        "JOIN addonlinkrepo ON addonlinkrepo.idAddon=addon.id "
        "JOIN repo ON repo.id=addonlinkrepo.idRepo "
        "WHERE "
        "repo.checksum IS NOT NULL AND repo.checksum != '' "
        "AND EXISTS (SELECT * FROM installed WHERE installed.addonID=repoID AND installed.enabled=1) "
        "AND NOT EXISTS (SELECT * FROM  broken WHERE broken.addonID=addon.addonID) "
        "AND addon.addonID='%s'", addonId.c_str());

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
      "SELECT addon.id, addon.addonID, repo.addonID AS repoID FROM addon "
      "JOIN addonlinkrepo ON addonlinkrepo.idAddon=addon.id "
      "JOIN repo ON repo.id=addonlinkrepo.idRepo "
      "WHERE addon.addonID='%s' AND addon.version='%s' AND repoID='%s'",
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
    std::string sql = PrepareSQL("select id,version from addon where addonID='%s'",id.c_str());
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

    std::string sql = "SELECT addon.*,"
                      "       broken.reason,"
                      "       addonextra.key, addonextra.value,"
                      "       dependencies.addon, dependencies.version, dependencies.optional"
                      "  FROM addon"
                      "    LEFT JOIN broken"
                      "      ON broken.addonID = addon.addonID"
                      "    LEFT JOIN addonextra"
                      "      ON addonextra.id = addon.id"
                      "    LEFT JOIN dependencies"
                      "      ON dependencies.id = addon.id";

    sql += PrepareSQL(" WHERE addon.id=%i", id);

    m_pDS2->query(sql);
    if (!m_pDS2->eof())
    {
      const dbiplus::query_data &data = m_pDS2->get_result_set().records;
      const dbiplus::sql_record* const record = data[0];

      CAddonBuilder builder;
      builder.SetId(record->at(addon_addonID).get_asString());
      builder.SetType(TranslateType(record->at(addon_type).get_asString()));
      builder.SetVersion(AddonVersion(record->at(addon_version).get_asString()));
      builder.SetMinVersion(AddonVersion(record->at(addon_minversion).get_asString()));
      builder.SetName(record->at(addon_name).get_asString());
      builder.SetSummary(record->at(addon_summary).get_asString());
      builder.SetDescription(record->at(addon_description).get_asString());
      builder.SetChangelog(record->at(addon_changelog).get_asString());
      builder.SetDisclaimer(record->at(addon_disclaimer).get_asString());
      builder.SetAuthor(record->at(addon_author).get_asString());
      builder.SetBroken(record->at(broken_reason).get_asString());
      builder.SetPath(record->at(addon_path).get_asString());
      builder.SetIcon(record->at(addon_icon).get_asString());
      builder.SetFanart(record->at(addon_fanart).get_asString());

      InfoMap extrainfo;
      ADDONDEPS dependencies;
      /* while this is a cartesion join and we'll typically get multiple rows, we rely on the fact that
         extrainfo and dependencies are maps, so insert() will insert the first instance only */
      for (dbiplus::query_data::const_iterator i = data.begin(); i != data.end(); ++i)
      {
        const dbiplus::sql_record* const record = *i;
        if (!record->at(addonextra_key).get_asString().empty())
          extrainfo.insert(std::make_pair(record->at(addonextra_key).get_asString(), record->at(addonextra_value).get_asString()));
        if (!m_pDS2->fv(dependencies_addon).get_asString().empty())
          dependencies.insert(std::make_pair(record->at(dependencies_addon).get_asString(), std::make_pair(AddonVersion(record->at(dependencies_version).get_asString()), record->at(dependencies_optional).get_asBool())));
      }
      builder.SetExtrainfo(std::move(extrainfo));
      builder.SetDependencies(std::move(dependencies));

      addon = builder.Build();
      return NULL != addon.get();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %i", __FUNCTION__, id);
  }
  addon.reset();
  return false;
}

bool CAddonDatabase::GetRepositoryContent(VECADDONS& addons)
{
  return GetRepositoryContent("", addons);
}

bool CAddonDatabase::GetRepositoryContent(const std::string& repository, VECADDONS& addons)
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

      if (!repository.empty())
        sql += PrepareSQL(" AND repo.addonId='%s'", repository.c_str());

      m_pDS->query(sql);
      while (!m_pDS->eof())
      {
        repoIds.emplace_back(m_pDS->fv(0).get_asString());
        m_pDS->next();
      }
    }

    if (repoIds.empty())
    {
      CLog::Log(LOGDEBUG, "CAddonDatabase: no valid repository matching '%s'", repository.c_str());
      return false;
    }

    std::string commonConstraint = PrepareSQL(
        " JOIN addonlinkrepo ON addon.id=addonlinkrepo.idAddon"
        " WHERE addonlinkrepo.idRepo IN (%s)"
        " ORDER BY addon.addonID", StringUtils::Join(repoIds, ",").c_str());

    std::vector<CAddonBuilder> result;
    // Read basic info from the `addon` table
    {
      std::string sql = PrepareSQL("SELECT addon.*, broken.reason FROM addon "
          "LEFT JOIN broken ON broken.addonID=addon.addonID ") + commonConstraint;
      auto start = XbmcThreads::SystemClockMillis();
      m_pDS->query(sql);
      CLog::Log(LOGDEBUG, "CAddonDatabase: query %s returned %d rows in %d ms", sql.c_str(),
          m_pDS->num_rows(), XbmcThreads::SystemClockMillis() - start);

      while (!m_pDS->eof())
      {
        std::string addonId = m_pDS->fv(addon_addonID).get_asString();
        AddonVersion version(m_pDS->fv(addon_version).get_asString());

        if (!result.empty() && result.back().GetId() == addonId && result.back().GetVersion() >= version)
        {
          // We already have a version of this addon in our list which is newer.
          m_pDS->next();
          continue;
        }

        CAddonBuilder builder;
        builder.SetId(addonId);
        builder.SetVersion(version);
        builder.SetType(TranslateType(m_pDS->fv(addon_type).get_asString()));
        builder.SetMinVersion(AddonVersion(m_pDS->fv(addon_minversion).get_asString()));
        builder.SetName(m_pDS->fv(addon_name).get_asString());
        builder.SetSummary(m_pDS->fv(addon_summary).get_asString());
        builder.SetDescription(m_pDS->fv(addon_description).get_asString());
        builder.SetChangelog(m_pDS->fv(addon_changelog).get_asString());
        builder.SetDisclaimer(m_pDS->fv(addon_disclaimer).get_asString());
        builder.SetAuthor(m_pDS->fv(addon_author).get_asString());
        builder.SetPath(m_pDS->fv(addon_path).get_asString());
        builder.SetIcon(m_pDS->fv(addon_icon).get_asString());
        builder.SetFanart(m_pDS->fv(addon_fanart).get_asString());
        builder.SetBroken(m_pDS->fv(broken_reason).get_asString());
        if (!result.empty() && result.back().GetId() == addonId)
          result.back() = std::move(builder);
        else
          result.push_back(std::move(builder));
        m_pDS->next();
      }
    }

    // Read extra info.
    {
      std::string sql = PrepareSQL(
          "SELECT addon.addonID as owner, addonextra.key, addonextra.value "
          "FROM addon JOIN addonextra ON addon.id=addonextra.id ") + commonConstraint;

      auto start = XbmcThreads::SystemClockMillis();
      m_pDS->query(sql);
      CLog::Log(LOGDEBUG, "CAddonDatabase: query %s returned %d rows in %d ms", sql.c_str(),
          m_pDS->num_rows(), XbmcThreads::SystemClockMillis() - start);

      for (auto& builder : result)
      {
        //move cursor to current or next addon
        while (!m_pDS->eof() && m_pDS->fv(0).get_asString() < builder.GetId())
          m_pDS->next();

        InfoMap extraInfo;
        while (!m_pDS->eof() && m_pDS->fv(0).get_asString() == builder.GetId())
        {
          extraInfo.emplace(m_pDS->fv(1).get_asString(), m_pDS->fv(2).get_asString());
          m_pDS->next();
        }
        builder.SetExtrainfo(std::move(extraInfo));
      }
    }

    // Read dependency info.
    {
      std::string sql = PrepareSQL(
          "SELECT addon.addonID as owner, dependencies.addon, dependencies.version, dependencies.optional "
          "FROM addon JOIN dependencies ON addon.id=dependencies.id ") + commonConstraint;

      auto start = XbmcThreads::SystemClockMillis();
      m_pDS->query(sql);
      CLog::Log(LOGDEBUG, "CAddonDatabase: query %s returned %d rows in %d ms", sql.c_str(),
          m_pDS->num_rows(), XbmcThreads::SystemClockMillis() - start);

      for (auto& builder : result)
      {
        //move cursor to the current or next addon
        while (!m_pDS->eof() && m_pDS->fv(0).get_asString() < builder.GetId())
          m_pDS->next();

        ADDONDEPS dependencies;
        while (!m_pDS->eof() && m_pDS->fv(0).get_asString() == builder.GetId())
        {
          dependencies.emplace(m_pDS->fv(1).get_asString(),
              std::make_pair(AddonVersion(m_pDS->fv(2).get_asString()), m_pDS->fv(3).get_asBool()));
          m_pDS->next();
        }
        builder.SetDependencies(std::move(dependencies));
      }
    }
    m_pDS->close();

    for (auto& builder : result)
    {
      auto addon = builder.Build();
      if (addon)
        addons.push_back(std::move(addon));
    }

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
    if (!m_pDS->eof())
      return;

    int idRepo = m_pDS->fv(0).get_asInt();

    m_pDS->exec(PrepareSQL("DELETE FROM repo WHERE id=%i", idRepo));
    m_pDS->exec(PrepareSQL("DELETE FROM addon WHERE id IN (SELECT idAddon FROM addonlinkrepo WHERE idRepo=%i)", idRepo));
    m_pDS->exec(PrepareSQL("DELETE FROM addonextra WHERE id IN (SELECT idAddon FROM addonlinkrepo WHERE idRepo=%i)", idRepo));
    m_pDS->exec(PrepareSQL("DELETE FROM dependencies WHERE id IN (SELECT idAddon FROM addonlinkrepo WHERE idRepo=%i)", idRepo));
    m_pDS->exec(PrepareSQL("DELETE FROM addonlinkrepo WHERE idRepo=%i", idRepo));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
}

bool CAddonDatabase::UpdateRepositoryContent(const std::string& id, const VECADDONS& addons,
    const std::string& checksum, const AddonVersion& version)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    DeleteRepository(id);

    if (!SetLastChecked(id, version, CDateTime::GetCurrentDateTime().GetAsDBDateTime()))
      return false;

    m_pDB->start_transaction();
    m_pDS->exec(PrepareSQL("UPDATE repo SET checksum='%s' WHERE addonID='%s'", checksum.c_str(), id.c_str()));

    int idRepo = static_cast<int>(m_pDS->lastinsertid());
    for (const auto& addon : addons)
      AddAddon(addon, idRepo);

    m_pDB->commit_transaction();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
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
    strSQL=PrepareSQL("SELECT addonID FROM addon WHERE name LIKE '%%%s%%' OR summary LIKE '%%%s%%' OR description LIKE '%%%s%%'", search.c_str(), search.c_str(), search.c_str());
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

std::string CAddonDatabase::IsAddonBroken(const std::string &addonID)
{
  return GetSingleValue(PrepareSQL("SELECT reason FROM broken WHERE addonID='%s'", addonID.c_str()));
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

  //TODO: should be done before uninstall to avoid any race conditions
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
