#pragma once

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

namespace dbiplus {
  class Database;
  class Dataset;
}

#include <memory>
#include <string>
#include <vector>

class DatabaseSettings; // forward
class CDbUrl;
struct SortDescription;

class CDatabase
{
public:
  class Filter
  {
  public:
    Filter() : fields("*") {};
    Filter(const char *w) : fields("*"), where(w) {};
    Filter(const std::string &w) : fields("*"), where(w) {};
    
    void AppendField(const std::string &strField);
    void AppendJoin(const std::string &strJoin);
    void AppendWhere(const std::string &strWhere, bool combineWithAnd = true);
    void AppendOrder(const std::string &strOrder);
    void AppendGroup(const std::string &strGroup);

    std::string fields;
    std::string join;
    std::string where;
    std::string order;
    std::string group;
    std::string limit;
  };

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

  std::string PrepareSQL(std::string strStmt, ...) const;

  /*!
   * @brief Get a single value from a table.
   * @remarks The values of the strWhereClause and strOrderBy parameters have to be FormatSQL'ed when used.
   * @param strTable The table to get the value from.
   * @param strColumn The column to get.
   * @param strWhereClause If set, use this WHERE clause.
   * @param strOrderBy If set, use this ORDER BY clause.
   * @return The requested value or an empty string if it wasn't found.
   */
  std::string GetSingleValue(const std::string &strTable, const std::string &strColumn, const std::string &strWhereClause = std::string(), const std::string &strOrderBy = std::string());
  std::string GetSingleValue(const std::string &query);

  /*! \brief Get a single value from a query on a dataset.
   \param query the query in question.
   \param ds the dataset to use for the query.
   \return the value from the query, empty on failure.
   */
  std::string GetSingleValue(const std::string &query, std::unique_ptr<dbiplus::Dataset> &ds);

  /*!
   * @brief Delete values from a table.
   * @param strTable The table to delete the values from.
   * @param filter The Filter to apply to this query.
   * @return True if the query was executed successfully, false otherwise.
   */
  bool DeleteValues(const std::string &strTable, const Filter &filter = Filter());

  /*!
   * @brief Execute a query that does not return any result.
   *        Note that if BeginMultipleExecute() has been called, the
   *        query will be queued until CommitMultipleExecute() is called.
   * @param strQuery The query to execute.
   * @return True if the query was executed successfully, false otherwise.
   * @sa BeginMultipleExecute, CommitMultipleExecute
   */
  bool ExecuteQuery(const std::string &strQuery);

  /*!
   * @brief Execute a query that returns a result.
   * @remarks Call m_pDS->close(); to clean up the dataset when done.
   * @param strQuery The query to execute.
   * @return True if the query was executed successfully, false otherwise.
   */
  bool ResultQuery(const std::string &strQuery);

  /*!
   * @brief Start a multiple execution queue. Any ExecuteQuery() function
   *        following this call will be queued rather than executed until
   *        CommitMultipleExecute() is performed.
   *          NOTE: Queries that rely on any queued execute query will not
   *                function as expected during this period!
   * @return true if we could start a multiple execution queue, false otherwise.
   * @sa CommitMultipleExecute, ExecuteQuery
   */
  bool BeginMultipleExecute();

  /*!
   * @brief Commit the multiple execution queue to the database.
   *        Queries are performed within a transaction, and the transaction
   *        is rolled back should any one query fail.
   * @return True if the queries were executed successfully, false otherwise.
   * @sa BeginMultipleExecute, ExecuteQuery
   */
  bool CommitMultipleExecute();

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
  bool QueueInsertQuery(const std::string &strQuery);

  /*!
   * @brief Commit all queries in the queue.
   * @return True if all queries were executed successfully, false otherwise.
   */
  bool CommitInsertQueries();

  virtual bool GetFilter(CDbUrl &dbUrl, Filter &filter, SortDescription &sorting) { return true; }
  virtual bool BuildSQL(const std::string &strBaseDir, const std::string &strQuery, Filter &filter, std::string &strSQL, CDbUrl &dbUrl);
  virtual bool BuildSQL(const std::string &strBaseDir, const std::string &strQuery, Filter &filter, std::string &strSQL, CDbUrl &dbUrl, SortDescription &sorting);

protected:
  friend class CDatabaseManager;
  bool Update(const DatabaseSettings &db);

  void Split(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName);

  virtual bool Open();

  /*! \brief Create database tables and analytics as needed.
   Calls CreateTables() and CreateAnalytics() on child classes.
   */
  bool CreateDatabase();

  /* \brief Create tables for the current database schema.
   Will be called on database creation.
   */
  virtual void CreateTables()=0;

  /* \brief Create views, indices and triggers for the current database schema.
   Will be called on database creation and database update.
   */
  virtual void CreateAnalytics()=0;

  /* \brief Update database tables to the current version.
   Note that analytics (views, indices, triggers) are not present during this
   function, so don't rely on them.
   */
  virtual void UpdateTables(int version) {};

  /* \brief The minimum schema version that we support updating from.
   */
  virtual int GetMinSchemaVersion() const { return 0; };

  /* \brief The current schema version.
   */
  virtual int GetSchemaVersion() const=0;
  virtual const char *GetBaseDBName() const=0;

  int GetDBVersion();
  bool UpdateVersion(const std::string &dbName);

  bool BuildSQL(const std::string &strQuery, const Filter &filter, std::string &strSQL);

  bool m_sqlite; ///< \brief whether we use sqlite (defaults to true)

  std::unique_ptr<dbiplus::Database> m_pDB;
  std::unique_ptr<dbiplus::Dataset> m_pDS;
  std::unique_ptr<dbiplus::Dataset> m_pDS2;

private:
  void InitSettings(DatabaseSettings &dbSettings);
  bool Connect(const std::string &dbName, const DatabaseSettings &db, bool create);
  void UpdateVersionNumber();

  bool m_bMultiWrite; /*!< True if there are any queries in the queue, false otherwise */
  unsigned int m_openCount;

  bool m_multipleExecute;
  std::vector<std::string> m_multipleQueries;
};
