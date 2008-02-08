#include "stdafx.h"
#include "LibraryLoader.h"
#include <string.h>
#include <stdlib.h>

LibraryLoader::LibraryLoader(const char* libraryFile)
{
  m_sFileName = strdup(libraryFile);

  char* sPath = strrchr(m_sFileName, '\\');
  if (sPath)
  {
    sPath++;
    m_sPath=(char*)malloc(sPath - m_sFileName+1);
    strncpy(m_sPath, m_sFileName, sPath - m_sFileName);
    m_sPath[sPath - m_sFileName] = 0;
  }
  else 
    m_sPath=NULL;

  m_iRefCount = 1;
}

LibraryLoader::~LibraryLoader()
{
  free(m_sFileName);
  if (m_sPath) free(m_sPath);
}

char* LibraryLoader::GetName()
{
  if (m_sFileName)
  {
    char* sName = strrchr(m_sFileName, '\\');
    if (sName) return sName + 1;
    else return m_sFileName;
  }
  return (char*)"";
}

char* LibraryLoader::GetFileName()
{
  if (m_sFileName) return m_sFileName;
  return (char*)"";
}

char* LibraryLoader::GetPath()
{
  if (m_sPath) return m_sPath;
  return (char*)"";
}
  
int LibraryLoader::IncrRef()
{
  m_iRefCount++;
  return m_iRefCount;
}

int LibraryLoader::DecrRef()
{
  m_iRefCount--;
  return m_iRefCount;
}
