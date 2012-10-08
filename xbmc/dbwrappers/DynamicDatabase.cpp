/*
 *      Copyright (C) 2012 Garrett Brown
 *      Copyright (C) 2012 Team XBMC
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

#include "DynamicDatabase.h"
#include "threads/SystemClock.h"
#include "utils/IDBInfoTag.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "FileItem.h"
#include "URL.h"

#include <map>
#include <queue>

using namespace std;
using namespace dbiplus;

/* static inline */
string CDynamicDatabase::MakeForeignKeyName(const string &primary, const string &foreign)
{
  return "FK_" + foreign + "_" + primary; // FK_foreign_primary
}

/* static inline */
string CDynamicDatabase::MakeForeignKeyClause(const string &primary, const string &foreign)
{
  return "CONSTRAINT " + MakeForeignKeyName(primary, foreign) + " " // CONSTRAINT FK_foreign_primary
         "FOREIGN KEY (id" + foreign + ") "                         // FOREIGN KEY (idforeign)
         "REFERENCES " + foreign + " (id" + foreign + ") "          // REFERENCES foreign (idforeign)
         "ON DELETE SET NULL";                                      // ON DELETE SET NULL
}

/* static inline */
string CDynamicDatabase::MakeIndexName(const string &table, const string &column)
{
  return "idx_" + table + "_" + column; // idx_table_column
}

/* static inline */
string CDynamicDatabase::MakeIndexClause(const string &table, const string &column)
{
  return "INDEX " + MakeIndexName(table, column) + " (id" + column + ")"; // INDEX idx_table_column (idcolumn)
}

/* static inline */
string CDynamicDatabase::MakeLinkTableName(const string &primary, const string &secondary)
{
  return primary + "link" + secondary; // primary_link_secondary
}

/* static inline */
string CDynamicDatabase::MakeUniqueIndexName(const string &primary, const string &secondary, int ord)
{
  CStdString indexName;
  indexName.Format("idx_%s_link%d", MakeLinkTableName(primary, secondary).c_str(), ord); // idx_primarylinkseconday_link1
  return indexName;
}

/* static inline */
string CDynamicDatabase::MakeUniqueIndexClause(const string &primary, const string &secondary, int ord)
{
  string strSQL;
  if (ord == 1)
  {
    strSQL = "CONSTRAINT " + MakeUniqueIndexName(primary, secondary, ord) + " "
             "UNIQUE INDEX (id" + primary + ", id" + secondary + ")";
  }
  else
  {
    strSQL = "CONSTRAINT " + MakeUniqueIndexName(primary, secondary, ord) + " "
             "UNIQUE INDEX (id" + secondary + ", id" + primary + ")";
  }
  return strSQL.c_str();
}

/* static inline */
bool CDynamicDatabase::IsText(CStdString type)
{
  type.ToUpper();
  return (type.substr(0, 4) == "CHAR" || type.substr(0, 7) == "VARCHAR" || type.substr(0, 4) == "TEXT" ||
      type.substr(0, 4) == "ENUM" || type.substr(0, 3) == "SET");
}

/* static inline */
bool CDynamicDatabase::IsFloat(CStdString type)
{
  type.ToUpper();
  // Assume that DECIMAL and NUMERIC are using a precision modifier
  return (type.substr(0, 5) == "FLOAT" || type.substr(0, 4) == "REAL" || type.substr(0, 6) == "DOUBLE" ||
      type.substr(0, 7) == "DECIMAL" || type.substr(0, 7) == "NUMERIC");
}

/* static inline */
bool CDynamicDatabase::IsBool(CStdString type)
{
  type.ToUpper();
  return (type.substr(0, 4) == "BOOL");
}

/* static inline */
bool CDynamicDatabase::IsInteger(CStdString type)
{
  type.ToUpper();
  // Ignore DECIMAL and NUMERIC, as we assume they are using a precision modifier
  return (type.substr(0, 3) == "INT" || type.substr(0, 8) == "SMALLINT" || type.substr(0, 7) == "TINYINT" ||
      type.substr(0, 9) == "MEDIUMINT" || type.substr(0, 6) == "BIGINT" || type.substr(0, 8) == "SMALLINT");
}

/* static */
CVariant CDynamicDatabase::FieldAsVarient(const field_value &fv, const string &type)
{
  if (IsText(type))
    return fv.get_asString();
  else if (IsInteger(type))
    return fv.get_asInt();
  else if (IsBool(type))
    return fv.get_asBool();
  else if (IsFloat(type))
    return fv.get_asDouble();
  else
    return fv.get_asString();
}

string CDynamicDatabase::PrepareVariant(const CVariant &value, const string &type)
{
  if (IsText(type))
    return PrepareSQL("'%s'", value.asString().c_str()).c_str();
  else if (IsInteger(type) || IsBool(type))
    return PrepareSQL("%i", value.asInteger()).c_str();
  else if (IsFloat(type))
    return PrepareSQL("%f", value.asFloat()).c_str();
  else
    // Probably a datetime. Interpret as a string for now
    return PrepareSQL("'%s'", value.asString().c_str()).c_str();
}

/* static */
void CDynamicDatabase::DeserializeJSON(const string &json, int id, IDBInfoTag *obj)
{
  // Parse the JSON string into a CVariant
  string strContent = json;
  CVariant content = CJSONVariantParser::Parse(reinterpret_cast<const unsigned char*>(strContent.c_str()),
    strContent.length());

  // Fill in the database ID (remember, json was serialized before the object was added to the table)
  content["databaseid"] = id;

  // This is done polymorphically, so a picture and a game tag will each deserialize
  // the JSON data appropriately
  obj->Deserialize(content);
}

void CDynamicDatabase::CreateIndex(const string &table, const string &column, bool isFK)
{
  // if isFK, refer to the item as "idItem", else refer to it as simply "item"
  string strSQL = "CREATE INDEX " + MakeIndexName(table, column) + " "   // CREATE INDEX ix_table_column
                  "ON " + table + (isFK ? " (id" : " (") + column + ")"; // ON table (idcolumn)
  m_pDS->exec(strSQL.c_str());
}

void CDynamicDatabase::CreateUniqueLinks(const string &primary, const string &secondary)
{
  string strSQL1 = "CREATE UNIQUE INDEX " + MakeUniqueIndexName(primary, secondary, 1) + " "
                   "ON " + MakeLinkTableName(primary, secondary) + " (id" + primary + ", id" + secondary + ")";
  string strSQL2 = "CREATE UNIQUE INDEX " + MakeUniqueIndexName(primary, secondary, 2) + " "
                   "ON " + MakeLinkTableName(primary, secondary) + " (id" + secondary + ", id" + primary + ")";
  m_pDS->exec(strSQL1.c_str());
  m_pDS->exec(strSQL2.c_str());
}


/**************************************************************************************/
CDynamicDatabase::CDynamicDatabase(const char *predominantObject)
  : m_table(predominantObject)
{
}

bool CDynamicDatabase::CreateTables()
{
  CDatabase::CreateTables();

  InitializeMainTable();

  // Install the relationships declared by the subclass
  for (vector<Item>::const_iterator it = m_indices.begin(); it != m_indices.end(); it++)
    AddIndexInternal(it->name.c_str(), it->type.c_str());

  for (vector<Item>::const_iterator it = m_singleLinks.begin(); it != m_singleLinks.end(); it++)
    AddOneToManyInternal(it->name.c_str(), it->type.c_str(), it->bIndex);

  for (vector<Item>::const_iterator it = m_multiLinks.begin(); it != m_multiLinks.end(); it++)
    AddManyToManyInternal(it->name.c_str(), it->type.c_str(), it->bIndex);

  return true;
}

void CDynamicDatabase::InitializeMainTable(bool recreate /* = false */)
{
  CStdString strSQL;

  string tableName = (recreate ? string(m_table) + "_temp" : m_table);

  strSQL = PrepareSQL(
    "CREATE TABLE %s ("
      "id%s INTEGER PRIMARY KEY, "
      "strContent TEXT"
    ")",
    tableName.c_str(), m_table, MakeForeignKeyClause(m_table, "path").c_str()
  );
  m_pDS->exec(strSQL.c_str());

  if (recreate)
  {
    // Clone the data for the new table
    strSQL = PrepareSQL(
      "INSERT INTO %s (id%s, strContent) "
      "SELECT id%s, strContent "
      "FROM %s",
      tableName.c_str(), m_table, m_table, m_table
    );
    m_pDS->exec(strSQL.c_str());

    strSQL = PrepareSQL("DROP TABLE %s", m_table);
    m_pDS->exec(strSQL.c_str());

    // The temporary table is now the official one
    strSQL = PrepareSQL("ALTER TABLE %s RENAME TO %s", tableName.c_str(), m_table);
    m_pDS->exec(strSQL.c_str());
  }
}

void CDynamicDatabase::BeginDeclarations()
{
  // This resets the relations so that they can be re-defined in UpdateOldVersion()
  m_indices.clear();
  m_singleLinks.clear();
  m_multiLinks.clear();

  m_bBegin = true;
}

void CDynamicDatabase::DeclareIndex(const char *column, const char *type)
{
  // Programmer's check - warn the user if BeginDeclarations() was not called first
  if (!m_bBegin)
    throw DbErrors("Cannot declare index before calling BeginDeclares()");
  CStdString typeUpper = type;
  typeUpper.ToUpper();
  if (typeUpper == "TEXT")
    throw DbErrors("Cannot place INDEX constraint on TEXT");

  Item index;
  index.name   = column;
  index.type   = type;
  index.bIndex = true;
  m_indices.push_back(index);
}

void CDynamicDatabase::DeclareOneToMany(const char *column, const char *type /* = "TEXT"*/, bool index /* = false*/)
{
  if (!m_bBegin)
    throw DbErrors("Cannot declare one-to-many before calling BeginDeclares()");
  CStdString typeUpper = type;
  typeUpper.ToUpper();
  if (typeUpper == "TEXT" && index)
  {
    CLog::Log(LOGERROR, "%s - Cannot place INDEX constraint on TEXT, forcing off (column %s)", __FUNCTION__, column);
    index = false;
  }

  Item item;
  item.name   = column;
  item.type   = typeUpper.c_str();
  item.bIndex = index;
  m_singleLinks.push_back(item);
}

void CDynamicDatabase::DeclareManyToMany(const char *column, const char *type /* = "TEXT"*/, bool index /* = false*/)
{
  if (!m_bBegin)
    throw DbErrors("Cannot declare many-to-many before calling BeginDeclares()");
  CStdString typeUpper = type;
  typeUpper.ToUpper();
  if (typeUpper == "TEXT" && index)
  {
    CLog::Log(LOGERROR, "%s - Cannot place INDEX constraint on TEXT, forcing off (column %s)", __FUNCTION__, column);
    index = false;
  }

  Item item;
  item.name   = column;
  item.type   = typeUpper.c_str();
  item.bIndex = index;
  m_multiLinks.push_back(item);
}

void CDynamicDatabase::AddIndex(const char *column, const char *type)
{
  if (NULL == m_pDB.get() || NULL == m_pDS.get())
    throw DbErrors("Error: Calling AddIndex() before initialization, did you mean DeclareIndex()?");
  if (GetType(column) != "")
    return; // already exists
  DeclareIndex(column, type);
  AddIndexInternal(column, type);
}

void CDynamicDatabase::AddOneToMany(const char *column, const char *type /* = "TEXT"*/, bool index /* = false*/)
{
  if (NULL == m_pDB.get() || NULL == m_pDS.get())
    throw DbErrors("Error: Calling AddOneToMany() before initialization, did you mean DeclareOneToMany()?");
  if (GetType(column) != "")
    return; // already exists
  DeclareOneToMany(column, type, index);
  AddOneToManyInternal(column, type, index);
}

void CDynamicDatabase::AddManyToMany(const char *column, const char *type /* = "TEXT"*/, bool index /* = false*/)
{
  if (NULL == m_pDB.get() || NULL == m_pDS.get())
    throw DbErrors("Error: Calling AddManyToMany() before initialization, did you mean DeclareManyToMany()?");
  if (GetType(column) != "")
    return; // already exists
  DeclareManyToMany(column, type, index);
  AddManyToManyInternal(column, type, index);
}

void CDynamicDatabase::AddIndexInternal(const char *column, const char *type)
{
  CStdString strSQL;

  // Add the column to the main table
  strSQL = PrepareSQL("ALTER TABLE %s ADD %s %s", m_table, column, type);
  m_pDS->exec(strSQL.c_str());

  // Create an index on the new column
  CreateIndex(m_table, column, false);

  // Populate the new column with values from the strContent column. Use a
  // queue to avoid re-allocation (a la vector) and indexed lookups (a la map)
  // TODO: Use m_pDS2
  queue<pair<int, CVariant> > indexValues; // idObject -> new column value (depending on "type")
  strSQL = PrepareSQL("SELECT id%s, strContent FROM %s", m_table, m_table);
  if (m_pDS->query(strSQL.c_str()))
  {
    if (m_pDS->num_rows() != 0)
    {
      while (!m_pDS->eof())
      {
        string strContent = m_pDS->fv(1).get_asString();
        indexValues.push(pair<int, CVariant>(
          m_pDS->fv(0).get_asInt(),
          CJSONVariantParser::Parse(reinterpret_cast<const unsigned char*>(strContent.c_str()), strContent.length())
        ));
        m_pDS->next();
      }
    }
    m_pDS->close();
  }

  while (indexValues.size())
  {
    int idObject = indexValues.front().first;
    CVariant json = indexValues.front().second;
    indexValues.pop();

    if (json[column].isNull())
      continue;

    strSQL = PrepareSQL(("UPDATE %s SET %s=" + PrepareVariant(json[column], type) + " WHERE id%s=%i").c_str(),
      m_table, column, m_table, idObject);

    m_pDS->exec(strSQL.c_str());
  }
}

void CDynamicDatabase::DropIndex(const char *column)
{
  // First, forget about the index
  vector<Item>::iterator it = std::find(m_indices.begin(), m_indices.end(), column);
  if (it != m_indices.end())
    m_indices.erase(it);

  CStdString strSQL;

  // Tip of the day: the SQLite dataset removes "ON %s" automatically
  strSQL = PrepareSQL("DROP INDEX %s ON %s", MakeIndexName(m_table, column).c_str(), m_table);
  m_pDS->exec(strSQL.c_str());

  if (m_sqlite)
  {
    // :)
    // This was not my face when I discovered SQLite doesn't support dropping
    // columns. But consider this: because our data is fully denormalized, why
    // not rebuild the database from that data using our known relationships?

    for (vector<Item>::const_iterator it = m_singleLinks.begin(); it != m_singleLinks.end(); it++)
      DropOneToManyInternal(it->name.c_str(), true); // true = leave behind enough data to rebuild

    for (vector<Item>::const_iterator it = m_multiLinks.begin(); it != m_multiLinks.end(); it++)
      DropManyToManyInternal(it->name.c_str(), true);

    InitializeMainTable(true); // true = drop columns by recreating the tabe

    for (vector<Item>::const_iterator it = m_indices.begin(); it != m_indices.end(); it++)
      AddIndexInternal(it->name.c_str(), it->type.c_str());

    for (vector<Item>::const_iterator it = m_singleLinks.begin(); it != m_singleLinks.end(); it++)
      AddOneToManyInternal(it->name.c_str(), it->type.c_str(), it->bIndex);

    for (vector<Item>::const_iterator it = m_multiLinks.begin(); it != m_multiLinks.end(); it++)
      AddManyToManyInternal(it->name.c_str(), it->type.c_str(), it->bIndex);
  }
  else
  {
    strSQL = PrepareSQL("ALTER TABLE %s DROP %s", m_table, column);
    m_pDS->exec(strSQL.c_str());
  }
}

void CDynamicDatabase::AddOneToManyInternal(const char *column, const char *type, bool index)
{
  CStdString strSQL;

  if (m_sqlite || !index) // on mysql, just use the SQLite CREATE query if there is no index
  {
    CLog::Log(LOGDEBUG, "%s - creating table '%s'", __FUNCTION__, column);
    strSQL = PrepareSQL(
      "CREATE TABLE %s ("
        "id%s INTEGER PRIMARY KEY, "
        "%s %s"
      ")", column, column, column, type
    );
    m_pDS->exec(strSQL.c_str());
    if (index)
      CreateIndex(column, column, true);
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - creating table '%s'", __FUNCTION__, column);
    strSQL = PrepareSQL(
      "CREATE TABLE %s ("
        "id%s INTEGER PRIMARY KEY, "
        "%s %s, "
        "%s"
      ")", column, column, column, type, MakeIndexClause(column, column).c_str()
    );
    m_pDS->exec(strSQL.c_str());
  }

  // Now add the relationship to the main table
  strSQL = PrepareSQL("ALTER TABLE %s ADD id%s INTEGER", m_table, column);
  m_pDS->exec(strSQL.c_str());

  // Foreign key requires an indexed column "to avoid scanning the entire table"
  CreateIndex(m_table, column, true);

  // SQLite *doesn't support* the ADD CONSTRAINT variant of the ALTER TABLE.
  // If a FK constraint is required, the table will have to be recreated and
  // copied over. How FK'd up is that?
  if (!m_sqlite)
  {
    strSQL = PrepareSQL("ALTER TABLE %s ADD %s;", m_table, MakeForeignKeyClause(m_table, column).c_str());
    m_pDS->exec(strSQL.c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG, "Column %s - SQLite only supports FK constraints on table creation, skipping FK.", column);
  }

  // Populate the new foreign key value
  // TODO: Use m_pDS2
  queue<pair<int, CVariant> > idItems; // idObject -> item value
  strSQL = PrepareSQL("SELECT id%s, strContent FROM %s", m_table, m_table);
  if (m_pDS->query(strSQL.c_str()))
  {
    if (m_pDS->num_rows() != 0)
    {
      while (!m_pDS->eof())
      {
        string strContent = m_pDS->fv(1).get_asString();
        CVariant varContent = CJSONVariantParser::Parse(reinterpret_cast<const unsigned char*>(strContent.c_str()),
              strContent.length());

        if (!varContent[column].isNull())
          idItems.push(pair<int, CVariant>(m_pDS->fv(0).get_asInt(), varContent[column]));;

        m_pDS->next();
      }
    }
    m_pDS->close();
  }
  
  while (idItems.size())
  {
    int idObject = idItems.front().first;
    CVariant value = idItems.front().second;
    idItems.pop();

    // Need to resolve a string into a foreign key ID
    int fkId;

    // Test to see if the item exists in the new table
    strSQL = PrepareSQL(
      ("SELECT id%s "
      "FROM %s "
      "WHERE %s=" + PrepareVariant(value, type) + " "
      "LIMIT 1").c_str(),
      column, column, column
    );
    if (!m_pDS->query(strSQL.c_str()))
      continue;
    if (m_pDS->num_rows() == 0)
    {
      // Item doesn't exist in new table, create it now
      m_pDS->close();

      strSQL = PrepareSQL(
        ("INSERT INTO %s (id%s, %s) "
        "VALUES (NULL, " + PrepareVariant(value, type) + ")").c_str(),
        column, column, column
      );
      m_pDS->exec(strSQL.c_str());
      fkId = (int)m_pDS->lastinsertid();
      if (fkId < 0)
        continue;
    }
    else
    {
      fkId = m_pDS->fv(0).get_asInt();
      m_pDS->close();
    }
    strSQL = PrepareSQL("UPDATE %s SET id%s=%i WHERE id%s=%i", m_table, column, fkId, m_table, idObject);
    m_pDS->exec(strSQL.c_str());
  }

}

void CDynamicDatabase::DropOneToManyInternal(const char *column, bool tempDrop /* = false */)
{
  // tempDrop is forced to false on MySQL
  if (!m_sqlite)
    tempDrop = false;

  // If tempDrop is true, leave the table data
  if (!tempDrop)
  {
    vector<Item>::iterator it = std::find(m_singleLinks.begin(), m_singleLinks.end(), column);
    if (it != m_singleLinks.end())
      m_singleLinks.erase(it);
  }

  CStdString strSQL;

  if (!m_sqlite)
  {
    // MySQL doesn't have an IF EXISTS for fk's, so use a try-catch
    try
    {
      strSQL = PrepareSQL("ALTER TABLE %s DROP FOREIGN KEY %s", m_table, MakeForeignKeyName(m_table, column).c_str());
      m_pDS->exec(strSQL.c_str());
    }
    catch (...)
    {
      CLog::Log(LOGDEBUG, "%s - Error dropping foreign key %s", __FUNCTION__, MakeForeignKeyName(m_table, column).c_str());
    }
  }

  strSQL = PrepareSQL("DROP INDEX %s ON %s", MakeIndexName(m_table, column).c_str(), m_table);
  m_pDS->exec(strSQL.c_str());

  // If tempDrop is true, we're calling this from another Drop*Internal() function
  if (!tempDrop)
  {
    if (m_sqlite)
    {
      // SQLite's ALTER TABLE doesn't support dropping columns. Rebuild from scratch
      for (vector<Item>::const_iterator it = m_singleLinks.begin(); it != m_singleLinks.end(); it++)
        DropOneToManyInternal(it->name.c_str(), true); // true = leave behind enough data to rebuild

      for (vector<Item>::const_iterator it = m_multiLinks.begin(); it != m_multiLinks.end(); it++)
        DropManyToManyInternal(it->name.c_str(), true);

      InitializeMainTable(true);

      // Install the relationships declared by the subclass
      for (vector<Item>::const_iterator it = m_indices.begin(); it != m_indices.end(); it++)
        AddIndexInternal(it->name.c_str(), it->type.c_str());

      for (vector<Item>::const_iterator it = m_singleLinks.begin(); it != m_singleLinks.end(); it++)
        AddOneToManyInternal(it->name.c_str(), it->type.c_str(), it->bIndex);

      for (vector<Item>::const_iterator it = m_multiLinks.begin(); it != m_multiLinks.end(); it++)
        AddManyToManyInternal(it->name.c_str(), it->type.c_str(), it->bIndex);
    }
    else
    {
      strSQL = PrepareSQL("ALTER TABLE %s DROP id%s", m_table, column);
      m_pDS->exec(strSQL.c_str());
    }
  }

  strSQL = PrepareSQL("DROP TABLE %s", column);
  m_pDS->exec(strSQL.c_str());
}

void CDynamicDatabase::AddManyToManyInternal(const char *column, const char *type, bool index)
{
  CStdString strSQL;

  if (m_sqlite || !index)
  {
    CLog::Log(LOGDEBUG, "%s - creating table '%s'", __FUNCTION__, column);
    strSQL = PrepareSQL(
      "CREATE TABLE %s ("
        "id%s INTEGER PRIMARY KEY, "
        "%s %s"
      ")", column, column, column, type
    );
    m_pDS->exec(strSQL.c_str());
    if (index)
      CreateIndex(column, column, true);
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - creating table '%s'", __FUNCTION__, column);
    strSQL = PrepareSQL(
      "CREATE TABLE %s ("
        "id%s INTEGER PRIMARY KEY, "
        "%s %s, "
        "%s"
      ")", column, column, column, type, MakeIndexClause(column, column).c_str()
    );
    m_pDS->exec(strSQL.c_str());
  }

  if (m_sqlite)
  {
    CLog::Log(LOGDEBUG, "%s - creating table '%s'", __FUNCTION__, MakeLinkTableName(m_table, column).c_str());
    strSQL = PrepareSQL(
      "CREATE TABLE %s ("
        "id%s INTEGER, " // column
        "id%s INTEGER, " // m_table
        "%s, " // fk
        "%s"   // fk
      ")",
      MakeLinkTableName(m_table, column).c_str(), m_table, column,
      MakeForeignKeyClause(MakeLinkTableName(m_table, column), m_table).c_str(),
      MakeForeignKeyClause(MakeLinkTableName(m_table, column), column).c_str()
    );
    m_pDS->exec(strSQL.c_str());

    CreateIndex(MakeLinkTableName(m_table, column), m_table, true);
    CreateIndex(MakeLinkTableName(m_table, column), column, true);
    CreateUniqueLinks(m_table, column);
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - creating table '%s'", __FUNCTION__, MakeLinkTableName(m_table, column).c_str());
    strSQL = PrepareSQL(
      "CREATE TABLE %s ("
        "id%s INTEGER, " // column
        "id%s INTEGER, " // m_table
        "%s, " // index
        "%s, " // index
        "%s, " // unique index
        "%s, " // unique index
        "%s, " // fk
        "%s"   // fk
      ")",
      MakeLinkTableName(m_table, column).c_str(), m_table, column,
      MakeIndexClause(MakeLinkTableName(m_table, column), m_table).c_str(),
      MakeIndexClause(MakeLinkTableName(m_table, column), column).c_str(),
      MakeUniqueIndexClause(m_table, column, 1).c_str(),
      MakeUniqueIndexClause(m_table, column, 2).c_str(),
      MakeForeignKeyClause(MakeLinkTableName(m_table, column), m_table).c_str(),
      MakeForeignKeyClause(MakeLinkTableName(m_table, column), column).c_str()
    );
    m_pDS->exec(strSQL.c_str());
  }

  // Populate the new foreign key value
  // TODO: Use m_pDS2
  queue<pair<int, CVariant> > idItems; // idObject -> vector of item values
  strSQL = PrepareSQL("SELECT id%s, strContent FROM %s", m_table, m_table);
  if (m_pDS->query(strSQL.c_str()))
  {
    if (m_pDS->num_rows() != 0)
    {
      while (!m_pDS->eof())
      {
        // Get the JSON data and convert to CVariant
        string strContent = m_pDS->fv(1).get_asString();
        CVariant varContent = CJSONVariantParser::Parse(reinterpret_cast<const unsigned char*>(strContent.c_str()),
              strContent.length());

        // Push an array, if a single value is found push an array of 1 item (that value)
        if (varContent[column].isArray())
          idItems.push(pair<int, CVariant>(m_pDS->fv(0).get_asInt(), varContent[column]));
        else if (!varContent[column].isNull() && !varContent[column].isObject())
        {
          CVariant singleItemArray;
          singleItemArray[0] = varContent[column];
          idItems.push(pair<int, CVariant>(m_pDS->fv(0).get_asInt(), singleItemArray));
        }
        m_pDS->next();
      }
    }
    m_pDS->close();
  }

  while (idItems.size())
  {
    int idObject = idItems.front().first;
    CVariant items = idItems.front().second;
    idItems.pop();

    // Each item needs its own entry in its table and link in the link table
    for (CVariant::const_iterator_array it = items.begin_array(); it != items.end_array(); it++)
    {
      CVariant value = *it;

      // Need to resolve a value into a foreign key ID
      int fkId;

      strSQL = PrepareSQL(
        ("SELECT id%s "
        "FROM %s "
        "WHERE %s=" + PrepareVariant(value, type) + " "
        "LIMIT 1").c_str(),
        column, column, column
      );

      if (!m_pDS->query(strSQL.c_str()))
        continue;
      if (m_pDS->num_rows() == 0)
      {
        // Item doesn't exist in new table, create it now
        m_pDS->close();

        strSQL = PrepareSQL(
          ("INSERT INTO %s (id%s, %s) "
          "VALUES (NULL, " + PrepareVariant(value, type) + ")").c_str(),
          column, column, column
        );
        m_pDS->exec(strSQL.c_str());
        fkId = (int)m_pDS->lastinsertid();
        if (fkId < 0)
          continue;
      }
      else
      {
        fkId = m_pDS->fv(0).get_asInt();
        m_pDS->close();
      }
      // Add relation to the link table
      strSQL = PrepareSQL("INSERT INTO %s (id%s, id%s) VALUES (%i, %i)",
          MakeLinkTableName(m_table, column).c_str(), m_table, column, idObject, fkId);
      m_pDS->exec(strSQL.c_str());
    }
  }
}

void CDynamicDatabase::DropManyToManyInternal(const char *column, bool tempDrop /* = false */)
{
  if (!m_sqlite)
    tempDrop = false;

  // If it's a temporary drop, remember the table names for when we rebuild
  if (!tempDrop)
  {
    vector<Item>::iterator it = std::find(m_multiLinks.begin(), m_multiLinks.end(), column);
    if (it != m_multiLinks.end())
      m_multiLinks.erase(it);
  }

  CStdString strSQL;

  strSQL = PrepareSQL("DROP TABLE %s", MakeLinkTableName(m_table, column).c_str());
  m_pDS->exec(strSQL.c_str());

  strSQL = PrepareSQL("DROP TABLE %s", column);
  m_pDS->exec(strSQL.c_str());
}

int CDynamicDatabase::AddObject(const IDBInfoTag *obj, bool bUpdate /* = true */)
{
  if (!obj) return -1;
  if (NULL == m_pDB.get()) return -1;
  if (NULL == m_pDS.get()) return -1;

  CStdString strSQL;
  bool bExists = false;

  // The main payload
  CVariant object;
  obj->Serialize(object);

  if (!IsValid(object))
    return -1;

  // Extract the database ID from the info tag, if set
  int idObject = (object["databaseid"].isNull() ? -1 : (int)object["databaseid"].asInteger());

  try
  {
    // If we weren't given a database ID, check now to see if the item exists
    // in the database by its filename and path
    if (idObject == -1)
      bExists = Exists(object, idObject);

    // The music database uses REPLACE INTO to update objects. Internally, MySQL
    // does a DELETE and re-INSERT, which causes the FKs in the other tables to
    // be set to NULL (because the ON DELETE SET NULL flag is used). Calling
    // DeleteObject() avoids that, and also makes sure to clean up these items
    // to avoid orphaning items.
    if (bExists && bUpdate)
    {
      DeleteObject(idObject);
      bExists = false;
    }

    if (!bExists)
    {
      // Extract values for indexed fields
      map<string, string> indices; // column -> value (formatted for VALUES() statement)
      for (vector<Item>::const_iterator it = m_indices.begin(); it != m_indices.end(); it++)
        indices[it->name] = PrepareVariant(object[it->name], it->type);

      // Extract values for one-to-many links and add them to the database
      map<string, int> idItems; // field -> idValue (id of parent)
      for (vector<Item>::const_iterator it = m_singleLinks.begin(); it != m_singleLinks.end(); it++)
      {
        int valueIndex = AddOneToManyItem(it->name, it->type, object[it->name]);
        if (valueIndex != -1)
        {
          // Found the FK of an item
          idItems[it->name] = valueIndex;
        }
      }

      // Start building the insert query - build column names and values in parallel
      CStdString COLUMNS = "id" + CStdString(m_table);
      CStdString VALUES;
      if (idObject >= 0)
        VALUES.Format("%d", idObject);
      else
        VALUES = "NULL";

      // Need to prepare here so that our surrounding quotes don't get escaped
      COLUMNS += ", strContent";
      VALUES += PrepareSQL(", '%s'", CJSONVariantWriter::Write(object, true).c_str());

      // Add the indexed pairs (this keeps our indexed values in sync)
      for (map<string, string>::const_iterator it = indices.begin(); it != indices.end(); it++)
      {
        COLUMNS += ", " + it->first;
        VALUES += ", " + it->second; // (remember, already sanitized)
      }

      // Add one-to-many pairs (this keeps our foreign keys in sync)
      for (map<string, int>::const_iterator it = idItems.begin(); it != idItems.end(); it++)
      {
        COLUMNS += ", id" + it->first;
        CStdString id;
        id.Format("%d", it->second);
        VALUES += ", " + id;
      }

      // We use REPLACE because it can handle both inserting a new object and
      // replacing an existing objects's record if the given idObject already exists
      strSQL = "INSERT INTO " + CStdString(m_table) + " (" + COLUMNS + ") VALUES (" + VALUES + ")";
      m_pDS->exec(strSQL.c_str());

      if (idObject < 0)
        idObject = (int)m_pDS->lastinsertid();

      // Now that we have our object ID, we can update the link tables. Like
      // above, this may lead orphaned sibblings, requiring a prune function.
      for (vector<Item>::const_iterator it = m_multiLinks.begin(); it != m_multiLinks.end(); it++)
      {
        // Only arrays can be used for multi-links
        CVariant values = object[it->name];
        if (!values.isArray())
          continue;
        for (CVariant::const_iterator_array it2 = values.begin_array(); it2 != values.end_array(); it2++)
        {
          int idItem = AddOneToManyItem(it->name, it->type, *it2);
          if (idItem >= 0)
            AddLink(it->name, idObject, idItem);
        }
      }
    }
    //AnnounceUpdate(m_table, idObject);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to add object. SQL: %s", __FUNCTION__, strSQL.c_str());
  }
  return idObject;
}

// If the type is not known, call GetType(item) and pass the result to this function
// parent is the parent table referenced by e.g. the "idparent" column
int CDynamicDatabase::AddOneToManyItem(const string &parent, const string &type, const CVariant &value)
{
  if (value.isNull() || PrepareVariant(value, type) == "''")
    return -1;

  CStdString strSQL;

  // First, check to see if it already exists
  strSQL = PrepareSQL(
    ("SELECT id%s "
    "FROM %s "
    "WHERE %s=" + PrepareVariant(value, type)).c_str(),
    parent.c_str(), parent.c_str(), parent.c_str()
  );
  m_pDS->query(strSQL.c_str());
  if (m_pDS->num_rows() == 0)
  {
    m_pDS->close();

    // Doesn't exist, add it
    strSQL = PrepareSQL(
      ("INSERT INTO %s (id%s, %s) "
      "VALUES (NULL, " + PrepareVariant(value, type) + ")").c_str(),
      parent.c_str(), parent.c_str(), parent.c_str()
    );
    m_pDS->exec(strSQL.c_str());
    return (int)m_pDS->lastinsertid();
  }
  else
  {
    // Already exists, just return the ID
    int idItem = m_pDS->fv(0).get_asInt();
    m_pDS->close();
    return idItem;
  }
}

void CDynamicDatabase::AddLink(const string &item, int idObject, int idItem)
{
  CStdString strSQL;
  strSQL = PrepareSQL(
    "SELECT id%s, id%s "
    "FROM %s "
    "WHERE id%s=%d AND id%s=%d",
    m_table, item.c_str(), MakeLinkTableName(m_table, item).c_str(), m_table, idObject, item.c_str(), idItem
  );
  m_pDS->query(strSQL.c_str());

  if (m_pDS->num_rows() == 0)
  {
    m_pDS->close();

    // Doesn't exist, add it
    strSQL = PrepareSQL(
      "INSERT INTO %s (id%s, id%s) "
      "VALUES (%i, %i)",
      MakeLinkTableName(m_table, item).c_str(), m_table, item.c_str(), idObject, idItem
    );
    m_pDS->exec(strSQL.c_str());
  }
  else
  {
    m_pDS->close();
  }
}

string CDynamicDatabase::GetType(const string &column)
{
  vector<Item>::const_iterator it;
  if ((it = std::find(m_indices.begin(), m_indices.end(), column)) == m_indices.end())
    if ((it = std::find(m_singleLinks.begin(), m_singleLinks.end(), column)) == m_singleLinks.end())
      if ((it = std::find(m_multiLinks.begin(), m_multiLinks.end(), column)) == m_multiLinks.end())
        return "";
  return it->type;
}

bool CDynamicDatabase::GetObjectByID(int idObject, IDBInfoTag *obj)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  CStdString strSQL;

  try
  {
    strSQL = PrepareSQL("SELECT strContent FROM %s WHERE id%s=%i", m_table, m_table, idObject);

    if (m_pDS->query(strSQL.c_str()))
    {
      if (m_pDS->num_rows() != 0)
      {
        DeserializeJSON(m_pDS->fv(0).get_asString(), idObject, obj);
        m_pDS->close(); // cleanup recordset data
        return true;
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to get object. SQL: %s", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CDynamicDatabase::GetObjectByIndex(const string &column, const CVariant &value, IDBInfoTag *obj)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  CStdString strSQL;

  try
  {
    // Look up the column type
    string type;
    vector<Item>::const_iterator it = std::find(m_indices.begin(), m_indices.end(), column);
    if (it != m_indices.end())
      type = it->type;
    else
      return false;

    strSQL = PrepareSQL(
      ("SELECT id%s, strContent "
      "FROM %s "
      "WHERE %s=" + PrepareVariant(value, type) + " LIMIT 1").c_str(),
      m_table, m_table, column.c_str()
    );

    // Run the query and, if found, use the object ID to instantiate the info tag
    if (m_pDS->query(strSQL.c_str()))
    {
      if (m_pDS->num_rows() != 0)
      {
        DeserializeJSON(m_pDS->fv("strContent").get_asString(), m_pDS->fv(0).get_asInt(), obj);
        m_pDS->close();
        return true;
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to get object. SQL: %s", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CDynamicDatabase::GetObjectsNav(CFileItemList &items, map<string, long> predicates)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  CStdString strSQL;

  try
  {
    // The query for <genre=1> (N:N) and <year=2> (1:N) is:
    //  SELECT object.idobject, strContent
    //  FROM object JOIN objectlinkgenre ON objectlinkgenre.idobject = object.idobject
    //  WHERE idyear=5 AND idgenre=3

    vector<CStdString> joins;
    vector<CStdString> wheres;
    bool firstWhere = true;

    for (map<string, long>::iterator it = predicates.begin(); it != predicates.end(); /* empty */)
    {
      string item = it->first;
      if (std::find(m_multiLinks.begin(), m_multiLinks.end(), item) != m_multiLinks.end())
      {
        long idValue = it->second;
        CStdString join = PrepareSQL("JOIN %s ON %s.id%s=%s.id%s ", MakeLinkTableName(m_table, item).c_str(),
          MakeLinkTableName(m_table, item).c_str(), m_table, m_table, m_table);
        joins.push_back(join);
        CStdString wherePiece = (firstWhere ? "WHERE " : "AND ") + PrepareSQL("id%s=%d ", item.c_str(), idValue);
        wheres.push_back(wherePiece);
        firstWhere = false;
        // Erase the element as there's no need to check later. GCC under C++03 isn't
        // happy with erase() returning an iterator, hence the manual increment.
        predicates.erase(it++);
        continue;
      }
      it++;
    }

    for (map<string, long>::iterator it = predicates.begin(); it != predicates.end(); it++)
    {
      string item = it->first;
      if (std::find(m_singleLinks.begin(), m_singleLinks.end(), item) != m_singleLinks.end())
      {
        long idValue = it->second;
        CStdString wherePiece = (firstWhere ? "WHERE " : "AND ") + PrepareSQL("id%s=%d ", item.c_str(), idValue);
        wheres.push_back(wherePiece);
        firstWhere = false;
      }
    }

    // Build the query
    strSQL = PrepareSQL("SELECT %s.id%s, strContent FROM %s ", m_table, m_table, m_table);
    for (vector<CStdString>::const_iterator it = joins.begin(); it != joins.end(); it++)
      strSQL += *it;
    for (vector<CStdString>::const_iterator it = wheres.begin(); it != wheres.end(); it++)
      strSQL += *it;

    if (m_pDS->query(strSQL.c_str()))
    {
      while (!m_pDS->eof())
      {
        // Use the CreateFileItem() callback provided by the subclass to instantiate the object
        items.Add(CFileItemPtr(CreateFileItem(m_pDS->fv(1).get_asString(), m_pDS->fv(0).get_asInt())));
        m_pDS->next();
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to enumerate objects. SQL: %s", __FUNCTION__, strSQL.c_str());
  }
  return true;
}

// Includes parents (one-to-many) and many-to-many tables
bool CDynamicDatabase::GetItemByID(const string &itemTable, int idItem, CVariant &value)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  CStdString strSQL;

  // Need to get the item type, could be a one-to-many or many-to-many item
  vector<Item>::const_iterator it;
  if ((it = std::find(m_singleLinks.begin(), m_singleLinks.end(), itemTable)) == m_singleLinks.end())
    if ((it = std::find(m_multiLinks.begin(), m_multiLinks.end(), itemTable)) == m_multiLinks.end())
      return false;
  string type = it->type;

  try
  {
    strSQL = PrepareSQL("SELECT %s FROM %s WHERE id%s=%i", itemTable.c_str(), itemTable.c_str(), itemTable.c_str(), idItem);
    if (m_pDS->query(strSQL.c_str()))
    {
      if (m_pDS->num_rows() > 0)
      {
        value = FieldAsVarient(m_pDS->fv(0), type.c_str());
        m_pDS->close();
        return true;
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to get item. SQL: %s", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

// Includes parents (one-to-many) and many-to-many tables
bool CDynamicDatabase::GetItemID(const string &itemTable, const CVariant &value, int &idItem)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  CStdString strSQL;

  // Need to get the item type, could be a one-to-many or many-to-many item
  vector<Item>::const_iterator it;
  if ((it = std::find(m_singleLinks.begin(), m_singleLinks.end(), itemTable)) == m_singleLinks.end())
    if ((it = std::find(m_multiLinks.begin(), m_multiLinks.end(), itemTable)) == m_multiLinks.end())
      return false;
  string type = it->type;

  try
  {
    strSQL = PrepareSQL(("SELECT id%s FROM %s WHERE %s=" + PrepareVariant(value, type)).c_str(),
        itemTable.c_str(), itemTable.c_str(), itemTable.c_str());
    if (m_pDS->query(strSQL.c_str()))
    {
      if (m_pDS->num_rows() > 0)
      {
        idItem = m_pDS->fv(0).get_asInt();
        m_pDS->close();
        return true;
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to get item. SQL: %s", __FUNCTION__, strSQL.c_str());
  }
  return false;
}

// Browse the database using key/value pairs. See filesystem/PictureDatabaseDirectory.cpp
bool CDynamicDatabase::GetItemNav(const char *column, CFileItemList &items, const string &strPath,
                                  map<string, long> predicates /* = empty */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  // Check the validity of column
  vector<Item>::const_iterator it;
  if ((it = std::find(m_singleLinks.begin(), m_singleLinks.end(), column)) == m_singleLinks.end())
    if ((it = std::find(m_multiLinks.begin(), m_multiLinks.end(), column)) == m_multiLinks.end())
      return false;

  CStdString strSQL;

  try
  {
    CStdString strSubquery;

    if (predicates.size() != 0)
    {
      // We want a semi-join. We could use SELECT DISTINCT, but generating all
      // matching rows first only to eliminate duplicates later is inefficient.
      // With a sub-query, the optimizer can recognize the IN clause and return
      // only one instance of each item from the main table.

      // The resulting query for <genre=1>, <tag=2>, <year=3> (N:N / N:N / 1:N) is:
      //  SELECT set, idset  ( <- set is 1:N)
      //  FROM set
      //  WHERE idset IN (SELECT idset
      //                  FROM object
      //                  WHERE idobject IN (SELECT idobject
      //                                     FROM objectlinkgenre
      //                                     WHERE idgenre=1)
      //                  AND idobject IN   (SELECT idobject
      //                                     FROM objectlinktag
      //                                     WHERE idtag=2)
      //                  AND idyear=3)
      //
      // For N:N nav, the query looks a little different. We select from the
      // link table and single link constraints now become a subquery into the
      // main object table. For performance, multiple 1:N constraints are
      // grouped into one WHERE clause:
      //  SELECT tag, idtag
      //  FROM tag
      //  WHERE idtag IN (SELECT idtag
      //                  FROM objectlinktag
      //                  WHERE idobject IN (SELECT idobject
      //                                     FROM objectlinkgenre
      //                                     WHERE idgenre=1)
      //                  AND idObject IN   (SELECT idobject
      //                                     FROM object
      //                                     WHERE idyear=3 AND idset=4))

      vector<string> subqueryConditions;

      // First build the many-to-many subqueries embedded in the WHERE of the main subquery
      for (map<string, long>::iterator it = predicates.begin(); it != predicates.end(); /* empty */)
      {
        string item = it->first;
        if (std::find(m_multiLinks.begin(), m_multiLinks.end(), item) != m_multiLinks.end())
        {
          long idValue = it->second;
          string subq = PrepareSQL("id%s IN (SELECT id%s FROM %s WHERE id%s=%d) ", m_table, m_table,
              MakeLinkTableName(m_table, item).c_str(), item.c_str(), idValue).c_str();
          subqueryConditions.push_back(subq);
          // Erase the element as there's no need to check later. GCC under C++03 isn't
          // happy with erase() returning an iterator, hence the manual increment.
          predicates.erase(it++);
          continue;
        }
        it++;
      }

      // Next, build the single link table predicates.
      // The WHERE conditions specified by the 1:N predicates need to be tested in
      // the main object table. If column is N:N, we are selecting from the link
      // table instead of the object table, so the condition needs to be a subquery
      // in the object table with the combined WHERE conditions instead of individual
      // WHERE conditions (see the examples above).

      bool isManyToMany = (std::find(m_multiLinks.begin(), m_multiLinks.end(), column) != m_multiLinks.end());

      // If column is N:N, this will be used
      vector<string> multiLinkNavConditions;

      for (map<string, long>::iterator it = predicates.begin(); it != predicates.end(); it++)
      {
        string item = it->first;
        if (std::find(m_singleLinks.begin(), m_singleLinks.end(), item) != m_singleLinks.end())
        {
          long idValue = it->second;
          string subq = PrepareSQL("id%s=%d ", item.c_str(), idValue).c_str();
          // Use subqueryConditions for column=1:N, use multiLinkNavConditions for column=N:N
          if (isManyToMany)
            multiLinkNavConditions.push_back(subq);
          else
            subqueryConditions.push_back(subq);
        }
      }

      // If column is N:N, we need to further process our 1:N predicates. They need
      // to be combined and turned into a single subquery on the main object table.
      if (isManyToMany && multiLinkNavConditions.size())
      {
        CStdString combinedWhere;
        for (vector<string>::const_iterator it = multiLinkNavConditions.begin(); it != multiLinkNavConditions.end(); it++)
        {
          if (!combinedWhere.empty())
            combinedWhere += " AND ";
          combinedWhere += *it;
        }
        CStdString cond = PrepareSQL("id%s IN (SELECT id%s FROM %s WHERE %s) ",
            m_table, m_table, m_table, combinedWhere.c_str());
        subqueryConditions.push_back(cond.c_str());
      }

      // Did we find any valid predicates?
      CStdString subqueryParts;
      for (vector<string>::const_iterator it = subqueryConditions.begin(); it != subqueryConditions.end(); it++)
      {
        if (subqueryParts.empty())
          subqueryParts = "WHERE " + *it;
        else
          subqueryParts += "AND " + *it;
      }
      // Pad the accumulated subquery pieces to make it a full SQL sentence
      if (!subqueryParts.empty())
      {
        string table = m_table;
        // Remember, if column is N:N, select from link table instead of main object table
        if (isManyToMany)
          table = MakeLinkTableName(m_table, column);
        strSubquery = PrepareSQL("SELECT id%s FROM %s %s", column, table.c_str(), subqueryParts.c_str());
      }
    }

    if (strSubquery.empty())
      strSQL = PrepareSQL("SELECT id%s, %s FROM %s", column, column, column);
    else
      strSQL = PrepareSQL("SELECT id%s, %s FROM %s WHERE id%s IN (%s)",
          column, column, column, column, strSubquery.c_str());

    if (m_pDS->query(strSQL.c_str()))
    {
      while (!m_pDS->eof())
      {
        CFileItemPtr item(new CFileItem);
        item->SetLabel(m_pDS->fv(1).get_asString());

        CURL url(strPath);
        CStdString columnId;
        columnId.Format("%d", m_pDS->fv(0).get_asInt());
        url.SetFileName(url.GetFileName() + columnId + "/");
        item->SetPath(url.Get());

        item->SetIconImage("DefaultFolder.png");
        item->m_bIsFolder = true;
        items.Add(item);
        m_pDS->next();
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to enumerate items. SQL: %s", __FUNCTION__, strSQL.c_str());
  }
  return true;
}

int CDynamicDatabase::Count(string column /* = "" */)
{
  if (NULL == m_pDB.get()) return 0;
  if (NULL == m_pDS.get()) return 0;

  // Use the following prioritized sequence of tests to find the table name
  string table;
  if (column == "")
    table = m_table;
  else if (std::find(m_indices.begin(), m_indices.end(), column) != m_indices.end())
    table = m_table;
  else if (std::find(m_singleLinks.begin(), m_singleLinks.end(), column) != m_singleLinks.end())
    table = column;
  else if (std::find(m_multiLinks.begin(), m_multiLinks.end(), column) != m_multiLinks.end())
    table = column;
  else
    return 0; // column is invalid
  
  CStdString strSQL;

  try
  {
    CStdString strSQL = PrepareSQL("SELECT COUNT(*) FROM %s", table.c_str());

    if (m_pDS->query(strSQL.c_str()))
    {
      if (m_pDS->num_rows() != 0)
      {
        int count = m_pDS->fv(0).get_asInt();
        m_pDS->close();
        return count;
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to count %s. SQL: %s", __FUNCTION__, table.c_str(), strSQL.c_str());
  }
  return 0;
}

bool CDynamicDatabase::DeleteObject(int idObject, bool killOrphansMwahahaha /* = true */)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;

  CStdString strSQL;

  try
  {
    if (killOrphansMwahahaha)
    {
      // Items to be deleted, including one-to-many and many-to-many items
      map<string, int> idItems; // item type -> item ID

      // First, delete the many-to-many links
      for (vector<Item>::const_iterator it = m_multiLinks.begin(); it != m_multiLinks.end(); it++)
      {
        string item = it->name;
        // Obtaining orphans: we want to select the valid links (those which
        // belong to an object not being deleted), and use these to find the
        // orphaned ones
        strSQL = PrepareSQL(
          "SELECT id%s " // *it is the item name
          "FROM %s "
          "WHERE id%s NOT IN ("
            "SELECT id%s "
            "FROM %s "
            "WHERE id%s<>%i"
          ")",
          item.c_str(), MakeLinkTableName(m_table, item).c_str(), item.c_str(), item.c_str(),
          MakeLinkTableName(m_table, item).c_str(), m_table, idObject
        );

        if (!m_pDS->query(strSQL.c_str()))
          return false;
        if (m_pDS->num_rows() != 0)
        {
          while (!m_pDS->eof())
          {
            idItems[item] = m_pDS->fv(0).get_asInt(); // item name -> item id
            m_pDS->next();
          }
        }
        m_pDS->close();
      }

      // Now we have a list of many-to-many items to be deleted. We delete the
      // items from the link table now, and defer the actual item deletion
      // until the list also includes one-to-many items as well
      for (map<string, int>::const_iterator it = idItems.begin(); it != idItems.end(); it++)
      {
        strSQL = PrepareSQL("DELETE FROM %s WHERE id%s=%i",
          MakeLinkTableName(m_table, it->first).c_str(), m_table, idObject);
        m_pDS->exec(strSQL.c_str());
      }

      // On to the one-to-many items... FROM becomes a comma-separated list of index column names
      CStdString FROM;
      for (vector<Item>::const_iterator it = m_singleLinks.begin(); it != m_singleLinks.end(); it++)
        FROM += (FROM.IsEmpty() ? "id" : ", id") + it->name;

      strSQL = PrepareSQL("SELECT %s FROM %s WHERE id%s=%i", FROM.c_str(), m_table, m_table, idObject);

      if (!m_pDS->query(strSQL.c_str()))
        return false;

      // Result is all indices, we need to filter them out by counting the occurrence of their values
      map<string, int> idItemCandidates;
      if (m_pDS->num_rows() != 0)
        for (unsigned int i = 0; i < m_singleLinks.size() && i < (unsigned int)m_pDS->fieldCount(); i++)
          idItemCandidates[m_singleLinks[i].name] = m_pDS->fv(i).get_asInt();

      m_pDS->close();

      // Kill parent rows of one-to-many relationships
      for (map<string, int>::const_iterator it = idItemCandidates.begin(); it != idItemCandidates.end(); it++)
      {
        string index = it->first;
        int foreignKey = it->second;

        // Before we delete the object record, count the number of rows with the same item FK
        strSQL = PrepareSQL("SELECT COUNT(*) FROM %s WHERE id%s=%i", m_table, index.c_str(), foreignKey);

        if (!m_pDS->query(strSQL.c_str()))
          return false;

        if (m_pDS->num_rows() == 0)
        {
          // This shouldn't happen, but continue anyways
          m_pDS->close();
          continue;
        }

        int count = m_pDS->fv(0).get_asInt();
        m_pDS->close();
        if (count == 1)
        {
          // Only one record found, off with its head
          idItems[index] = foreignKey;
        }
      }

      // Our list has been compiled. This includes many-to-many siblings and one-to-many
      // parents, both of which have been screened for "forsakenness".
      for (map<string, int>::const_iterator it = idItems.begin(); it != idItems.end(); it++)
      {
        strSQL = PrepareSQL("DELETE FROM %s WHERE id%s=%i", it->first.c_str(), it->first.c_str(), it->second);
        m_pDS->exec(strSQL.c_str());
      }
    }

    strSQL = PrepareSQL("DELETE FROM %s WHERE id%s=%i", m_table, m_table, idObject);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unable to delete object. SQL: %s", __FUNCTION__, strSQL.c_str());
  }
  return false;
}
