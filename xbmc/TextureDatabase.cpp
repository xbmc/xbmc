/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "TextureDatabase.h"
#include "utils/log.h"
#include "XBDateTime.h"
#include "dbwrappers/dataset.h"
#include "URL.h"

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
    m_pDS->exec("CREATE TABLE texture (id integer primary key, url text, cachedurl text, imagehash text, lasthashcheck text)");

    CLog::Log(LOGINFO, "create textures index");
    m_pDS->exec("CREATE INDEX idxTexture ON texture(url)");

    CLog::Log(LOGINFO, "create sizes table, index,  and trigger");
    m_pDS->exec("CREATE TABLE sizes (idtexture integer, size integer, width integer, height integer, usecount integer, lastusetime text)");
    m_pDS->exec("CREATE INDEX idxSize ON sizes(idtexture, size)");
    m_pDS->exec("CREATE INDEX idxSize2 ON sizes(idtexture, width, height)");
    m_pDS->exec("CREATE TRIGGER textureDelete AFTER delete ON texture FOR EACH ROW BEGIN delete from sizes where sizes.idtexture=old.id; END");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path (id integer primary key, url text, type text, texture text)\n");

    // TODO: Should the path index be a covering index? (we need only retrieve texture)
    CLog::Log(LOGINFO, "create path index");
    m_pDS->exec("CREATE INDEX idxPath ON path(url, type)");
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
  if (version < 7)
  { // update all old thumb://foo urls to image://foo?size=thumb
    m_pDS->query("select id,texture from path where texture like 'thumb://%'");
    while (!m_pDS->eof())
    {
      unsigned int id = m_pDS->fv(0).get_asInt();
      CURL url(m_pDS->fv(1).get_asString());
      m_pDS2->exec(PrepareSQL("update path set texture='image://%s?size=thumb' where id=%u", url.GetHostName().c_str(), id));
      m_pDS->next();
    }
    m_pDS->query("select id, url from texture where url like 'thumb://%'");
    while (!m_pDS->eof())
    {
      unsigned int id = m_pDS->fv(0).get_asInt();
      CURL url(m_pDS->fv(1).get_asString());
      m_pDS2->exec(PrepareSQL("update texture set url='image://%s?size=thumb', urlhash=0 where id=%u", url.GetHostName().c_str(), id));
      m_pDS->next();
    }
    m_pDS->close();
  }
  if (version < 8)
  { // get rid of old cached thumbs as they were previously set to the cached thumb name instead of the source thumb
    m_pDS->exec("delete from path");
  }
  if (version < 9)
  { // get rid of the old path table and add the type column
    m_pDS->dropIndex("path", "idxPath");
    m_pDS->exec("DROP TABLE path");
    m_pDS->exec("CREATE TABLE path (id integer primary key, urlhash integer, url text, type text, texture text)\n");
    m_pDS->exec("CREATE INDEX idxPath ON path(urlhash, type)");
  }
  if (version < 10)
  { // get rid of urlhash in both tables...
    m_pDS->dropIndex("path", "idxPath");
    m_pDS->exec("DROP TABLE path");
    m_pDS->exec("CREATE TABLE path (id integer primary key, url text, type text, texture text)\n");
    m_pDS->exec("CREATE INDEX idxPath ON path(url, type)");

    m_pDS->dropIndex("texture", "idxTexture");
    m_pDS->exec("CREATE TEMPORARY TABLE texture_backup(id,url,cachedurl,usecount,lastusetime,imagehash,lasthashcheck)");
    m_pDS->exec("INSERT INTO texture_backup SELECT id,url,cachedurl,usecount,lastusetime,imagehash,lasthashcheck FROM texture");
    m_pDS->exec("DROP TABLE texture");
    m_pDS->exec("CREATE TABLE texture (id integer primary key, url text, cachedurl text, usecount integer, lastusetime text, imagehash text, lasthashcheck text)");
    m_pDS->exec("CREATE INDEX idxTexture ON texture(url)");
    m_pDS->exec("INSERT INTO texture SELECT * FROM texture_backup");
    m_pDS->exec("DROP TABLE texture_backup");
  }
  if (version < 11)
  { // get rid of cached URLs that don't have the correct extension
    m_pDS->exec("DELETE FROM texture WHERE SUBSTR(cachedUrl,-4,4) NOT IN ('.jpg', '.png')");
  }
  if (version < 12)
  { // create new sizes table and move usecount info to it.
    m_pDS->exec("DROP TABLE texture");
    m_pDS->exec("CREATE TABLE texture (id integer primary key, url text, cachedurl text, imagehash text, lasthashcheck text)");
    m_pDS->exec("CREATE INDEX idxTexture ON texture(url)");
    m_pDS->exec("CREATE TABLE sizes (idtexture integer, size integer, width integer, height integer, usecount integer, lastusetime text)");
    m_pDS->exec("CREATE INDEX idxSize ON sizes(idtexture, size)");
    m_pDS->exec("CREATE TRIGGER textureDelete AFTER delete ON texture FOR EACH ROW BEGIN delete from sizes where sizes.idtexture=old.id; END");
  }
  if (version < 13)
  { // index for updateusecount
    m_pDS->exec("CREATE INDEX idxSize2 ON sizes(idtexture, width, height)");
  }
  return true;
}

bool CTextureDatabase::IncrementUseCount(const CTextureDetails &details)
{
  CStdString sql = PrepareSQL("UPDATE sizes SET usecount=usecount+1, lastusetime=CURRENT_TIMESTAMP WHERE idtexture=%u AND width=%u AND height=%u", details.id, details.width, details.height);
  return ExecuteQuery(sql);
}

bool CTextureDatabase::GetCachedTexture(const CStdString &url, CTextureDetails &details)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("SELECT id, cachedurl, lasthashcheck, imagehash, width, height FROM texture JOIN sizes ON (texture.id=sizes.idtexture AND sizes.size=1) WHERE url='%s'", url.c_str());
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // have some information
      details.id = m_pDS->fv(0).get_asInt();
      details.file  = m_pDS->fv(1).get_asString();
      CDateTime lastCheck;
      lastCheck.SetFromDBDateTime(m_pDS->fv(2).get_asString());
      if (lastCheck.IsValid() && lastCheck + CDateTimeSpan(1,0,0,0) < CDateTime::GetCurrentDateTime())
        details.hash = m_pDS->fv(3).get_asString();
      details.width = m_pDS->fv(4).get_asInt();
      details.height = m_pDS->fv(5).get_asInt();
      m_pDS->close();
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

bool CTextureDatabase::SetCachedTextureValid(const CStdString &url, bool updateable)
{
  CStdString date = updateable ? CDateTime::GetCurrentDateTime().GetAsDBDateTime() : "";
  CStdString sql = PrepareSQL("UPDATE texture SET lasthashcheck='%s' WHERE url='%s'", date.c_str(), url.c_str());
  return ExecuteQuery(sql);
}

bool CTextureDatabase::AddCachedTexture(const CStdString &url, const CTextureDetails &details)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("DELETE FROM texture WHERE url='%s'", url.c_str());
    m_pDS->exec(sql.c_str());

    CStdString date = details.updateable ? CDateTime::GetCurrentDateTime().GetAsDBDateTime() : "";
    sql = PrepareSQL("INSERT INTO texture (id, url, cachedurl, imagehash, lasthashcheck) VALUES(NULL, '%s', '%s', '%s', '%s')", url.c_str(), details.file.c_str(), details.hash.c_str(), date.c_str());
    m_pDS->exec(sql.c_str());
    int textureID = (int)m_pDS->lastinsertid();

    // set the size information
    sql = PrepareSQL("INSERT INTO sizes (idtexture, size, usecount, lastusetime, width, height) VALUES(%u, 1, 1, CURRENT_TIMESTAMP, %u, %u)", textureID, details.width, details.height);
    m_pDS->exec(sql.c_str());
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

    CStdString sql = PrepareSQL("select id, cachedurl from texture where url='%s'", url.c_str());
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

bool CTextureDatabase::InvalidateCachedTexture(const CStdString &url)
{
  CStdString date = (CDateTime::GetCurrentDateTime() - CDateTimeSpan(2, 0, 0, 0)).GetAsDBDateTime();
  CStdString sql = PrepareSQL("UPDATE texture SET lasthashcheck='%s' WHERE url='%s'", date.c_str(), url.c_str());
  return ExecuteQuery(sql);
}

CStdString CTextureDatabase::GetTextureForPath(const CStdString &url, const CStdString &type)
{
  try
  {
    if (NULL == m_pDB.get()) return "";
    if (NULL == m_pDS.get()) return "";

    if (url.empty())
      return "";

    CStdString sql = PrepareSQL("select texture from path where url='%s' and type='%s'", url.c_str(), type.c_str());
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

void CTextureDatabase::SetTextureForPath(const CStdString &url, const CStdString &type, const CStdString &texture)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (url.empty())
      return;

    CStdString sql = PrepareSQL("select id from path where url='%s' and type='%s'", url.c_str(), type.c_str());
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
      sql = PrepareSQL("insert into path (id, url, type, texture) values(NULL, '%s', '%s', '%s')", url.c_str(), type.c_str(), texture.c_str());
      m_pDS->exec(sql.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return;
}

void CTextureDatabase::ClearTextureForPath(const CStdString &url, const CStdString &type)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString sql = PrepareSQL("DELETE FROM path WHERE url='%s' and type='%s'", url.c_str(), type.c_str());
    m_pDS->exec(sql.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return;
}
