/**********************************************************************
 * Copyright (c) 2002, Leo Seib, Hannover
 *
 * Project:SQLiteDataset C++ Dynamic Library
 * Module: SQLiteDataset class realisation file
 * Author: Leo Seib      E-Mail: lev@almaty.pointstrike.net
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


#include "stdafx.h"
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
//#include <iostream.h>
#include <string.h>
#include "sqlitedataset.h"
#include "../../utils/log.h"

#pragma warning (disable:4018)

//************* Callback function ***************************

int callback(void* res_ptr,int ncol, char** reslt,char** cols){

   result_set* r = (result_set*)res_ptr;//dynamic_cast<result_set*>(res_ptr); 
   int sz = r->records.size();
 
   //if (reslt == NULL ) cout << "EMPTY!!!\n";
   if (!r->record_header.size()) 
     for (int i=0; i < ncol; i++) {
      r->record_header[i].name = cols[i];
     }


   sql_record rec;
   field_value v;
 
   if (reslt != NULL) {
     for (int i=0; i<ncol; i++){ 
       if (reslt[i] == NULL) {
	 v.set_asString("");
	 v.set_isNull();
       }
       else {
	 v.set_asString(reslt[i]);
       }
     rec[i] = v;//(long)5;//reslt[i];
     }
   r->records[sz] = rec;
   }
   //cout <<"Fsz:"<<r->record_header.size()<<"\n";
   // cout << "Recs:"<<r->records.size() <<" val |" <<reslt<<"|"<<cols<<"|"<<"\n\n";
  return 0;  
 }


//************* SqliteDatabase implementation ***************

SqliteDatabase::SqliteDatabase() {

  active = false;	
  _in_transaction = false;		// for transaction

  error = "Unknown database error";//S_NO_CONNECTION;
  host = "localhost";
  port = "";
  db = "sqlite.db";
  login = "root";
  passwd, "";
}

SqliteDatabase::~SqliteDatabase() {
  disconnect();
}


Dataset* SqliteDatabase::CreateDataset() const {
	return new SqliteDataset((SqliteDatabase*)this); 
}

int SqliteDatabase::status(void) {
  if (active == false) return DB_CONNECTION_NONE;
  return DB_CONNECTION_OK;
}

int SqliteDatabase::setErr(int err_code){
  switch (err_code) {
  case SQLITE_OK: error ="Successful result";
    break;
  case SQLITE_ERROR: error = "SQL error or missing database";
    break;
  case SQLITE_INTERNAL: error = "An internal logic error in SQLite";
    break;
  case SQLITE_PERM: error ="Access permission denied";
    break;
  case SQLITE_ABORT: error = "Callback routine requested an abort";
    break;
  case SQLITE_BUSY: error = "The database file is locked";
    break;
  case SQLITE_LOCKED: error = "A table in the database is locked";
    break;
  case SQLITE_NOMEM: error = "A malloc() failed";
    break;
  case SQLITE_READONLY: error = "Attempt to write a readonly database";
    break;
  case SQLITE_INTERRUPT: error = "Operation terminated by sqlite_interrupt()";
    break;
  case  SQLITE_IOERR: error = "Some kind of disk I/O error occurred";
    break;
  case  SQLITE_CORRUPT: error = "The database disk image is malformed";
    break;
  case SQLITE_NOTFOUND: error = "(Internal Only) Table or record not found";
    break;
  case SQLITE_FULL: error = "Insertion failed because database is full";
    break;
  case SQLITE_CANTOPEN: error = "Unable to open the database file";
    break;
  case SQLITE_PROTOCOL: error = "Database lock protocol error";
    break;
  case SQLITE_EMPTY:  error = "(Internal Only) Database table is empty";
    break;
  case SQLITE_SCHEMA: error = "The database schema changed";
    break;
  case SQLITE_TOOBIG: error = "Too much data for one row of a table";
    break;
  case SQLITE_CONSTRAINT: error = "Abort due to contraint violation";
    break;
  case SQLITE_MISMATCH:  error = "Data type mismatch";
    break;
  default : error = "Undefined SQLite error";
  }
  return err_code;
}

const char *SqliteDatabase::getErrorMsg() {
   return error.c_str();
}

int SqliteDatabase::connect() {
	try
	{
		disconnect();
		if (conn = sqlite_open(db.c_str(),0,NULL)) 
		{
			char* err=NULL;
			if (setErr(sqlite_exec(getHandle(),"PRAGMA empty_result_callbacks=ON",NULL,NULL,&err)) != SQLITE_OK) 
			{
				CLog::Log("sqlite:unable to set callback on:%s", error.c_str());
				throw DbErrors(getErrorMsg());
			}
			active = true;
			return DB_CONNECTION_OK;
		}
		CLog::Log("unable to open database:%s (%i)", db.c_str(),GetLastError());
		return DB_CONNECTION_NONE;
	}
	catch(...)
	{
		CLog::Log("unable to open database:%s (%i)", db.c_str(),GetLastError());
	}
	return DB_CONNECTION_NONE;
};

void SqliteDatabase::disconnect(void) {
  if (active == false) return;
  sqlite_close(conn);
  active = false;
};

int SqliteDatabase::create() {
  return connect();
};

int SqliteDatabase::drop() {
  if (active == false) return DB_ERROR;
  disconnect();
  if (!unlink(db.c_str())) 
     return DB_ERROR;
  return DB_COMMAND_OK;
};


long SqliteDatabase::nextid(const char* sname) {
  if (!active) return DB_UNEXPECTED_RESULT;
  int id;//,nrow,ncol;
  result_set res;
  char sqlcmd[512];
  sprintf(sqlcmd,"select nextid from %s where seq_name = '%s'",sequence_table.c_str(), sname);
  if (last_err = sqlite_exec(getHandle(),sqlcmd,&callback,&res,NULL) != SQLITE_OK) {
    return DB_UNEXPECTED_RESULT;
    }
  if (res.records.size() == 0) {
    id = 1;
    sprintf(sqlcmd,"insert into %s (nextid,seq_name) values (%d,'%s')",sequence_table.c_str(),id,sname);
    if (last_err = sqlite_exec(conn,sqlcmd,NULL,NULL,NULL) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
    return id;
  }
  else {
    id = res.records[0][0].get_asInteger()+1;
    sprintf(sqlcmd,"update %s set nextid=%d where seq_name = '%s'",sequence_table.c_str(),id,sname);
    if (last_err = sqlite_exec(conn,sqlcmd,NULL,NULL,NULL) != SQLITE_OK) return DB_UNEXPECTED_RESULT;
    return id;    
  }
  return DB_UNEXPECTED_RESULT;
}


// methods for transactions
// ---------------------------------------------
void SqliteDatabase::start_transaction() {
  if (active) {
    sqlite_exec(conn,"begin",NULL,NULL,NULL);
    _in_transaction = true;
  }
}

void SqliteDatabase::commit_transaction() {
  if (active) {
    sqlite_exec(conn,"commit",NULL,NULL,NULL);
    _in_transaction = false;
  }
}

void SqliteDatabase::rollback_transaction() {
  if (active) {
    sqlite_exec(conn,"rollback",NULL,NULL,NULL);
    _in_transaction = false;
  }  
}



//************* SqliteDataset implementation ***************

SqliteDataset::SqliteDataset():Dataset() {
  haveError = false;
  db = NULL;
}


SqliteDataset::SqliteDataset(SqliteDatabase *newDb):Dataset(newDb) {
  haveError = false;
  db = newDb;
}

 SqliteDataset::~SqliteDataset(){
   if (errmsg) sqlite_free_table(&errmsg);
 }



//--------- protected functions implementation -----------------//

sqlite* SqliteDataset::handle(){
  if (db != NULL){
    return dynamic_cast<SqliteDatabase*>(db)->getHandle();
      }
  else return NULL;
}

void SqliteDataset::make_query(StringList &_sql) {
  string query;

 try {

  if (autocommit) db->start_transaction();


  if (db == NULL) throw DbErrors("No Database Connection");

  //close();


  for (list<string>::iterator i =_sql.begin(); i!=_sql.end(); i++) {
	query = *i;
	char* err=NULL; 
	Dataset::parse_sql(query);
	//cout << "Executing: "<<query<<"\n\n";
	if (db->setErr(sqlite_exec(this->handle(),query.c_str(),NULL,NULL,&err))!=SQLITE_OK) {
//	  fprintf(stderr,"Error: %s",err);
	  throw DbErrors(db->getErrorMsg());
	}
  } // end of for


  if (db->in_transaction() && autocommit) db->commit_transaction();

  active = true;
  ds_state = dsSelect;		
  refresh();

 } // end of try
 catch(...) {
  if (db->in_transaction()) db->rollback_transaction();
 }

}


void SqliteDataset::make_insert() {
  make_query(insert_sql);
  last();
}


void SqliteDataset::make_edit() {
  make_query(update_sql);
}


void SqliteDataset::make_deletion() {
  make_query(delete_sql);
}


void SqliteDataset::fill_fields() {
  //cout <<"rr "<<result.records.size()<<"|" << frecno <<"\n";
  if ((db == NULL) || (result.record_header.size() == 0) || (result.records.size() < frecno)) return;
  if (fields_object->size() == 0) // Filling columns name
    for (int i = 0; i < result.record_header.size(); i++) {
      (*fields_object)[i].props = result.record_header[i];
      (*edit_object)[i].props = result.record_header[i];
    }

  //Filling result
  if (result.records.size() != 0) {
   for (int i = 0; i < result.records[frecno].size(); i++){
    (*fields_object)[i].val = result.records[frecno][i];
    (*edit_object)[i].val = result.records[frecno][i];
   }
  }
  else
   for (int i = 0; i < result.record_header.size(); i++){
    (*fields_object)[i].val = "";
    (*edit_object)[i].val = "";
   }    

}


//------------- public functions implementation -----------------//

int SqliteDataset::exec(const string &sql) {
  if (!handle()) throw DbErrors("No Database Connection");
  int res;
  exec_res.record_header.clear();
  exec_res.records.clear();
  //if ((strncmp("select",sql.c_str(),6) == 0) || (strncmp("SELECT",sql.c_str(),6) == 0)) 
  if(res = db->setErr(sqlite_exec(handle(),sql.c_str(),&callback,&exec_res,&errmsg)) == SQLITE_OK)
    return res;
  else
    {
//      fprintf(stderr,"Error: %s",errmsg);
      throw DbErrors(db->getErrorMsg());
    }
  //    else
  //return sqlite_exec(handle(),sql.c_str(),NULL,NULL,NULL);
	}

int SqliteDataset::exec() {
	return exec(sql);
}

const void* SqliteDataset::getExecRes() {
  return &exec_res;
}


bool SqliteDataset::query(const char *query) {

  try{
    if (db == NULL) throw DbErrors("Database is not Defined");
    if(dynamic_cast<SqliteDatabase*>(db)->getHandle() == NULL) throw DbErrors("No Database Connection");
  if (strncmp("select",query,6) != 0) throw DbErrors("MUST be select SQL!"); 

  close();

  //cout <<  "Curr size "<<num_rows()<<"\n\n";

  if (  db->setErr(sqlite_exec(handle(),query,&callback,&result,&errmsg)) == SQLITE_OK) {
        active = true;
	ds_state = dsSelect;
	//cout <<  "Curr size2 "<<num_rows()<<"\n";
	this->first();
	//cout <<  "Curr size3 "<<num_rows()<<"\n";
	//cout <<  "Curr fcount "<<field_count()<<"\n\n";
	return true;
      }
      else {
//	fprintf(stderr,"Error: %s",errmsg);
          throw DbErrors(db->getErrorMsg());
      }
  
 } // end of try
 catch(...) { return false; }
}

bool SqliteDataset::query(const string &q){
  return query(q.c_str());
}

void SqliteDataset::open(const string &sql) {
	set_select_sql(sql);
	open();
}

void SqliteDataset::open() {
  if (select_sql.size()) {
    //cout <<select_sql <<"  open\n\n";
    query(select_sql.c_str()); 
  }
  else {
    ds_state = dsInactive;
  }
}


void SqliteDataset::close() {
  Dataset::close();
  result.record_header.clear();
  result.records.clear();
  edit_object->clear();
  fields_object->clear();
  ds_state = dsInactive;
  active = false;
}


void SqliteDataset::cancel() {
  if ((ds_state == dsInsert) || (ds_state==dsEdit))
    if (result.record_header.size()) ds_state = dsSelect;
    else ds_state = dsInactive;
}


int SqliteDataset::num_rows() {
  return result.records.size();
}


bool SqliteDataset::eof() {
  return feof;
}


bool SqliteDataset::bof() {
  return fbof;
}


void SqliteDataset::first() {
  Dataset::first();
  this->fill_fields();
  //cout << "In first "<< fields_object->size()<<"\n";
}

void SqliteDataset::last() {
  Dataset::last();
  fill_fields();
}

void SqliteDataset::prev(void) {
  Dataset::prev();
  fill_fields();
}

void SqliteDataset::next(void) {
  Dataset::next();
  if (!eof()) 
      fill_fields();
}


bool SqliteDataset::seek(int pos) {
  if (ds_state == dsSelect) {
    Dataset::seek(pos);
    fill_fields();
    return true;	
    }
  return false;
}



long SqliteDataset::nextid(const char *seq_name) {
  if (handle()) return db->nextid(seq_name);
  else return DB_UNEXPECTED_RESULT;
}

