/*
 *  Copyright (C) 2002, Leo Seib, Hannover
 *
 *  Project:Dataset C++ Dynamic Library
 *  Module: Dataset abstraction layer header file
 *  Author: Leo Seib      E-Mail: leoseib@web.de
 *  Begin: 5/04/2002
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "qry_dat.h"

#include <cstdio>
#include <list>
#include <map>
#include <stdarg.h>
#include <string>
#include <unordered_map>

namespace dbiplus
{
class Dataset; // forward declaration of class Dataset

#define S_NO_CONNECTION "No active connection";

#define DB_BUFF_MAX 8 * 1024 // Maximum buffer's capacity

#define DB_CONNECTION_NONE 0
#define DB_CONNECTION_OK 1
#define DB_CONNECTION_BAD 2

#define DB_COMMAND_OK 0 // OK - command executed
#define DB_EMPTY_QUERY 1 // Query didn't return tuples
#define DB_TUPLES_OK 2 // Query returned tuples
#define DB_ERROR 5
#define DB_BAD_RESPONSE 6
#define DB_UNEXPECTED 7 // This shouldn't ever happen
#define DB_UNEXPECTED_RESULT -1 //For integer functions

/******************* Class Database definition ********************

   represents  connection with database server;

******************************************************************/
class Database
{
protected:
  bool active;
  bool compression;
  std::string error, // Error description
      host, port, db, login, passwd, //Login info
      sequence_table, //Sequence table for nextid
      default_charset, //Default character set
      key, cert, ca, capath, ciphers; //SSL - Encryption info

public:
  /* constructor */
  Database();
  /* destructor */
  virtual ~Database();
  virtual Dataset* CreateDataset() const = 0;
  /* sets a new host name */
  virtual void setHostName(const char* newHost) { host = newHost; }
  /* gets a host name */
  const char* getHostName(void) const { return host.c_str(); }
  /* sets a new port */
  void setPort(const char* newPort) { port = newPort; }
  /* gets a port */
  const char* getPort(void) const { return port.c_str(); }
  /* sets a new database name */
  virtual void setDatabase(const char* newDb) { db = newDb; }
  /* gets a database name */
  const char* getDatabase(void) const { return db.c_str(); }
  /* sets a new login to database */
  void setLogin(const char* newLogin) { login = newLogin; }
  /* gets a login */
  const char* getLogin(void) const { return login.c_str(); }
  /* sets a password */
  void setPasswd(const char* newPasswd) { passwd = newPasswd; }
  /* gets a password */
  const char* getPasswd(void) const { return passwd.c_str(); }
  /* active status is OK state */
  virtual bool isActive(void) const { return active; }
  /* Set new name of sequence table */
  void setSequenceTable(const char* new_seq_table) { sequence_table = new_seq_table; }
  /* Get name of sequence table */
  const char* getSequenceTable(void) { return sequence_table.c_str(); }
  /* Get the default character set */
  const char* getDefaultCharset(void) { return default_charset.c_str(); }
  /* Sets configuration */
  virtual void setConfig(const char* newKey,
                         const char* newCert,
                         const char* newCA,
                         const char* newCApath,
                         const char* newCiphers,
                         bool newCompression)
  {
    key = newKey;
    cert = newCert;
    ca = newCA;
    capath = newCApath;
    ciphers = newCiphers;
    compression = newCompression;
  }

  /* virtual methods that must be overloaded in derived classes */

  virtual int init(void) { return DB_COMMAND_OK; }
  virtual int status(void) { return DB_CONNECTION_NONE; }
  virtual int setErr(int err_code, const char* qry) = 0;
  virtual const char* getErrorMsg(void) { return error.c_str(); }

  virtual int connect(bool create) { return DB_COMMAND_OK; }
  virtual int connectFull(const char* newDb,
                          const char* newHost = NULL,
                          const char* newLogin = NULL,
                          const char* newPasswd = NULL,
                          const char* newPort = NULL,
                          const char* newKey = NULL,
                          const char* newCert = NULL,
                          const char* newCA = NULL,
                          const char* newCApath = NULL,
                          const char* newCiphers = NULL,
                          bool newCompression = false);
  virtual void disconnect(void) { active = false; }
  virtual int reset(void) { return DB_COMMAND_OK; }
  virtual int create(void) { return DB_COMMAND_OK; }
  virtual int drop(void) { return DB_COMMAND_OK; }
  virtual long nextid(const char* seq_name) = 0;

  /* \brief copy database */
  virtual int copy(const char* new_name) { return -1; }

  /* \brief drop all extra analytics from database */
  virtual int drop_analytics(void) { return -1; }

  virtual bool exists(void) { return false; }

  /* virtual methods for transaction */

  virtual void start_transaction() {}
  virtual void commit_transaction() {}
  virtual void rollback_transaction() {}

  /* virtual methods for formatting */

  /*! \brief Prepare a SQL statement for execution or querying using C printf nomenclature.
   \param format - C printf compliant format string
   \param ... - optional comma separated list of variables for substitution in format string placeholders.
   \return escaped and formatted string.
   */
  virtual std::string prepare(const char* format, ...);

  /*! \brief Prepare a SQL statement for execution or querying using C printf nomenclature
   \param format - C printf compliant format string
   \param args - va_list of variables for substitution in format string placeholders.
   \return escaped and formatted string.
   */
  virtual std::string vprepare(const char* format, va_list args) = 0;

  virtual bool in_transaction() { return false; }
};

/******************* Class Dataset definition *********************

  global abstraction for using Databases

******************************************************************/

// define Dataset States type
enum dsStates
{
  dsSelect,
  dsInsert,
  dsEdit,
  dsUpdate,
  dsDelete,
  dsInactive
};
enum sqlType
{
  sqlSelect,
  sqlUpdate,
  sqlInsert,
  sqlDelete,
  sqlExec
};

typedef std::list<std::string> StringList;
typedef std::map<std::string, field_value> ParamList;

class Dataset
{
protected:
  /*  char *Host     = ""; //WORK_HOST;
  char *Database = ""; //WORK_DATABASE;
  char *User     = ""; //WORK_USER;
  char *Password = ""; //WORK_PASSWORD;
*/

  Database* db; // info about db connection
  dsStates ds_state; // current state
  Fields *fields_object, *edit_object;
  std::unordered_map<std::string, unsigned int>
      name2indexMap; // Lower case field name -> database index

  /* query results*/
  result_set result;
  result_set exec_res;
  bool autorefresh;

  bool active; // Is Query Opened?
  bool haveError;
  int frecno; // number of current row bei bewegung
  std::string sql;

  ParamList plist; // Paramlist for locate
  bool fbof, feof;
  bool autocommit; // for transactions

  /* Variables to store SQL statements */
  std::string empty_sql; // Executed when result set is empty
  std::string select_sql; // May be only single string variable

  StringList update_sql; // May be an array in complex queries
  /* Field values for updating must has prefix :NEW_ and :OLD_ and field name
   Example:
   update  wt_story set idobject set idobject=:NEW_idobject,body=:NEW_body
   where idobject=:OLD_idobject
   Essentially fields idobject and body must present in the
   result set (select_sql statement) */

  StringList insert_sql; // May be an array in complex queries
  /* Field values for inserting must has prefix :NEW_ and field name
   Example:
   insert into wt_story (idobject, body) values (:NEW_idobject, :NEW_body)
   Essentially fields idobject and body must present in the
   result set (select_sql statement) */

  StringList delete_sql; // May be an array in complex queries
  /* Field values for deleing must has prefix :OLD_ and field name
   Example:
   delete from wt_story where idobject=:OLD_idobject
   Essentially field idobject must present in the
   result set (select_sql statement) */

  /* Arrays for searching */
  //  StringList names, values;

  /* Makes direct inserts into database via mysql_query function */
  virtual void make_insert() = 0;
  /* Edit SQL */
  virtual void make_edit() = 0;
  /* Delete SQL */
  virtual void make_deletion() = 0;

  /* This function works only with MySQL database
   Filling the fields information from select statement */
  virtual void fill_fields(void) = 0;

  /* Parse Sql - replacing fields with prefixes :OLD_ and :NEW_ with current values of OLD or NEW field. */
  void parse_sql(std::string& sql);

  /* Returns old field value (for :OLD) */
  virtual field_value f_old(const char* f);

  /* fast string tolower helper */
  char* str_toLower(char* s);

public:
  /* constructor */
  Dataset();
  explicit Dataset(Database* newDb);

  /* destructor */
  virtual ~Dataset();

  /* sets a new value of connection to database */
  void setDatabase(Database* newDb) { db = newDb; }
  /* retrieves  a database which connected */
  Database* getDatabase(void) { return db; }

  /* sets a new query string to database server */
  void setExecSql(const char* newSql) { sql = newSql; }
  /* retrieves a query string */
  const char* getExecSql(void) { return sql.c_str(); }

  /* status active is OK query */
  virtual bool isActive(void) { return active; }

  virtual void setSqlParams(sqlType t, const char* sqlFrmt, ...);

  /* error handling */
  //  virtual void halt(const char *msg);

  /* last inserted id */
  virtual int64_t lastinsertid() = 0;
  /* sequence numbers */
  virtual long nextid(const char* seq_name) = 0;
  /* sequence numbers */
  virtual int num_rows() = 0;

  /* Open SQL query */
  virtual void open(const std::string& sql) = 0;
  virtual void open() = 0;
  /* func. executes a query without results to return */
  virtual int exec(const std::string& sql) = 0;
  virtual int exec() = 0;
  virtual const void* getExecRes() = 0;
  /* as open, but with our query exec Sql */
  virtual bool query(const std::string& sql) = 0;
  /* Close SQL Query*/
  virtual void close();
  /* This function looks for field Field_name with value equal Field_value
   Returns true if found (position of dataset is set to founded position)
   and false another way (position is not changed). */
  //  virtual bool lookup(char *field_name, char*field_value);
  /* Refresh dataset (reopen it and set the same cursor position) */
  virtual void refresh();

  /*! \brief Drop an index from the database table, provided it exists.
   \param table - name of the table the index to be dropped is associated with
   \param index - name of the index to be dropped
   \return true when the index is guaranteed to no longer exist in the database.
   */
  virtual bool dropIndex(const char* table, const char* index) { return false; }

  /* Go to record No (starting with 0) */
  virtual bool seek(int pos = 0);
  /* Go to record No (starting with 1) */
  virtual bool goto_rec(int pos = 1);
  /* Go to the first record in dataset */
  virtual void first();
  /* Go to next record in dataset */
  virtual void next();
  /* Go to previous record */
  virtual void prev();
  /* Go to last record in dataset */
  virtual void last();

  /* Check for Ending dataset */
  virtual bool eof(void) { return feof; }
  /* Check for Beginning dataset */
  virtual bool bof(void) { return fbof; }

  /* Start the insert mode */
  virtual void insert();
  /* Start the insert mode (alias for insert() function) */
  virtual void append() { insert(); }
  /* Start the edit mode */
  virtual void edit();
  /* Start the delete mode */
  virtual void del();

  /* Add changes, that were made during insert or edit states of dataset into the database */
  virtual void post();
  /* Delete statements from database */
  virtual void deletion();
  /* Cancel changes, made in insert or edit states of dataset */
  virtual void cancel() {}
  /* interrupt any pending database operation  */
  virtual void interrupt() {}

  virtual void setParamList(const ParamList& params);
  virtual bool locate();
  virtual bool locate(const ParamList& params);
  virtual bool findNext();

  /* func. retrieves a number of fields */
  /* Number of fields in a record */
  virtual int field_count();
  virtual int fieldCount();
  /* func. retrieves a field name with 'n' index */
  virtual const char* fieldName(int n);
  /* func. retrieves a field index with 'fn' field name,return -1 when field name not found */
  virtual int fieldIndex(const char* fn);

  /* Set field value */
  virtual bool set_field_value(const char* f_name, const field_value& value);
  /* alias for set_field_value */
  virtual bool sf(const char* f, const field_value& v) { return set_field_value(f, v); }

  /* Return field name by it index */
  //  virtual char *field_name(int f_index) { return field_by_index(f_index)->get_field_name(); }

  /* Getting value of field for current record */
  virtual const field_value& get_field_value(const char* f_name);
  virtual const field_value& get_field_value(int index);
  /* Alias to get_field_value */
  const field_value& fv(const char* f) { return get_field_value(f); }
  const field_value& fv(int index) { return get_field_value(index); }

  /* ------------ for transaction ------------------- */
  void set_autocommit(bool v) { autocommit = v; }
  bool get_autocommit() { return autocommit; }

  /* ----------------- for debug -------------------- */
  Fields* get_fields_object() { return fields_object; }
  Fields* get_edit_object() { return edit_object; }

  /* --------------- for fast access ---------------- */
  const result_set& get_result_set() { return result; }
  const sql_record* get_sql_record();

private:
  Dataset(const Dataset&) = delete;
  Dataset& operator=(const Dataset&) = delete;

  /* Get the column index from a string field_value request */
  bool get_index_map_entry(const char* f_name);

  void set_ds_state(dsStates new_state) { ds_state = new_state; }

public:
  /* return ds_state value */
  dsStates get_state() { return ds_state; }

  /*add a new value to select_sql*/
  void set_select_sql(const char* sel_sql);
  void set_select_sql(const std::string& select_sql);
  /*add a new value to update_sql*/
  void add_update_sql(const char* upd_sql);
  void add_update_sql(const std::string& upd_sql);
  /*add a new value to insert_sql*/
  void add_insert_sql(const char* ins_sql);
  void add_insert_sql(const std::string& ins_sql);
  /*add a new value to delete_sql*/
  void add_delete_sql(const char* del_sql);
  void add_delete_sql(const std::string& del_sql);

  /*clear update_sql*/
  void clear_update_sql();
  /*clear insert_sql*/
  void clear_insert_sql();
  /*clear delete_sql*/
  void clear_delete_sql();

  /* size of insert_sql*/
  size_t insert_sql_count();
  /* size of delete_sql*/
  size_t delete_sql_count();

  /*get value of select_sql*/
  const char* get_select_sql();
};

/******************** Class DbErrors definition *********************

			   error handling

******************************************************************/
class DbErrors
{

public:
  /* constructor */
  DbErrors();
  DbErrors(const char* msg, ...);

  const char* getMsg();

private:
  std::string msg_;
};

} // namespace dbiplus
