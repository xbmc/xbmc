#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include "JNIBase.h"
#include "MediaStore.h"

class CJNIURI;

class CJNICursor : public CJNIBase
{
public:
  CJNICursor(const jni::jhobject &object) : CJNIBase(object){};
  ~CJNICursor(){};

  int  getCount();
  int  getPosition();
  bool move(int offset);
  bool moveToPosition(int position);
  bool moveToFirst();
  bool moveToLast();
  bool moveToNext();
  bool moveToPrevious();
  bool isFirst();
  bool isLast();
  bool isBeforeFirst();
  bool isAfterLast();
  int  getColumnIndex(const std::string &columnName);
  std::string getColumnName(int columnIndex);
  std::vector<std::string> getColumnNames();
  int  getColumnCount();
  std::string getString(int columnIndex);
  short getShort(int columnIndex);
  int  getInt(int columnIndex);
  int64_t getLong(int columnIndex);
  float getFloat(int columnIndex);
  double getDouble(int columnIndex);
  int  getType(int columnIndex);
  bool isNull(int columnIndex);
  void deactivate();
  bool requery();
  void close();
  bool isClosed();

  static void PopulateStaticFields();
  static int FIELD_TYPE_NULL;
  static int FIELD_TYPE_INTEGER;
  static int FIELD_TYPE_FLOAT;
  static int FIELD_TYPE_STRING;
  static int FIELD_TYPE_BLOB;


private:
  CJNICursor();
};
