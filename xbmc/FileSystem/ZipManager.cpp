#include "stdafx.h"
#include "ZipManager.h"
#include "../Util.h"

using namespace XFILE;

CZipManager g_ZipManager;

CZipManager::CZipManager()
{
}

CZipManager::~CZipManager()
{

}

bool CZipManager::HasMultipleEntries(const CStdString& strPath)
{
  // no comments ;D
  CFile mFile;
  if (mFile.Open(strPath.c_str()))
  {
    char buffer[23];
    for (int i=22;i<1024;++i)
    {
      mFile.Seek(mFile.GetLength()-i,SEEK_SET);
      mFile.Read(buffer,4);
      if (*((int*)buffer) == 0x06054b50)
      {
        mFile.Seek(6,SEEK_CUR);
        short iEntries;
        mFile.Read(&iEntries,2);
        mFile.Close();
        return iEntries > 1;
      }
    }
    mFile.Close();
  }

  return true;
}

bool CZipManager::GetZipList(const CStdString& strPath, std::vector<SZipEntry>& items)
{
  CURL url(strPath);
  __stat64 m_StatData;

  CStdString strFile = url.GetHostName();
 
  std::map<CStdString,std::vector<SZipEntry> >::iterator it = mZipMap.find(strFile);
  if (it != mZipMap.end()) // already listed, just return it if not changed, else release and reread
  {
    std::map<CStdString,__int64>::iterator it2=mZipDate.find(strFile);
    if (CFile::Stat(strFile,&m_StatData))
      CLog::Log(LOGDEBUG,"statdata: %i, new: %i",it2->second,m_StatData.st_mtime);
      if (m_StatData.st_mtime == it2->second)
      {
        items = it->second;
        return true;
      }
      mZipMap.erase(it);
      mZipDate.erase(it2);
  }

  CFile mFile;
  if (!mFile.Open(strFile))
  {
    CLog::Log(LOGDEBUG,"ZipManager: unable to open file %s!",strFile.c_str());
    return false;
  }

  SZipEntry ze;
  readHeader(mFile, ze);
  if( ze.header != ZIP_LOCAL_HEADER ) 
  {      
    CLog::Log(LOGDEBUG,"ZipManager: not a zip file!");
    mFile.Close();
    return false;
  }
  // push date for update detection
  CFile::Stat(strFile,&m_StatData);
  mZipDate.insert(std::make_pair<CStdString,__int64>(strFile,m_StatData.st_mtime));
  
  // now list'em
  mFile.Seek(0,SEEK_SET);
  CStdString strSkip;

  while (mFile.GetPosition() != mFile.GetLength())
  {
    readHeader(mFile, ze);
    if (ze.header != ZIP_LOCAL_HEADER)
      if (ze.header != ZIP_CENTRAL_HEADER)
      {
        CLog::Log(LOGDEBUG,"ZipManager: broken file %s!",strFile.c_str());
        return false;
      }
      else // no handling of zip central header, we are done
      {        
        mZipMap.insert(std::make_pair<CStdString,std::vector<SZipEntry> >(strFile,items));
        mFile.Close();
        return true;
      }

      CStdString strName;
      mFile.Read(strName.GetBuffer(ze.flength), ze.flength);
      strName.ReleaseBuffer();
      g_charsetConverter.stringCharsetToUtf8(strName);
      ZeroMemory(ze.name, 255);
      strncpy(ze.name, strName.c_str(), strName.size()>254 ? 254 : strName.size());
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

  CStdString strFile = url.GetHostName();

  std::map<CStdString,std::vector<SZipEntry> >::iterator it = mZipMap.find(strFile);
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
  //strZipPath.Format("zip://Z:\\temp,1,,%s,\\",strArchive.c_str());
  CUtil::CreateZipPath(strZipPath, strArchive, "", 1);  
  GetZipList(strZipPath,entry);
  for (std::vector<SZipEntry>::iterator it=entry.begin();it != entry.end();++it)
  {
    if (it->name[strlen(it->name)-1] == '/') // skip dirs
      continue;
    CStdString strFilePath(it->name);
    
    
    //strZipPath.Format("zip://Z:\\temp,1,,%s,\\%s",strArchive.c_str(),strFilePath.c_str());
    CUtil::CreateZipPath(strZipPath, strArchive, strFilePath, 1);  
    strFilePath.Replace("/","\\");
    if (!CFile::Cache(strZipPath.c_str(),(strPath+strFilePath).c_str()))
      return false;
  }
  return true;
}

void CZipManager::CleanUp(const CStdString& strArchive, const CStdString& strPath)
{
  std::vector<SZipEntry> entry;
  CStdString strZipPath;
  //strZipPath.Format("zip://Z:\\temp,1,,%s,\\",strArchive.c_str());
  CUtil::CreateZipPath(strZipPath, strArchive, "", 1);  

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

void CZipManager::readHeader(CFile& mFile, SZipEntry& info)
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

