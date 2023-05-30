/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "mysqldataset.h"

#include "Util.h"
#include "network/DNSNameCache.h"
#include "network/WakeOnAccess.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <string>
#ifdef HAS_MYSQL
#include <mysql/errmsg.h>
#elif defined(HAS_MARIADB)
#include <mariadb/errmsg.h>
#endif

#ifdef TARGET_POSIX
#include "platform/posix/ConvUtils.h"
#endif

#define MYSQL_OK 0
#define ER_BAD_DB_ERROR 1049

namespace dbiplus
{

//************* MysqlDatabase implementation ***************

MysqlDatabase::MysqlDatabase()
{

  active = false;
  _in_transaction = false; // for transaction

  error = "Unknown database error"; //S_NO_CONNECTION;
  host = "localhost";
  port = "3306";
  db = "mysql";
  login = "root";
  passwd = "null";
  conn = NULL;
  default_charset = "";
}

MysqlDatabase::~MysqlDatabase()
{
  disconnect();
}

Dataset* MysqlDatabase::CreateDataset() const
{
  return new MysqlDataset(const_cast<MysqlDatabase*>(this));
}

int MysqlDatabase::status(void)
{
  if (active == false)
    return DB_CONNECTION_NONE;
  return DB_CONNECTION_OK;
}

int MysqlDatabase::setErr(int err_code, const char* qry)
{
  switch (err_code)
  {
    case MYSQL_OK:
      error = "Successful result";
      break;
    case CR_COMMANDS_OUT_OF_SYNC:
      error = "Commands were executed in an improper order";
      break;
    case CR_SERVER_GONE_ERROR:
      error = "The MySQL server has gone away";
      break;
    case CR_SERVER_LOST:
      error = "The connection to the server was lost during this query";
      break;
    case CR_UNKNOWN_ERROR:
      error = "An unknown error occurred";
      break;
    case 1146: /* ER_NO_SUCH_TABLE */
      error = "The table does not exist";
      break;
    default:
      char err[256];
      snprintf(err, sizeof(err), "Undefined MySQL error: Code (%d)", err_code);
      error = err;
  }
  error = "[" + db + "] " + error;
  error += "\nQuery: ";
  error += qry;
  error += "\n";
  return err_code;
}

const char* MysqlDatabase::getErrorMsg()
{
  return error.c_str();
}

void MysqlDatabase::configure_connection()
{
  char sqlcmd[512];
  int ret;

  // MySQL 5.7.5+: See #8393
  strcpy(sqlcmd,
         "SET SESSION sql_mode = (SELECT REPLACE(@@SESSION.sql_mode,'ONLY_FULL_GROUP_BY',''))");
  if ((ret = mysql_real_query(conn, sqlcmd, strlen(sqlcmd))) != MYSQL_OK)
    throw DbErrors("Can't disable sql_mode ONLY_FULL_GROUP_BY: '%s' (%d)", db.c_str(), ret);

  // MySQL 5.7.6+: See #8393. Non-fatal if error, as not supported by MySQL 5.0.x
  strcpy(sqlcmd, "SELECT @@SESSION.optimizer_switch");
  if ((ret = mysql_real_query(conn, sqlcmd, strlen(sqlcmd))) == MYSQL_OK)
  {
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;

    if (res)
    {
      if ((row = mysql_fetch_row(res)) != NULL)
      {
        std::string column = row[0];
        std::vector<std::string> split = StringUtils::Split(column, ',');

        for (std::string& itIn : split)
        {
          if (StringUtils::Trim(itIn) == "derived_merge=on")
          {
            strcpy(sqlcmd, "SET SESSION optimizer_switch = 'derived_merge=off'");
            if ((ret = mysql_real_query(conn, sqlcmd, strlen(sqlcmd))) != MYSQL_OK)
              throw DbErrors("Can't set optimizer_switch = '%s': '%s' (%d)",
                             StringUtils::Trim(itIn).c_str(), db.c_str(), ret);
            break;
          }
        }
      }
      mysql_free_result(res);
    }
  }
  else
    CLog::Log(LOGWARNING, "Unable to query optimizer_switch: '{}' ({})", db, ret);
}

int MysqlDatabase::connect(bool create_new)
{
  if (host.empty() || db.empty())
    return DB_CONNECTION_NONE;

  std::string resolvedHost;
  if (!StringUtils::EqualsNoCase(host, "localhost") && CDNSNameCache::Lookup(host, resolvedHost))
  {
    CLog::Log(LOGDEBUG, "{} replacing configured host {} with resolved host {}", __FUNCTION__, host,
              resolvedHost);
    host = resolvedHost;
  }

  try
  {
    disconnect();

    if (conn == NULL)
    {
      conn = mysql_init(conn);
      mysql_ssl_set(conn, key.empty() ? NULL : key.c_str(), cert.empty() ? NULL : cert.c_str(),
                    ca.empty() ? NULL : ca.c_str(), capath.empty() ? NULL : capath.c_str(),
                    ciphers.empty() ? NULL : ciphers.c_str());
    }

    if (!CWakeOnAccess::GetInstance().WakeUpHost(host, "MySQL : " + db))
      return DB_CONNECTION_NONE;

    // establish connection with just user credentials
    if (mysql_real_connect(conn, host.c_str(), login.c_str(), passwd.c_str(), NULL,
                           atoi(port.c_str()), NULL, compression ? CLIENT_COMPRESS : 0) != NULL)
    {
      static bool showed_ver_info = false;
      if (!showed_ver_info)
      {
        std::string version_string = mysql_get_server_info(conn);
        CLog::Log(LOGINFO, "MYSQL: Connected to version {}", version_string);
        showed_ver_info = true;
        unsigned long version = mysql_get_server_version(conn);
        // Minimum for MySQL: 5.6 (5.5 is EOL)
        unsigned long min_version = 50600;
        if (version_string.find("MariaDB") != std::string::npos)
        {
          // Minimum for MariaDB: 5.5 (still supported)
          min_version = 50500;
        }

        if (version < min_version)
        {
          CLog::Log(
              LOGWARNING,
              "MYSQL: Your database server version {} is very old and might not be supported in "
              "future Kodi versions. Please consider upgrading to MySQL 5.7 or MariaDB 10.2.",
              version_string);
        }
      }

      // disable mysql autocommit since we handle it
      //mysql_autocommit(conn, false);

      // enforce utf8 charset usage
      default_charset = mysql_character_set_name(conn);
      if (mysql_set_character_set(conn, "utf8")) // returns 0 on success
      {
        CLog::Log(LOGERROR, "Unable to set utf8 charset: {} [{}]({})", db, mysql_errno(conn),
                  mysql_error(conn));
      }

      configure_connection();

      // check existence
      if (exists())
      {
        // nothing to see here
      }
      else if (create_new)
      {
        char sqlcmd[512];
        int ret;

        snprintf(sqlcmd, sizeof(sqlcmd),
                 "CREATE DATABASE `%s` CHARACTER SET utf8 COLLATE utf8_general_ci", db.c_str());
        if ((ret = query_with_reconnect(sqlcmd)) != MYSQL_OK)
        {
          throw DbErrors("Can't create new database: '%s' (%d)", db.c_str(), ret);
        }
      }

      if (mysql_select_db(conn, db.c_str()) == 0)
      {
        active = true;
        return DB_CONNECTION_OK;
      }
    }

    // if we failed above, either credentials were incorrect or the database didn't exist
    if (mysql_errno(conn) == ER_BAD_DB_ERROR && create_new)
    {

      if (create() == MYSQL_OK)
      {
        active = true;

        return DB_CONNECTION_OK;
      }
    }

    CLog::Log(LOGERROR, "Unable to open database: {} [{}]({})", db, mysql_errno(conn),
              mysql_error(conn));

    return DB_CONNECTION_NONE;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Unable to open database: {} ({})", db, GetLastError());
  }
  return DB_CONNECTION_NONE;
}

void MysqlDatabase::disconnect(void)
{
  if (conn != NULL)
  {
    mysql_close(conn);
    conn = NULL;
  }

  active = false;
}

int MysqlDatabase::create()
{
  return connect(true);
}

int MysqlDatabase::drop()
{
  if (!active)
    throw DbErrors("Can't drop database: no active connection...");
  char sqlcmd[512];
  int ret;
  snprintf(sqlcmd, sizeof(sqlcmd), "DROP DATABASE `%s`", db.c_str());
  if ((ret = query_with_reconnect(sqlcmd)) != MYSQL_OK)
  {
    throw DbErrors("Can't drop database: '%s' (%d)", db.c_str(), ret);
  }
  disconnect();
  return DB_COMMAND_OK;
}

int MysqlDatabase::copy(const char* backup_name)
{
  if (!active || conn == NULL)
    throw DbErrors("Can't copy database: no active connection...");

  char sql[4096];
  int ret;

  // ensure we're connected to the db we are about to copy
  if ((ret = mysql_select_db(conn, db.c_str())) != MYSQL_OK)
    throw DbErrors("Can't connect to source database: '%s'", db.c_str());

  // grab a list of base tables only (no views)
  snprintf(sql, sizeof(sql), "SHOW FULL TABLES WHERE Table_type = 'BASE TABLE'");
  if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
    throw DbErrors("Can't determine base tables for copy.");

  // get list of all tables from old DB
  MYSQL_RES* res = mysql_store_result(conn);

  if (res)
  {
    if (mysql_num_rows(res) == 0)
    {
      mysql_free_result(res);
      throw DbErrors("The source database was unexpectedly empty.");
    }

    // create the new database
    snprintf(sql, sizeof(sql), "CREATE DATABASE `%s` CHARACTER SET utf8 COLLATE utf8_general_ci",
             backup_name);
    if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
    {
      mysql_free_result(res);
      throw DbErrors("Can't create database for copy: '%s' (%d)", db.c_str(), ret);
    }

    MYSQL_ROW row;

    // duplicate each table from old db to new db
    while ((row = mysql_fetch_row(res)) != NULL)
    {
      // copy the table definition
      snprintf(sql, sizeof(sql), "CREATE TABLE `%s`.%s LIKE %s", backup_name, row[0], row[0]);

      if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
      {
        mysql_free_result(res);
        throw DbErrors("Can't copy schema for table '%s'\nError: %d", row[0], ret);
      }

      // copy the table data
      snprintf(sql, sizeof(sql), "INSERT INTO `%s`.%s SELECT * FROM %s", backup_name, row[0],
               row[0]);

      if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
      {
        mysql_free_result(res);
        throw DbErrors("Can't copy data for table '%s'\nError: %d", row[0], ret);
      }
    }
    mysql_free_result(res);

    // we don't recreate views, indices, or triggers on copy
    // as we'll be dropping and recreating them anyway
  }

  return 1;
}

int MysqlDatabase::drop_analytics(void)
{
  if (!active || conn == NULL)
    throw DbErrors("Can't clean database: no active connection...");

  char sql[4096];
  int ret;

  // ensure we're connected to the db we are about to clean from stuff
  if ((ret = mysql_select_db(conn, db.c_str())) != MYSQL_OK)
    throw DbErrors("Can't connect to database: '%s'", db.c_str());

  // getting a list of indexes in the database
  snprintf(sql, sizeof(sql),
           "SELECT DISTINCT table_name, index_name "
           "FROM information_schema.statistics "
           "WHERE index_name != 'PRIMARY' AND table_schema = '%s'",
           db.c_str());
  if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
    throw DbErrors("Can't determine list of indexes to drop.");

  // we will acquire lists here
  MYSQL_RES* res = mysql_store_result(conn);
  MYSQL_ROW row;

  if (res)
  {
    while ((row = mysql_fetch_row(res)) != NULL)
    {
      snprintf(sql, sizeof(sql), "ALTER TABLE `%s`.%s DROP INDEX %s", db.c_str(), row[0], row[1]);

      if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
      {
        mysql_free_result(res);
        throw DbErrors("Can't drop index '%s'\nError: %d", row[0], ret);
      }
    }
    mysql_free_result(res);
  }

  // next topic is a views list
  snprintf(sql, sizeof(sql),
           "SELECT table_name FROM information_schema.views WHERE table_schema = '%s'", db.c_str());
  if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
    throw DbErrors("Can't determine list of views to drop.");

  res = mysql_store_result(conn);

  if (res)
  {
    while ((row = mysql_fetch_row(res)) != NULL)
    {
      /* we do not need IF EXISTS because these views are exist */
      snprintf(sql, sizeof(sql), "DROP VIEW `%s`.%s", db.c_str(), row[0]);

      if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
      {
        mysql_free_result(res);
        throw DbErrors("Can't drop view '%s'\nError: %d", row[0], ret);
      }
    }
    mysql_free_result(res);
  }

  // triggers
  snprintf(sql, sizeof(sql),
           "SELECT trigger_name FROM information_schema.triggers WHERE event_object_schema = '%s'",
           db.c_str());
  if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
    throw DbErrors("Can't determine list of triggers to drop.");

  res = mysql_store_result(conn);

  if (res)
  {
    while ((row = mysql_fetch_row(res)) != NULL)
    {
      snprintf(sql, sizeof(sql), "DROP TRIGGER `%s`.%s", db.c_str(), row[0]);

      if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
      {
        mysql_free_result(res);
        throw DbErrors("Can't drop trigger '%s'\nError: %d", row[0], ret);
      }
    }
    mysql_free_result(res);
  }

  // Native functions
  snprintf(sql, sizeof(sql),
           "SELECT routine_name FROM information_schema.routines "
           "WHERE routine_type = 'FUNCTION' and routine_schema = '%s'",
           db.c_str());
  if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
    throw DbErrors("Can't determine list of routines to drop.");

  res = mysql_store_result(conn);

  if (res)
  {
    while ((row = mysql_fetch_row(res)) != NULL)
    {
      snprintf(sql, sizeof(sql), "DROP FUNCTION `%s`.%s", db.c_str(), row[0]);

      if ((ret = query_with_reconnect(sql)) != MYSQL_OK)
      {
        mysql_free_result(res);
        throw DbErrors("Can't drop function '%s'\nError: %d", row[0], ret);
      }
    }
    mysql_free_result(res);
  }

  return 1;
}

int MysqlDatabase::query_with_reconnect(const char* query)
{
  int attempts = 5;
  int result;

  // try to reconnect if server is gone
  while (((result = mysql_real_query(conn, query, strlen(query))) != MYSQL_OK) &&
         ((result = mysql_errno(conn)) == CR_SERVER_GONE_ERROR || result == CR_SERVER_LOST) &&
         (attempts-- > 0))
  {
    CLog::Log(LOGINFO, "MYSQL server has gone. Will try {} more attempt(s) to reconnect.",
              attempts);
    active = false;
    connect(true);
  }

  return result;
}

long MysqlDatabase::nextid(const char* sname)
{
  CLog::Log(LOGDEBUG, "MysqlDatabase::nextid for {}", sname);
  if (!active)
    return DB_UNEXPECTED_RESULT;
  const char* seq_table = "sys_seq";
  int id; /*,nrow,ncol;*/
  MYSQL_RES* res;
  char sqlcmd[512];
  snprintf(sqlcmd, sizeof(sqlcmd), "SELECT nextid FROM %s WHERE seq_name = '%s'", seq_table, sname);
  CLog::Log(LOGDEBUG, "MysqlDatabase::nextid will request");
  if ((last_err = query_with_reconnect(sqlcmd)) != 0)
  {
    return DB_UNEXPECTED_RESULT;
  }
  res = mysql_store_result(conn);
  if (res)
  {
    if (mysql_num_rows(res) == 0)
    {
      id = 1;
      snprintf(sqlcmd, sizeof(sqlcmd), "INSERT INTO %s (nextid,seq_name) VALUES (%d,'%s')",
               seq_table, id, sname);
      mysql_free_result(res);
      if ((last_err = query_with_reconnect(sqlcmd)) != 0)
        return DB_UNEXPECTED_RESULT;
      return id;
    }
    else
    {
      id = -1;
      snprintf(sqlcmd, sizeof(sqlcmd), "UPDATE %s SET nextid=%d WHERE seq_name = '%s'", seq_table,
               id, sname);
      mysql_free_result(res);
      if ((last_err = query_with_reconnect(sqlcmd)) != 0)
        return DB_UNEXPECTED_RESULT;
      return id;
    }
  }
  return DB_UNEXPECTED_RESULT;
}

// methods for transactions
// ---------------------------------------------
void MysqlDatabase::start_transaction()
{
  if (active)
  {
    mysql_autocommit(conn, false);
    CLog::Log(LOGDEBUG, "Mysql Start transaction");
    _in_transaction = true;
  }
}

void MysqlDatabase::commit_transaction()
{
  if (active)
  {
    mysql_commit(conn);
    mysql_autocommit(conn, true);
    CLog::Log(LOGDEBUG, "Mysql commit transaction");
    _in_transaction = false;
  }
}

void MysqlDatabase::rollback_transaction()
{
  if (active)
  {
    mysql_rollback(conn);
    mysql_autocommit(conn, true);
    CLog::Log(LOGDEBUG, "Mysql rollback transaction");
    _in_transaction = false;
  }
}

bool MysqlDatabase::exists(void)
{
  bool ret = false;

  if (conn == NULL || mysql_ping(conn))
  {
    CLog::Log(LOGERROR, "Not connected to database, test of existence is not possible.");
    return ret;
  }

  MYSQL_RES* result = mysql_list_dbs(conn, db.c_str());
  if (result == NULL)
  {
    CLog::Log(LOGERROR, "Database is not present, does the user has CREATE DATABASE permission");
    return false;
  }

  ret = (mysql_num_rows(result) > 0);
  mysql_free_result(result);

  // Check if there is some tables ( to permit user with no create database rights
  if (ret)
  {
    result = mysql_list_tables(conn, NULL);
    if (result != NULL)
      ret = (mysql_num_rows(result) > 0);

    mysql_free_result(result);
  }

  return ret;
}

// methods for formatting
// ---------------------------------------------
std::string MysqlDatabase::vprepare(const char* format, va_list args)
{
  std::string strFormat = format;
  std::string strResult = "";
  size_t pos;

  //  %q is the sqlite format string for %s.
  //  Any bad character, like "'", will be replaced with a proper one
  pos = 0;
  while ((pos = strFormat.find("%s", pos)) != std::string::npos)
    strFormat.replace(pos++, 2, "%q");

  strResult = mysql_vmprintf(strFormat.c_str(), args);
  //  RAND() is the mysql form of RANDOM()
  pos = 0;
  while ((pos = strResult.find("RANDOM()", pos)) != std::string::npos)
  {
    strResult.replace(pos++, 8, "RAND()");
    pos += 6;
  }

  // Replace some dataypes in CAST statements:
  // before: CAST(iFoo AS TEXT), CAST(foo AS INTEGER)
  // after:  CAST(iFoo AS CHAR), CAST(foo AS SIGNED INTEGER)
  pos = strResult.find("CAST(");
  while (pos != std::string::npos)
  {
    size_t pos2 = strResult.find(" AS TEXT)", pos + 1);
    if (pos2 != std::string::npos)
      strResult.replace(pos2, 9, " AS CHAR)");
    else
    {
      pos2 = strResult.find(" AS INTEGER)", pos + 1);
      if (pos2 != std::string::npos)
        strResult.replace(pos2, 12, " AS SIGNED INTEGER)");
    }
    pos = strResult.find("CAST(", pos + 1);
  }

  // Remove COLLATE NOCASE the SQLite case insensitive collation.
  // In MySQL all tables are defined with case insensitive collation utf8_general_ci
  pos = 0;
  while ((pos = strResult.find(" COLLATE NOCASE", pos)) != std::string::npos)
    strResult.erase(pos++, 15);

  // Remove COLLATE ALPHANUM the SQLite custom collation.
  pos = 0;
  while ((pos = strResult.find(" COLLATE ALPHANUM", pos)) != std::string::npos)
    strResult.erase(pos++, 15);

  return strResult;
}

/* vsprintf() functionality is based on sqlite3.c functions */

/*
** Conversion types fall into various categories as defined by the
** following enumeration.
*/
#define etRADIX 1 /* Integer types.  %d, %x, %o, and so forth */
#define etFLOAT 2 /* Floating point.  %f */
#define etEXP 3 /* Exponential notation. %e and %E */
#define etGENERIC 4 /* Floating or exponential, depending on exponent. %g */
#define etSIZE 5 /* Return number of characters processed so far. %n */
#define etSTRING 6 /* Strings. %s */
#define etDYNSTRING 7 /* Dynamically allocated strings. %z */
#define etPERCENT 8 /* Percent symbol. %% */
#define etCHARX 9 /* Characters. %c */
/* The rest are extensions, not normally found in printf() */
#define etSQLESCAPE 10 /* Strings with '\'' doubled. Strings with '\\' escaped.  %q */
#define etSQLESCAPE2 \
  11 /* Strings with '\'' doubled and enclosed in '',
                          NULL pointers replaced by SQL NULL.  %Q */
#define etPOINTER 14 /* The %p conversion */
#define etSQLESCAPE3 15 /* %w -> Strings with '\"' doubled */

#define etINVALID 0 /* Any unrecognized conversion type */

/*
** An "etByte" is an 8-bit unsigned value.
*/
typedef unsigned char etByte;

/*
** Each builtin conversion character (ex: the 'd' in "%d") is described
** by an instance of the following structure
*/
typedef struct et_info
{ /* Information about each format field */
  char fmttype; /* The format field code letter */
  etByte base; /* The base for radix conversion */
  etByte flags; /* One or more of FLAG_ constants below */
  etByte type; /* Conversion paradigm */
  etByte charset; /* Offset into aDigits[] of the digits string */
  etByte prefix; /* Offset into aPrefix[] of the prefix string */
} et_info;

/*
** An objected used to accumulate the text of a string where we
** do not necessarily know how big the string will be in the end.
*/
struct StrAccum
{
  char* zBase; /* A base allocation.  Not from malloc. */
  char* zText; /* The string collected so far */
  int nChar; /* Length of the string so far */
  int nAlloc; /* Amount of space allocated in zText */
  int mxAlloc; /* Maximum allowed string length */
  bool mallocFailed; /* Becomes true if any memory allocation fails */
  bool tooBig; /* Becomes true if string size exceeds limits */
};

/*
** Allowed values for et_info.flags
*/
#define FLAG_SIGNED 1 /* True if the value to convert is signed */
#define FLAG_INTERN 2 /* True if for internal use only */
#define FLAG_STRING 4 /* Allow infinity precision */

/*
** The following table is searched linearly, so it is good to put the
** most frequently used conversion types first.
*/
static const char aDigits[] = "0123456789ABCDEF0123456789abcdef";
static const char aPrefix[] = "-x0\000X0";
// clang-format off
constexpr std::array<et_info, 20> fmtinfo = {{
  {'d', 10, 1, etRADIX, 0, 0},
  {'s', 0, 4, etSTRING, 0, 0},
  {'g', 0, 1, etGENERIC, 30, 0},
  {'z', 0, 4, etDYNSTRING, 0, 0},
  {'q', 0, 4, etSQLESCAPE, 0, 0},
  {'Q', 0, 4, etSQLESCAPE2, 0, 0},
  {'w', 0, 4, etSQLESCAPE3, 0, 0},
  {'c', 0, 0, etCHARX, 0, 0},
  {'o', 8, 0, etRADIX, 0, 2},
  {'u', 10, 0, etRADIX, 0, 0},
  {'x', 16, 0, etRADIX, 16, 1},
  {'X', 16, 0, etRADIX, 0, 4},
  {'f', 0, 1, etFLOAT, 0, 0},
  {'e', 0, 1, etEXP, 30, 0},
  {'E', 0, 1, etEXP, 14, 0},
  {'G', 0, 1, etGENERIC, 14, 0},
  {'i', 10, 1, etRADIX, 0, 0},
  {'n', 0, 0, etSIZE, 0, 0},
  {'%', 0, 0, etPERCENT, 0, 0},
  {'p', 16, 0, etPOINTER, 0, 1},
}};
// clang-format on

/*
** "*val" is a double such that 0.1 <= *val < 10.0
** Return the ascii code for the leading digit of *val, then
** multiply "*val" by 10.0 to renormalize.
**
** Example:
**     input:     *val = 3.14159
**     output:    *val = 1.4159    function return = '3'
**
** The counter *cnt is incremented each time.  After counter exceeds
** 16 (the number of significant digits in a 64-bit float) '0' is
** always returned.
*/
char MysqlDatabase::et_getdigit(double* val, int* cnt)
{
  int digit;
  double d;
  if ((*cnt)++ >= 16)
    return '0';
  digit = (int)*val;
  d = digit;
  digit += '0';
  *val = (*val - d) * 10.0;
  return (char)digit;
}

/*
** Append N space characters to the given string buffer.
*/
void MysqlDatabase::appendSpace(StrAccum* pAccum, int N)
{
  static const char zSpaces[] = "                             ";
  while (N >= (int)sizeof(zSpaces) - 1)
  {
    mysqlStrAccumAppend(pAccum, zSpaces, sizeof(zSpaces) - 1);
    N -= sizeof(zSpaces) - 1;
  }
  if (N > 0)
  {
    mysqlStrAccumAppend(pAccum, zSpaces, N);
  }
}

#ifndef MYSQL_PRINT_BUF_SIZE
#define MYSQL_PRINT_BUF_SIZE 350
#endif

#define etBUFSIZE MYSQL_PRINT_BUF_SIZE /* Size of the output buffer */

/*
** The maximum length of a TEXT or BLOB in bytes.   This also
** limits the size of a row in a table or index.
**
** The hard limit is the ability of a 32-bit signed integer
** to count the size: 2^31-1 or 2147483647.
*/
#ifndef MYSQL_MAX_LENGTH
#define MYSQL_MAX_LENGTH 1000000000
#endif

/*
** The root program.  All variations call this core.
**
** INPUTS:
**   func   This is a pointer to a function taking three arguments
**            1. A pointer to anything.  Same as the "arg" parameter.
**            2. A pointer to the list of characters to be output
**               (Note, this list is NOT null terminated.)
**            3. An integer number of characters to be output.
**               (Note: This number might be zero.)
**
**   arg    This is the pointer to anything which will be passed as the
**          first argument to "func".  Use it for whatever you like.
**
**   fmt    This is the format string, as in the usual print.
**
**   ap     This is a pointer to a list of arguments.  Same as in
**          vfprint.
**
** OUTPUTS:
**          The return value is the total number of characters sent to
**          the function "func".  Returns -1 on a error.
**
** Note that the order in which automatic variables are declared below
** seems to make a big difference in determining how fast this beast
** will run.
*/
void MysqlDatabase::mysqlVXPrintf(StrAccum* pAccum, /* Accumulate results here */
                                  int useExtended, /* Allow extended %-conversions */
                                  const char* fmt, /* Format string */
                                  va_list ap /* arguments */
)
{
  int c; /* Next character in the format string */
  char* bufpt; /* Pointer to the conversion buffer */
  int precision; /* Precision of the current field */
  int length; /* Length of the field */
  int idx; /* A general purpose loop counter */
  int width; /* Width of the current field */
  etByte flag_leftjustify; /* True if "-" flag is present */
  etByte flag_plussign; /* True if "+" flag is present */
  etByte flag_blanksign; /* True if " " flag is present */
  etByte flag_alternateform; /* True if "#" flag is present */
  etByte flag_altform2; /* True if "!" flag is present */
  etByte flag_zeropad; /* True if field width constant starts with zero */
  etByte flag_long; /* True if "l" flag is present */
  etByte flag_longlong; /* True if the "ll" flag is present */
  etByte done; /* Loop termination flag */
  uint64_t longvalue; /* Value for integer types */
  double realvalue; /* Value for real types */
  const et_info* infop; /* Pointer to the appropriate info structure */
  char buf[etBUFSIZE]; /* Conversion buffer */
  char prefix; /* Prefix character.  "+" or "-" or " " or '\0'. */
  etByte xtype = 0; /* Conversion paradigm */
  char* zExtra; /* Extra memory used for etTCLESCAPE conversions */
  int exp, e2; /* exponent of real numbers */
  double rounder; /* Used for rounding floating point values */
  etByte flag_dp; /* True if decimal point should be shown */
  etByte flag_rtz; /* True if trailing zeros should be removed */
  etByte flag_exp; /* True to force display of the exponent */
  int nsd; /* Number of significant digits returned */

  length = 0;
  bufpt = 0;
  for (; (c = (*fmt)) != 0; ++fmt)
  {
    bool isLike = false;
    if (c != '%')
    {
      int amt;
      bufpt = const_cast<char*>(fmt);
      amt = 1;
      while ((c = (*++fmt)) != '%' && c != 0)
        amt++;
      isLike = mysqlStrAccumAppend(pAccum, bufpt, amt);
      if (c == 0)
        break;
    }
    if ((c = (*++fmt)) == 0)
    {
      mysqlStrAccumAppend(pAccum, "%", 1);
      break;
    }
    /* Find out what flags are present */
    flag_leftjustify = flag_plussign = flag_blanksign = flag_alternateform = flag_altform2 =
        flag_zeropad = 0;
    done = 0;
    do
    {
      switch (c)
      {
        case '-':
          flag_leftjustify = 1;
          break;
        case '+':
          flag_plussign = 1;
          break;
        case ' ':
          flag_blanksign = 1;
          break;
        case '#':
          flag_alternateform = 1;
          break;
        case '!':
          flag_altform2 = 1;
          break;
        case '0':
          flag_zeropad = 1;
          break;
        default:
          done = 1;
          break;
      }
    } while (!done && (c = (*++fmt)) != 0);
    /* Get the field width */
    width = 0;
    if (c == '*')
    {
      width = va_arg(ap, int);
      if (width < 0)
      {
        flag_leftjustify = 1;
        width = -width;
      }
      c = *++fmt;
    }
    else
    {
      while (c >= '0' && c <= '9')
      {
        width = width * 10 + c - '0';
        c = *++fmt;
      }
    }
    if (width > etBUFSIZE - 10)
    {
      width = etBUFSIZE - 10;
    }
    /* Get the precision */
    if (c == '.')
    {
      precision = 0;
      c = *++fmt;
      if (c == '*')
      {
        precision = va_arg(ap, int);
        if (precision < 0)
          precision = -precision;
        c = *++fmt;
      }
      else
      {
        while (c >= '0' && c <= '9')
        {
          precision = precision * 10 + c - '0';
          c = *++fmt;
        }
      }
    }
    else
    {
      precision = -1;
    }
    /* Get the conversion type modifier */
    if (c == 'l')
    {
      flag_long = 1;
      c = *++fmt;
      if (c == 'l')
      {
        flag_longlong = 1;
        c = *++fmt;
      }
      else
      {
        flag_longlong = 0;
      }
    }
    else
    {
      flag_long = flag_longlong = 0;
    }
    /* Fetch the info entry for the field */
    infop = fmtinfo.data();
    xtype = etINVALID;

    for (const auto& info : fmtinfo)
    {
      if (c != info.fmttype)
        continue;

      infop = &info;

      if (useExtended || (infop->flags & FLAG_INTERN) == 0)
      {
        xtype = infop->type;
      }
      else
      {
        return;
      }

      break;
    }

    zExtra = 0;

    /* Limit the precision to prevent overflowing buf[] during conversion */
    if (precision > etBUFSIZE - 40 && (infop->flags & FLAG_STRING) == 0)
    {
      precision = etBUFSIZE - 40;
    }

    /*
    ** At this point, variables are initialized as follows:
    **
    **   flag_alternateform          TRUE if a '#' is present.
    **   flag_altform2               TRUE if a '!' is present.
    **   flag_plussign               TRUE if a '+' is present.
    **   flag_leftjustify            TRUE if a '-' is present or if the
    **                               field width was negative.
    **   flag_zeropad                TRUE if the width began with 0.
    **   flag_long                   TRUE if the letter 'l' (ell) prefixed
    **                               the conversion character.
    **   flag_longlong               TRUE if the letter 'll' (ell ell) prefixed
    **                               the conversion character.
    **   flag_blanksign              TRUE if a ' ' is present.
    **   width                       The specified field width.  This is
    **                               always non-negative.  Zero is the default.
    **   precision                   The specified precision.  The default
    **                               is -1.
    **   xtype                       The class of the conversion.
    **   infop                       Pointer to the appropriate info struct.
    */
    switch (xtype)
    {
      case etPOINTER:
        flag_longlong = sizeof(char*) == sizeof(int64_t);
        flag_long = sizeof(char*) == sizeof(long int);
        /* Fall through into the next case */
        [[fallthrough]];
      case etRADIX:
        if (infop->flags & FLAG_SIGNED)
        {
          int64_t v;
          if (flag_longlong)
          {
            v = va_arg(ap, int64_t);
          }
          else if (flag_long)
          {
            v = va_arg(ap, long int);
          }
          else
          {
            v = va_arg(ap, int);
          }
          if (v < 0)
          {
            longvalue = -v;
            prefix = '-';
          }
          else
          {
            longvalue = v;
            if (flag_plussign)
              prefix = '+';
            else if (flag_blanksign)
              prefix = ' ';
            else
              prefix = 0;
          }
        }
        else
        {
          if (flag_longlong)
          {
            longvalue = va_arg(ap, uint64_t);
          }
          else if (flag_long)
          {
            longvalue = va_arg(ap, unsigned long int);
          }
          else
          {
            longvalue = va_arg(ap, unsigned int);
          }
          prefix = 0;
        }
        if (longvalue == 0)
          flag_alternateform = 0;
        if (flag_zeropad && precision < width - (prefix != 0))
        {
          precision = width - (prefix != 0);
        }
        bufpt = &buf[etBUFSIZE - 1];
        {
          const char* cset;
          int base;
          cset = &aDigits[infop->charset];
          base = infop->base;
          do
          { /* Convert to ascii */
            *(--bufpt) = cset[longvalue % base];
            longvalue = longvalue / base;
          } while (longvalue > 0);
        }
        length = (int)(&buf[etBUFSIZE - 1] - bufpt);
        for (idx = precision - length; idx > 0; idx--)
        {
          *(--bufpt) = '0'; /* Zero pad */
        }
        if (prefix)
          *(--bufpt) = prefix; /* Add sign */
        if (flag_alternateform && infop->prefix)
        { /* Add "0" or "0x" */
          const char* pre;
          char x;
          pre = &aPrefix[infop->prefix];
          for (; (x = (*pre)) != 0; pre++)
            *(--bufpt) = x;
        }
        length = (int)(&buf[etBUFSIZE - 1] - bufpt);
        bufpt[length] = 0;
        break;
      case etFLOAT:
      case etEXP:
      case etGENERIC:
        realvalue = va_arg(ap, double);
        if (precision < 0)
          precision = 6; /* Set default precision */
        if (precision > etBUFSIZE / 2 - 10)
          precision = etBUFSIZE / 2 - 10;
        if (realvalue < 0.0)
        {
          realvalue = -realvalue;
          prefix = '-';
        }
        else
        {
          if (flag_plussign)
            prefix = '+';
          else if (flag_blanksign)
            prefix = ' ';
          else
            prefix = 0;
        }
        if (xtype == etGENERIC && precision > 0)
          precision--;
        /* It makes more sense to use 0.5 */
        for (idx = precision, rounder = 0.5; idx > 0; idx--, rounder *= 0.1)
        {
        }
        if (xtype == etFLOAT)
          realvalue += rounder;
        /* Normalize realvalue to within 10.0 > realvalue >= 1.0 */
        exp = 0;
#if 0
        if( mysqlIsNaN((double)realvalue) ){
          bufpt = "NaN";
          length = 3;
          break;
        }
#endif
        if (realvalue > 0.0)
        {
          while (realvalue >= 1e32 && exp <= 350)
          {
            realvalue *= 1e-32;
            exp += 32;
          }
          while (realvalue >= 1e8 && exp <= 350)
          {
            realvalue *= 1e-8;
            exp += 8;
          }
          while (realvalue >= 10.0 && exp <= 350)
          {
            realvalue *= 0.1;
            exp++;
          }
          while (realvalue < 1e-8)
          {
            realvalue *= 1e8;
            exp -= 8;
          }
          while (realvalue < 1.0)
          {
            realvalue *= 10.0;
            exp--;
          }
          if (exp > 350)
          {
            if (prefix == '-')
            {
              bufpt = const_cast<char*>("-Inf");
            }
            else if (prefix == '+')
            {
              bufpt = const_cast<char*>("+Inf");
            }
            else
            {
              bufpt = const_cast<char*>("Inf");
            }
            length = strlen(bufpt);
            break;
          }
        }
        bufpt = buf;
        /*
        ** If the field type is etGENERIC, then convert to either etEXP
        ** or etFLOAT, as appropriate.
        */
        flag_exp = xtype == etEXP;
        if (xtype != etFLOAT)
        {
          realvalue += rounder;
          if (realvalue >= 10.0)
          {
            realvalue *= 0.1;
            exp++;
          }
        }
        if (xtype == etGENERIC)
        {
          flag_rtz = !flag_alternateform;
          if (exp < -4 || exp > precision)
          {
            xtype = etEXP;
          }
          else
          {
            precision = precision - exp;
            xtype = etFLOAT;
          }
        }
        else
        {
          flag_rtz = 0;
        }
        if (xtype == etEXP)
        {
          e2 = 0;
        }
        else
        {
          e2 = exp;
        }
        nsd = 0;
        flag_dp = (precision > 0 ? 1 : 0) | flag_alternateform | flag_altform2;
        /* The sign in front of the number */
        if (prefix)
        {
          *(bufpt++) = prefix;
        }
        /* Digits prior to the decimal point */
        if (e2 < 0)
        {
          *(bufpt++) = '0';
        }
        else
        {
          for (; e2 >= 0; e2--)
          {
            *(bufpt++) = et_getdigit(&realvalue, &nsd);
          }
        }
        /* The decimal point */
        if (flag_dp)
        {
          *(bufpt++) = '.';
        }
        /* "0" digits after the decimal point but before the first
        ** significant digit of the number */
        for (e2++; e2 < 0; precision--, e2++)
        {
          //ASSERT( precision>0 );
          *(bufpt++) = '0';
        }
        /* Significant digits after the decimal point */
        while ((precision--) > 0)
        {
          *(bufpt++) = et_getdigit(&realvalue, &nsd);
        }
        /* Remove trailing zeros and the "." if no digits follow the "." */
        if (flag_rtz && flag_dp)
        {
          while (bufpt[-1] == '0')
            *(--bufpt) = 0;
          //ASSERT( bufpt>buf );
          if (bufpt[-1] == '.')
          {
            if (flag_altform2)
            {
              *(bufpt++) = '0';
            }
            else
            {
              *(--bufpt) = 0;
            }
          }
        }
        /* Add the "eNNN" suffix */
        if (flag_exp || xtype == etEXP)
        {
          *(bufpt++) = aDigits[infop->charset];
          if (exp < 0)
          {
            *(bufpt++) = '-';
            exp = -exp;
          }
          else
          {
            *(bufpt++) = '+';
          }
          if (exp >= 100)
          {
            *(bufpt++) = (char)((exp / 100) + '0'); /* 100's digit */
            exp %= 100;
          }
          *(bufpt++) = (char)(exp / 10 + '0'); /* 10's digit */
          *(bufpt++) = (char)(exp % 10 + '0'); /* 1's digit */
        }
        *bufpt = 0;

        /* The converted number is in buf[] and zero terminated. Output it.
        ** Note that the number is in the usual order, not reversed as with
        ** integer conversions. */
        length = (int)(bufpt - buf);
        bufpt = buf;

        /* Special case:  Add leading zeros if the flag_zeropad flag is
        ** set and we are not left justified */
        if (flag_zeropad && !flag_leftjustify && length < width)
        {
          int i;
          int nPad = width - length;
          for (i = width; i >= nPad; i--)
          {
            bufpt[i] = bufpt[i - nPad];
          }
          i = prefix != 0;
          while (nPad--)
            bufpt[i++] = '0';
          length = width;
        }
        break;
      case etSIZE:
        *(va_arg(ap, int*)) = pAccum->nChar;
        length = width = 0;
        break;
      case etPERCENT:
        buf[0] = '%';
        bufpt = buf;
        length = 1;
        break;
      case etCHARX:
        c = va_arg(ap, int);
        buf[0] = (char)c;
        if (precision >= 0)
        {
          for (idx = 1; idx < precision; idx++)
            buf[idx] = (char)c;
          length = precision;
        }
        else
        {
          length = 1;
        }
        bufpt = buf;
        break;
      case etSTRING:
      case etDYNSTRING:
        bufpt = va_arg(ap, char*);
        if (bufpt == 0)
        {
          bufpt = const_cast<char*>("");
        }
        else if (xtype == etDYNSTRING)
        {
          zExtra = bufpt;
        }
        if (precision >= 0)
        {
          for (length = 0; length < precision && bufpt[length]; length++)
          {
          }
        }
        else
        {
          length = strlen(bufpt);
        }
        break;
      case etSQLESCAPE:
      case etSQLESCAPE2:
      case etSQLESCAPE3:
      {
        int i, j, k, n, isnull;
        int needQuote;
        char ch;
        char q = ((xtype == etSQLESCAPE3) ? '"' : '\''); /* Quote character */
        std::string arg = va_arg(ap, char*);
        if (isLike)
          StringUtils::Replace(arg, "\\", "\\\\");
        const char* escarg = arg.c_str();

        isnull = escarg == 0;
        if (isnull)
          escarg = (xtype == etSQLESCAPE2 ? "NULL" : "(NULL)");
        k = precision;
        for (i = 0; k != 0 && (ch = escarg[i]) != 0; i++, k--)
          ;
        needQuote = !isnull && xtype == etSQLESCAPE2;
        n = i * 2 + 1 + needQuote * 2;
        if (n > etBUFSIZE)
        {
          bufpt = zExtra = (char*)malloc(n);
          if (bufpt == 0)
          {
            pAccum->mallocFailed = true;
            return;
          }
        }
        else
        {
          bufpt = buf;
        }
        j = 0;
        if (needQuote)
          bufpt[j++] = q;
        k = i;
        j += mysql_real_escape_string(conn, bufpt, escarg, k);
        if (needQuote)
          bufpt[j++] = q;
        bufpt[j] = 0;
        length = j;
        /* The precision in %q and %Q means how many input characters to
        ** consume, not the length of the output...
        ** if( precision>=0 && precision<length ) length = precision; */
        break;
      }
      default:
      {
        return;
      }
    } /* End switch over the format type */
    /*
    ** The text of the conversion is pointed to by "bufpt" and is
    ** "length" characters long.  The field width is "width".  Do
    ** the output.
    */
    if (!flag_leftjustify)
    {
      int nspace;
      nspace = width - length;
      if (nspace > 0)
      {
        appendSpace(pAccum, nspace);
      }
    }
    if (length > 0)
    {
      mysqlStrAccumAppend(pAccum, bufpt, length);
    }
    if (flag_leftjustify)
    {
      int nspace;
      nspace = width - length;
      if (nspace > 0)
      {
        appendSpace(pAccum, nspace);
      }
    }
    if (zExtra)
    {
      free(zExtra);
    }
  } /* End for loop over the format string */
} /* End of function */

/*
** Append N bytes of text from z to the StrAccum object.
*/
bool MysqlDatabase::mysqlStrAccumAppend(StrAccum* p, const char* z, int N)
{
  if (p->tooBig | p->mallocFailed)
  {
    return false;
  }
  if (N < 0)
  {
    N = strlen(z);
  }
  if (N == 0 || z == 0)
  {
    return false;
  }
  if (p->nChar + N >= p->nAlloc)
  {
    char* zNew;
    int szNew = p->nChar;
    szNew += N + 1;
    if (szNew > p->mxAlloc)
    {
      mysqlStrAccumReset(p);
      p->tooBig = true;
      return false;
    }
    else
    {
      p->nAlloc = szNew;
    }
    zNew = (char*)malloc(p->nAlloc);
    if (zNew)
    {
      memcpy(zNew, p->zText, p->nChar);
      mysqlStrAccumReset(p);
      p->zText = zNew;
    }
    else
    {
      p->mallocFailed = true;
      mysqlStrAccumReset(p);
      return false;
    }
  }

  bool isLike = false;
  std::string testString(z, N);
  if (testString.find("LIKE") != std::string::npos || testString.find("like") != std::string::npos)
  {
    CLog::Log(LOGDEBUG,
              "This query part contains a like, we will double backslash in the next field: {}",
              testString);
    isLike = true;
  }

  memcpy(&p->zText[p->nChar], z, N);
  p->nChar += N;
  return isLike;
}

/*
** Finish off a string by making sure it is zero-terminated.
** Return a pointer to the resulting string.  Return a NULL
** pointer if any kind of error was encountered.
*/
char* MysqlDatabase::mysqlStrAccumFinish(StrAccum* p)
{
  if (p->zText)
  {
    p->zText[p->nChar] = 0;
    if (p->zText == p->zBase)
    {
      p->zText = (char*)malloc(p->nChar + 1);
      if (p->zText)
      {
        memcpy(p->zText, p->zBase, p->nChar + 1);
      }
      else
      {
        p->mallocFailed = true;
      }
    }
  }
  return p->zText;
}

/*
** Reset an StrAccum string.  Reclaim all malloced memory.
*/
void MysqlDatabase::mysqlStrAccumReset(StrAccum* p)
{
  if (p->zText != p->zBase)
  {
    free(p->zText);
  }
  p->zText = 0;
}

/*
** Initialize a string accumulator
*/
void MysqlDatabase::mysqlStrAccumInit(StrAccum* p, char* zBase, int n, int mx)
{
  p->zText = p->zBase = zBase;
  p->nChar = 0;
  p->nAlloc = n;
  p->mxAlloc = mx;
  p->tooBig = false;
  p->mallocFailed = false;
}

/*
** Print into memory obtained from mysql_malloc().  Omit the internal
** %-conversion extensions.
*/
std::string MysqlDatabase::mysql_vmprintf(const char* zFormat, va_list ap)
{
  char zBase[MYSQL_PRINT_BUF_SIZE];
  StrAccum acc;

  mysqlStrAccumInit(&acc, zBase, sizeof(zBase), MYSQL_MAX_LENGTH);
  mysqlVXPrintf(&acc, 0, zFormat, ap);
  return mysqlStrAccumFinish(&acc);
}

//************* MysqlDataset implementation ***************

MysqlDataset::MysqlDataset() : Dataset()
{
  haveError = false;
  db = NULL;
  autorefresh = false;
}

MysqlDataset::MysqlDataset(MysqlDatabase* newDb) : Dataset(newDb)
{
  haveError = false;
  db = newDb;
  autorefresh = false;
}

MysqlDataset::~MysqlDataset()
{
}

void MysqlDataset::set_autorefresh(bool val)
{
  autorefresh = val;
}

//--------- protected functions implementation -----------------//

MYSQL* MysqlDataset::handle()
{
  if (db != NULL)
  {
    return static_cast<MysqlDatabase*>(db)->getHandle();
  }

  return NULL;
}

void MysqlDataset::make_query(StringList& _sql)
{
  std::string query;
  if (db == NULL)
    throw DbErrors("No Database Connection");
  try
  {
    if (autocommit)
      db->start_transaction();

    for (const std::string& i : _sql)
    {
      query = i;
      Dataset::parse_sql(query);
      if ((static_cast<MysqlDatabase*>(db)->query_with_reconnect(query.c_str())) != MYSQL_OK)
      {
        throw DbErrors(db->getErrorMsg());
      }
    } // end of for

    if (db->in_transaction() && autocommit)
      db->commit_transaction();

    active = true;
    ds_state = dsSelect;
    if (autorefresh)
      refresh();
  } // end of try
  catch (...)
  {
    if (db->in_transaction())
      db->rollback_transaction();
    throw;
  }
}

void MysqlDataset::make_insert()
{
  make_query(insert_sql);
  last();
}

void MysqlDataset::make_edit()
{
  make_query(update_sql);
}

void MysqlDataset::make_deletion()
{
  make_query(delete_sql);
}

void MysqlDataset::fill_fields()
{
  if ((db == NULL) || (result.record_header.empty()) ||
      (result.records.size() < (unsigned int)frecno))
    return;

  if (fields_object->size() == 0) // Filling columns name
  {
    const unsigned int ncols = result.record_header.size();
    fields_object->resize(ncols);
    for (unsigned int i = 0; i < ncols; i++)
    {
      (*fields_object)[i].props = result.record_header[i];
      std::string name = result.record_header[i].name;
      name2indexMap.insert({str_toLower(name.data()), i});
    }
  }

  //Filling result
  if (result.records.size() != 0)
  {
    const sql_record* row = result.records[frecno];
    if (row)
    {
      const unsigned int ncols = row->size();
      fields_object->resize(ncols);
      for (unsigned int i = 0; i < ncols; i++)
        (*fields_object)[i].val = row->at(i);
      return;
    }
  }
  const unsigned int ncols = result.record_header.size();
  fields_object->resize(ncols);
  for (unsigned int i = 0; i < ncols; i++)
    (*fields_object)[i].val = "";
}

//------------- public functions implementation -----------------//
bool MysqlDataset::dropIndex(const char* table, const char* index)
{
  std::string sql;
  std::string sql_prepared;

  sql = "SELECT * FROM information_schema.statistics WHERE TABLE_SCHEMA=DATABASE() AND "
        "table_name='%s' AND index_name='%s'";
  sql_prepared = static_cast<MysqlDatabase*>(db)->prepare(sql.c_str(), table, index);

  if (!query(sql_prepared))
    return false;

  if (num_rows())
  {
    sql = "ALTER TABLE %s DROP INDEX %s";
    sql_prepared = static_cast<MysqlDatabase*>(db)->prepare(sql.c_str(), table, index);

    if (exec(sql_prepared) != MYSQL_OK)
      return false;
  }

  return true;
}

static bool ci_test(char l, char r)
{
  return tolower(l) == tolower(r);
}

static size_t ci_find(const std::string& where, const std::string& what)
{
  std::string::const_iterator loc =
      std::search(where.begin(), where.end(), what.begin(), what.end(), ci_test);
  if (loc == where.end())
    return std::string::npos;
  else
    return loc - where.begin();
}

int MysqlDataset::exec(const std::string& sql)
{
  if (!handle())
    throw DbErrors("No Database Connection");
  std::string qry = sql;
  int res = 0;
  exec_res.clear();

  // enforce the "auto_increment" keyword to be appended to "integer primary key"
  size_t loc;

  if ((loc = ci_find(qry, "integer primary key")) != std::string::npos)
  {
    qry = qry.insert(loc + 19, " auto_increment ");
  }

  // force the charset and collation to UTF-8
  if (ci_find(qry, "CREATE TABLE") != std::string::npos ||
      ci_find(qry, "CREATE TEMPORARY TABLE") != std::string::npos)
  {
    // If CREATE TABLE ... SELECT Syntax is used we need to add the encoding after the table before the select
    // e.g. CREATE TABLE x CHARACTER SET utf8 COLLATE utf8_general_ci [AS] SELECT * FROM y
    if ((loc = qry.find(" AS SELECT ")) != std::string::npos ||
        (loc = qry.find(" SELECT ")) != std::string::npos)
    {
      qry = qry.insert(loc, " CHARACTER SET utf8 COLLATE utf8_general_ci");
    }
    else
      qry += " CHARACTER SET utf8 COLLATE utf8_general_ci";
  }

  CLog::Log(LOGDEBUG, "Mysql execute: {}", qry);

  if (db->setErr(static_cast<MysqlDatabase*>(db)->query_with_reconnect(qry.c_str()), qry.c_str()) !=
      MYSQL_OK)
  {
    throw DbErrors(db->getErrorMsg());
  }
  else
  {
    //! @todo collect results and store in exec_res
    return res;
  }
}

int MysqlDataset::exec()
{
  return exec(sql);
}

const void* MysqlDataset::getExecRes()
{
  return &exec_res;
}

bool MysqlDataset::query(const std::string& query)
{
  if (!handle())
    throw DbErrors("No Database Connection");
  std::string qry = query;
  int fs = qry.find("select");
  int fS = qry.find("SELECT");
  if (!(fs >= 0 || fS >= 0))
    throw DbErrors("MUST be select SQL!");

  close();

  size_t loc;

  // mysql doesn't understand CAST(foo as integer) => change to CAST(foo as signed integer)
  while ((loc = ci_find(qry, "as integer)")) != std::string::npos)
    qry = qry.insert(loc + 3, "signed ");

  MYSQL_RES* stmt = NULL;

  if (static_cast<MysqlDatabase*>(db)->setErr(
          static_cast<MysqlDatabase*>(db)->query_with_reconnect(qry.c_str()), qry.c_str()) !=
      MYSQL_OK)
    throw DbErrors(db->getErrorMsg());

  MYSQL* conn = handle();
  stmt = mysql_store_result(conn);
  if (stmt == NULL)
    throw DbErrors("Missing result set!");

  // column headers
  const unsigned int numColumns = mysql_num_fields(stmt);
  MYSQL_FIELD* fields = mysql_fetch_fields(stmt);
  MYSQL_ROW row;
  result.record_header.resize(numColumns);
  for (unsigned int i = 0; i < numColumns; i++)
    result.record_header[i].name = fields[i].name;

  // returned rows
  while ((row = mysql_fetch_row(stmt)))
  { // have a row of data
    sql_record* res = new sql_record;
    res->resize(numColumns);
    for (unsigned int i = 0; i < numColumns; i++)
    {
      field_value& v = res->at(i);
      switch (fields[i].type)
      {
        case MYSQL_TYPE_LONGLONG:
          if (row[i] != nullptr)
          {
            v.set_asInt64(strtoll(row[i], nullptr, 10));
          }
          else
          {
            v.set_asInt64(0);
          }
          break;
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
          if (row[i] != NULL)
          {
            v.set_asInt(atoi(row[i]));
          }
          else
          {
            v.set_asInt(0);
          }
          break;
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
          if (row[i] != NULL)
          {
            v.set_asDouble(atof(row[i]));
          }
          else
          {
            v.set_asDouble(0);
          }
          break;
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_VARCHAR:
          if (row[i] != NULL)
            v.set_asString((const char*)row[i]);
          break;
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
          if (row[i] != NULL)
            v.set_asString((const char*)row[i]);
          break;
        case MYSQL_TYPE_NULL:
        default:
          CLog::Log(LOGDEBUG, "MYSQL: Unknown field type: {}", fields[i].type);
          v.set_asString("");
          v.set_isNull();
          break;
      }
    }
    result.records.push_back(res);
  }
  mysql_free_result(stmt);
  active = true;
  ds_state = dsSelect;
  this->first();
  return true;
}

void MysqlDataset::open(const std::string& sql)
{
  set_select_sql(sql);
  open();
}

void MysqlDataset::open()
{
  if (select_sql.size())
  {
    query(select_sql);
  }
  else
  {
    ds_state = dsInactive;
  }
}

void MysqlDataset::close()
{
  Dataset::close();
  result.clear();
  edit_object->clear();
  fields_object->clear();
  ds_state = dsInactive;
  active = false;
}

void MysqlDataset::cancel()
{
  if ((ds_state == dsInsert) || (ds_state == dsEdit))
  {
    if (result.record_header.size())
      ds_state = dsSelect;
    else
      ds_state = dsInactive;
  }
}

int MysqlDataset::num_rows()
{
  return result.records.size();
}

bool MysqlDataset::eof()
{
  return feof;
}

bool MysqlDataset::bof()
{
  return fbof;
}

void MysqlDataset::first()
{
  Dataset::first();
  this->fill_fields();
}

void MysqlDataset::last()
{
  Dataset::last();
  fill_fields();
}

void MysqlDataset::prev(void)
{
  Dataset::prev();
  fill_fields();
}

void MysqlDataset::next(void)
{
  Dataset::next();
  if (!eof())
    fill_fields();
}

void MysqlDataset::free_row(void)
{
  if (frecno < 0 || (unsigned int)frecno >= result.records.size())
    return;

  sql_record* row = result.records[frecno];
  if (row)
  {
    delete row;
    result.records[frecno] = NULL;
  }
}

bool MysqlDataset::seek(int pos)
{
  if (ds_state == dsSelect)
  {
    Dataset::seek(pos);
    fill_fields();
    return true;
  }

  return false;
}

int64_t MysqlDataset::lastinsertid()
{
  if (!handle())
    throw DbErrors("No Database Connection");
  return mysql_insert_id(handle());
}

long MysqlDataset::nextid(const char* seq_name)
{
  if (handle())
    return db->nextid(seq_name);

  return DB_UNEXPECTED_RESULT;
}

void MysqlDataset::interrupt()
{
  // Impossible
}

} // namespace dbiplus
