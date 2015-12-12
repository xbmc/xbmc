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

class CJNIURI;
class CJNIIntent : public CJNIBase
{
public:
  CJNIIntent(const std::string &action = "");
  CJNIIntent(const jni::jhobject &intent) : CJNIBase(intent) {};
  ~CJNIIntent() {};

  std::string getAction() const;
  std::string getDataString() const ;
  std::string getPackage() const;
  std::string getType() const ;

  int getIntExtra(const std::string &name, int defaultValue) const;
  std::string getStringExtra(const std::string &name) const;
  jni::jhobject getParcelableExtra(const std::string &name) const;

  bool hasExtra(const std::string &name) const;
  bool hasCategory(const std::string &category) const;

  void addFlags(int flags);
  void addCategory(const std::string &category);
  void setFlags(int flags);
  void setAction(const std::string &action);
  void setClassName(const std::string &packageName, const std::string &className);

  // Note that these are strings. We auto-convert to uri objects.
  void setDataAndType(const CJNIURI &uri, const std::string &type);
  void setData(const std::string &uri);

  void setPackage(const std::string &packageName);
  void setType(const std::string &type);
  CJNIURI getData() const;

  static void PopulateStaticFields();
  static std::string EXTRA_KEY_EVENT;
};
