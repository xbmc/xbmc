/*
 *      Copyright (C) 2012-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <sys/stat.h>
#include <stdlib.h>
#include <string>
#include <elf.h>
#include <errno.h>
#include <vector>
#include <dlfcn.h>
#include <fcntl.h>
#include "platform/android/activity/XBMCApp.h"
#include "AndroidDyload.h"
#include "utils/StringUtils.h"
#include "CompileInfo.h"

//#define DEBUG_SPEW

#ifdef __LP64__
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Dyn Elf64_Dyn
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Dyn Elf32_Dyn
#endif

std::list<recursivelib> CAndroidDyload::m_recursivelibs;
solib CAndroidDyload::m_libs;

bool CAndroidDyload::IsSystemLib(const std::string &filename)
{
  {
    CSingleLock lock(m_libLock);
    for ( solibit i = m_libs.begin() ; i != m_libs.end(); ++i )
    {
      if (i->first == filename)
        return i->second.system;
    }
  }

  std::string result = FindLib(filename, false);
  if (!result.empty())
    return false;
  result = FindLib(filename, true);
  return result.size() > 0;
}

std::string CAndroidDyload::FindLib(const std::string &filename, bool checkSystem)
{
  struct stat st;
  std::string path;
  strings searchpaths;
  std::string systemLibs = (getenv("XBMC_ANDROID_SYSTEM_LIBS"));
  std::string localLibs = getenv("XBMC_ANDROID_LIBS");
  std::string dirname = filename.substr(0,filename.find_last_of('/'));

  while (true)
  {
    size_t pos = systemLibs.find(":");
    searchpaths.push_back(systemLibs.substr(0, pos));

    if (pos != std::string::npos)
      systemLibs.erase(0, pos + 1);
    else
      break;
  }

  // Check xbmc package libs
  path = (localLibs+"/"+filename.substr(filename.find_last_of('/') +1));
  if (stat((path).c_str(), &st) == 0)
    return(path);

  // Check system libs. If we're not looking in system libs, bail.
  // Note that we also bail if the explicit path happens to itself be a system
  // lib and checkSystem == false.
  for (strings::iterator j = searchpaths.begin(); j != searchpaths.end(); ++j)
  {
    path = (*j+"/"+filename.substr(filename.find_last_of('/') +1));
    if (stat((path).c_str(), &st) == 0)
    {
      if (checkSystem)
        return(path);
      else
      {
        if (dirname == *j)
          return "";
      }
    }
  }

  // Nothing found yet, try the full given path.
  if (stat((filename).c_str(), &st) == 0)
    return(filename);

  return "";
}

void* CAndroidDyload::Find(const std::string &filename)
{
  CSingleLock lock(m_libLock);
  solibit i = m_libs.find(filename);
  return i == m_libs.end() ? NULL : i->second.handle;
}

std::string CAndroidDyload::Find(void *handle)
{
  CSingleLock lock(m_libLock);
  for ( solibit i = m_libs.begin() ; i != m_libs.end(); ++i )
  {
    if (i->second.handle == handle)
      return i->first;
  }
  return "";
}

void *CAndroidDyload::FindInDeps(const std::string &filename)
{
  CSingleLock lock(m_depsLock);
  for (std::list<recursivelibdep>::iterator k = m_lib.deps.begin(); k != m_lib.deps.end(); ++k)
  {
    if (k->filename == filename)
      return k->handle;
  }
  return NULL;
}

int CAndroidDyload::AddRef(const std::string &filename)
{
  CSingleLock lock(m_libLock);
  if (m_libs.find(filename) == m_libs.end())
    return -1;
  return (++(m_libs[filename].refcount));
}

int CAndroidDyload::DecRef(const std::string &filename)
{
  CSingleLock lock(m_libLock);
  if (m_libs.find(filename) == m_libs.end())
    return -1;
  return (--(m_libs[filename].refcount));
}

void CAndroidDyload::GetDeps(std::string filename, strings *results)
{
  Elf_Ehdr header;
  char *data = NULL;
  int fd, i;

  fd = open(filename.c_str(), O_RDONLY);
  if(fd < 0)
  {
    CXBMCApp::android_printf("Cannot open %s: %s\n", filename.c_str(), strerror(errno));
    return;
  }

  if(read(fd, &header, sizeof(header)) < 0)
  {
    CXBMCApp::android_printf("Cannot read elf header: %s\n", strerror(errno));
    close(fd);
    return;
  }

  lseek(fd, header.e_shoff, SEEK_SET);

  for(i = 0; i < header.e_shnum; i++)
  {
    Elf_Shdr sheader;

    lseek(fd, header.e_shoff + (i * header.e_shentsize), SEEK_SET);
    read(fd, &sheader, sizeof(sheader));

    if(sheader.sh_type == SHT_DYNSYM)
    {
      Elf_Shdr symheader;
      lseek(fd, header.e_shoff + (sheader.sh_link * header.e_shentsize), SEEK_SET);
      read(fd, &symheader, sizeof(Elf_Shdr));
      lseek(fd, symheader.sh_offset, SEEK_SET);
      data = (char*)malloc(symheader.sh_size);
      read(fd, data, symheader.sh_size);
      break;
    }
  }

  if(!data)
  { 
    close(fd);
    return;
  }

  for(i = 0; i < header.e_shnum; i++)
  {
    Elf_Shdr sheader;

    lseek(fd, header.e_shoff + (i * header.e_shentsize), SEEK_SET);
    read(fd, &sheader, sizeof(Elf_Shdr));

    if (sheader.sh_type == SHT_DYNAMIC)
    {
      unsigned int j;

      lseek(fd, sheader.sh_offset, SEEK_SET);
      for(j = 0; j < sheader.sh_size / sizeof(Elf_Dyn); j++)
      {
        Elf_Dyn cur;
        read(fd, &cur, sizeof(Elf_Dyn));
        if(cur.d_tag == DT_NEEDED)
        {
          char *final = data + cur.d_un.d_val;
          results->push_back(final);
        }
      }
    }
  }
  close(fd);
  return;
}

void* CAndroidDyload::Open(const char * path)
{
  std::string filename = path;
  filename = filename.substr(filename.find_last_of('/') +1);
  void *handle = NULL;
  m_lib.deps.clear();
  handle = Find(filename);
  if (handle)
  {
    AddRef(filename);
    return handle;
  }
  bool checkSystem = IsSystemLib(path);
  handle = Open_Internal(std::string(path), checkSystem);
  if (handle != NULL)
  {
    CSingleLock lock(m_depsLock);
    m_lib.handle = handle;
    m_lib.filename = filename;
    m_recursivelibs.push_back(m_lib);
#if defined(DEBUG_SPEW)
    CXBMCApp::android_printf("xb_dlopen: opening lib: %s", filename.c_str());
    Dump();
#endif
  }
  return handle;
}

void* CAndroidDyload::Open_Internal(std::string filename, bool checkSystem)
{
  strings deps;
  std::string deppath;
  libdata lib;
  void *handle = NULL;

  std::string path = FindLib(filename, checkSystem);
  if (!path.size())
    return NULL;

  GetDeps(path, &deps);

  for (strings::iterator j = deps.begin(); j != deps.end(); ++j)
  {
    std::string appName = CCompileInfo::GetAppName();
    std::string libName = "lib" + appName + ".so";
    StringUtils::ToLower(libName);
    // Don't traverse into libkodi's deps, they're guaranteed to be loaded.
    if (*j == libName.c_str())
      continue;

    // Don't dlopen system libs
    if (IsSystemLib(*j))
      continue;

    if (FindInDeps(*j))
      continue;

    handle = Find(*j);
    if (handle)
    {
      recursivelibdep dep;
      dep.handle = handle;
      dep.filename = *j;
      m_lib.deps.push_back(dep);
      AddRef(*j);
      continue;
    }

    Open_Internal(*j, checkSystem);
  }

  handle = dlopen(path.c_str(), RTLD_LOCAL);

  recursivelibdep dep;
  dep.handle = handle;
  dep.filename = filename.substr(filename.find_last_of('/') +1);
  m_lib.deps.push_back(dep);

  lib.refcount = 1;
  lib.handle = handle;
  lib.system = checkSystem;

  CSingleLock lock(m_libLock);
  m_libs[filename] = lib;

  return handle;
}

int CAndroidDyload::Close(void *handle)
{
  CSingleLock lock(m_depsLock);
  for (std::list<recursivelib>::iterator i = m_recursivelibs.begin(); i != m_recursivelibs.end(); ++i)
  {
    if (i->handle == handle) 
    {
      for (std::list<recursivelibdep>::iterator j = i->deps.begin(); j != i->deps.end(); ++j)
      {
        if (DecRef(j->filename) == 0)
        {
          if (dlclose(j->handle))
            CXBMCApp::android_printf("xb_dlopen: Error from dlopen(%s): %s", j->filename.c_str(), dlerror());

          CSingleLock lock(m_libLock);
          m_libs.erase(j->filename);
        }
      }
      m_recursivelibs.erase(i);
#if defined(DEBUG_SPEW)
      Dump();
#endif
      return 0;
    }
  }
  return 1;
}

void CAndroidDyload::Dump()
{
  CSingleLock liblock(m_libLock);
  for ( solibit i = m_libs.begin() ; i != m_libs.end(); ++i )
  {
    CXBMCApp::android_printf("lib: %s. refcount: %i",i->first.c_str(), i->second.refcount);
  }

  CSingleLock depslock(m_depsLock);
  for (std::list<recursivelib>::iterator i = m_recursivelibs.begin(); i != m_recursivelibs.end(); ++i)
  {
    CXBMCApp::android_printf("xb_dlopen: recursive dep: %s", i->filename.c_str());
    for (std::list<recursivelibdep>::iterator j = i->deps.begin(); j != i->deps.end(); ++j)
    {
      CXBMCApp::android_printf("xb_dlopen: recursive dep: \\-- %s", j->filename.c_str());
    }
  }
}
