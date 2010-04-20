/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <iostream>
#include <string>
#include <set>

#include "mysqldataset.h"
#include "utils/log.h"
#include "system.h" // for GetLastError()
#ifdef _WIN32
#include "../../../lib/libmysql_win32/include/errmsg.h"
#pragma comment(lib, "../../lib/libmysql_win32/lib/mysqlclient.lib")
#else
#include "mysql/errmsg.h"
#endif


#define MYSQL_OK        0

using namespace std;

namespace dbiplus {

//************* MysqlDatabase implementation ***************

MysqlDatabase::MysqlDatabase() {

  active = false;
  _in_transaction = false;     // for transaction

  error = "Unknown database error";//S_NO_CONNECTION;
  host = "localhost";
  port = "3306";
  db = "mysql";
  login = "root";
  passwd = "null";
  conn = NULL;
}

MysqlDatabase::~MysqlDatabase() {
  disconnect();
}

Dataset* MysqlDatabase::CreateDataset() const {
   return new MysqlDataset((MysqlDatabase*)this);
}

int MysqlDatabase::status(void) {
  if (active == false) return DB_CONNECTION_NONE;
  return DB_CONNECTION_OK;
}

int MysqlDatabase::setErr(int err_code, const char * qry) {
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
      snprintf(err, 256, "Undefined MySQL error: Code (%d)", err_code);
      error = err;
  }
  error += "\nQuery: ";
  error += qry;
  error += "\n";
  return err_code;
}

const char *MysqlDatabase::getErrorMsg() {
   return error.c_str();
}

int MysqlDatabase::connect() {
  try
  {
    // don't reconnect if ping is ok
    if (conn != NULL && mysql_ping(conn) == 0)
    {
      return DB_CONNECTION_OK;
    }
    disconnect();

    if (conn == NULL)
    {
      conn = mysql_init(conn);
    }

    // TODO block to avoid multiple connect on db
    if (mysql_real_connect(conn,host.c_str(),login.c_str(),passwd.c_str(),db.c_str(),atoi(port.c_str()),NULL,0) != NULL)
    {
      active = true;
      return DB_CONNECTION_OK;
    }
    // Database doesn't exists
    if (mysql_errno(conn) == 1049)
    {
      if (create() == MYSQL_OK)
      {
        active = true;

        // disable mysql autocommit since we handle it
        mysql_autocommit(conn, false);

        return DB_CONNECTION_OK;
      }
    }

    CLog::Log(LOGERROR, "Unable to open database: %s [%d](%s)",
              db.c_str(), mysql_errno(conn), mysql_error(conn));
    return DB_CONNECTION_NONE;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Unable to open database: %s (%u)",
              db.c_str(), GetLastError());
  }
  return DB_CONNECTION_NONE;
}

void MysqlDatabase::disconnect(void) {
  if (active == false) return;
  if (conn == NULL) return;
  mysql_close(conn);
  conn = NULL;
  active = false;
}

int MysqlDatabase::create() {
  const char* seq_table = "sys_seq";
  if (mysql_real_connect(conn, host.c_str(), login.c_str(), passwd.c_str(), NULL, atoi(port.c_str()),NULL,0) == NULL)
  {
    throw DbErrors("Can't create database: '%s'\nCan't connect to server",db.c_str());
  }
  char sqlcmd[512];
  sprintf(sqlcmd, "CREATE DATABASE `%s`", db.c_str());
  if ( query_with_reconnect(sqlcmd) )
  {
    throw DbErrors("Can't create database: '%s'\nError: %s", db.c_str(), strerror(errno));
  }
  // Reconnect to the server
  if (mysql_real_connect(conn, host.c_str(), login.c_str(), passwd.c_str(), db.c_str(), atoi(port.c_str()), NULL, 0) == NULL)
  {
    throw DbErrors("Can't create database: '%s'\nCan't reconnect to server",db.c_str());
  }
  // Create the sequence table
  sprintf(sqlcmd,"CREATE TABLE `%s` (`seq_name` VARCHAR( 64 ) NOT NULL, `nextid` INT NOT NULL, PRIMARY KEY ( `seq_name` ))", seq_table);
  if ( query_with_reconnect(sqlcmd) )
  {
    throw DbErrors("Can't create sequence table in '%s'\nError: %s", db.c_str(), strerror(errno));
  }
  return MYSQL_OK;
}

int MysqlDatabase::drop() {
  if (active == false) throw DbErrors("Can't drop database: no active connection...");
  char sqlcmd[512];
  sprintf(sqlcmd,"DROP DATABASE `%s`", db.c_str());
  if ( query_with_reconnect(sqlcmd) )
  {
    throw DbErrors("Can't drop database: '%s'\nError: %s", db.c_str(), strerror(errno));
  }
  disconnect();
  return DB_COMMAND_OK;
}

int MysqlDatabase::query_with_reconnect(const char* query) {
  int attempts = 5;
  int result;

  // try to reconnect if server is gone (up to 3 times)
  while ( ((result = mysql_real_query(conn, query, strlen(query))) == CR_SERVER_GONE_ERROR) &&
          (attempts-- > 0) )
  {
    CLog::Log(LOGINFO,"MYSQL server has gone. Will try %d more attempt(s) to reconnect.", attempts);
    active = false;
    connect();
  }

  // grab the latest error if not ok
  if (result != MYSQL_OK)
    result = mysql_errno(conn);

  // set the error return string and return
  return result;
}

long MysqlDatabase::nextid(const char* sname) {
  CLog::Log(LOGDEBUG,"MysqlDatabase::nextid for %s",sname);
  if (!active) return DB_UNEXPECTED_RESULT;
  const char* seq_table = "sys_seq";
  int id;/*,nrow,ncol;*/
  MYSQL_RES* res;
  char sqlcmd[512];
  sprintf(sqlcmd,"select nextid from %s where seq_name = '%s'",seq_table, sname);
  CLog::Log(LOGDEBUG,"MysqlDatabase::nextid will request");
  if ((last_err = query_with_reconnect(sqlcmd)) != 0)
  {
    return DB_UNEXPECTED_RESULT;
  }
  res = mysql_store_result(conn);
  if (mysql_num_rows(res) == 0)
  {
    id = 1;
    sprintf(sqlcmd,"insert into %s (nextid,seq_name) values (%d,'%s')",seq_table,id,sname);
    mysql_free_result(res);
    if ((last_err = query_with_reconnect(sqlcmd)) != 0) return DB_UNEXPECTED_RESULT;
    return id;
  }
  else
  {
    MYSQL_ROW row = mysql_fetch_row(res);
    //id = (int)row[0];
    id = -1;
    unsigned long *lengths;
    lengths = mysql_fetch_lengths(res);
    CLog::Log(LOGINFO,"Next id is [%.*s] ", (int) lengths[0], row[0]);
    sprintf(sqlcmd,"update %s set nextid=%d where seq_name = '%s'",seq_table,id,sname);
    if ((last_err = query_with_reconnect(sqlcmd) != 0)) return DB_UNEXPECTED_RESULT;
    return id;
  }
  return DB_UNEXPECTED_RESULT;
}

// methods for transactions
// ---------------------------------------------
void MysqlDatabase::start_transaction() {
  if (active)
  {
    CLog::Log(LOGDEBUG,"Mysql Start transaction");
    _in_transaction = true;
  }
}

void MysqlDatabase::commit_transaction() {
  if (active)
  {
    mysql_commit(conn);
    CLog::Log(LOGDEBUG,"Mysql commit transaction");
    _in_transaction = false;
  }
}

void MysqlDatabase::rollback_transaction() {
  if (active)
  {
    mysql_rollback(conn);
    CLog::Log(LOGDEBUG,"Mysql rollback transaction");
    _in_transaction = false;
  }
}

bool MysqlDatabase::exists() {
  // Uncorrect name, check if tables are present inside the db
  connect();
  if (active && conn != NULL)
  {
    MYSQL_RES* res = mysql_list_dbs(conn, db.c_str());
    if (res == NULL)
    {
      CLog::Log(LOGERROR,"Database is not present, does the user has CREATE DATABASE permission");
      return false;
    }

    bool result = (mysql_num_rows(res) > 0); // Avoid counting the sequence table
    mysql_free_result(res);
    // Check if there is some tables ( to permit user with no create database rights
    if (result == true)
    {
      res = mysql_list_tables(conn, NULL);
      if (res != NULL)
      {
        result = (mysql_num_rows(res) > 1);
      }
      mysql_free_result(res);
    }
    return result;
  }
  return false;
}

//************* MysqlDataset implementation ***************

MysqlDataset::MysqlDataset():Dataset() {
  haveError = false;
  db = NULL;
  errmsg = NULL;
  autorefresh = false;
}


MysqlDataset::MysqlDataset(MysqlDatabase *newDb):Dataset(newDb) {
  haveError = false;
  db = newDb;
  errmsg = NULL;
  autorefresh = false;
}

MysqlDataset::~MysqlDataset() {
   if (errmsg) free(errmsg);
 }


void MysqlDataset::set_autorefresh(bool val) {
    autorefresh = val;
}



//--------- protected functions implementation -----------------//

MYSQL* MysqlDataset::handle(){
  if (db != NULL)
  {
    return static_cast<MysqlDatabase*>(db)->getHandle();
  }

  return NULL;
}

void MysqlDataset::make_query(StringList &_sql) {
  string query;
  int result = 0;
  try
  {
    if (db == NULL) throw DbErrors("No Database Connection");
    if (autocommit) db->start_transaction();

    for (list<string>::iterator i =_sql.begin(); i!=_sql.end(); i++)
    {
      query = *i;
      Dataset::parse_sql(query);
      if ((result = static_cast<MysqlDatabase *>(db)->query_with_reconnect(query.c_str())) != MYSQL_OK)
      {
        throw DbErrors(db->getErrorMsg());
      }
    } // end of for

    if (db->in_transaction() && autocommit) db->commit_transaction();

    active = true;
    ds_state = dsSelect;
    if (autorefresh)
      refresh();
  } // end of try
  catch(...)
  {
    if (db->in_transaction()) db->rollback_transaction();
    throw;
  }

}

void MysqlDataset::make_insert() {
  make_query(insert_sql);
  last();
}

void MysqlDataset::make_edit() {
  make_query(update_sql);
}


void MysqlDataset::make_deletion() {
  make_query(delete_sql);
}

void MysqlDataset::fill_fields() {
  if ((db == NULL) || (result.record_header.size() == 0) || (result.records.size() < (unsigned int)frecno)) return;

  if (fields_object->size() == 0) // Filling columns name
  {
    const unsigned int ncols = result.record_header.size();
    fields_object->resize(ncols);
    for (unsigned int i = 0; i < ncols; i++)
      (*fields_object)[i].props = result.record_header[i];
  }

  //Filling result
  if (result.records.size() != 0)
  {
    const sql_record *row = result.records[frecno];
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
//FILE* file;
int MysqlDataset::exec(const string &sql) {
  if (!handle()) throw DbErrors("No Database Connection");
  string qry = sql;
  int res = 0;
  exec_res.clear();

  // enforce the "auto_increment" keyword to be appended to "integer primary key"
  const char* toFind = "integer primary key";
  size_t loc = qry.find(toFind);
  if (loc != string::npos)
  {
    qry = qry.insert(loc + strlen(toFind), " auto_increment ");
  }

  CLog::Log(LOGDEBUG,"Mysql execute: %s", qry.c_str());

  if (db->setErr( static_cast<MysqlDatabase *>(db)->query_with_reconnect(qry.c_str()), qry.c_str()) != MYSQL_OK)
  {
    throw DbErrors(db->getErrorMsg());
  }
  else
  {
    // TODO: collect results and store in exec_res


    return res;
  }
}

int MysqlDataset::exec() {
   return exec(sql);
}

const void* MysqlDataset::getExecRes() {
  return &exec_res;
}


bool MysqlDataset::query(const char *query) {
  if(!handle()) throw DbErrors("No Database Connection");
  std::string qry = query;
  int fs = qry.find("select");
  int fS = qry.find("SELECT");
  if (!( fs >= 0 || fS >=0))
    throw DbErrors("MUST be select SQL!");

  close();

  MYSQL_RES *stmt = NULL;

  if ( static_cast<MysqlDatabase*>(db)->setErr(static_cast<MysqlDatabase*>(db)->query_with_reconnect(query), query) != MYSQL_OK )
    throw DbErrors(db->getErrorMsg());

  MYSQL* conn = handle();
  stmt = mysql_store_result(conn);

  // column headers
  const unsigned int numColumns = mysql_num_fields(stmt);
  MYSQL_FIELD *fields = mysql_fetch_fields(stmt);
  MYSQL_ROW row;
  result.record_header.resize(numColumns);
  for (unsigned int i = 0; i < numColumns; i++)
    result.record_header[i].name = fields[i].name;

  // returned rows
  while ((row = mysql_fetch_row(stmt)))
  { // have a row of data
    sql_record *res = new sql_record;
    res->resize(numColumns);
    for (unsigned int i = 0; i < numColumns; i++)
    {
      field_value &v = res->at(i);
      switch (fields[i].type)
      {
      case MYSQL_TYPE_LONGLONG:
      case MYSQL_TYPE_DECIMAL:
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
          v.set_asDouble(atol(row[i]));
        }
        else
        {
          v.set_asDouble(0);
        }
        break;
      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_VARCHAR:
        if (row[i] != NULL) v.set_asString((const char *)row[i] );
        break;
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
        if (row[i] != NULL) v.set_asString((const char *)row[i]);
        break;
      case MYSQL_TYPE_NULL:
      default:
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

bool MysqlDataset::query(const string &q) {
  return query(q.c_str());
}

void MysqlDataset::open(const string &sql) {
   set_select_sql(sql);
   open();
}

void MysqlDataset::open() {
  if (select_sql.size())
  {
    query(select_sql.c_str());
  }
  else
  {
    ds_state = dsInactive;
  }
}

void MysqlDataset::close() {
  Dataset::close();
  result.clear();
  edit_object->clear();
  fields_object->clear();
  ds_state = dsInactive;
  active = false;
}


void MysqlDataset::cancel() {
  if ((ds_state == dsInsert) || (ds_state==dsEdit))
  {
    if (result.record_header.size())
      ds_state = dsSelect;
    else
      ds_state = dsInactive;
  }
}


int MysqlDataset::num_rows() {
  return result.records.size();
}


bool MysqlDataset::eof() {
  return feof;
}


bool MysqlDataset::bof() {
  return fbof;
}


void MysqlDataset::first() {
  Dataset::first();
  this->fill_fields();
}

void MysqlDataset::last() {
  Dataset::last();
  fill_fields();
}

void MysqlDataset::prev(void) {
  Dataset::prev();
  fill_fields();
}

void MysqlDataset::next(void) {
#ifdef _XBOX
  free_row();
#endif
  Dataset::next();
  if (!eof())
      fill_fields();
}

void MysqlDataset::free_row(void)
{
  if (frecno < 0 || (unsigned int)frecno >= result.records.size())
    return;

  sql_record *row = result.records[frecno];
  if (row)
  {
    delete row;
    result.records[frecno] = NULL;
  }
}

bool MysqlDataset::seek(int pos) {
  if (ds_state == dsSelect)
  {
    Dataset::seek(pos);
    fill_fields();
    return true;
  }

  return false;
}

int64_t MysqlDataset::lastinsertid() {
  if (!handle()) DbErrors("No Database Connection");
  return mysql_insert_id(handle());
}

long MysqlDataset::nextid(const char *seq_name) {
  if (handle())
    return db->nextid(seq_name);

  return DB_UNEXPECTED_RESULT;
}

void MysqlDataset::interrupt() {
  // Impossible
}

}//namespace

