/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "TextureDatabase.h"
#include "utils/log.h"
#include "XBDateTime.h"
#include "dbwrappers/dataset.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/DatabaseUtils.h"

enum TextureField
{
  TF_None = 0,
  TF_Id,
  TF_Url,
  TF_CachedUrl,
  TF_LastHashCheck,
  TF_ImageHash,
  TF_Width,
  TF_Height,
  TF_UseCount,
  TF_LastUsed,
  TF_Max
};

typedef struct
{
  char string[14];
  TextureField field;
  CDatabaseQueryRule::FIELD_TYPE type;
  bool browseable;
  int localizedString;
} translateField;

static const translateField fields[] = {
  { "none",          TF_None,          CDatabaseQueryRule::TEXT_FIELD    },
  { "textureid",     TF_Id,            CDatabaseQueryRule::NUMERIC_FIELD },
  { "url",           TF_Url,           CDatabaseQueryRule::TEXT_FIELD    },
  { "cachedurl",     TF_CachedUrl,     CDatabaseQueryRule::TEXT_FIELD    },
  { "lasthashcheck", TF_LastHashCheck, CDatabaseQueryRule::TEXT_FIELD    },
  { "imagehash",     TF_ImageHash,     CDatabaseQueryRule::TEXT_FIELD    },
  { "width",         TF_Width,         CDatabaseQueryRule::NUMERIC_FIELD },
  { "height",        TF_Height,        CDatabaseQueryRule::NUMERIC_FIELD },
  { "usecount",      TF_UseCount,      CDatabaseQueryRule::NUMERIC_FIELD },
  { "lastused",      TF_LastUsed,      CDatabaseQueryRule::TEXT_FIELD    }
};

static const size_t NUM_FIELDS = sizeof(fields) / sizeof(translateField);

int CTextureRule::TranslateField(const char *field) const
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (StringUtils::EqualsNoCase(field, fields[i].string)) return fields[i].field;
  return FieldNone;
}

std::string CTextureRule::TranslateField(int field) const
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].string;
  return "none";
}

std::string CTextureRule::GetField(int field, const std::string &type) const
{
  if (field == TF_Id) return "texture.id";
  else if (field == TF_Url) return "texture.url";
  else if (field == TF_CachedUrl) return "texture.cachedurl";
  else if (field == TF_LastHashCheck) return "texture.lasthashcheck";
  else if (field == TF_ImageHash) return "texture.imagehash";
  else if (field == TF_Width) return "sizes.width";
  else if (field == TF_Height) return "sizes.height";
  else if (field == TF_UseCount) return "sizes.usecount";
  else if (field == TF_LastUsed) return "sizes.lastusetime";
  return "";
}

CDatabaseQueryRule::FIELD_TYPE CTextureRule::GetFieldType(int field) const
{
  for (unsigned int i = 0; i < NUM_FIELDS; i++)
    if (field == fields[i].field) return fields[i].type;
  return TEXT_FIELD;
}

std::string CTextureRule::FormatParameter(const std::string &operatorString,
                                          const std::string &param,
                                          const CDatabase &db,
                                          const std::string &strType) const
{
  std::string parameter(param);
  if (m_field == TF_Url)
    parameter = CTextureUtils::UnwrapImageURL(param);
  return CDatabaseQueryRule::FormatParameter(operatorString, parameter, db, strType);
}

void CTextureRule::GetAvailableFields(std::vector<std::string> &fieldList)
{
  // start at 1 to skip TF_None
  for (unsigned int i = 1; i < NUM_FIELDS; i++)
    fieldList.push_back(fields[i].string);
}

std::string CTextureUtils::GetWrappedImageURL(const std::string &image, const std::string &type, const std::string &options)
{
  if (StringUtils::StartsWith(image, "image://"))
    return image; // already wrapped

  CURL url;
  url.SetProtocol("image");
  url.SetUserName(type);
  url.SetHostName(image);
  if (!options.empty())
  {
    url.SetFileName("transform");
    url.SetOptions("?" + options);
  }
  return url.Get();
}

std::string CTextureUtils::GetWrappedThumbURL(const std::string &image)
{
  return GetWrappedImageURL(image, "", "size=thumb");
}

std::string CTextureUtils::UnwrapImageURL(const std::string &image)
{
  if (StringUtils::StartsWith(image, "image://"))
  {
    CURL url(image);
    if (url.GetUserName().empty() && url.GetOptions().empty())
      return url.GetHostName();
  }
  return image;
}

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

void CTextureDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create texture table");
  m_pDS->exec("CREATE TABLE texture (id integer primary key, url text, cachedurl text, imagehash text, lasthashcheck text)");

  CLog::Log(LOGINFO, "create sizes table, index,  and trigger");
  m_pDS->exec("CREATE TABLE sizes (idtexture integer, size integer, width integer, height integer, usecount integer, lastusetime text)");

  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec("CREATE TABLE path (id integer primary key, url text, type text, texture text)\n");
}

void CTextureDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxTexture ON texture(url)");
  m_pDS->exec("CREATE INDEX idxSize ON sizes(idtexture, size)");
  m_pDS->exec("CREATE INDEX idxSize2 ON sizes(idtexture, width, height)");
  // TODO: Should the path index be a covering index? (we need only retrieve texture)
  m_pDS->exec("CREATE INDEX idxPath ON path(url, type)");

  CLog::Log(LOGINFO, "%s creating triggers", __FUNCTION__);
  m_pDS->exec("CREATE TRIGGER textureDelete AFTER delete ON texture FOR EACH ROW BEGIN delete from sizes where sizes.idtexture=old.id; END");
}

void CTextureDatabase::UpdateTables(int version)
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
    m_pDS->exec("DROP TABLE IF EXISTS path");
    m_pDS->exec("CREATE TABLE path (id integer primary key, urlhash integer, url text, type text, texture text)\n");
  }
  if (version < 10)
  { // get rid of urlhash in both tables...
    m_pDS->exec("DROP TABLE IF EXISTS path");
    m_pDS->exec("CREATE TABLE path (id integer primary key, url text, type text, texture text)\n");

    m_pDS->exec("CREATE TEMPORARY TABLE texture_backup(id,url,cachedurl,usecount,lastusetime,imagehash,lasthashcheck)");
    m_pDS->exec("INSERT INTO texture_backup SELECT id,url,cachedurl,usecount,lastusetime,imagehash,lasthashcheck FROM texture");
    m_pDS->exec("DROP TABLE texture");
    m_pDS->exec("CREATE TABLE texture (id integer primary key, url text, cachedurl text, usecount integer, lastusetime text, imagehash text, lasthashcheck text)");
    m_pDS->exec("INSERT INTO texture SELECT * FROM texture_backup");
    m_pDS->exec("DROP TABLE texture_backup");
  }
  if (version < 11)
  { // get rid of cached URLs that don't have the correct extension
    m_pDS->exec("DELETE FROM texture WHERE SUBSTR(cachedUrl,-4,4) NOT IN ('.jpg', '.png')");
  }
  if (version < 12)
  { // create new sizes table and move usecount info to it.
    m_pDS->exec("DROP TABLE IF EXISTS texture");
    m_pDS->exec("CREATE TABLE texture (id integer primary key, url text, cachedurl text, imagehash text, lasthashcheck text)");
    m_pDS->exec("CREATE TABLE sizes (idtexture integer, size integer, width integer, height integer, usecount integer, lastusetime text)");
  }
}

bool CTextureDatabase::IncrementUseCount(const CTextureDetails &details)
{
  std::string sql = PrepareSQL("UPDATE sizes SET usecount=usecount+1, lastusetime=CURRENT_TIMESTAMP WHERE idtexture=%u AND width=%u AND height=%u", details.id, details.width, details.height);
  return ExecuteQuery(sql);
}

bool CTextureDatabase::GetCachedTexture(const std::string &url, CTextureDetails &details)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("SELECT id, cachedurl, lasthashcheck, imagehash, width, height FROM texture JOIN sizes ON (texture.id=sizes.idtexture AND sizes.size=1) WHERE url='%s'", url.c_str());
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

bool CTextureDatabase::GetTextures(CVariant &items, const Filter &filter)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = "SELECT %s FROM texture JOIN sizes ON (texture.id=sizes.idtexture AND sizes.size=1)";
    std::string sqlFilter;
    if (!CDatabase::BuildSQL("", filter, sqlFilter))
      return false;

    sql = PrepareSQL(sql, !filter.fields.empty() ? filter.fields.c_str() : "*") + sqlFilter;
    if (!m_pDS->query(sql.c_str()))
      return false;

    while (!m_pDS->eof())
    {
      CVariant texture;
      texture["textureid"] = m_pDS->fv(0).get_asInt();
      texture["url"] = m_pDS->fv(1).get_asString();
      texture["cachedurl"] = m_pDS->fv(2).get_asString();
      texture["imagehash"] = m_pDS->fv(3).get_asString();
      texture["lasthashcheck"] = m_pDS->fv(4).get_asString();
      CVariant size(CVariant::VariantTypeObject);
      // 5 is sizes.idtexture
      size["size"]  = m_pDS->fv(6).get_asInt();
      size["width"] = m_pDS->fv(7).get_asInt();
      size["height"] = m_pDS->fv(8).get_asInt();
      size["usecount"] = m_pDS->fv(9).get_asInt();
      size["lastused"] = m_pDS->fv(10).get_asString();
      texture["sizes"] = CVariant(CVariant::VariantTypeArray);
      texture["sizes"].push_back(size);
      items.push_back(texture);
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s, failed", __FUNCTION__);
  }
  return false;
}

bool CTextureDatabase::SetCachedTextureValid(const std::string &url, bool updateable)
{
  std::string date = updateable ? CDateTime::GetCurrentDateTime().GetAsDBDateTime() : "";
  std::string sql = PrepareSQL("UPDATE texture SET lasthashcheck='%s' WHERE url='%s'", date.c_str(), url.c_str());
  return ExecuteQuery(sql);
}

bool CTextureDatabase::AddCachedTexture(const std::string &url, const CTextureDetails &details)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("DELETE FROM texture WHERE url='%s'", url.c_str());
    m_pDS->exec(sql.c_str());

    std::string date = details.updateable ? CDateTime::GetCurrentDateTime().GetAsDBDateTime() : "";
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

bool CTextureDatabase::ClearCachedTexture(const std::string &url, std::string &cacheFile)
{
  std::string id = GetSingleValue(PrepareSQL("select id from texture where url='%s'", url.c_str()));
  return !id.empty() ? ClearCachedTexture(strtol(id.c_str(), NULL, 10), cacheFile) : false;
}

bool CTextureDatabase::ClearCachedTexture(int id, std::string &cacheFile)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("select cachedurl from texture where id=%u", id);
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      cacheFile = m_pDS->fv(0).get_asString();
      m_pDS->close();
      // remove it
      sql = PrepareSQL("delete from texture where id=%u", id);
      m_pDS->exec(sql.c_str());
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s, failed on texture id %u", __FUNCTION__, id);
  }
  return false;
}

bool CTextureDatabase::InvalidateCachedTexture(const std::string &url)
{
  std::string date = (CDateTime::GetCurrentDateTime() - CDateTimeSpan(2, 0, 0, 0)).GetAsDBDateTime();
  std::string sql = PrepareSQL("UPDATE texture SET lasthashcheck='%s' WHERE url='%s'", date.c_str(), url.c_str());
  return ExecuteQuery(sql);
}

std::string CTextureDatabase::GetTextureForPath(const std::string &url, const std::string &type)
{
  try
  {
    if (NULL == m_pDB.get()) return "";
    if (NULL == m_pDS.get()) return "";

    if (url.empty())
      return "";

    std::string sql = PrepareSQL("select texture from path where url='%s' and type='%s'", url.c_str(), type.c_str());
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      std::string texture = m_pDS->fv(0).get_asString();
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

void CTextureDatabase::SetTextureForPath(const std::string &url, const std::string &type, const std::string &texture)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (url.empty())
      return;

    std::string sql = PrepareSQL("select id from path where url='%s' and type='%s'", url.c_str(), type.c_str());
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

void CTextureDatabase::ClearTextureForPath(const std::string &url, const std::string &type)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    std::string sql = PrepareSQL("DELETE FROM path WHERE url='%s' and type='%s'", url.c_str(), type.c_str());
    m_pDS->exec(sql.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on url '%s'", __FUNCTION__, url.c_str());
  }
  return;
}

CDatabaseQueryRule *CTextureDatabase::CreateRule() const
{
  return new CTextureRule();
}

CDatabaseQueryRuleCombination *CTextureDatabase::CreateCombination() const
{
  return new CDatabaseQueryRuleCombination();
}
