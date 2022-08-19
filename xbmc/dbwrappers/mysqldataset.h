/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dataset.h"

#include <stdio.h>
#ifdef HAS_MYSQL
#include <mysql/mysql.h>
#elif defined(HAS_MARIADB)
#include <mariadb/mysql.h>
#endif

namespace dbiplus
{
/***************** Class MysqlDatabase definition ******************

       class 'MysqlDatabase' connects with MySQL-server

******************************************************************/
class MysqlDatabase : public Database
{
protected:
  /* connect descriptor */
  MYSQL* conn;
  bool _in_transaction;
  int last_err;

public:
  /* default constructor */
  MysqlDatabase();
  /* destructor */
  ~MysqlDatabase() override;

  Dataset* CreateDataset() const override;

  /* func. returns connection handle with MySQL-server */
  MYSQL* getHandle() { return conn; }
  /* func. returns current status about MySQL-server connection */
  int status() override;
  int setErr(int err_code, const char* qry) override;
  /* func. returns error message if error occurs */
  const char* getErrorMsg() override;

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
  int query_with_reconnect(const char* query);
  void configure_connection();

private:
  typedef struct StrAccum StrAccum;

  char et_getdigit(double* val, int* cnt);
  void appendSpace(StrAccum* pAccum, int N);
  void mysqlVXPrintf(StrAccum* pAccum, int useExtended, const char* fmt, va_list ap);
  bool mysqlStrAccumAppend(StrAccum* p, const char* z, int N);
  char* mysqlStrAccumFinish(StrAccum* p);
  void mysqlStrAccumReset(StrAccum* p);
  void mysqlStrAccumInit(StrAccum* p, char* zBase, int n, int mx);
  std::string mysql_vmprintf(const char* zFormat, va_list ap);
};

/***************** Class MysqlDataset definition *******************

       class 'MysqlDataset' does a query to MySQL-server

******************************************************************/

class MysqlDataset : public Dataset
{
protected:
  MYSQL* handle();

  /* Makes direct queries to database */
  virtual void make_query(StringList& _sql);
  /* Makes direct inserts into database */
  void make_insert() override;
  /* Edit SQL */
  void make_edit() override;
  /* Delete SQL */
  void make_deletion() override;

  /* This function works only with MySQL database
  Filling the fields information from select statement */
  void fill_fields() override;
  /* Changing field values during dataset navigation */
  virtual void free_row(); // free the memory allocated for the current row

public:
  /* constructor */
  MysqlDataset();
  explicit MysqlDataset(MysqlDatabase* newDb);

  /* destructor */
  ~MysqlDataset() override;

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
  /* last insert id */
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
