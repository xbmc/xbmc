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
#include "addons/AddonManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "XBDateTime.h"
#include "dbwrappers/dataset.h"

using namespace ADDON;
using namespace std;

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

  CLog::Log(LOGINFO, "create disabled table");
  m_pDS->exec("CREATE TABLE disabled (id integer primary key, addonID text)\n");

  CLog::Log(LOGINFO, "create broken table");
  m_pDS->exec("CREATE TABLE broken (id integer primary key, addonID text, reason text)\n");

  CLog::Log(LOGINFO, "create blacklist table");
  m_pDS->exec("CREATE TABLE blacklist (id integer primary key, addonID text, version text)\n");

  CLog::Log(LOGINFO, "create package table");
  m_pDS->exec("CREATE TABLE package (id integer primary key, addonID text, filename text, hash text)\n");

  CLog::Log(LOGINFO, "create system table");
  m_pDS->exec("CREATE TABLE system (id integer primary key, addonID text)\n");
}

void CAddonDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxAddon ON addon(addonID)");
  m_pDS->exec("CREATE INDEX idxAddonExtra ON addonextra(id)");
  m_pDS->exec("CREATE INDEX idxDependencies ON dependencies(id)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_addonlinkrepo_1 ON addonlinkrepo ( idAddon, idRepo )\n");
  m_pDS->exec("CREATE UNIQUE INDEX ix_addonlinkrepo_2 ON addonlinkrepo ( idRepo, idAddon )\n");
  m_pDS->exec("CREATE UNIQUE INDEX idxDisabled ON disabled(addonID)");
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
}

int CAddonDatabase::GetAddonId(const ADDON::AddonPtr& item)
{
  std::string value = GetSingleValue("addon", "id", StringUtils::Format("name = '%s'", item->Name().c_str()), "id desc");
  return value.empty() || !StringUtils::IsInteger(value) ? -1 : atoi(value.c_str());
}

int CAddonDatabase::AddAddon(const AddonPtr& addon,
                             int idRepo)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string sql = PrepareSQL("insert into addon (id, type, name, summary,"
                               "description, stars, path, icon, changelog, "
                               "fanart, addonID, version, author, disclaimer, minversion)"
                               " values(NULL, '%s', '%s', '%s', '%s', %i,"
                               "'%s', '%s', '%s', '%s', '%s','%s','%s','%s','%s')",
                               TranslateType(addon->Type(),false).c_str(),
                               addon->Name().c_str(), addon->Summary().c_str(),
                               addon->Description().c_str(),addon->Stars(),
                               addon->Path().c_str(), addon->Props().icon.c_str(),
                               addon->ChangeLog().c_str(),addon->FanArt().c_str(),
                               addon->ID().c_str(), addon->Version().asString().c_str(),
                               addon->Author().c_str(),addon->Disclaimer().c_str(),
                               addon->MinVersion().asString().c_str());
    m_pDS->exec(sql.c_str());
    int idAddon = (int)m_pDS->lastinsertid();

    sql = PrepareSQL("insert into addonlinkrepo (idRepo, idAddon) values (%i,%i)",idRepo,idAddon);
    m_pDS->exec(sql.c_str());

    const InfoMap &info = addon->ExtraInfo();
    for (InfoMap::const_iterator i = info.begin(); i != info.end(); ++i)
    {
      sql = PrepareSQL("insert into addonextra(id, key, value) values (%i, '%s', '%s')", idAddon, i->first.c_str(), i->second.c_str());
      m_pDS->exec(sql.c_str());
    }
    const ADDONDEPS &deps = addon->GetDeps();
    for (ADDONDEPS::const_iterator i = deps.begin(); i != deps.end(); ++i)
    {
      sql = PrepareSQL("insert into dependencies(id, addon, version, optional) values (%i, '%s', '%s', %i)", idAddon, i->first.c_str(), i->second.first.asString().c_str(), i->second.second ? 1 : 0);
      m_pDS->exec(sql.c_str());
    }
    return idAddon;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addon->Name().c_str());
  }
  return -1;
}

AddonVersion CAddonDatabase::GetAddonVersion(const std::string &id)
{
  AddonVersion maxversion("0.0.0");
  try
  {
    if (NULL == m_pDB.get()) return maxversion;
    if (NULL == m_pDS2.get()) return maxversion;

    // there may be multiple addons with this id (eg from different repositories) in the database,
    // so we want to retrieve the latest version.  Order by version won't work as the database
    // won't know that 1.10 > 1.2, so grab them all and order outside
    std::string sql = PrepareSQL("select version from addon where addonID='%s'",id.c_str());
    m_pDS2->query(sql.c_str());

    if (m_pDS2->eof())
      return maxversion;

    while (!m_pDS2->eof())
    {
      AddonVersion version(m_pDS2->fv(0).get_asString());
      if (version > maxversion)
        maxversion = version;
      m_pDS2->next();
    }
    return maxversion;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, id.c_str());
  }
  return maxversion;
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
    m_pDS2->query(sql.c_str());

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

bool CAddonDatabase::GetRepoForAddon(const std::string& addonID, std::string& repo)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    std::string sql = PrepareSQL("select repo.addonID from repo join addonlinkrepo on repo.id=addonlinkrepo.idRepo join addon on addonlinkrepo.idAddon=addon.id where addon.addonID like '%s'", addonID.c_str()); 
    m_pDS2->query(sql.c_str());
    if (!m_pDS2->eof())
    {
      repo = m_pDS2->fv(0).get_asString();
      m_pDS2->close();
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed for addon %s", __FUNCTION__, addonID.c_str());
  }
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

    m_pDS2->query(sql.c_str());
    if (!m_pDS2->eof())
    {
      const dbiplus::query_data &data = m_pDS2->get_result_set().records;
      const dbiplus::sql_record* const record = data[0];
      AddonProps props(record->at(addon_addonID).get_asString(),
                       TranslateType(record->at(addon_type).get_asString()),
                       record->at(addon_version).get_asString(),
                       record->at(addon_minversion).get_asString());
      props.name = record->at(addon_name).get_asString();
      props.summary = record->at(addon_summary).get_asString();
      props.description = record->at(addon_description).get_asString();
      props.changelog = record->at(addon_changelog).get_asString();
      props.path = record->at(addon_path).get_asString();
      props.icon = record->at(addon_icon).get_asString();
      props.fanart = record->at(addon_fanart).get_asString();
      props.author = record->at(addon_author).get_asString();
      props.disclaimer = record->at(addon_disclaimer).get_asString();
      props.broken = record->at(broken_reason).get_asString();

      /* while this is a cartesion join and we'll typically get multiple rows, we rely on the fact that
         extrainfo and dependencies are maps, so insert() will insert the first instance only */
      for (dbiplus::query_data::const_iterator i = data.begin(); i != data.end(); ++i)
      {
        const dbiplus::sql_record* const record = *i;
        if (!record->at(addonextra_key).get_asString().empty())
          props.extrainfo.insert(make_pair(record->at(addonextra_key).get_asString(), record->at(addonextra_value).get_asString()));
        if (!m_pDS2->fv(dependencies_addon).get_asString().empty())
          props.dependencies.insert(make_pair(record->at(dependencies_addon).get_asString(), make_pair(AddonVersion(record->at(dependencies_version).get_asString()), record->at(dependencies_optional).get_asBool())));
      }

      addon = CAddonMgr::AddonFromProps(props);
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

bool CAddonDatabase::GetAddons(VECADDONS& addons, const ADDON::TYPE &type /* = ADDON::ADDON_UNKNOWN */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    std::string sql = PrepareSQL("SELECT DISTINCT a.addonID FROM addon a, addonlinkrepo b WHERE b.idRepo > 0 AND a.id = b.idAddon AND "
                                 "NOT EXISTS (SELECT repo.id FROM repo, disabled WHERE repo.addonID=disabled.addonID AND repo.id=b.idRepo)");
    if (type != ADDON_UNKNOWN)
    {
      std::string strType;
      if (type >= ADDON_VIDEO && type <= ADDON_EXECUTABLE)
        strType = TranslateType(ADDON_PLUGIN);
      else
        strType = TranslateType(type);

      if (!strType.empty())
        sql += PrepareSQL(" AND a.type = '%s'", strType.c_str());
    }

    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      AddonPtr addon;
      if (GetAddon(m_pDS->fv(0).get_asString(),addon))
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

void CAddonDatabase::DeleteRepository(const std::string& id)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    std::string sql = PrepareSQL("select id from repo where addonID='%s'",id.c_str());
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
      DeleteRepository(m_pDS->fv(0).get_asInt());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
}

void CAddonDatabase::DeleteRepository(int idRepo)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    std::string sql = PrepareSQL("delete from repo where id=%i",idRepo);
    m_pDS->exec(sql.c_str());
    sql = PrepareSQL("delete from addon where id in (select idAddon from addonlinkrepo where idRepo=%i)",idRepo);
    m_pDS->exec(sql.c_str());
    sql = PrepareSQL("delete from addonextra where id in (select idAddon from addonlinkrepo where idRepo=%i)",idRepo);
    m_pDS->exec(sql.c_str());
    sql = PrepareSQL("delete from dependencies where id in (select idAddon from addonlinkrepo where idRepo=%i)",idRepo);
    m_pDS->exec(sql.c_str());
    sql = PrepareSQL("delete from addonlinkrepo where idRepo=%i",idRepo);
    m_pDS->exec(sql.c_str());

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo %i", __FUNCTION__, idRepo);
  }
}

int CAddonDatabase::AddRepository(const std::string& id, const VECADDONS& addons, const std::string& checksum, const AddonVersion& version)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string sql;
    int idRepo = GetRepoChecksum(id,sql);
    if (idRepo > -1)
      DeleteRepository(idRepo);

    BeginTransaction();

    CDateTime time = CDateTime::GetCurrentDateTime();
    sql = PrepareSQL("insert into repo (id,addonID,checksum,lastcheck,version) values (NULL,'%s','%s','%s','%s')",
                     id.c_str(), checksum.c_str(), time.GetAsDBDateTime().c_str(), version.asString().c_str());
    m_pDS->exec(sql.c_str());
    idRepo = (int)m_pDS->lastinsertid();
    for (unsigned int i=0;i<addons.size();++i)
      AddAddon(addons[i],idRepo);

    CommitTransaction();
    return idRepo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
    RollbackTransaction();
  }
  return -1;
}

int CAddonDatabase::GetRepoChecksum(const std::string& id, std::string& checksum)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
    m_pDS->query(strSQL.c_str());
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

CDateTime CAddonDatabase::GetRepoTimestamp(const std::string& id)
{
  CDateTime date;
  try
  {
    if (NULL == m_pDB.get()) return date;
    if (NULL == m_pDS.get()) return date;

    std::string strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
    {
      date.SetFromDBDateTime(m_pDS->fv("lastcheck").get_asString());
      return date;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
  return date;
}


AddonVersion CAddonDatabase::GetRepoVersion(const std::string& id)
{
  AddonVersion version("0.0.0");
  try
  {
    if (NULL == m_pDB.get()) return version;
    if (NULL == m_pDS2.get()) return version;

    std::string strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
    {
      return AddonVersion(m_pDS->fv("version").get_asString());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, id.c_str());
  }
  return version;
}

bool CAddonDatabase::SetRepoTimestamp(const std::string& id, const std::string& time, const ADDON::AddonVersion& version)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("UPDATE repo SET lastcheck='%s', version='%s' WHERE addonID='%s'",
                                 time.c_str(), version.asString().c_str(), id.c_str());
    m_pDS->exec(sql.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
  return false;
}

bool CAddonDatabase::GetRepository(int id, VECADDONS& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL = PrepareSQL("select * from addonlinkrepo where idRepo=%i",id);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      AddonPtr addon;
      if (GetAddon(m_pDS->fv("idAddon").get_asInt(),addon))
        addons.push_back(addon);
      m_pDS->next();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo %i", __FUNCTION__, id);
  }
  return false;
}

bool CAddonDatabase::GetRepository(const std::string& id, VECADDONS& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL = PrepareSQL("select id from repo where addonID='%s'",id.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      return GetRepository(m_pDS->fv(0).get_asInt(),addons);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo %s", __FUNCTION__, id.c_str());
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

    if (!m_pDS->query(strSQL.c_str())) return false;
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

void CAddonDatabase::SetPropertiesFromAddon(const AddonPtr& addon,
                                           CFileItemPtr& pItem)
{
  pItem->SetProperty("Addon.ID", addon->ID());
  pItem->SetProperty("Addon.Type", TranslateType(addon->Type(),true));
  pItem->SetProperty("Addon.intType", TranslateType(addon->Type()));
  pItem->SetProperty("Addon.Name", addon->Name());
  pItem->SetProperty("Addon.Version", addon->Version().asString());
  pItem->SetProperty("Addon.Summary", addon->Summary());
  pItem->SetProperty("Addon.Description", addon->Description());
  pItem->SetProperty("Addon.Creator", addon->Author());
  pItem->SetProperty("Addon.Disclaimer", addon->Disclaimer());
  pItem->SetProperty("Addon.Rating", addon->Stars());
  std::string starrating = StringUtils::Format("rating%d.png", addon->Stars());
  pItem->SetProperty("Addon.StarRating",starrating);
  pItem->SetProperty("Addon.Path", addon->Path());
  if (addon->Props().broken == "DEPSNOTMET")
    pItem->SetProperty("Addon.Broken", g_localizeStrings.Get(24044));
  else
    pItem->SetProperty("Addon.Broken", addon->Props().broken);
  std::map<std::string,std::string>::iterator it = 
                    addon->Props().extrainfo.find("language");
  if (it != addon->Props().extrainfo.end())
    pItem->SetProperty("Addon.Language", it->second);
}

bool CAddonDatabase::DisableAddon(const std::string &addonID, bool disable /* = true */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (disable)
    {
      if (!IsAddonDisabled(addonID)) // Enabled
      {
        std::string sql = PrepareSQL("insert into disabled(id, addonID) values(NULL, '%s')", addonID.c_str());
        m_pDS->exec(sql);
        return true;
      }
      return false; // already disabled or failed query
    }
    else
    {
      std::string sql = PrepareSQL("delete from disabled where addonID='%s'", addonID.c_str());
      m_pDS->exec(sql);
    }
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

bool CAddonDatabase::HasAddon(const std::string &addonID)
{
  std::string strWhereClause = PrepareSQL("addonID = '%s'", addonID.c_str());
  std::string strHasAddon = GetSingleValue("addon", "id", strWhereClause);
  
  return !strHasAddon.empty();
}

bool CAddonDatabase::IsAddonDisabled(const std::string &addonID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("select id from disabled where addonID='%s'", addonID.c_str());
    m_pDS->query(sql.c_str());
    bool ret = !m_pDS->eof(); // in the disabled table -> disabled
    m_pDS->close();
    return ret;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, addonID.c_str());
  }
  return false;
}

bool CAddonDatabase::GetDisabled(std::vector<std::string>& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("SELECT addonID FROM disabled");
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      addons.push_back(m_pDS->fv(0).get_asString());
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

bool CAddonDatabase::IsSystemPVRAddonEnabled(const std::string &addonID)
{
  std::string strWhereClause = PrepareSQL("addonID = '%s'", addonID.c_str());
  std::string strEnabled = GetSingleValue("pvrenabled", "id", strWhereClause);

  return !strEnabled.empty();
}

std::string CAddonDatabase::IsAddonBroken(const std::string &addonID)
{
  return GetSingleValue(PrepareSQL("SELECT reason FROM broken WHERE addonID='%s'", addonID.c_str()));
}

bool CAddonDatabase::HasDisabledAddons()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    m_pDS->query("select count(id) from disabled");
    bool ret = !m_pDS->eof() && m_pDS->fv(0).get_asInt() > 0; // have rows -> have disabled addons
    m_pDS->close();
    return ret;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CAddonDatabase::BlacklistAddon(const std::string& addonID,
                                    const std::string& version)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("insert into blacklist(id, addonID, version) values(NULL, '%s', '%s')", addonID.c_str(),version.c_str());
    m_pDS->exec(sql);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s' for version '%s'", __FUNCTION__, addonID.c_str(),version.c_str());
  }
  return false;
}

bool CAddonDatabase::IsAddonBlacklisted(const std::string& addonID,
                                        const std::string& version)
{
  std::string where = PrepareSQL("addonID='%s' and version='%s'",addonID.c_str(),version.c_str());
  return !GetSingleValue("blacklist","addonID",where).empty();
}

bool CAddonDatabase::RemoveAddonFromBlacklist(const std::string& addonID,
                                              const std::string& version)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("delete from blacklist where addonID='%s' and version='%s'",addonID.c_str(),version.c_str());
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s' for version '%s'", __FUNCTION__, addonID.c_str(),version.c_str());
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

bool CAddonDatabase::AddSystemAddon(const std::string &addonID)
{
  try
  {
    if (NULL == m_pDB.get())
      return false;
    if (NULL == m_pDS.get())
      return false;

    if (!IsSystemAddonRegistered(addonID)) // Enabled
    {
      std::string sql = PrepareSQL("insert into system(id, addonID) values(NULL, '%s')", addonID.c_str());
      m_pDS->exec(sql);
      return true;
    }
    return false; // already registered or failed query
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addonID.c_str());
  }
  return false;
}

bool CAddonDatabase::IsSystemAddonRegistered(const std::string &addonID)
{
  try
  {
    if (NULL == m_pDB.get())
      return false;
    if (NULL == m_pDS.get())
      return false;

    std::string sql = PrepareSQL("select id from system where addonID='%s'", addonID.c_str());
    m_pDS->query(sql.c_str());
    bool ret = !m_pDS->eof();
    m_pDS->close();
    return ret;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, addonID.c_str());
  }
  return false;
}
