/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "TextureDatabase.h"
#include "utils/log.h"
#include "utils/Crc32.h"
#include "DateTime.h"
#include "dbwrappers/dataset.h"

CTextureDatabase::CTextureDatabase()
{
}

CTextureDatabase::~CTextureDatabase()
{
}

bool CTextureDatabase::Open()
{
  return CDatabase::Open();
}

bool CTextureDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create texture table");
    m_pDS->exec("CREATE TABLE texture (id integer primary key, urlhash integer, url text, cachedurl text, usecount integer, lastusetime text, imagehash text, lasthashcheck text)\n");

    CLog::Log(LOGINFO, "create textures index");
    m_pDS->exec("CREATE INDEX idxTexture ON texture(urlhash)");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path (id integer primary key, urlhash integer, url text, texture text)\n");

    CLog::Log(LOGINFO, "create path index");
    m_pDS->exec("CREATE INDEX idxPath ON path(urlhash)");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables", __FUNCTION__);
    return false;
  }

  return true;
}

bool CTextureDatabase::UpdateOldVersion(int version)
{
  if (version < 4)
  {
    m_pDS->exec("DROP TABLE texture");
    m_pDS->exec("CREATE TABLE texture (id integer primary key, urlhash integer, url text, cachedurl text, usecount integer, lastusetime text, imagehash text)\n");
  }
  if (version < 5)
  {
    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path (id integer primary key, urlhash integer, url text, texture text)\n");

    CLog::Log(LOGINFO, "create path index");
    m_pDS->exec("CREATE INDEX idxPath ON path(urlhash)");
  }
  if (version < 6)
  {
    m_pDS->exec("ALTER TABLE texture ADD lasthashcheck text");
  }
  return true;
}

bool CTextureDatabase::GetCachedTexture(const CStdString &url, CStdString &cacheFile, CStdString &imageHash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    unsigned int hash = GetURLHash(url);

    CStdString sql = PrepareSQL("select id, cachedurl, lasthashcheck, imagehash from texture where urlhash=%u", hash);
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      int textureID = m_pDS->fv(0).get_asInt();
      cacheFile = m_pDS->fv(1).get_asString();
      CDateTime lastCheck;
      lastCheck.SetFromDBDateTime(m_pDS->fv(2).get_asString());
      if (!lastCheck.IsValid() || lastCheck + CDateTimeSpan(1,0,0,0) < CDateTime::GetCurrentDateTime())
        imageHash = m_pDS->fv(3).get_asString();
      m_pDS->close();
      // update the use count
      sql = PrepareSQL("update texture set usecount=usecount+1, lastusetime=CURRENT_TIMESTAMP where id=%u", textureID);
      m_pDS->exec(sql.c_str());
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s, failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return false;
}

bool CTextureDatabase::AddCachedTexture(const CStdString &url, const CStdString &cacheFile, const CStdString &imageHash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    unsigned int hash = GetURLHash(url);
    CStdString date = CDateTime::GetCurrentDateTime().GetAsDBDateTime();

    CStdString sql = PrepareSQL("select id from texture where urlhash=%u", hash);
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // update
      int textureID = m_pDS->fv(0).get_asInt();
      m_pDS->close();
      if (!imageHash.IsEmpty())
        sql = PrepareSQL("update texture set cachedurl='%s', usecount=1, lastusetime=CURRENT_TIMESTAMP, imagehash='%s', lasthashcheck='%s' where id=%u", cacheFile.c_str(), imageHash.c_str(), date.c_str(), textureID);
      else
        sql = PrepareSQL("update texture set cachedurl='%s', usecount=1, lastusetime=CURRENT_TIMESTAMP where id=%u", cacheFile.c_str(), textureID);        
      m_pDS->exec(sql.c_str());
    }
    else
    { // add the texture
      m_pDS->close();
      sql = PrepareSQL("insert into texture (id, urlhash, url, cachedurl, usecount, lastusetime, imagehash, lasthashcheck) values(NULL, %u, '%s', '%s', 1, CURRENT_TIMESTAMP, '%s', '%s')", hash, url.c_str(), cacheFile.c_str(), imageHash.c_str(), date.c_str());
      m_pDS->exec(sql.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return true;
}

bool CTextureDatabase::ClearCachedTexture(const CStdString &url, CStdString &cacheFile)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    unsigned int hash = GetURLHash(url);

    CStdString sql = PrepareSQL("select id, cachedurl from texture where urlhash=%u", hash);
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      int textureID = m_pDS->fv(0).get_asInt();
      cacheFile = m_pDS->fv(1).get_asString();
      m_pDS->close();
      // remove it
      sql = PrepareSQL("delete from texture where id=%u", textureID);
      m_pDS->exec(sql.c_str());
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s, failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return false;
}

unsigned int CTextureDatabase::GetURLHash(const CStdString &url) const
{
  Crc32 crc;
  crc.ComputeFromLowerCase(url);
  return (unsigned int)crc;
}

CStdString CTextureDatabase::GetTextureForPath(const CStdString &url)
{
  try
  {
    if (NULL == m_pDB.get()) return "";
    if (NULL == m_pDS.get()) return "";

    unsigned int hash = GetURLHash(url);

    CStdString sql = PrepareSQL("select texture from path where urlhash=%u", hash);
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      CStdString texture = m_pDS->fv(0).get_asString();
      m_pDS->close();
      return texture;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s, failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return "";
}

void CTextureDatabase::SetTextureForPath(const CStdString &url, const CStdString &texture)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    unsigned int hash = GetURLHash(url);

    CStdString sql = PrepareSQL("select id from path where urlhash=%u", hash);
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // update
      int pathID = m_pDS->fv(0).get_asInt();
      m_pDS->close();
      sql = PrepareSQL("update path set texture='%s' where id=%u", texture.c_str(), pathID);
      m_pDS->exec(sql.c_str());
    }
    else
    { // add the texture
      m_pDS->close();
      sql = PrepareSQL("insert into path (id, urlhash, url, texture) values(NULL, %u, '%s', '%s')", hash, url.c_str(), texture.c_str());
      m_pDS->exec(sql.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return;
}
