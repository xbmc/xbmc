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

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>

#ifndef __GNUC__
#pragma warning(disable : 4800)
#endif

namespace dbiplus
{
//************* Database implementation ***************

Database::Database()
  : error(), //S_NO_CONNECTION,
    host(),
    port(),
    db(),
    login(),
    passwd(),
    sequence_table("db_sequence")
{
  active = false; // No connection yet
  compression = false;
}

Database::~Database()
{
  disconnect(); // Disconnect if connected to database
}

int Database::connectFull(const char* newHost,
                          const char* newPort,
                          const char* newDb,
                          const char* newLogin,
                          const char* newPasswd,
                          const char* newKey,
                          const char* newCert,
                          const char* newCA,
                          const char* newCApath,
                          const char* newCiphers,
                          bool newCompression)
{
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

std::string Database::prepare(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  std::string result = vprepare(format, args);
  va_end(args);

  return result;
}

//************* Dataset implementation ***************

Dataset::Dataset() : select_sql("")
{

  db = NULL;
  haveError = active = false;
  frecno = 0;
  fbof = feof = true;
  autocommit = true;

  fields_object = new Fields();

  edit_object = new Fields();
}

Dataset::Dataset(Database* newDb) : select_sql("")
{

  db = newDb;
  haveError = active = false;
  frecno = 0;
  fbof = feof = true;
  autocommit = true;

  fields_object = new Fields();

  edit_object = new Fields();
}

Dataset::~Dataset()
{
  update_sql.clear();
  insert_sql.clear();
  delete_sql.clear();

  delete fields_object;
  delete edit_object;
}

void Dataset::setSqlParams(sqlType t, const char* sqlFrmt, ...)
{
  va_list ap;
  char sqlCmd[DB_BUFF_MAX + 1];

  va_start(ap, sqlFrmt);
#ifndef TARGET_POSIX
  _vsnprintf(sqlCmd, DB_BUFF_MAX - 1, sqlFrmt, ap);
#else
  vsnprintf(sqlCmd, DB_BUFF_MAX - 1, sqlFrmt, ap);
#endif
  va_end(ap);

  switch (t)
  {
    case sqlSelect:
      set_select_sql(sqlCmd);
      break;
    case sqlUpdate:
      add_update_sql(sqlCmd);
      break;
    case sqlInsert:
      add_insert_sql(sqlCmd);
      break;
    case sqlDelete:
      add_delete_sql(sqlCmd);
      break;
    case sqlExec:
      sql = sqlCmd;
      break;
  }
}

void Dataset::set_select_sql(const char* sel_sql)
{
  select_sql = sel_sql;
}

void Dataset::set_select_sql(const std::string& sel_sql)
{
  select_sql = sel_sql;
}

void Dataset::parse_sql(std::string& sql)
{
  std::string fpattern, by_what;
  for (unsigned int i = 0; i < fields_object->size(); i++)
  {
    fpattern = ":OLD_" + (*fields_object)[i].props.name;
    by_what = "'" + (*fields_object)[i].val.get_asString() + "'";
    int idx = 0;
    int next_idx = 0;
    while ((idx = sql.find(fpattern, next_idx)) >= 0)
    {
      next_idx = idx + fpattern.size();
      if (sql.length() > ((unsigned int)next_idx))
        if (isalnum(sql[next_idx]) || sql[next_idx] == '_')
        {
          continue;
        }
      sql.replace(idx, fpattern.size(), by_what);
    } //while
  } //for

  for (unsigned int i = 0; i < edit_object->size(); i++)
  {
    fpattern = ":NEW_" + (*edit_object)[i].props.name;
    by_what = "'" + (*edit_object)[i].val.get_asString() + "'";
    int idx = 0;
    int next_idx = 0;
    while ((idx = sql.find(fpattern, next_idx)) >= 0)
    {
      next_idx = idx + fpattern.size();
      if (sql.length() > ((unsigned int)next_idx))
        if (isalnum(sql[next_idx]) || sql[next_idx] == '_')
        {
          continue;
        }
      sql.replace(idx, fpattern.size(), by_what);
    } //while
  } //for
}

void Dataset::close(void)
{
  haveError = false;
  frecno = 0;
  fbof = feof = true;
  active = false;

  name2indexMap.clear();
}

bool Dataset::seek(int pos)
{
  frecno = (pos < num_rows() - 1) ? pos : num_rows() - 1;
  frecno = (frecno < 0) ? 0 : frecno;
  fbof = feof = (num_rows() == 0) ? true : false;
  return ((bool)frecno);
}

void Dataset::refresh()
{
  int row = frecno;
  if ((row != 0) && active)
  {
    close();
    open();
    seek(row);
  }
  else
    open();
}

void Dataset::first()
{
  if (ds_state == dsSelect)
  {
    frecno = 0;
    feof = fbof = (num_rows() > 0) ? false : true;
  }
}

void Dataset::next()
{
  if (ds_state == dsSelect)
  {
    fbof = false;
    if (frecno < num_rows() - 1)
    {
      frecno++;
      feof = false;
    }
    else
      feof = true;
    if (num_rows() <= 0)
      fbof = feof = true;
  }
}

void Dataset::prev()
{
  if (ds_state == dsSelect)
  {
    feof = false;
    if (frecno)
    {
      frecno--;
      fbof = false;
    }
    else
      fbof = true;
    if (num_rows() <= 0)
      fbof = feof = true;
  }
}

void Dataset::last()
{
  if (ds_state == dsSelect)
  {
    frecno = (num_rows() > 0) ? num_rows() - 1 : 0;
    feof = fbof = (num_rows() > 0) ? false : true;
  }
}

bool Dataset::goto_rec(int pos)
{
  if (ds_state == dsSelect)
  {
    return seek(pos - 1);
  }
  return false;
}

void Dataset::insert()
{
  edit_object->resize(field_count());
  for (int i = 0; i < field_count(); i++)
  {
    (*fields_object)[i].val = "";
    (*edit_object)[i].val = "";
    (*edit_object)[i].props = (*fields_object)[i].props;
  }
  ds_state = dsInsert;
}

void Dataset::edit()
{
  if (ds_state != dsSelect)
  {
    throw DbErrors("Editing is possible only when query exists!");
  }
  edit_object->resize(field_count());
  for (unsigned int i = 0; i < fields_object->size(); i++)
  {
    (*edit_object)[i].props = (*fields_object)[i].props;
    (*edit_object)[i].val = (*fields_object)[i].val;
  }
  ds_state = dsEdit;
}

void Dataset::post()
{
  if (ds_state == dsInsert)
    make_insert();
  else if (ds_state == dsEdit)
    make_edit();
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

bool Dataset::set_field_value(const char* f_name, const field_value& value)
{
  if ((ds_state == dsInsert) || (ds_state == dsEdit))
  {
    const int idx = fieldIndex(f_name);
    if (idx >= 0)
    {
      (*edit_object)[idx].val = value;
      return true;
    }
    throw DbErrors("Field not found: %s", f_name);
  }
  throw DbErrors("Not in Insert or Edit state");
  //  return false;
}

const field_value& Dataset::get_field_value(const char* f_name)
{
  if (ds_state != dsInactive)
  {
    if (ds_state == dsEdit || ds_state == dsInsert)
    {
      const int idx = fieldIndex(f_name);
      if (idx >= 0)
        return (*edit_object)[idx].val;

      throw DbErrors("Field not found: %s", f_name);
    }
    else
    {
      int idx = fieldIndex(f_name);
      if (idx < 0)
      {
        const char* name = strstr(f_name, ".");
        if (name)
          name++;

        if (name)
          idx = fieldIndex(name);
      }

      if (idx >= 0)
        return (*fields_object)[idx].val;

      throw DbErrors("Field not found: %s", f_name);
    }
  }
  throw DbErrors("Dataset state is Inactive");
}

const field_value& Dataset::get_field_value(int index)
{
  if (ds_state != dsInactive)
  {
    if (ds_state == dsEdit || ds_state == dsInsert)
    {
      if (index < 0 || index >= field_count())
        throw DbErrors("Field index not found: %d", index);

      return (*edit_object)[index].val;
    }
    else
    {
      if (index < 0 || index >= field_count())
        throw DbErrors("Field index not found: %d", index);

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

field_value Dataset::f_old(const char* f_name)
{
  if (ds_state != dsInactive)
    for (int unsigned i = 0; i < fields_object->size(); i++)
      if ((*fields_object)[i].props.name == f_name)
        return (*fields_object)[i].val;
  return {};
}

void Dataset::setParamList(const ParamList& params)
{
  plist = params;
}

bool Dataset::locate()
{
  bool result;
  if (plist.empty())
    return false;

  std::map<std::string, field_value>::const_iterator i;
  first();
  while (!eof())
  {
    result = true;
    for (i = plist.begin(); i != plist.end(); ++i)
      if (fv(i->first.c_str()).get_asString() == i->second.get_asString())
      {
        continue;
      }
      else
      {
        result = false;
        break;
      }
    if (result)
    {
      return result;
    }
    next();
  }
  return false;
}

bool Dataset::locate(const ParamList& params)
{
  plist = params;
  return locate();
}

bool Dataset::findNext(void)
{
  bool result;
  if (plist.empty())
    return false;

  std::map<std::string, field_value>::const_iterator i;
  while (!eof())
  {
    result = true;
    for (i = plist.begin(); i != plist.end(); ++i)
      if (fv(i->first.c_str()).get_asString() == i->second.get_asString())
      {
        continue;
      }
      else
      {
        result = false;
        break;
      }
    if (result)
    {
      return result;
    }
    next();
  }
  return false;
}

void Dataset::add_update_sql(const char* upd_sql)
{
  std::string s = upd_sql;
  update_sql.push_back(s);
}

void Dataset::add_update_sql(const std::string& upd_sql)
{
  update_sql.push_back(upd_sql);
}

void Dataset::add_insert_sql(const char* ins_sql)
{
  std::string s = ins_sql;
  insert_sql.push_back(s);
}

void Dataset::add_insert_sql(const std::string& ins_sql)
{
  insert_sql.push_back(ins_sql);
}

void Dataset::add_delete_sql(const char* del_sql)
{
  std::string s = del_sql;
  delete_sql.push_back(s);
}

void Dataset::add_delete_sql(const std::string& del_sql)
{
  delete_sql.push_back(del_sql);
}

void Dataset::clear_update_sql()
{
  update_sql.clear();
}

void Dataset::clear_insert_sql()
{
  insert_sql.clear();
}

void Dataset::clear_delete_sql()
{
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

int Dataset::field_count()
{
  return fields_object->size();
}
int Dataset::fieldCount()
{
  return fields_object->size();
}

const char* Dataset::fieldName(int n)
{
  if (n < field_count() && n >= 0)
    return (*fields_object)[n].props.name.c_str();
  else
    return NULL;
}

char* Dataset::str_toLower(char* s)
{
  for (char* p = s; *p; p++)
    *p = std::tolower(*p);

  return s;
}

int Dataset::fieldIndex(const char* fn)
{
  std::string name(fn);
  const auto it = name2indexMap.find(str_toLower(name.data()));
  if (it != name2indexMap.end())
    return (*it).second;
  else
    return -1;
}

//************* DbErrors implementation ***************

DbErrors::DbErrors() : msg_("Unknown Database Error")
{
}

DbErrors::DbErrors(const char* msg, ...)
{
  va_list vl;
  va_start(vl, msg);
  char buf[DB_BUFF_MAX] = "";
#ifndef TARGET_POSIX
  _vsnprintf(buf, DB_BUFF_MAX - 1, msg, vl);
#else
  vsnprintf(buf, DB_BUFF_MAX - 1, msg, vl);
#endif
  va_end(vl);
  msg_ = "SQL: ";
  msg_ += buf;

  CLog::Log(LOGERROR, "{}", msg_);
}

const char* DbErrors::getMsg()
{
  return msg_.c_str();
}

} // namespace dbiplus
