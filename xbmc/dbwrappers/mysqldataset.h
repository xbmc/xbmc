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

#ifndef _MYSQLDATASET_H
#define _MYSQLDATASET_H

#include <stdio.h>
#include "dataset.h"
#include "mysql/mysql.h"

namespace dbiplus {
/***************** Class MysqlDatabase definition ******************

       class 'MysqlDatabase' connects with MySQL-server

******************************************************************/
class MysqlDatabase: public Database {
protected:
/* connect descriptor */
  MYSQL* conn;
  bool _in_transaction;
  int last_err;


public:
/* default constructor */
  MysqlDatabase();
/* destructor */
  ~MysqlDatabase();

  Dataset *CreateDataset() const;

/* func. returns connection handle with MySQL-server */
  MYSQL *getHandle() {  return conn; }
/* func. returns current status about MySQL-server connection */
  virtual int status();
  virtual int setErr(int err_code,const char * qry);
/* func. returns error message if error occurs */
  virtual const char *getErrorMsg();

/* func. connects to database-server */
  virtual int connect(bool create);
/* func. disconnects from database-server */
  virtual void disconnect();
/* func. creates new database */
  virtual int create();
/* func. deletes database */
  virtual int drop();
/* check if database exists (ie has tables/views defined) */
  virtual bool exists();

/* \brief copy database */
  virtual int copy(const char *backup_name);

/* \brief drop all extra analytics from database */
  virtual int drop_analytics(void);

  virtual long nextid(const char* seq_name);

/* virtual methods for transaction */

  virtual void start_transaction();
  virtual void commit_transaction();
  virtual void rollback_transaction();

/* virtual methods for formatting */
  virtual std::string vprepare(const char *format, va_list args);

  bool in_transaction() {return _in_transaction;};
  int query_with_reconnect(const char* query);

private:

  typedef struct StrAccum StrAccum;

  char et_getdigit(double *val, int *cnt);
  void appendSpace(StrAccum *pAccum, int N);
  void mysqlVXPrintf(StrAccum *pAccum, int useExtended, const char *fmt, va_list ap);
  void mysqlStrAccumAppend(StrAccum *p, const char *z, int N);
  char * mysqlStrAccumFinish(StrAccum *p);
  void mysqlStrAccumReset(StrAccum *p);
  void mysqlStrAccumInit(StrAccum *p, char *zBase, int n, int mx);
  char *mysql_vmprintf(const char *zFormat, va_list ap);
};



/***************** Class MysqlDataset definition *******************

       class 'MysqlDataset' does a query to MySQL-server

******************************************************************/

class MysqlDataset : public Dataset {
protected:
  MYSQL* handle();

/* Makes direct queries to database */
  virtual void make_query(StringList &_sql);
/* Makes direct inserts into database */
  virtual void make_insert();
/* Edit SQL */
  virtual void make_edit();
/* Delete SQL */
  virtual void make_deletion();


/* This function works only with MySQL database
  Filling the fields information from select statement */
  virtual void fill_fields();
/* Changing field values during dataset navigation */
  virtual void free_row();  // free the memory allocated for the current row

public:
/* constructor */
  MysqlDataset();
  MysqlDataset(MysqlDatabase *newDb);

/* destructor */
  ~MysqlDataset();

/* set autorefresh boolean value (if true - refresh the data after edit() 
or insert() operations default = false) */
  void set_autorefresh(bool val);

/* opens a query  & then sets a query results */
  virtual void open();
  virtual void open(const std::string &sql);
/* func. executes a query without results to return */
  virtual int  exec ();
  virtual int  exec (const std::string &sql);
  virtual const void* getExecRes();
/* as open, but with our query exept Sql */
  virtual bool query(const std::string &query);
/* func. closes a query */
  virtual void close(void);
/* Cancel changes, made in insert or edit states of dataset */
  virtual void cancel();
/* last insert id */
  virtual int64_t lastinsertid();
/* sequence numbers */
  virtual long nextid(const char *seq_name);
/* sequence numbers */
  virtual int num_rows();
/* interupt any pending database operation  */
  virtual void interrupt();

  virtual bool bof();
  virtual bool eof();
  virtual void first();
  virtual void last();
  virtual void prev();
  virtual void next();
/* Go to record No (starting with 0) */
  virtual bool seek(int pos=0);

  virtual bool dropIndex(const char *table, const char *index);
};
} //namespace
#endif
