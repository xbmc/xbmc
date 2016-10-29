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

#include "Cursor.h"
#include "jutils/jutils-details.hpp"
#include "URI.h"

using namespace jni;

int CJNICursor::FIELD_TYPE_NULL(0);
int CJNICursor::FIELD_TYPE_INTEGER(0);
int CJNICursor::FIELD_TYPE_FLOAT(0);
int CJNICursor::FIELD_TYPE_STRING(0);
int CJNICursor::FIELD_TYPE_BLOB(0);

void CJNICursor::PopulateStaticFields()
{
  jhclass clazz = find_class("android/database/Cursor");
  FIELD_TYPE_NULL     = (get_static_field<int>(clazz, "FIELD_TYPE_NULL"));
  FIELD_TYPE_INTEGER  = (get_static_field<int>(clazz, "FIELD_TYPE_INTEGER"));
  FIELD_TYPE_FLOAT    = (get_static_field<int>(clazz, "FIELD_TYPE_FLOAT"));
  FIELD_TYPE_STRING   = (get_static_field<int>(clazz, "FIELD_TYPE_STRING"));
  FIELD_TYPE_BLOB     = (get_static_field<int>(clazz, "FIELD_TYPE_BLOB"));
}

int CJNICursor::getCount()
{
  return call_method<jint>(m_object,
    "getCount", "()I");
}

int CJNICursor::getPosition()
{
  return call_method<jint>(m_object,
    "getPosition", "()I");
}

bool CJNICursor::move(int offset)
{
  return call_method<jboolean>(m_object,
    "move", "(I)Z",
    offset);
}

bool CJNICursor::moveToPosition(int position)
{
  return call_method<jboolean>(m_object,
    "moveToPosition", "(I)Z", position);
}

bool CJNICursor::moveToFirst()
{
  return call_method<jboolean>(m_object,
    "moveToFirst", "()Z");
}

bool CJNICursor::moveToLast()
{
  return call_method<jboolean>(m_object,
    "moveToLast", "()Z");
}

bool CJNICursor::moveToNext()
{
  return call_method<jboolean>(m_object,
    "moveToNext", "()Z");
}

bool CJNICursor::moveToPrevious()
{
  return call_method<jboolean>(m_object,
    "moveToPrevious", "()Z");
}

bool CJNICursor::isFirst()
{
  return call_method<jboolean>(m_object,
    "isFirst", "()Z");
}

bool CJNICursor::isLast()
{
  return call_method<jboolean>(m_object,
    "isLast", "()Z");
}

bool CJNICursor::isBeforeFirst()
{
  return call_method<jboolean>(m_object,
    "isBeforeFirst", "()Z");
}

bool CJNICursor::isAfterLast()
{
  return call_method<jboolean>(m_object,
    "isAfterLast", "()Z");
}

int CJNICursor::getColumnIndex(const std::string &columnName)
{
  return call_method<jint>(m_object,
    "getColumnIndex", "(Ljava/lang/String;)I",
    jcast<jhstring>(columnName));
}

std::string CJNICursor::getColumnName(int columnIndex)
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getColumnName", "(I)Ljava/lang/String;",
    columnIndex));
}

std::vector<std::string> CJNICursor::getColumnNames()
{
  return jcast<std::vector<std::string>>(call_method<jhobjectArray>(m_object,
    "getColumnNames", "()[Ljava/lang/String;"));
}

int CJNICursor::getColumnCount()
{
  return call_method<jint>(m_object,
    "getColumnCount", "()I");
}

std::string CJNICursor::getString(int columnIndex)
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getString", "(I)Ljava/lang/String;",
    columnIndex));
}

short CJNICursor::getShort(int columnIndex)
{
  return call_method<jshort>(m_object,
    "getShort", "(I)S",
    columnIndex);
}

int CJNICursor::getInt(int columnIndex)
{
  return call_method<jint>(m_object,
    "getInt", "(I)I",
    columnIndex);
}

int64_t CJNICursor::getLong(int columnIndex)
{
  return call_method<jlong>(m_object,
    "getLong", "(I)J",
    columnIndex);
}

float CJNICursor::getFloat(int columnIndex)
{
  return call_method<jfloat>(m_object,
    "getFloat", "(I)F",
    columnIndex);
}

double CJNICursor::getDouble(int columnIndex)
{
  return call_method<jdouble>(m_object,
    "getDouble", "(I)D",
    columnIndex);
}

int CJNICursor::getType(int columnIndex)
{
  return call_method<jint>(m_object,
    "getType", "(I)I",
    columnIndex);
}

bool CJNICursor::isNull(int columnIndex)
{
  return call_method<jboolean>(m_object,
    "isNull", "(I)Z",
    columnIndex);
}

void CJNICursor::deactivate()
{
  call_method<void>(m_object,
    "deactivate", "()V");
}

bool CJNICursor::requery()
{
  return call_method<jboolean>(m_object,
    "requery", "()Z");
}

void CJNICursor::close()
{
  call_method<void>(m_object,
    "close", "()V");
}

bool CJNICursor::isClosed()
{
  return call_method<jboolean>(m_object,
    "isClosed", "()Z");
}

