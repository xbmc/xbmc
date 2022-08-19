/*
 *  Copyright (C) 2004, Leo Seib, Hannover
 *
 *  Project:SQLiteDataset C++ Dynamic Library
 *  Module: SQLiteDataset class header file
 *  Author: Leo Seib      E-Mail: leoseib@web.de
 *  Begin: 5/04/2002
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dataset.h"

#include <stdio.h>

#include <sqlite3.h>

namespace dbiplus
{
/***************** Class SqliteDatabase definition ******************

       class 'SqliteDatabase' connects with Sqlite-server

******************************************************************/
class SqliteDatabase : public Database
{
protected:
  /* connect descriptor */
  sqlite3* conn;
  bool _in_transaction;
  int last_err;

public:
  /* default constructor */
  SqliteDatabase();
  /* destructor */
  ~SqliteDatabase() override;

  Dataset* CreateDataset() const override;

  /* func. returns connection handle with SQLite-server */
  sqlite3* getHandle() { return conn; }
  /* func. returns current status about SQLite-server connection */
  int status() override;
  int setErr(int err_code, const char* qry) override;
  /* func. returns error message if error occurs */
  const char* getErrorMsg() override;
  /* sets a new host name */
  void setHostName(const char* newHost) override;
  /* sets a database name */
  void setDatabase(const char* newDb) override;

  /* func. connects to database-server */

  int connect(bool create) override;
  /* func. disconnects from database-server */
  void disconnect() override;
  /* func. creates new database */
  int create() override;
  /* func. deletes database */
  int drop() override;
  /* check if database exists (ie has tables/views defined) */
  bool exists() override;

  /* \brief copy database */
  int copy(const char* backup_name) override;

  /* \brief drop all extra analytics from database */
  int drop_analytics(void) override;

  long nextid(const char* seq_name) override;

  /* virtual methods for transaction */

  void start_transaction() override;
  void commit_transaction() override;
  void rollback_transaction() override;

  /* virtual methods for formatting */
  std::string vprepare(const char* format, va_list args) override;

  bool in_transaction() override { return _in_transaction; }
};

/***************** Class SqliteDataset definition *******************

       class 'SqliteDataset' does a query to SQLite-server

******************************************************************/

class SqliteDataset : public Dataset
{
protected:
  sqlite3* handle();

  /* Makes direct queries to database */
  virtual void make_query(StringList& _sql);
  /* Makes direct inserts into database */
  void make_insert() override;
  /* Edit SQL */
  void make_edit() override;
  /* Delete SQL */
  void make_deletion() override;

  //static int sqlite_callback(void* res_ptr,int ncol, char** result, char** cols);

  /* This function works only with MySQL database
  Filling the fields information from select statement */
  void fill_fields() override;
  /* Changing field values during dataset navigation */
  virtual void free_row(); // free the memory allocated for the current row

public:
  /* constructor */
  SqliteDataset();
  explicit SqliteDataset(SqliteDatabase* newDb);

  /* destructor */
  ~SqliteDataset() override;

  /* set autorefresh boolean value (if true - refresh the data after edit()
or insert() operations default = false) */
  void set_autorefresh(bool val);

  /* opens a query  & then sets a query results */
  void open() override;
  void open(const std::string& sql) override;
  /* func. executes a query without results to return */
  int exec() override;
  int exec(const std::string& sql) override;
  const void* getExecRes() override;
  /* as open, but with our query exec Sql */
  bool query(const std::string& query) override;
  /* func. closes a query */
  void close(void) override;
  /* Cancel changes, made in insert or edit states of dataset */
  void cancel() override;
  /* last inserted id */
  int64_t lastinsertid() override;
  /* sequence numbers */
  long nextid(const char* seq_name) override;
  /* sequence numbers */
  int num_rows() override;
  /* interrupt any pending database operation  */
  void interrupt() override;

  bool bof() override;
  bool eof() override;
  void first() override;
  void last() override;
  void prev() override;
  void next() override;
  /* Go to record No (starting with 0) */
  bool seek(int pos = 0) override;

  bool dropIndex(const char* table, const char* index) override;
};
} // namespace dbiplus
