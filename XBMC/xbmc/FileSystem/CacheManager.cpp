/*
* XBMC
* CacheManager
* Copyright (c) 2008 topfs2
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "stdafx.h"
#include "CacheManager.h"
#include "../utils/log.h"
//#include "../linux/XTimeUtils.h" //This is for timeGetTime() but I suspect it should be included else were to be crossplattform

CCacheManager g_CacheManager;

/**********************************************************
          CFileInf
**********************************************************/
CCache::CCache(CStdString Path, bool IsFolder, __int64 Size)
{
  m_Path = Path;
  m_IsFolder = IsFolder;
  m_Size = Size;
}

bool CCache::IsFolder() const
{
  return m_IsFolder;
}

CStdString CCache::Path() const
{
  return m_Path;
}

const __int64 CCache::Size() const
{
  return m_Size;
}

bool CCache::Equals(const CStdString& comp) const
{
  return comp.Equals(m_Path);
}

/**********************************************************
      CCacheEntry
**********************************************************/
CCacheEntry::CCacheEntry()
{
  CheckIn();
  m_AutoDelete = true;
}

CCacheEntry::CCacheEntry(CStdString ID)
{
  m_ID = ID;
  CheckIn();
  m_AutoDelete = true;
}

CCacheEntry::~CCacheEntry()
{
  Clear();
}

void CCacheEntry::CheckIn()
{
//  printf("%i\n", timeGetTime());
  m_Time = (timeGetTime() + TIMEOUT);
}

unsigned int CCacheEntry::GetTimeOut() const
{
  return m_Time;
}

bool CCacheEntry::Contains(const CStdString& Path) const
{
  for (unsigned int i = 0; i < m_FilesInCache.size(); i++)
  {
    if (m_FilesInCache[i]->Equals(Path))
      return true;
  }
  return false;
}
void CCacheEntry::Add(CCache *Path)
{    
  CheckIn();
  if (!Contains(Path->Path()))
    m_FilesInCache.push_back(Path);
}

void CCacheEntry::Clear()
{
  for (unsigned int i = 0; i < m_FilesInCache.size(); i++)
    delete m_FilesInCache[i];
  m_FilesInCache.clear();
}

void CCacheEntry::Print() const
{
  printf("\t%s %i\n", m_ID.c_str(), m_FilesInCache.size());
  for (unsigned int i = 0; i < m_FilesInCache.size(); i++)
    printf("\t\t%s\n", m_FilesInCache[i]->Path().c_str());
}

const CCache *CCacheEntry::GetEntry(const CStdString& Path)
{
  CheckIn();
  for (unsigned int i = 0; i < m_FilesInCache.size(); i++)
  {
    if (m_FilesInCache[i]->Equals(Path))
      return m_FilesInCache[i];
  }
  return NULL;
}

bool CCacheEntry::AutoDelete() const
{
  return m_AutoDelete;
}

bool CCacheEntry::AutoDelete(bool OK)
{
  return (m_AutoDelete = OK);
}

const std::vector<CCache *> CCacheEntry::List(const CStdString& strPathInCacheEntry)
{
  CheckIn();

  std::vector<CStdString> inTokens;
  
  CUtil::Tokenize(strPathInCacheEntry, inTokens, "/");

  unsigned int size = strPathInCacheEntry.size();

  std::vector<CCache *> vec;
  for (unsigned int i = 0; i < m_FilesInCache.size(); i++)
  {

    std::vector<CStdString> testTokens;
    CUtil::Tokenize(m_FilesInCache[i]->Path(), testTokens, "/");

    bool Approved = (inTokens.size() + 1 == testTokens.size());
//    if (Approved) printf("First PASS Approved\n");
    for (unsigned int j = 0; Approved && j < inTokens.size(); j++)
    {
      if (!testTokens[j].Equals(inTokens[j].c_str()))
        Approved = false;
    }
    if (Approved)
    {

      vec.push_back(m_FilesInCache[i]);
    }
    
  }
  return vec;
}

CStdString CCacheEntry::GetID() const
{
  return m_ID;
}

/**********************************************************
      CCacheManager
**********************************************************/

// Constructor
CCacheManager::CCacheManager()
{

}

// DeConstructor
CCacheManager::~CCacheManager()
{

}


// Public
bool CCacheManager::List(CFileItemList& vecpItems, const CStdString& strCacheEntry, const CStdString& strPathInCacheEntry)
{
  CCacheEntry *ent = NULL;
  int pos = GetCacheEntry(strCacheEntry);
  if (pos == -1)
    return false;


  ent = m_Cached[pos];
  std::vector<CCache *> vec = ent->List(strPathInCacheEntry);
  for (unsigned int i = 0; i < vec.size(); i++)
  {

// Detta måste sparas i FILEINFO
    std::vector<CStdString> tokens; 
    CUtil::Tokenize(vec[i]->Path(), tokens, "/");
/*    */
    CFileItem* pItem = new CFileItem(tokens[(tokens.size() - 1)]);

    CStdString strPath;
    CUtil::CreateArchivePath(strPath,"7z", strCacheEntry, "");


    if (vec[i]->IsFolder())
      pItem->m_strPath.Format("%s%s/", strPath.c_str(), vec[i]->Path().c_str());
    else
      pItem->m_strPath.Format("%s%s", strPath.c_str(), vec[i]->Path().c_str());

    pItem->m_bIsFolder = vec[i]->IsFolder();
    pItem->m_dwSize = vec[i]->Size();
    vecpItems.Add(pItem);
  }
  return true;
}

bool CCacheManager::IsInCache(const CStdString &strCacheEntry, const CStdString &strPathInCacheEntry) const
{
  for (unsigned int i = 0; i < m_Cached.size(); i++)
  {
    if (m_Cached[i]->GetID() == strCacheEntry)
      return m_Cached[i]->Contains(strPathInCacheEntry);
  }
  return false;
}
bool CCacheManager::IsCached(const CStdString &strCacheEntry) const
{
  for (unsigned int i = 0; i < m_Cached.size(); i++)
  {
    if (m_Cached[i]->GetID() == strCacheEntry)
      return true;
  }
  return false;
}

bool CCacheManager::AddToCache(const CStdString &strCacheEntry, CCache *Cache)
{

  CCacheEntry *ent = NULL;
  int pos = GetCacheEntry(strCacheEntry);
  if (pos == -1)
  {

    ent = AddCacheEntry(strCacheEntry);
  }
  else
    ent = m_Cached[pos];

//  CFileInf *in = new CFileInf(Cache.Path(), Cache.IsFolder(), Cache.Size());
  ent->Add(Cache);
  return true;
}

bool CCacheManager::AddToCache(const CStdString &strCacheEntry, const CStdString &strPathInCacheEntry, bool IsFolder)
{

  CCacheEntry *ent = NULL;
  int pos = GetCacheEntry(strCacheEntry);
  if (pos == -1)
  {

    ent = AddCacheEntry(strCacheEntry);
  }
  else
    ent = m_Cached[pos];

  CCache *in = new CCache(strPathInCacheEntry, IsFolder);
  ent->Add(in);
  return true;
}
CCacheEntry *CCacheManager::AddCacheEntry(const CStdString &strCacheEntry)
{

  CCacheEntry *ent = new CCacheEntry(strCacheEntry);

  m_Cached.push_back(ent);
  return ent;
}

void CCacheManager::Clear(const CStdString& strCacheEntry)
{
  int pos = GetCacheEntry(strCacheEntry);
  Clear(pos);
}

void CCacheManager::Clear(int Pos)
{
  if (Pos == -1 || Pos > m_Cached.size())
    return;

  m_Cached[Pos]->Clear();
  m_Cached.erase(m_Cached.begin() + Pos);
  delete m_Cached[Pos];
}

void CCacheManager::Clear()
{
  for (unsigned int i = 0; i < m_Cached.size(); i++)
    Clear(i);

  // Just in case
  m_Cached.clear();
}

void CCacheManager::Print() const
{

  for (unsigned int i = 0; i < m_Cached.size(); i++)
    m_Cached[i]->Print();
}

int CCacheManager::GetCacheEntry(const CStdString& strCacheEntry) const
{
  for (unsigned int i = 0; i < m_Cached.size(); i++)
  {
    if (m_Cached[i]->GetID() == strCacheEntry)
    {
      return i;
    }
  }
  return -1;
}

const CCache* CCacheManager::GetCached(const CStdString& strCacheEntry, const CStdString& strPathInCacheEntry) const
{
  int Pos = GetCacheEntry(strCacheEntry);
  if (Pos == -1)
    return NULL;
  
  return m_Cached[Pos]->GetEntry(strPathInCacheEntry);
}

void CCacheManager::AutoDelete(const CStdString& strCacheEntry, bool AutoDelete)
{
  int Pos = GetCacheEntry(strCacheEntry);
  if (Pos == -1) return;
  m_Cached[Pos]->AutoDelete(AutoDelete);
}

#define CLEAR

void CCacheManager::CheckIn()
{
#ifdef CLEAR
  int time = timeGetTime();
//  printf("CheckIn %i | ", time);
  std::vector<int> vec;
  for (unsigned int i = 0; i < m_Cached.size(); i++)
  {
    int t = m_Cached[i]->GetTimeOut();
//    printf("%s %i | ", m_Cached[i]->GetID().c_str(), t);
    if (m_Cached[i]->AutoDelete() && t < time)
      vec.push_back(i);
  }
//  printf("\n");
  for (unsigned int i = 0; i < vec.size(); i++)
  {
    printf("CManager: \"%s\" is Overdue and will be cleared\n", m_Cached[vec[i]]->GetID().c_str());
    CLog::Log(LOGDEBUG, "CManager: \"%s\" is Overdue and will be cleared", m_Cached[vec[i]]->GetID().c_str());
    Clear(vec[i]);
  }
#endif
}
