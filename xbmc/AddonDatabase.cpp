/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AddonDatabase.h"
#include "utils/log.h"
#include "DateTime.h"

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

bool CAddonDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create addon table");
    m_pDS->exec("CREATE TABLE addon (id integer primary key, type text,"
                "name text, summary text, description text, stars integer,"
                "path text, addonID text, icon text, version text)\n");

    CLog::Log(LOGINFO, "create addon index");
    m_pDS->exec("CREATE INDEX idxAddon ON addon(addonID)");

    CLog::Log(LOGINFO, "create repo table");
    m_pDS->exec("CREATE TABLE repo (id integer primary key, addonID text,"
                "checksum text, lastcheck text)\n");

    CLog::Log(LOGINFO, "create addonlinkrepo table");
    m_pDS->exec("CREATE TABLE addonlinkrepo (idRepo integer, idAddon integer)\n");
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
  if (version < 2)
  {
    m_pDS->exec("alter table addon add description text");
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

    CStdString sql = FormatSQL("insert into addon (id, type, name, summary,"
                               "description,stars, path, icon, addonID,version)"
                               " values(NULL, '%s', '%s', '%s', '%s', %i,"
                               "'%s', '%s', '%s', '%s')",
                               TranslateType(addon->Type(),false).c_str(),
                               addon->Name().c_str(), addon->Summary().c_str(),
                               addon->Description().c_str(),addon->Stars(),
                               addon->Path().c_str(), addon->Props().icon.c_str(),
                               addon->ID().c_str(), addon->Version().str.c_str());
    m_pDS->exec(sql.c_str());
    int idAddon = (int)m_pDS->lastinsertid();

    sql = FormatSQL("insert into addonlinkrepo (idRepo, idAddon) values (%i,%i)",idRepo,idAddon);
    m_pDS->exec(sql.c_str());
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

    CStdString sql = FormatSQL("select id from addon where addonID='%s' order by version desc",id.c_str());
    m_pDS2->query(sql.c_str());
    if (!m_pDS2->eof())
      return GetAddon(m_pDS2->fv(0).get_asInt(),addon);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on addon %s", __FUNCTION__, id.c_str());
  }
  addon.reset();
  return false;
}

bool CAddonDatabase::GetAddon(int id, AddonPtr& addon)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    CStdString sql = FormatSQL("select * from addon where id=%i",id);
    m_pDS2->query(sql.c_str());
    if (!m_pDS2->eof())
    {
      AddonProps props(m_pDS2->fv("addonID" ).get_asString(),
                       TranslateType(m_pDS2->fv("type").get_asString()),
                       m_pDS2->fv("version").get_asString());
      props.name = m_pDS2->fv("name").get_asString();
      props.summary = m_pDS2->fv("summary").get_asString();
      props.description = m_pDS2->fv("description").get_asString();
      props.path = m_pDS2->fv("path").get_asString();
      props.icon = m_pDS2->fv("icon").get_asString();
      addon = CAddonMgr::AddonFromProps(props);
      return true;
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

    CStdString sql = FormatSQL("select distinct addonID from addon");
    m_pDS->query(sql.c_str());
    while (!m_pDS->eof())
    {
      sql = FormatSQL("select id from addon where addonID='%s' order by version desc",m_pDS->fv(0).get_asString().c_str());
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

    CStdString sql = FormatSQL("select id from repo where addonID='%s'",id.c_str());
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

    CStdString sql = FormatSQL("delete from repo where id=%i",idRepo);
    m_pDS->exec(sql.c_str());
    sql = FormatSQL("delete from addon where id in (select idAddon from addonlinkrepo where idRepo=%i)",idRepo);
    m_pDS->exec(sql.c_str());
    sql = FormatSQL("delete from addonlinkrepo where idRepo=%i",idRepo);
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

    CDateTime time = CDateTime::GetCurrentDateTime();
    sql = FormatSQL("insert into repo (id,addonID,checksum,lastcheck) values (NULL,'%s','%s','%s')",id.c_str(),checksum.c_str(),time.GetAsDBDateTime().c_str());
    m_pDS->exec(sql.c_str());
    idRepo = (int)m_pDS->lastinsertid();
    for (unsigned int i=0;i<addons.size();++i)
      AddAddon(addons[i],idRepo);

    return idRepo;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
  return -1;
}

int CAddonDatabase::GetRepoChecksum(const CStdString& id, CStdString& checksum)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL = FormatSQL("select * from repo where addonID='%s'",id.c_str());
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

int CAddonDatabase::GetRepoTimestamp(const CStdString& id, CStdString& timestamp)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL = FormatSQL("select * from repo where addonID='%s'",id.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
    {
      timestamp = m_pDS->fv("lastcheck").get_asString();
      return m_pDS->fv("id").get_asInt();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on repo '%s'", __FUNCTION__, id.c_str());
  }
  timestamp.Empty();
  return -1;
}

bool CAddonDatabase::SetRepoTimestamp(const CStdString& id, const CStdString& time)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = FormatSQL("update repo set lastcheck='%s' where addonID='%s'",time.c_str(),id.c_str());
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

    CStdString strSQL = FormatSQL("select * from addonlinkrepo where idRepo=%i",id);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      AddonPtr addon;
      GetAddon(m_pDS->fv("idAddon").get_asInt(),addon);
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

    CStdString strSQL = FormatSQL("select id from repo where addonID='%s'",id.c_str());
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

bool CAddonDatabase::Search(const CStdString& search, VECADDONS& items)
{
  // first grab all the addons that match
  SearchTitle(search,items);

  return true;
}

bool CAddonDatabase::SearchTitle(const CStdString& search, VECADDONS& addons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    strSQL=FormatSQL("select idAddon from addon where name like '%s%%'", search.c_str());

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
  pItem->SetProperty("Addon.Version", addon->Version().str);
  pItem->SetProperty("Addon.Summary", addon->Summary());
  pItem->SetProperty("Addon.Description", addon->Description());
  pItem->SetProperty("Addon.Creator", addon->Author());
  pItem->SetProperty("Addon.Disclaimer", addon->Disclaimer());
  pItem->SetProperty("Addon.Rating", addon->Stars());
  pItem->SetProperty("Addon.Path", addon->Path());
}
