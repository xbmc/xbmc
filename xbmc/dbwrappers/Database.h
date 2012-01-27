#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "utils/StdString.h"

namespace dbiplus {
  class Database;
  class Dataset;
}

#include <memory>

class DatabaseSettings; // forward

class CDatabase
{
public:
  CDatabase(void);
  virtual ~CDatabase(void);
  bool IsOpen();
  void Close();
  bool Compress(bool bForce=true);
  void Interupt();

  bool Open(const DatabaseSettings &db);

  void BeginTransaction();
  virtual bool CommitTransaction();
  void RollbackTransaction();
  bool InTransaction();

  static CStdString FormatSQL(CStdString strStmt, ...);
  CStdString PrepareSQL(CStdString strStmt, ...) const;

  /*!
   * @brief Get a single value from a table.
   * @remarks The values of the strWhereClause and strOrderBy parameters have to be FormatSQL'ed when used.
   * @param strTable The table to get the value from.
   * @param strColumn The column to get.
   * @param strWhereClause If set, use this WHERE clause.
   * @param strOrderBy If set, use this ORDER BY clause.
   * @return The requested value or an empty string if it wasn't found.
   */
  CStdString GetSingleValue(const CStdString &strTable, const CStdString &strColumn, const CStdString &strWhereClause = CStdString(), const CStdString &strOrderBy = CStdString());

  /*!
   * @brief Delete values from a table.
   * @remarks The value of the strWhereClause parameter has to be FormatSQL'ed when used.
   * @param strTable The table to delete the values from.
   * @param strWhereClause If set, use this WHERE clause.
   * @return True if the query was executed successfully, false otherwise.
   */
  bool DeleteValues(const CStdString &strTable, const CStdString &strWhereClause = CStdString());

  /*!
   * @brief Execute a query that does not return any result.
   * @param strQuery The query to execute.
   * @return True if the query was executed successfully, false otherwise.
   */
  bool ExecuteQuery(const CStdString &strQuery);

  /*!
   * @brief Execute a query that returns a result.
   * @remarks Call m_pDS->close(); to clean up the dataset when done.
   * @param strQuery The query to execute.
   * @return True if the query was executed successfully, false otherwise.
   */
  bool ResultQuery(const CStdString &strQuery);

  /*!
   * @brief Open a new dataset.
   * @return True if the dataset was created successfully, false otherwise.
   */
  bool OpenDS();

  /*!
   * @brief Put an INSERT or REPLACE query in the queue.
   * @param strQuery The query to queue.
   * @return True if the query was added successfully, false otherwise.
   */
  bool QueueInsertQuery(const CStdString &strQuery);

  /*!
   * @brief Commit all queries in the queue.
   * @return True if all queries were executed successfully, false otherwise.
   */
  bool CommitInsertQueries();

protected:
  void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  uint32_t ComputeCRC(const CStdString &text);

  virtual bool Open();
  virtual bool CreateTables();
  virtual void CreateViews() {};
  virtual bool UpdateOldVersion(int version) { return true; };

  virtual int GetMinVersion() const=0;
  virtual const char *GetBaseDBName() const=0;

  bool UpdateVersion(const CStdString &dbName);

  bool m_sqlite; ///< \brief whether we use sqlite (defaults to true)

  std::auto_ptr<dbiplus::Database> m_pDB;
  std::auto_ptr<dbiplus::Dataset> m_pDS;
  std::auto_ptr<dbiplus::Dataset> m_pDS2;

private:
  bool Connect(const DatabaseSettings &db, bool create);
  bool UpdateVersionNumber();

  bool m_bMultiWrite; /*!< True if there are any queries in the queue, false otherwise */
  unsigned int m_openCount;
};
