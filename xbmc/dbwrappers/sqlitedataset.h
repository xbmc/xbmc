/**********************************************************************
 * Copyright (c) 2004, Leo Seib, Hannover
 *
 * Project:SQLiteDataset C++ Dynamic Library
 * Module: SQLiteDataset class header file
 * Author: Leo Seib      E-Mail: leoseib@web.de
 * Begin: 5/04/2002
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **********************************************************************/

#ifndef _SQLITEDATASET_H
#define _SQLITEDATASET_H

#include <stdio.h>
#include "dataset.h"
#include <sqlite3.h>

namespace dbiplus {
/***************** Class SqliteDatabase definition ******************

       class 'SqliteDatabase' connects with Sqlite-server

******************************************************************/
class SqliteDatabase: public Database {
protected:
/* connect descriptor */
  sqlite3 *conn;
  bool _in_transaction;
  int last_err;

public:
/* default constructor */
  SqliteDatabase();
/* destructor */
  ~SqliteDatabase();

  Dataset *CreateDataset() const; 

/* func. returns connection handle with SQLite-server */
  sqlite3 *getHandle() {  return conn; }
/* func. returns current status about SQLite-server connection */
  virtual int status();
  virtual int setErr(int err_code,const char * qry);
/* func. returns error message if error occurs */
  virtual const char *getErrorMsg();
/* sets a new host name */
  virtual void setHostName(const char *newHost);
/* sets a database name */
  virtual void setDatabase(const char *newDb);

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

  virtual long nextid(const char* seq_name);

/* virtual methods for transaction */

  virtual void start_transaction();
  virtual void commit_transaction();
  virtual void rollback_transaction();

/* virtual methods for formatting */
  virtual std::string vprepare(const char *format, va_list args);

  bool in_transaction() {return _in_transaction;}; 	

};



/***************** Class SqliteDataset definition *******************

       class 'SqliteDataset' does a query to SQLite-server

******************************************************************/

class SqliteDataset : public Dataset {
protected:
  sqlite3* handle();

/* Makes direct queries to database */
  virtual void make_query(StringList &_sql);
/* Makes direct inserts into database */
  virtual void make_insert();
/* Edit SQL */
  virtual void make_edit();
/* Delete SQL */
  virtual void make_deletion();

  //static int sqlite_callback(void* res_ptr,int ncol, char** reslt, char** cols);

/* This function works only with MySQL database
  Filling the fields information from select statement */
  virtual void fill_fields();
/* Changing field values during dataset navigation */
  virtual void free_row();  // free the memory allocated for the current row

public:
/* constructor */
  SqliteDataset();
  SqliteDataset(SqliteDatabase *newDb);

/* destructor */
  ~SqliteDataset();

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
  virtual bool query(const char *query);
  virtual bool query(const std::string &query);
/* func. closes a query */
  virtual void close(void);
/* Cancel changes, made in insert or edit states of dataset */
  virtual void cancel();
/* last inserted id */
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
