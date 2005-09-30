#include "ZipManager.h"
#include "../url.h"
#include "../utils/log.h"


CZipManager g_ZipManager;

CZipManager::CZipManager()
{
}

CZipManager::~CZipManager()
{

}

bool CZipManager::GetZipList(const CStdString& strPath, std::vector<SZipEntry>& items)
{
  CURL url(strPath);
  std::map<CStdString,std::vector<SZipEntry> >::iterator it = mZipMap.find(url.GetHostName());
  if (it != mZipMap.end()) // already listed, just return it if not changed, else release and reread
  {
    std::map<CStdString,__int64>::iterator it2=mZipDate.find(url.GetHostName());
    if (mFile.Stat(url.GetHostName().c_str(),&m_StatData))
      CLog::Log(LOGDEBUG,"statdata: %i, new: %i",it2->second,m_StatData.st_mtime);
      if (m_StatData.st_mtime == it2->second)
      {
        items = it->second;
        return true;
      }
      mZipMap.erase(it);
      mZipDate.erase(it2);
  }

  mFile.Close();
  if (!mFile.Open(url.GetHostName()))
  {
    CLog::Log(LOGDEBUG,"ZipManager: unable to open file!");
    return false;
  }

  SZipEntry ze;
  readHeader(ze);
  if( ze.header != ZIP_LOCAL_HEADER ) 
  {      
    CLog::Log(LOGDEBUG,"ZipManager: not a zip file!");
    mFile.Close();
    return false;
  }
  // push date for update detection
  CFile fileStat;
  fileStat.Stat(url.GetHostName().c_str(),&m_StatData);
  mZipDate.insert(std::make_pair<CStdString,__int64>(url.GetHostName(),m_StatData.st_mtime));
  
  // now list'em
  mFile.Seek(0,SEEK_SET);
  CStdString strSkip;

  while (mFile.GetPosition() != mFile.GetLength())
  {
    readHeader(ze);
    if (ze.header != ZIP_LOCAL_HEADER)
      if (ze.header != ZIP_CENTRAL_HEADER)
      {
        CLog::Log(LOGDEBUG,"ZipManager: broken file!");
        return false;
      }
      else // no handling of zip central header, we are done
      {        
        mZipMap.insert(std::make_pair<CStdString,std::vector<SZipEntry> >(url.GetHostName(),items));
        mFile.Close();
        return true;
      }

      mFile.Read(ze.name,ze.flength);
      ze.name[ze.flength] = '\0';
      mFile.Seek(ze.elength,SEEK_CUR);
      ze.offset = mFile.GetPosition();
      mFile.Seek(ze.csize,SEEK_CUR);
      if (ze.flags & 8)
      {
        mFile.Read(&ze.crc32,4);
        mFile.Read(&ze.csize,4);
        mFile.Read(&ze.usize,4);
      }
      items.push_back(ze);
  }
  mFile.Close();
  return false; // should never get here with healthy .zips until central header is dealt with
}

bool CZipManager::GetZipEntry(const CStdString& strPath, SZipEntry& item)
{
  CURL url(strPath);
  std::map<CStdString,std::vector<SZipEntry> >::iterator it = mZipMap.find(url.GetHostName());
  std::vector<SZipEntry> items;
  if (it == mZipMap.end()) // we need to list the zip
  {
    GetZipList(strPath,items);
  }
  else
  {
    items = it->second;
  }

  CStdString strFileName = url.GetFileName();
  strFileName.Replace("\\","/");
  for (std::vector<SZipEntry>::iterator it2=items.begin();it2 != items.end();++it2)
  {
    if (CStdString(it2->name) == strFileName)
    {
      memcpy(&item,&(*it2),sizeof(SZipEntry));
      return true;
    }
  }
  return false;
}

bool CZipManager::ExtractArchive(const CStdString& strArchive, const CStdString& strPath)
{
  std::vector<SZipEntry> entry;
  CStdString strZipPath;
  strZipPath.Format("zip://Z:\\temp,1,,%s,\\",strArchive.c_str());
  GetZipList(strZipPath,entry);
  for (std::vector<SZipEntry>::iterator it=entry.begin();it != entry.end();++it)
  {
    if (it->name[strlen(it->name)-1] == '/') // skip dirs
      continue;
    CStdString strFilePath(it->name);
    strFilePath.Replace("/","\\");
    strZipPath.Format("zip://Z:\\temp,1,,%s,\\%s",strArchive.c_str(),strFilePath.c_str());
    if (!CFile::Cache(strZipPath.c_str(),(strPath+strFilePath).c_str()))
      return false;
  }
  return true;
}

void CZipManager::CleanUp(const CStdString& strArchive, const CStdString& strPath)
{
  std::vector<SZipEntry> entry;
  CStdString strZipPath;
  strZipPath.Format("zip://Z:\\temp,1,,%s,\\",strArchive.c_str());
  GetZipList(strZipPath,entry);
  for (std::vector<SZipEntry>::iterator it=entry.begin();it != entry.end();++it)
  {
    if (it->name[strlen(it->name)-1] == '/') // skip dirs
      continue;
    CStdString strFilePath(it->name);
    strFilePath.Replace("/","\\");
    CLog::Log(LOGDEBUG,"delete file: %s",(strPath+strFilePath).c_str());
    CFile::Delete((strPath+strFilePath).c_str());
  }
}

void CZipManager::readHeader(SZipEntry& info)
{
  mFile.Read(&info.header,4);
  mFile.Read(&info.version,2);
  mFile.Read(&info.flags,2);
  mFile.Read(&info.method,2);
  mFile.Read(&info.mod_time,2);
  mFile.Read(&info.mod_date,2);
  mFile.Read(&info.crc32,4);
  mFile.Read(&info.csize,4);
  mFile.Read(&info.usize,4);
  mFile.Read(&info.flength,2);
  mFile.Read(&info.elength,2);
}

void CZipManager::release(const CStdString& strPath)
{
  CURL url(strPath);
  std::map<CStdString,std::vector<SZipEntry> >::iterator it= mZipMap.find(url.GetHostName());
  if (it != mZipMap.end())
  {
    std::map<CStdString,__int64>::iterator it2=mZipDate.find(url.GetHostName());
    mZipMap.erase(it);
    mZipDate.erase(it2);
  }
}