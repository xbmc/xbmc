/*
 *      Copyright (C) 2012-2013 Team XBMC
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
#pragma once
#include <stdlib.h>
#include <string>
#include <list>
#include <map>
#include "threads/SingleLock.h"

struct recursivelibdep
{
  void* handle;
  std::string filename;
};

struct recursivelib
{
  void*  handle;
  std::string filename;
  std::list<recursivelibdep> deps;
};

struct libdata
{
  void   *handle;
  int    refcount;
  bool   system;
};

typedef std::map<std::string, libdata> solib;
typedef std::map<std::string, libdata>::iterator solibit;

typedef std::list<std::string> strings;
typedef std::list<void *> handles;

class CAndroidDyload
{
public:
  void* Open(const char *path);
  int Close(void* handle);

private:
  void *Open_Internal(std::string filename, bool checkSystem);
  void* Find(const std::string &filename);
  bool IsSystemLib(const std::string &filename);
  std::string Find(void *handle);
  std::string FindLib(const std::string &filename, bool checkSystem);
  void* FindInDeps(const std::string &filename);
  int AddRef(const std::string &filename);
  int DecRef(const std::string &filename);
  void GetDeps(std::string filename, strings *results);
  void Dump();

  recursivelib m_lib;
  static std::list<recursivelib> m_recursivelibs;
  static solib m_libs;
  CCriticalSection m_libLock;
  CCriticalSection m_depsLock;
};