/*
 *  Copyright (C) 2004, Leo Seib, Hannover
 *
 *  Project: C++ Dynamic Library
 *  Module: Dataset abstraction layer realisation file
 *  Author: Leo Seib      E-Mail: leoseib@web.de
 *  Begin: 5/04/2002
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSES/README.md for more information.
 */

#include "dataset.h"

#include "utils/log.h"

#include <algorithm>
#include <cstring>

#ifndef __GNUC__
#pragma warning (disable:4800)
#endif

namespace dbiplus {
//************* Database implementation ***************

Database::Database():
  error(), //S_NO_CONNECTION,
  host(),
  port(),
  db(),
  login(),
  passwd(),
  sequence_table("db_sequence")
{
  active = false;	// No connection yet
  compression = false;
}

Database::~Database() {
  disconnect();		// Disconnect if connected to database
}

int Database::connectFull(const char *newHost, const char *newPort, const char *newDb, const char *newLogin,
                          const char *newPasswd, const char *newKey, const char *newCert, const char *newCA,
                          const char *newCApath, const char *newCiphers, bool newCompression) {
  host = newHost;
  port = newPort;
  db = newDb;
  login = newLogin;
  passwd = newPasswd;
  key = newKey;
  cert = newCert;
  ca = newCA;
  capath = newCApath;
  ciphers = newCiphers;
  compression = newCompression;
  return connect(true);
}

std::string Database::prepare(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  std::string result = vprepare(format, args);
  va_end(args);

  return result;
}

//************* Dataset implementation ***************

Dataset::Dataset():
  select_sql("")
{

  db = NULL;
  haveError = active = false;
  frecno = 0;
  fbof = feof = true;
  autocommit = true;
  fieldIndexMapID = ~0;

  fields_object = new Fields();

  edit_object = new Fields();
}



Dataset::Dataset(Database *newDb):
  select_sql("")
{

  db = newDb;
  haveError = active = false;
  frecno = 0;
  fbof = feof = true;
  autocommit = true;
  fieldIndexMapID = ~0;

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
#ifndef TARGET_POSIX
  _vsnprintf(sqlCmd, DB_BUFF_MAX-1, sqlFrmt, ap);
#else
  vsnprintf(sqlCmd, DB_BUFF_MAX-1, sqlFrmt, ap);
#endif
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

void Dataset::set_select_sql(const std::string &sel_sql) {
 select_sql = sel_sql;
}


void Dataset::parse_sql(std::string &sql) {
  std::string fpattern,by_what;
  for (unsigned int i=0;i< fields_object->size();i++) {
    fpattern = ":OLD_"+(*fields_object)[i].props.name;
    by_what = "'"+(*fields_object)[i].val.get_asString()+"'";
		int idx=0; int next_idx=0;
		while ((idx = sql.find(fpattern,next_idx))>=0) {
		       	   next_idx=idx+fpattern.size();
			       if (sql.length() > ((unsigned int)next_idx))
			       if(isalnum(sql[next_idx])  || sql[next_idx]=='_') {
			       	   continue;
			       	}
			      sql.replace(idx,fpattern.size(),by_what);
		}//while
    }//for

  for (unsigned int i=0;i< edit_object->size();i++) {
    fpattern = ":NEW_"+(*edit_object)[i].props.name;
    by_what = "'"+(*edit_object)[i].val.get_asString()+"'";
		int idx=0; int next_idx=0;
		while ((idx = sql.find(fpattern,next_idx))>=0) {
		       	   next_idx=idx+fpattern.size();
			       if (sql.length() > ((unsigned int)next_idx))
			       if(isalnum(sql[next_idx]) || sql[next_idx]=='_') {
			       	   continue;
			       	}
			      sql.replace(idx,fpattern.size(),by_what);
			}//while
  } //for
}


void Dataset::close(void) {
  haveError  = false;
  frecno = 0;
  fbof = feof = true;
  active = false;

  fieldIndexMap_Entries.clear();
  fieldIndexMap_Sorter.clear();
  fieldIndexMapID = ~0;
}


bool Dataset::seek(int pos) {
  frecno = (pos<num_rows()-1)? pos: num_rows()-1;
  frecno = (frecno<0)? 0: frecno;
  fbof = feof = (num_rows()==0)? true: false;
  return ((bool)frecno);
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
   edit_object->resize(field_count());
   for (int i=0; i<field_count(); i++) {
     (*fields_object)[i].val = "";
     (*edit_object)[i].val = "";
     (*edit_object)[i].props = (*fields_object)[i].props;
   }
  ds_state = dsInsert;
}


void Dataset::edit() {
  if (ds_state != dsSelect) {
    throw DbErrors("Editing is possible only when query exists!");
  }
  edit_object->resize(field_count());
  for (unsigned int i=0; i<fields_object->size(); i++) {
       (*edit_object)[i].props = (*fields_object)[i].props;
       (*edit_object)[i].val = (*fields_object)[i].val;
  }
  ds_state = dsEdit;
}


void Dataset::post() {
  if (ds_state == dsInsert) make_insert();
  else if (ds_state == dsEdit) make_edit();
}


void Dataset::del()
{
  ds_state = dsDelete;
}


void Dataset::deletion()
{
  if (ds_state == dsDelete)
    make_deletion();
}


bool Dataset::set_field_value(const char *f_name, const field_value &value) {
  bool found = false;
  if ((ds_state == dsInsert) || (ds_state == dsEdit)) {
      for (unsigned int i=0; i < fields_object->size(); i++)
	if (str_compare((*edit_object)[i].props.name.c_str(), f_name)==0) {
			     (*edit_object)[i].val = value;
			     found = true;
	}
      if (!found) throw DbErrors("Field not found: %s",f_name);
    return true;
  }
  throw DbErrors("Not in Insert or Edit state");
  //  return false;
}

/********* INDEXMAP SECTION START *********/
bool Dataset::get_index_map_entry(const char *f_name) {
  if (~fieldIndexMapID)
  {
    unsigned int next(fieldIndexMapID+1 >= fieldIndexMap_Entries.size() ? 0 : fieldIndexMapID + 1);
    if (fieldIndexMap_Entries[next].strName == f_name) //Yes, our assumption hits.
    {
      fieldIndexMapID = next;
      return true;
    }
  }
  // indexMap not found on the expected way, either first row strange retrieval order
  FieldIndexMapEntry tmp(f_name);
  std::vector<unsigned int>::iterator ins(lower_bound(fieldIndexMap_Sorter.begin(), fieldIndexMap_Sorter.end(), tmp, FieldIndexMapComparator(fieldIndexMap_Entries)));
  if (ins == fieldIndexMap_Sorter.end() || (tmp <  fieldIndexMap_Entries[*ins])) //new entry
  {
    //Insert the new item just behind last retrieved item
    //In general this should be always end(), but could be different
    fieldIndexMap_Sorter.insert(ins, ++fieldIndexMapID);
    fieldIndexMap_Entries.insert(fieldIndexMap_Entries.begin() + fieldIndexMapID, tmp);
  }
  else //entry already existing!
  {
    fieldIndexMapID = *ins;
    return true;
  }
  return false; //invalid
}
/********* INDEXMAP SECTION END *********/

const field_value Dataset::get_field_value(const char *f_name) {
  if (ds_state != dsInactive)
  {
    if (ds_state == dsEdit || ds_state == dsInsert){
      for (unsigned int i=0; i < edit_object->size(); i++)
        if (str_compare((*edit_object)[i].props.name.c_str(), f_name)==0) {
          return (*edit_object)[i].val;
        }
      throw DbErrors("Field not found: %s",f_name);
    }
    else
    {
      //Lets try to reuse a string ->index conversation
      if (get_index_map_entry(f_name))
        return get_field_value(static_cast<int>(fieldIndexMap_Entries[fieldIndexMapID].fieldIndex));

      const char* name=strstr(f_name, ".");
      if (name)
        name++;

      for (unsigned int i=0; i < fields_object->size(); i++)
        if (str_compare((*fields_object)[i].props.name.c_str(), f_name) == 0 || (name && str_compare((*fields_object)[i].props.name.c_str(), name) == 0)) {
          fieldIndexMap_Entries[fieldIndexMapID].fieldIndex = i;
          return (*fields_object)[i].val;
        }
    }
    throw DbErrors("Field not found: %s",f_name);
  }
  throw DbErrors("Dataset state is Inactive");
  //field_value fv;
  //return fv;
}

const field_value Dataset::get_field_value(int index) {
  if (ds_state != dsInactive) {
    if (ds_state == dsEdit || ds_state == dsInsert){
      if (index < 0 || index >= field_count())
        throw DbErrors("Field index not found: %d",index);

      return (*edit_object)[index].val;
    }
    else
    {
      if (index < 0 || index >= field_count())
        throw DbErrors("Field index not found: %d",index);

      return (*fields_object)[index].val;
    }
  }
  throw DbErrors("Dataset state is Inactive");
}

const sql_record* Dataset::get_sql_record()
{
  if (result.records.empty() || frecno >= (int)result.records.size())
    return NULL;

  return result.records[frecno];
}

const field_value Dataset::f_old(const char *f_name) {
  if (ds_state != dsInactive)
    for (int unsigned i=0; i < fields_object->size(); i++)
      if ((*fields_object)[i].props.name == f_name)
	return (*fields_object)[i].val;
  field_value fv;
  return fv;
}

int Dataset::str_compare(const char * s1, const char * s2) {
 	std::string ts1 = s1;
 	std::string ts2 = s2;
 	std::string::const_iterator p = ts1.begin();
 	std::string::const_iterator p2 = ts2.begin();
 	while (p!=ts1.end() && p2 != ts2.end()) {
 		if (toupper(*p)!=toupper(*p2))
 			return (toupper(*p)<toupper(*p2)) ? -1 : 1;
 		++p;
 		++p2;
 	}
 	return (ts2.size() == ts1.size())? 0:
 		(ts1.size()<ts2.size())? -1 : 1;
 }


void Dataset::setParamList(const ParamList &params){
  plist = params;
}


bool Dataset::locate(){
  bool result;
  if (plist.empty()) return false;

  std::map<std::string, field_value>::const_iterator i;
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

  std::map<std::string, field_value>::const_iterator i;
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
  std::string s = upd_sql;
  update_sql.push_back(s);
}


void Dataset::add_update_sql(const std::string &upd_sql){
  update_sql.push_back(upd_sql);
}

void Dataset::add_insert_sql(const char *ins_sql){
  std::string s = ins_sql;
  insert_sql.push_back(s);
}


void Dataset::add_insert_sql(const std::string &ins_sql){
  insert_sql.push_back(ins_sql);
}

void Dataset::add_delete_sql(const char *del_sql){
  std::string s = del_sql;
  delete_sql.push_back(s);
}


void Dataset::add_delete_sql(const std::string &del_sql){
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

size_t Dataset::insert_sql_count()
{
  return insert_sql.size();
}

size_t Dataset::delete_sql_count()
{
  return delete_sql.size();
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
for (unsigned int i=0; i < fields_object->size(); i++)
      if ((*fields_object)[i].props.name == fn)
	return i;
  return -1;
}



//************* DbErrors implementation ***************

DbErrors::DbErrors():
  msg_("Unknown Database Error")
{
}


DbErrors::DbErrors(const char *msg, ...) {
  va_list vl;
  va_start(vl, msg);
  char buf[DB_BUFF_MAX]="";
#ifndef TARGET_POSIX
  _vsnprintf(buf, DB_BUFF_MAX-1, msg, vl);
#else
  vsnprintf(buf, DB_BUFF_MAX-1, msg, vl);
#endif
  va_end(vl);
  msg_ =   "SQL: ";
  msg_ += buf;

  CLog::Log(LOGERROR, "%s", msg_.c_str());
}

const char * DbErrors::getMsg() {
	return msg_.c_str();

}

}// namespace
