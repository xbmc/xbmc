/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "CommonDatabase.h"
//system.h defines HAS_MYSQL and thus is needed here
#include "system.h"

#ifdef HAS_MYSQL
#include <odb/mysql/database.hxx>
#include <odb/mysql/connection-factory.hxx>
#endif
#include <odb/sqlite/database.hxx>
#include <odb/sqlite/connection-factory.hxx>

#include <odb/transaction.hxx>
#include <odb/session.hxx>
#include <odb/schema-catalog.hxx>

#include <odb/odb_gen/ODBBookmark.h>
#include <odb/odb_gen/ODBBookmark_odb.h>
#include <odb/odb_gen/ODBFile.h>
#include <odb/odb_gen/ODBFile_odb.h>
#include <odb/odb_gen/ODBGenre.h>
#include <odb/odb_gen/ODBGenre_odb.h>
#include <odb/odb_gen/ODBMovie.h>
#include <odb/odb_gen/ODBMovie_odb.h>
#include <odb/odb_gen/ODBPath.h>
#include <odb/odb_gen/ODBPath_odb.h>
#include <odb/odb_gen/ODBPerson.h>
#include <odb/odb_gen/ODBPerson_odb.h>
#include <odb/odb_gen/ODBPersonLink.h>
#include <odb/odb_gen/ODBPersonLink_odb.h>
#include <odb/odb_gen/ODBRating.h>
#include <odb/odb_gen/ODBRating_odb.h>
#include <odb/odb_gen/ODBSet.h>
#include <odb/odb_gen/ODBSet_odb.h>
#include <odb/odb_gen/ODBStreamDetails.h>
#include <odb/odb_gen/ODBStreamDetails_odb.h>
#include <odb/odb_gen/ODBUniqueID.h>
#include <odb/odb_gen/ODBUniqueID_odb.h>

#include "filesystem/SpecialProtocol.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

class odb_logtracer: public odb::tracer
{
public:
  odb_logtracer(){};
  virtual ~odb_logtracer(){};
  
  virtual void
  prepare (odb::connection& c, const odb::statement& s)
  {
    CLog::Log(LOGDEBUG, "commondb: PREPARE - %s", s.text());
  }
  virtual void
  execute (odb::connection& c, const odb::statement& s)
  {
    CLog::Log(LOGDEBUG, "commondb: EXECUTE - %s", s.text());
  }
  virtual void
  execute (odb::connection& c, const char* statement)
  {
    CLog::Log(LOGDEBUG, "commondb: %s", statement);
  }
  virtual void
  deallocate (odb::connection& c, const odb::statement& s)
  {
    CLog::Log(LOGDEBUG, "commondb: DEALLOCATE - %s", s.text());
  }
};

odb_logtracer odb_tracer;

CCommonDatabase &CCommonDatabase::GetInstance()
{
  static CCommonDatabase s_commondb;
  return s_commondb;
}

CCommonDatabase::CCommonDatabase()
{
  DatabaseSettings settings = &g_advancedSettings.m_databaseCommon ? g_advancedSettings.m_databaseCommon : DatabaseSettings();

#ifdef HAS_MYSQL
  if (settings.type == "mysql")
  {
    std::auto_ptr<odb::mysql::connection_factory> mysql_pool(new odb::mysql::connection_pool_factory(20));
    m_db = std::shared_ptr<odb::core::database>( new odb::mysql::database(settings.user,
                                                                          settings.pass,
                                                                          "common",
                                                                          settings.host,
                                                                          std::stoi(settings.port),
                                                                          0,
                                                                          "utf8",
                                                                          0,
                                                                          mysql_pool));
    
    
  }

  //use sqlite3 per default
  else
#endif
  {
    std::string dbfolder = CSpecialProtocol::TranslatePath("special://database/");
    std::auto_ptr<odb::sqlite::connection_factory> sqlite_pool(new odb::sqlite::connection_pool_factory(20));
    m_db = std::shared_ptr<odb::core::database>( new odb::sqlite::database(dbfolder + "/common.db",
                                                                           SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                                                           true,
                                                                           "",
                                                                           sqlite_pool));
  }
  
  if (settings.tracer)
    m_db->tracer(odb_tracer);

  //if(!odb::session::has_current())
    //m_odb_session = std::shared_ptr<odb::session>(new odb::session);
}

void CCommonDatabase::init()
{
    odb::core::transaction t (m_db->begin());
    odb::core::schema_catalog::migrate (*m_db);
    t.commit();
}

std::shared_ptr<odb::transaction> CCommonDatabase::getTransaction()
{
  if (!odb::transaction::has_current())
    return std::shared_ptr<odb::transaction>(new odb::transaction(m_db->begin()));
  return nullptr;
}
