/**********************************************************************
 * Copyright (c) 2002, Leo Seib, Hannover
 *
 * Project: C++ Dynamic Library
 * Module: Dataset abstraction later realisation file
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
#include <string.h>

#include "dataset.h"


#pragma warning (disable:4018)
#pragma warning (disable:4800)

extern "C" {

//************* Database implementation ***************

Database::Database() {
  active = false;	// No connection yet
  error = "";//S_NO_CONNECTION;
  host = "";
  port = "";
  db = "";
  login = "";
  passwd = "";
  sequence_table = "db_sequence";
}

Database::~Database() {
  disconnect();		// Disconnect if connected to database
}

int Database::connectFull(const char *newHost, const char *newPort, const char *newDb, const char *newLogin, const char *newPasswd) {
  host = newHost;
  port = newPort;
  db = newDb;
  login = newLogin;
  passwd = newPasswd;
  return connect();
}

}


//************* Dataset implementation ***************

Dataset::Dataset() {

  db = NULL;
  haveError = active = false;
  frecno = 0;
  fbof = feof = true;
  autocommit = true;

  select_sql = "";

  fields_object = new Fields();
  
  edit_object = new Fields();
}



Dataset::Dataset(Database *newDb) {

  db = newDb;
  haveError = active = false;
  frecno = 0;
  fbof = feof = true;
  autocommit = true;

  select_sql = "";
 
  fields_object = new Fields();

  edit_object = new Fields();

}


Dataset::~Dataset() {
  update_sql.clear();
  insert_sql.clear();
  delete_sql.clear();


  delete fields_object;
  delete edit_object;
  
}


void Dataset::setSqlParams(const char *sqlFrmt, sqlType t, ...) {
  va_list ap;
  char sqlCmd[DB_BUFF_MAX+1];

  va_start(ap, t);
  sprintf(sqlCmd, sqlFrmt, ap);
  va_end(ap);
  
   switch (t) {
       case sqlSelect: set_select_sql(sqlCmd);
                       break;
       case sqlUpdate: add_update_sql(sqlCmd);
                       break;
       case sqlInsert: add_insert_sql(sqlCmd);
                       break;
       case sqlDelete: add_delete_sql(sqlCmd);
                       break;
       case sqlExec: sql = sqlCmd;
             	    break;
		    
  }    
}



void Dataset::set_select_sql(const char *sel_sql) {
 select_sql = sel_sql;
}

void Dataset::set_select_sql(const string &sel_sql) {
 select_sql = sel_sql;
}


void Dataset::parse_sql(string &sql) {
  string fpattern,by_what;
  pars.set_str(sql.c_str());
  for (int i=0;i< fields_object->size();i++) {
    fpattern = ":OLD_"+(*fields_object)[i].props.name;
    by_what = "'"+(*fields_object)[i].val.get_asString()+"'";
    //cout << "parsing " << fpattern <<by_what<<"\n\n";
    sql = pars.replace(fpattern,by_what);
    }

  for (int i=0;i< edit_object->size();i++) {
    fpattern = ":NEW_"+(*edit_object)[i].props.name;
    by_what = "'"+(*edit_object)[i].val.get_asString()+"'";
    sql = pars.replace(fpattern,by_what);
  }
  
//   StringList before_array, after_array;
//   int tag = 0;
//   bool eol_reached = false,
//        was_changed = false,
//        flag = false;
//   ExtString which_before, which_after;
//   ExtString bef, aft, prev_before, right_side, which_field, changed_field, f_value;

//   before_array.add(":NEW_", tag);
//   before_array.add(":OLD_", tag);

//   after_array.add(")", tag);
//   after_array.add(",", tag);
//   after_array.add(" ", tag);

//   sq.squish();
//   bef = sq.before_arr(before_array, which_before);

//   while (!(bef == prev_before)) {
// 	right_side = sq.after(which_before, flag);
// 	right_side.squish();

// 	aft = right_side.after_arr(after_array, which_after);
//         aft.squish();
//  	which_field = right_side.before(which_after);

// 	// checking whather we reach end of line
// 	if ((which_field == "\0") && (which_before != "\0")) {    			
// 		which_field = right_side;
// 		eol_reached = true;
// 		}

// 	// If new field and is in insert or edit mode - looks in edit_object
// 	if ((which_before == ":NEW_") && (which_field != "\0")) {
// 		which_field.squish();
//                 f_value.assign(fv(which_field.getChars()));
// 		f_value.addslashes();
// 		changed_field.assign("'");
// 		changed_field + f_value + "'";
// 	}
// 	else
// 	// else looking old value in the current result set
// 	   if ((which_before == ":OLD_") && (which_field != "\0")) {
// 		which_field.squish();
//                 f_value.assign(f_old(which_field.getChars()));
// 		f_value.addslashes();
// 		changed_field.assign("'");
// 		changed_field + f_value + "'";
// 		}

// 	if (!eol_reached)  {

// 		sq.assign(bef + changed_field + which_after + aft);	
// 		}
// 	else {
// 		if (!was_changed && (which_field != "\0")) {

// 			sq.assign(bef + changed_field + which_after + aft);
// 			was_changed = true;
// 			}
// 		}


// 	prev_before = bef;
// 	bef = sq.before_arr(before_array, which_before);

//   }

}


void Dataset::close(void) {
  haveError  = false;
  frecno = 0;
  fbof = feof = true;
  active = false;
}


bool Dataset::seek(int pos) {
  frecno = (pos<num_rows()-1)? pos: num_rows()-1;
  frecno = (frecno<0)? 0: frecno;
  fbof = feof = (num_rows()==0)? true: false;
  return frecno;
}


void Dataset::refresh() {
  int row = frecno;				
  if ((row != 0) && active) {
    close();
    open();
    seek(row);
  }
  else open();		
}


void Dataset::first() {
  if (ds_state == dsSelect) {
    frecno = 0;
    feof = fbof = (num_rows()>0)? false : true;
  }
}

void Dataset::next() {
  if (ds_state == dsSelect) {
    fbof = false;
    if (frecno<num_rows()-1) {
      frecno++;
      feof = false;
    } else feof = true;
    if (num_rows()<=0) fbof = feof = true;
  }
}

void Dataset::prev() {
  if (ds_state == dsSelect) {
    feof = false;
    if (frecno) {
      frecno--;
      fbof = false;
    } else fbof = true;
    if (num_rows()<=0) fbof = feof = true;
  }
}

void Dataset::last() {
  if (ds_state == dsSelect) {
    frecno = (num_rows()>0)? num_rows()-1: 0;
    feof = fbof = (num_rows()>0)? false : true;
  }
}

bool Dataset::goto_rec(int pos) {
  if (ds_state == dsSelect) {
    return seek(pos - 1);
  }
  return false;
}


void Dataset::insert() {
  //cout << "insert\n\n";
   for (int i=0; i<field_count(); i++) {
     (*fields_object)[i].val = "";
     (*edit_object)[i].val = "";
     //cout <<"Insert:"<<i<<"\n\n";
   }
  ds_state = dsInsert;
}


void Dataset::edit() {
  if (ds_state != dsSelect) {
    //cerr<<"Editing is possible only when query exists!";
    return;
  }
  for (int i=0; i<fields_object->size(); i++) {
       (*edit_object)[i].val = (*fields_object)[i].val; 
  }
  ds_state = dsEdit;
}


void Dataset::post() {
  if (ds_state == dsInsert) make_insert();
  else if (ds_state == dsEdit) make_edit();
}


void Dataset::deletion() {
  if (ds_state == dsSelect) make_deletion();
}


bool Dataset::set_field_value(const char *f_name, const field_value &value) {
  bool found = false;
  if ((ds_state == dsInsert) || (ds_state == dsEdit)) {
      for (int i=0; i < fields_object->size(); i++) 
	if ((*edit_object)[i].props.name == f_name) {
			     (*edit_object)[i].val = value;
			     found = true;
	}
      if (!found) throw DbErrors("Field not found: %s",f_name);
    return true;
  }
  throw DbErrors("Not in Insert or Edit state");
  //  return false;
}


const field_value Dataset::get_field_value(const char *f_name) {
  if (ds_state != dsInactive) {
    if (ds_state == dsEdit || ds_state == dsInsert)
		{
      for (int i=0; i < edit_object->size(); i++) 
			if ((*edit_object)[i].props.name == f_name)
				return (*edit_object)[i].val;
			throw DbErrors("Field not found: %s",f_name);
    }
    else
      for (int i=0; i < fields_object->size(); i++) 
			{
			  //OutputDebugString( (*fields_object)[i].props.name.c_str());
        //OutputDebugString( "\n");
				if ((*fields_object)[i].props.name == f_name)
					return (*fields_object)[i].val;
			}
      throw DbErrors("Field not found: %s",f_name);
    }
  throw DbErrors("Dataset state is Inactive");
  //field_value fv;
  //return fv;
}


const field_value Dataset::f_old(const char *f_name) {
  if (ds_state != dsInactive)
    for (int i=0; i < fields_object->size(); i++) 
      if ((*fields_object)[i].props.name == f_name)
	return (*fields_object)[i].val;
  field_value fv;
  return fv;
}



void Dataset::setParamList(const ParamList &params){
  plist = params;
}


bool Dataset::locate(){
  bool result;
  if (plist.empty()) return false;

  std::map<string,field_value>::const_iterator i;
  first();
  while (!eof()) {
    result = true;
    for (i=plist.begin();i!=plist.end();++i)
      if (fv(i->first.c_str()).get_asString() == i->second.get_asString()) {
	continue;
      }
      else {result = false; break;}
    if (result) { return result;}
    next();
  }
  return false;
}

bool Dataset::locate(const ParamList &params) {
  plist = params;
  return locate();
}

bool Dataset::findNext(void) {
  bool result;
  if (plist.empty()) return false;

  std::map<string,field_value>::const_iterator i;
  while (!eof()) {
    result = true;
    for (i=plist.begin();i!=plist.end();++i)
      if (fv(i->first.c_str()).get_asString() == i->second.get_asString()) {
	continue;
      }
      else {result = false; break;}
    if (result) { return result;}
    next();
  }
  return false;
}


void Dataset::add_update_sql(const char *upd_sql){
  string s = upd_sql;
  update_sql.push_back(s);
}


void Dataset::add_update_sql(const string &upd_sql){
  update_sql.push_back(upd_sql);
}

void Dataset::add_insert_sql(const char *ins_sql){
  string s = ins_sql;
  insert_sql.push_back(s);
}


void Dataset::add_insert_sql(const string &ins_sql){
  insert_sql.push_back(ins_sql);
}

void Dataset::add_delete_sql(const char *del_sql){
  string s = del_sql;
  delete_sql.push_back(s);
}


void Dataset::add_delete_sql(const string &del_sql){
  delete_sql.push_back(del_sql);
}

void Dataset::clear_update_sql(){
  update_sql.clear();
}

void Dataset::clear_insert_sql(){
  insert_sql.clear();
}

void Dataset::clear_delete_sql(){
  delete_sql.clear();
}

int Dataset::field_count() { return fields_object->size();}
int Dataset::fieldCount() { return fields_object->size();}

const char *Dataset::fieldName(int n) {
  if ( n < field_count() && n >= 0)
    return (*fields_object)[n].props.name.c_str();
  else
    return NULL;
}

int Dataset::fieldSize(int n) {
  if ( n < field_count() && n >= 0)
    return (*fields_object)[n].props.field_len;
  else
    return 0;
}

int Dataset::fieldIndex(const char *fn) {
for (int i=0; i < fields_object->size(); i++) 
      if ((*fields_object)[i].props.name == fn)
	return i;
  return -1;
}



//************* DbErrors implementation ***************

DbErrors::DbErrors() {

  //fprintf(stderr, "\nUnknown Database Error\n");
}


DbErrors::DbErrors(const char *msg, ...) {
  va_list vl;
  va_start(vl, msg);
  //cout << "In db\n\n";
  char buf[DB_BUFF_MAX]="";
  sprintf(buf, msg, vl);
  va_end(vl);

  OutputDebugString("SQLite Error:");
  OutputDebugString(buf);
  OutputDebugString("\n");
}


