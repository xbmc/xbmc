/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureDatabase.h"

#include "URL.h"
#include "XBDateTime.h"
#include "dbwrappers/dataset.h"
#include "imagefiles/ImageFileURL.h"
#include "utils/DatabaseUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

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
} translateField;

static const translateField fields[] = {
  { "none",          TF_None,          CDatabaseQueryRule::TEXT_FIELD    },
  { "textureid",     TF_Id,            CDatabaseQueryRule::REAL_FIELD    },
  { "url",           TF_Url,           CDatabaseQueryRule::TEXT_FIELD    },
  { "cachedurl",     TF_CachedUrl,     CDatabaseQueryRule::TEXT_FIELD    },
  { "lasthashcheck", TF_LastHashCheck, CDatabaseQueryRule::TEXT_FIELD    },
  { "imagehash",     TF_ImageHash,     CDatabaseQueryRule::TEXT_FIELD    },
  { "width",         TF_Width,         CDatabaseQueryRule::REAL_FIELD    },
  { "height",        TF_Height,        CDatabaseQueryRule::REAL_FIELD    },
  { "usecount",      TF_UseCount,      CDatabaseQueryRule::REAL_FIELD    },
  { "lastused",      TF_LastUsed,      CDatabaseQueryRule::TEXT_FIELD    }
};

static const size_t NUM_FIELDS = sizeof(fields) / sizeof(translateField);

int CTextureRule::TranslateField(const char *field) const
{
  for (const translateField& f : fields)
    if (StringUtils::EqualsNoCase(field, f.string)) return f.field;
  return FieldNone;
}

std::string CTextureRule::TranslateField(int field) const
{
  for (const translateField& f : fields)
    if (field == f.field) return f.string;
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
  for (const translateField& f : fields)
    if (field == f.field) return f.type;
  return TEXT_FIELD;
}

std::string CTextureRule::FormatParameter(const std::string &operatorString,
                                          const std::string &param,
                                          const CDatabase &db,
                                          const std::string &strType) const
{
  std::string parameter(param);
  if (m_field == TF_Url)
    parameter = IMAGE_FILES::ToCacheKey(param);
  return CDatabaseQueryRule::FormatParameter(operatorString, parameter, db, strType);
}

void CTextureRule::GetAvailableFields(std::vector<std::string> &fieldList)
{
  // start at 1 to skip TF_None
  for (unsigned int i = 1; i < NUM_FIELDS; i++)
    fieldList.emplace_back(fields[i].string);
}

CTextureDatabase::CTextureDatabase() = default;

CTextureDatabase::~CTextureDatabase() = default;

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
  CLog::Log(LOGINFO, "{} creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxTexture ON texture(url)");
  m_pDS->exec("CREATE INDEX idxSize ON sizes(idtexture, size)");
  m_pDS->exec("CREATE INDEX idxSize2 ON sizes(idtexture, width, height)");
  //! @todo Should the path index be a covering index? (we need only retrieve texture)
  m_pDS->exec("CREATE INDEX idxPath ON path(url, type)");

  CLog::Log(LOGINFO, "{} creating triggers", __FUNCTION__);
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
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string sql = PrepareSQL("SELECT id, cachedurl, lasthashcheck, imagehash, width, height FROM texture JOIN sizes ON (texture.id=sizes.idtexture AND sizes.size=1) WHERE url='%s'", url.c_str());
    m_pDS->query(sql);
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
    CLog::Log(LOGERROR, "{}, failed on url '{}'", __FUNCTION__, url);
  }
  return false;
}

bool CTextureDatabase::GetTextures(CVariant &items, const Filter &filter)
{
  try
  {
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string sql = "SELECT %s FROM texture JOIN sizes ON (texture.id=sizes.idtexture AND sizes.size=1)";
    std::string sqlFilter;
    if (!CDatabase::BuildSQL("", filter, sqlFilter))
      return false;

    sql = PrepareSQL(sql, !filter.fields.empty() ? filter.fields.c_str() : "*") + sqlFilter;
    if (!m_pDS->query(sql))
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
    CLog::Log(LOGERROR, "{}, failed", __FUNCTION__);
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
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string sql = PrepareSQL("DELETE FROM texture WHERE url='%s'", url.c_str());
    m_pDS->exec(sql);

    std::string date = details.updateable ? CDateTime::GetCurrentDateTime().GetAsDBDateTime() : "";
    sql = PrepareSQL("INSERT INTO texture (id, url, cachedurl, imagehash, lasthashcheck) VALUES(NULL, '%s', '%s', '%s', '%s')", url.c_str(), details.file.c_str(), details.hash.c_str(), date.c_str());
    m_pDS->exec(sql);
    int textureID = (int)m_pDS->lastinsertid();

    // set the size information
    sql = PrepareSQL("INSERT INTO sizes (idtexture, size, usecount, lastusetime, width, height) VALUES(%u, 1, 1, CURRENT_TIMESTAMP, %u, %u)", textureID, details.width, details.height);
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on url '{}'", __FUNCTION__, url);
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
    if (!m_pDB)
      return false;
    if (!m_pDS)
      return false;

    std::string sql = PrepareSQL("select cachedurl from texture where id=%u", id);
    m_pDS->query(sql);

    if (!m_pDS->eof())
    { // have some information
      cacheFile = m_pDS->fv(0).get_asString();
      m_pDS->close();
      // remove it
      sql = PrepareSQL("delete from texture where id=%u", id);
      m_pDS->exec(sql);
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{}, failed on texture id {}", __FUNCTION__, id);
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
    if (!m_pDB)
      return "";
    if (!m_pDS)
      return "";

    if (url.empty())
      return "";

    std::string sql = PrepareSQL("select texture from path where url='%s' and type='%s'", url.c_str(), type.c_str());
    m_pDS->query(sql);

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
    CLog::Log(LOGERROR, "{}, failed on url '{}'", __FUNCTION__, url);
  }
  return "";
}

void CTextureDatabase::SetTextureForPath(const std::string &url, const std::string &type, const std::string &texture)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS)
      return;

    if (url.empty())
      return;

    std::string sql = PrepareSQL("select id from path where url='%s' and type='%s'", url.c_str(), type.c_str());
    m_pDS->query(sql);
    if (!m_pDS->eof())
    { // update
      int pathID = m_pDS->fv(0).get_asInt();
      m_pDS->close();
      sql = PrepareSQL("update path set texture='%s' where id=%u", texture.c_str(), pathID);
      m_pDS->exec(sql);
    }
    else
    { // add the texture
      m_pDS->close();
      sql = PrepareSQL("insert into path (id, url, type, texture) values(NULL, '%s', '%s', '%s')", url.c_str(), type.c_str(), texture.c_str());
      m_pDS->exec(sql);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on url '{}'", __FUNCTION__, url);
  }
}

void CTextureDatabase::ClearTextureForPath(const std::string &url, const std::string &type)
{
  try
  {
    if (!m_pDB)
      return;
    if (!m_pDS)
      return;

    std::string sql = PrepareSQL("DELETE FROM path WHERE url='%s' and type='%s'", url.c_str(), type.c_str());
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on url '{}'", __FUNCTION__, url);
  }
}

CDatabaseQueryRule *CTextureDatabase::CreateRule() const
{
  return new CTextureRule();
}

CDatabaseQueryRuleCombination *CTextureDatabase::CreateCombination() const
{
  return new CDatabaseQueryRuleCombination();
}
