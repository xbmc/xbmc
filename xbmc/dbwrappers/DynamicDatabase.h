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
#pragma once

#include "Database.h"
#include "dataset.h"

#include <string>
#include <vector>
#include <map>

class IDBInfoTag;
class CVariant;
class CFileItem;
class CFileItemList;

/*!
 * CDynamicDatabase: denormalized database with dynamic normalization
 *
 * Data is denormalized by providing a flat table to store an object ID and the
 * JSON serialized from an IDBInfoTag. Using this interface, GetObject() can
 * deserialize database objects polymorphically.
 *
 * The flat table can be augmented by specifying relationships between the JSON
 * fields. These relationships are specified in the subclass constructor:
 *
 * BeginDeclarations();
 * DeclareIndex("title", "VARCHAR(50)");
 * DeclareOneToMany("year", "INTEGER");
 * DeclareManyToMany("genre", "TEXT");
 *
 * Dynamic normalization can occur at runtime and the fields are kept in sync:
 *
 * AddManyOneToMany("director", "VARCHAR(20)");
 *
 * Relationships can be dropped as well:
 *
 * DropManyToMany("director");
 *
 * Developer's guide: subclasses can do direct database reads, but all write/delete
 * calls must be abstracted within this class to ensure database integrity.
 */
class CDynamicDatabase : public CDatabase
{
public:
  /*!
   * The parameter here becomes the name of the main table.
   */
  CDynamicDatabase(const char *predominantObject);
  virtual ~CDynamicDatabase() { }

  const char *Describe() const { return m_table; }

  /*!
   * If the item exists and bUpdate is false, no changes are made and the ID is returned.
   * @return database ID of obj, -1 on failure
   */
  int  AddObject(const IDBInfoTag *obj, bool bUpdate = true);

  /*!
   * idObject is set to ID of object if it exists, and untouched if it doesn't
   * exist. Must return false if IsValid(object) returns false. If the object's
   * "databaseid" field is set, that ID is used and Exist() is not called.
   * @throw dbiplus::DbErrors
   */
  virtual bool Exists(const CVariant &object, int &idObject) = 0;
  virtual bool IsValid(const CVariant &object) const = 0;

  bool GetObjectByID(int idObject, IDBInfoTag *obj);
  bool GetObjectByIndex(const std::string &column, const CVariant &value, IDBInfoTag *obj);

  /*!
   * @param predicates A 1:N or N:N item -> ID map, e.g. "year" -> 5 becomes WHERE idyear=5
   * Only intersection (AND) is supported, no union (OR) or complement (NOT) yet
   */
  bool GetObjectsNav(CFileItemList &items, std::map<std::string, long> predicates = std::map<std::string, long>());
  virtual CFileItem *CreateFileItem(const std::string &json, int id) const = 0; // Callback to instantiate data

  bool GetItemByID(const std::string &itemTable, int idItem, CVariant &value);
  bool GetItemID(const std::string &itemTable, const CVariant &value, int &idItem);

  /*!
   * @param column A 1:N or N:N item
   * @param predicates A 1:N or N:N item -> ID map, e.g. "year" -> 5 becomes WHERE idyear=5
   * Only intersection (AND) is supported, no union (OR) or complement (NOT) yet
   */
  bool GetItemNav(const char *column, CFileItemList &items, const std::string &strPath,
                  std::map<std::string, long> predicates = std::map<std::string, long>());

  int  Count(std::string column = "");

  /*!
   * @param deleteOrphans If true, extend delete operation to orphaned parents
   * @return true If the item was deleted or wasn't found in the database
   */
  bool DeleteObject(int idObject, bool deleteOrphans = true);

protected:
  /*!
   * Set up the tables using the relations declared in the subclass's constructor.
   */
  virtual bool CreateTables();

  /*!
   * Helper functions to avoid "FK_%s_%s_%s" games.
   */
  static inline std::string MakeForeignKeyName(const std::string &primary, const std::string &foreign);
  static inline std::string MakeForeignKeyClause(const std::string &primary, const std::string &foreign);
  static inline std::string MakeIndexName(const std::string &table, const std::string &column);
  static inline std::string MakeIndexClause(const std::string &table, const std::string &column);
  static inline std::string MakeLinkTableName(const std::string &primary, const std::string &secondary);
  static inline std::string MakeUniqueIndexName(const std::string &primary, const std::string &secondary, int ord);
  static inline std::string MakeUniqueIndexClause(const std::string &primary, const std::string &secondary, int ord);

  /*!
   * Helper functions used to unpolymorphize CVariants based on SQL column datatype.
   * TODO: Edit Wikipedia and make unpolymorphize a real word.
   */
  static inline bool IsText(CStdString type);
  static inline bool IsFloat(CStdString type);
  static inline bool IsBool(CStdString type);
  static inline bool IsInteger(CStdString type);
  static    CVariant FieldAsVarient(const dbiplus::field_value &fv, const std::string &type);
  static        void DeserializeJSON(const std::string &json, int id, IDBInfoTag *obj);

  /*!
   * Turn a variant into a SQL value, quoted where appropriate (e.g. "1" and "'string'").
   */
  std::string PrepareVariant(const CVariant &value, const std::string &type);

  /*!
   * Call this before the other Declare*() functions.
   */
  void BeginDeclarations();

  /*!
   * Establish relationships between various properties. Call these in the
   * constructor and in sections of UpdateOldVersion().
   */
  void DeclareIndex(const char *column, const char *type);
  void DeclareOneToMany(const char *column, const char *type = "TEXT", bool index = false);
  void DeclareManyToMany(const char *column, const char *type = "TEXT", bool index = false);

  /*!
   * Dynamically add a new relationship.
   */
  void AddIndex(const char *column, const char *type);
  void AddOneToMany(const char *column, const char *type = "TEXT", bool index = false);
  void AddManyToMany(const char *column, const char *type = "TEXT", bool index = false);

  /*!
   * Dynamically destroy an established relationship.
   * @throw dbiplus::DbErrors
   */
  void DropIndex(const char *column);
  void DropOneToMany(const char *column) { DropOneToManyInternal(column, false); }
  void DropManyToMany(const char *column) { DropManyToManyInternal(column, false); }

  /*!
   * Internal helper functions to add items to parent tables and link tables.
   * @throw dbiplus::DbErrors
   */
  int  AddOneToManyItem(const std::string &parent, const std::string &type, const CVariant &value);
  void AddLink(const std::string &item, int idObject, int idItem);

  std::string GetType(const std::string &column);

private:
  /*
   * @throw dbiplus::DbErrors
   */
  void InitializeMainTable(bool recreate = false);

  /*!
   * Declare*() is called from the subclass's constructor, and the actual
   * work is deferred until these functions here are called.
   * @throw dbiplus::DbErrors
   */
  void AddIndexInternal(const char *column, const char *type);
  void AddOneToManyInternal(const char *column, const char *type, bool index);
  void AddManyToManyInternal(const char *column, const char *type, bool index);

  /*!
   * @param tempDrop For rebuilding the database when SQLite doesn't support the DROP COLUMN clause.
   * @throw dbiplus::DbErrors
   */
  void DropOneToManyInternal(const char *column, bool tempDrop = false);
  void DropManyToManyInternal(const char *column, bool tempDrop = false);

  /*!
   * A column (object property)
   */
  struct Item
  {
    std::string name;   // column name and JSON object parameter
    std::string type;   // SQL type
    bool        bIndex; // Should the column be indexed
    bool operator==(const std::string &rhs) const { return name == rhs; } // equal if name matches
  };

  std::vector<Item> m_indices;
  std::vector<Item> m_singleLinks;
  std::vector<Item> m_multiLinks;

  /*!
   * Helper functions
   */
  void CreateIndex(const std::string &table, const std::string &column, bool isFK);
  void CreateUniqueLinks(const std::string &primary, const std::string &secondary);

  // Predominant table name
  const char *m_table;

  // Whether we can begin declaring relations or not (mostly for the implementer's safety)
  bool m_bBegin;
};
