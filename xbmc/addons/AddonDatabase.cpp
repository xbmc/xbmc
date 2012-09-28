/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
#include "XBDateTime.h"
#include "addons/Service.h"
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

bool CAddonDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create addon table");
    m_pDS->exec("CREATE TABLE addon (id integer primary key, type text,"
                "name text, summary text, description text, stars integer,"
                "path text, addonID text, icon text, version text, "
                "changelog text, fanart text, author text, disclaimer text,"
                "minversion text)\n");

    CLog::Log(LOGINFO, "create addon index");
    m_pDS->exec("CREATE INDEX idxAddon ON addon(addonID)");

    CLog::Log(LOGINFO, "create addonextra table");
    m_pDS->exec("CREATE TABLE addonextra (id integer, key text, value text)\n");

    CLog::Log(LOGINFO, "create addonextra index");
    m_pDS->exec("CREATE INDEX idxAddonExtra ON addonextra(id)");

    CLog::Log(LOGINFO, "create dependencies table");
    m_pDS->exec("CREATE TABLE dependencies (id integer, addon text, version text, optional boolean)\n");
    m_pDS->exec("CREATE INDEX idxDependencies ON dependencies(id)");

    CLog::Log(LOGINFO, "create repo table");
    m_pDS->exec("CREATE TABLE repo (id integer primary key, addonID text,"
                "checksum text, lastcheck text)\n");

    CLog::Log(LOGINFO, "create addonlinkrepo table");
    m_pDS->exec("CREATE TABLE addonlinkrepo (idRepo integer, idAddon integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_addonlinkrepo_1 ON addonlinkrepo ( idAddon, idRepo )\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_addonlinkrepo_2 ON addonlinkrepo ( idRepo, idAddon )\n");

    CLog::Log(LOGINFO, "create disabled table");
    m_pDS->exec("CREATE TABLE disabled (id integer primary key, addonID text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxDisabled ON disabled(addonID)");

    CLog::Log(LOGINFO, "create broken table");
    m_pDS->exec("CREATE TABLE broken (id integer primary key, addonID text, reason text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxBroken ON broken(addonID)");

    CLog::Log(LOGINFO, "create blacklist table");
    m_pDS->exec("CREATE TABLE blacklist (id integer primary key, addonID text, version text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxBlack ON blacklist(addonID)");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables", __FUNCTION__);
    return false;
  }

  return true;
}

bool CAddonDatabase::UpdateOldVersion(int version)
{
  if (version < 13)
  {
    m_pDS->exec("CREATE TABLE dependencies (id integer, addon text, version text, optional boolean)\n");
    m_pDS->exec("CREATE INDEX idxDependencies ON dependencies(id)");
  }
  if (version < 14)
  {
    m_pDS->exec("ALTER TABLE addon add minversion text");
  }
  if (version < 15)
  {
    m_pDS->exec("CREATE TABLE blacklist (id integer primary key, addonID text, version text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxBlack ON blacklist(addonID)");
  }
  return true;
}

int CAddonDatabase::AddAddon(const AddonPtr& addon,
                             int idRepo)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString sql = PrepareSQL("insert into addon (id, type, name, summary,"
                               "description, stars, path, icon, changelog, "
                               "fanart, addonID, version, author, disclaimer, minversion)"
                               " values(NULL, '%s', '%s', '%s', '%s', %i,"
                               "'%s', '%s', '%s', '%s', '%s','%s','%s','%s','%s')",
                               TranslateType(addon->Type(),false).c_str(),
                               addon->Name().c_str(), addon->Summary().c_str(),
                               addon->Description().c_str(),addon->Stars(),
                               addon->Path().c_str(), addon->Props().icon.c_str(),
                               addon->ChangeLog().c_str(),addon->FanArt().c_str(),
                               addon->ID().c_str(), addon->Version().c_str(),
                               addon->Author().c_str(),addon->Disclaimer().c_str(),
                               addon->MinVersion().c_str());
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
      sql = PrepareSQL("insert into dependencies(id, addon, version, optional) values (%i, '%s', '%s', %i)", idAddon, i->first.c_str(), i->second.first.c_str(), i->second.second ? 1 : 0);
      m_pDS->exec(sql.c_str());
    }
    // these need to be configured
    if (addon->Type() == ADDON_PVRDLL)
      DisableAddon(addon->ID(), true);
    return idAddon;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addon->Name().c_str());
  }
  return -1;
}

bool CAddonDatabase::GetAddon(const CStdString& id, AddonPtr& addon)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // there may be multiple addons with this id (eg from different repositories) in the database,
    // so we want to retrieve the latest version.  Order by version won't work as the database
    // won't know that 1.10 > 1.2, so grab them all and order outside
    CStdString sql = PrepareSQL("select id,version from addon where addonID='%s'",id.c_str());
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

bool CAddonDatabase::GetRepoForAddon(const CStdString& addonID, CStdString& repo)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    CStdString sql = PrepareSQL("select repo.addonID from repo join addonlinkrepo on repo.id=addonlinkrepo.idRepo join addon on addonlinkrepo.idAddon=addon.id where addon.addonID like '%s'", addonID.c_str()); 
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

bool CAddonDatabase::GetAddon(int id, AddonPtr& addon)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    CStdString sql = PrepareSQL("select * from addon where id=%i",id);
    m_pDS2->query(sql.c_str());
    if (!m_pDS2->eof())
    {
      AddonProps props(m_pDS2->fv("addonID" ).get_asString(),
                       TranslateType(m_pDS2->fv("type").get_asString()),
                       m_pDS2->fv("version").get_asString(),
                       m_pDS2->fv("minversion").get_asString());
      props.name = m_pDS2->fv("name").get_asString();
      props.summary = m_pDS2->fv("summary").get_asString();
      props.description = m_pDS2->fv("description").get_asString();
      props.changelog = m_pDS2->fv("changelog").get_asString();
      props.path = m_pDS2->fv("path").get_asString();
      props.icon = m_pDS2->fv("icon").get_asString();
      props.fanart = m_pDS2->fv("fanart").get_asString();
      props.author = m_pDS2->fv("author").get_asString();
      props.disclaimer = m_pDS2->fv("disclaimer").get_asString();
      sql = PrepareSQL("select reason from broken where addonID='%s'",props.id.c_str());
      m_pDS2->query(sql.c_str());
      if (!m_pDS2->eof())
        props.broken = m_pDS2->fv(0).get_asString();

      sql = PrepareSQL("select key,value from addonextra where id=%i", id);
      m_pDS2->query(sql.c_str());
      while (!m_pDS2->eof())
      {
        props.extrainfo.insert(make_pair(m_pDS2->fv(0).get_asString(), m_pDS2->fv(1).get_asString()));
        m_pDS2->next();
      }

      sql = PrepareSQL("select addon,version,optional from dependencies where id=%i", id);
      m_pDS2->query(sql.c_str());
      while (!m_pDS2->eof())
      {
        props.dependencies.insert(make_pair(m_pDS2->fv(0).get_asString(), make_pair(AddonVersion(m_pDS2->fv(1).get_asString()), m_pDS2->fv(2).get_asBool())));
        m_pDS2->next();
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

bool CAddonDatabase::GetAddons(VECADDONS& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    CStdString sql = PrepareSQL("select distinct addonID from addon");
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      sql = PrepareSQL("select id from addon where addonID='%s' order by version desc",m_pDS->fv(0).get_asString().c_str());
      m_pDS2->query(sql.c_str());
      AddonPtr addon;
      if (GetAddon(m_pDS2->fv(0).get_asInt(),addon))
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

void CAddonDatabase::DeleteRepository(const CStdString& id)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString sql = PrepareSQL("select id from repo where addonID='%s'",id.c_str());
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

    CStdString sql = PrepareSQL("delete from repo where id=%i",idRepo);
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

int CAddonDatabase::AddRepository(const CStdString& id, const VECADDONS& addons, const CStdString& checksum)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString sql;
    int idRepo = GetRepoChecksum(id,sql);
    if (idRepo > -1)
      DeleteRepository(idRepo);

    BeginTransaction();

    CDateTime time = CDateTime::GetCurrentDateTime();
    sql = PrepareSQL("insert into repo (id,addonID,checksum,lastcheck) values (NULL,'%s','%s','%s')",id.c_str(),checksum.c_str(),time.GetAsDBDateTime().c_str());
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

int CAddonDatabase::GetRepoChecksum(const CStdString& id, CStdString& checksum)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
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
  checksum.Empty();
  return -1;
}

CDateTime CAddonDatabase::GetRepoTimestamp(const CStdString& id)
{
  CDateTime date;
  try
  {
    if (NULL == m_pDB.get()) return date;
    if (NULL == m_pDS.get()) return date;

    CStdString strSQL = PrepareSQL("select * from repo where addonID='%s'",id.c_str());
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

bool CAddonDatabase::SetRepoTimestamp(const CStdString& id, const CStdString& time)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("update repo set lastcheck='%s' where addonID='%s'",time.c_str(),id.c_str());
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

    CStdString strSQL = PrepareSQL("select * from addonlinkrepo where idRepo=%i",id);
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

bool CAddonDatabase::GetRepository(const CStdString& id, VECADDONS& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = PrepareSQL("select id from repo where addonID='%s'",id.c_str());
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

bool CAddonDatabase::Search(const CStdString& search, VECADDONS& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    strSQL=PrepareSQL("SELECT id FROM addon WHERE name LIKE '%%%s%%' OR summary LIKE '%%%s%%' OR description LIKE '%%%s%%'", search.c_str(), search.c_str(), search.c_str());
    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, strSQL.c_str());

    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0) return false;

    while (!m_pDS->eof())
    {
      AddonPtr addon;
      GetAddon(m_pDS->fv(0).get_asInt(),addon);
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
  pItem->SetProperty("Addon.Version", addon->Version().c_str());
  pItem->SetProperty("Addon.Summary", addon->Summary());
  pItem->SetProperty("Addon.Description", addon->Description());
  pItem->SetProperty("Addon.Creator", addon->Author());
  pItem->SetProperty("Addon.Disclaimer", addon->Disclaimer());
  pItem->SetProperty("Addon.Rating", addon->Stars());
  CStdString starrating;
  starrating.Format("rating%d.png", addon->Stars());
  pItem->SetProperty("Addon.StarRating",starrating);
  pItem->SetProperty("Addon.Path", addon->Path());
  pItem->SetProperty("Addon.Broken", addon->Props().broken);
  std::map<CStdString,CStdString>::iterator it = 
                    addon->Props().extrainfo.find("language");
  if (it != addon->Props().extrainfo.end())
    pItem->SetProperty("Addon.Language", it->second);
}

bool CAddonDatabase::DisableAddon(const CStdString &addonID, bool disable /* = true */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (disable)
    {
      CStdString sql = PrepareSQL("select id from disabled where addonID='%s'", addonID.c_str());
      m_pDS->query(sql.c_str());
      if (m_pDS->eof()) // not found
      {
        m_pDS->close();
        sql = PrepareSQL("insert into disabled(id, addonID) values(NULL, '%s')", addonID.c_str());
        m_pDS->exec(sql);

        AddonPtr addon;
        // If the addon is a service, stop it
        if (CAddonMgr::Get().GetAddon(addonID, addon, ADDON_SERVICE, false) && addon)
        {
          boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(addon);
          if (service)
            service->Stop();
        }

        return true;
      }
      return false; // already disabled or failed query
    }
    else
    {
      CStdString sql = PrepareSQL("delete from disabled where addonID='%s'", addonID.c_str());
      m_pDS->exec(sql);

      AddonPtr addon;
      // If the addon is a service, start it
      if (CAddonMgr::Get().GetAddon(addonID, addon, ADDON_SERVICE, false) && addon)
      {
        boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(addon);
        if (service)
          service->Start();
      }

    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s'", __FUNCTION__, addonID.c_str());
  }
  return false;
}

bool CAddonDatabase::BreakAddon(const CStdString &addonID, const CStdString& reason)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("delete from broken where addonID='%s'", addonID.c_str());
    m_pDS->exec(sql);

    if (!reason.IsEmpty())
    { // broken
      sql = PrepareSQL("insert into broken(id, addonID, reason) values(NULL, '%s', '%s')", addonID.c_str(),reason.c_str());
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

bool CAddonDatabase::HasAddon(const CStdString &addonID)
{
  CStdString strWhereClause = PrepareSQL("addonID = '%s'", addonID.c_str());
  CStdString strHasAddon = GetSingleValue("addon", "id", strWhereClause);
  
  return !strHasAddon.IsEmpty();
}

bool CAddonDatabase::IsAddonDisabled(const CStdString &addonID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("select id from disabled where addonID='%s'", addonID.c_str());
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

bool CAddonDatabase::IsSystemPVRAddonEnabled(const CStdString &addonID)
{
  CStdString strWhereClause = PrepareSQL("addonID = '%s'", addonID.c_str());
  CStdString strEnabled = GetSingleValue("pvrenabled", "id", strWhereClause);

  return !strEnabled.IsEmpty();
}

CStdString CAddonDatabase::IsAddonBroken(const CStdString &addonID)
{
  try
  {
    if (NULL == m_pDB.get()) return "";
    if (NULL == m_pDS.get()) return "";

    CStdString sql = PrepareSQL("select reason from broken where addonID='%s'", addonID.c_str());
    m_pDS->query(sql.c_str());
    CStdString ret;
    if (!m_pDS->eof())
      ret = m_pDS->fv(0).get_asString();
    m_pDS->close();
    return ret;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, addonID.c_str());
  }
  return "";
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

bool CAddonDatabase::BlacklistAddon(const CStdString& addonID,
                                    const CStdString& version)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("insert into blacklist(id, addonID, version) values(NULL, '%s', '%s')", addonID.c_str(),version.c_str());
    m_pDS->exec(sql);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s' for version '%s'", __FUNCTION__, addonID.c_str(),version.c_str());
  }
  return false;
}

bool CAddonDatabase::IsAddonBlacklisted(const CStdString& addonID,
                                        const CStdString& version)
{
  CStdString where = PrepareSQL("addonID='%s' and version='%s'",addonID.c_str(),version.c_str());
  return !GetSingleValue("blacklist","addonID",where).IsEmpty();
}

bool CAddonDatabase::RemoveAddonFromBlacklist(const CStdString& addonID,
                                              const CStdString& version)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("delete from blacklist where addonID='%s' and version='%s'",addonID.c_str(),version.c_str());
    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon '%s' for version '%s'", __FUNCTION__, addonID.c_str(),version.c_str());
  }
  return false;
}
