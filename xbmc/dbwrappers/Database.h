/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace dbiplus
{
class Database;
class Dataset;
} // namespace dbiplus

class DatabaseSettings;
class CDbUrl;
class CProfileManager;
struct SortDescription;

class CDatabase
{
public:
  class Filter
  {
  public:
    Filter();
    explicit Filter(const char* w) : where(w) {}
    explicit Filter(const std::string& w) : where(w) {}

    void AppendField(const std::string& strField);
    void AppendJoin(const std::string& strJoin);
    void AppendWhere(const std::string& strWhere, bool combineWithAnd = true);
    void AppendOrder(const std::string& strOrder);
    void AppendGroup(const std::string& strGroup);

    std::string fields{"*"};
    std::string join;
    std::string where;
    std::string order;
    std::string group;
    std::string limit;
  };

  struct DatasetFieldInfo
  {
    DatasetFieldInfo(bool fetch, bool output, int recno)
      : fetch(fetch), output(output), recno(recno)
    {
    }

    bool fetch;
    bool output;
    int recno;
    std::string strField;
  };

  class DatasetLayout
  {
  public:
    explicit DatasetLayout(size_t totalfields);
    void SetField(int fieldNo, std::string_view strField, bool bOutput = false);
    void AdjustRecordNumbers(int offset);
    bool GetFetch(int fieldno);
    void SetFetch(int fieldno, bool bFetch = true);
    bool GetOutput(int fieldno);
    int GetRecNo(int fieldno);
    std::string GetFields() const;
    bool HasFilterFields() const;

  private:
    std::vector<DatasetFieldInfo> m_fields;
  };

  class ExistsSubQuery
  {
  public:
    explicit ExistsSubQuery(const std::string& table) : tablename(table) {}
    ExistsSubQuery(const std::string& table, const std::string& parameter)
      : tablename(table), param(parameter)
    {
    }
    void AppendJoin(const std::string& strJoin);
    void AppendWhere(std::string_view strWhere, bool combineWithAnd = true);
    bool BuildSQL(std::string& strSQL) const;

    std::string tablename;
    std::string param;
    std::string join;
    std::string where;
  };

  CDatabase();
  virtual ~CDatabase(void);
  bool IsOpen() const;
  virtual void Close();
  bool Compress(bool bForce = true);
  void Interrupt();

  bool Open(const DatabaseSettings& db);

  void BeginTransaction();
  virtual bool CommitTransaction();
  void RollbackTransaction();
  void CopyDB(const std::string& latestDb);
  void DropAnalytics();

  std::string PrepareSQL(std::string_view sqlFormat, ...) const;
  std::string GetDatabaseName(const DatabaseSettings& db);

  /*!
   * @brief Get a single value from a table.
   * @remarks The values of the strWhereClause and strOrderBy parameters have to be FormatSQL'ed when used.
   * @param strTable The table to get the value from.
   * @param strColumn The column to get.
   * @param strWhereClause If set, use this WHERE clause.
   * @param strOrderBy If set, use this ORDER BY clause.
   * @return The requested value or an empty string if it wasn't found.
   */
  std::string GetSingleValue(const std::string& strTable,
                             const std::string& strColumn,
                             const std::string& strWhereClause = std::string(),
                             const std::string& strOrderBy = std::string()) const;
  std::string GetSingleValue(const std::string& query) const;

  /*! \brief Get a single value from a query on a dataset.
   \param query the query in question.
   \param ds the dataset to use for the query.
   \return the value from the query, empty on failure.
   */
  std::string GetSingleValue(const std::string& query, dbiplus::Dataset& ds) const;

  /*!
 * @brief Get a single integer value from a table.
 * @remarks The values of the strWhereClause and strOrderBy parameters have to be FormatSQL'ed when used.
 * @param strTable The table to get the value from.
 * @param strColumn The column to get.
 * @param strWhereClause If set, use this WHERE clause.
 * @param strOrderBy If set, use this ORDER BY clause.
 * @return The requested value or 0 if it wasn't found.
 */
  int GetSingleValueInt(const std::string& strTable,
                        const std::string& strColumn,
                        const std::string& strWhereClause = std::string(),
                        const std::string& strOrderBy = std::string()) const;
  int GetSingleValueInt(const std::string& query) const;

  /*! \brief Get a single integer value from a query on a dataset.
   \param query the query in question.
   \param ds the dataset to use for the query.
   \return the value from the query, 0 on failure.
   */
  int GetSingleValueInt(const std::string& query, dbiplus::Dataset& ds) const;

  /*!
   * @brief Delete values from a table.
   * @param strTable The table to delete the values from.
   * @param filter The Filter to apply to this query.
   * @return True if the query was executed successfully, false otherwise.
   */
  bool DeleteValues(const std::string& strTable, const Filter& filter = Filter());

  /*!
   * @brief Execute a query that does not return any result.
   *        Note that if BeginMultipleExecute() has been called, the
   *        query will be queued until CommitMultipleExecute() is called.
   * @param strQuery The query to execute.
   * @return True if the query was executed successfully, false otherwise.
   * @sa BeginMultipleExecute, CommitMultipleExecute
   */
  bool ExecuteQuery(const std::string& strQuery);

  /*!
   * @brief Execute a query that returns a result.
   * @remarks Call m_pDS->close(); to clean up the dataset when done.
   * @param strQuery The query to execute.
   * @return True if the query was executed successfully, false otherwise.
   */
  bool ResultQuery(const std::string& strQuery) const;

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
   * @brief Put an INSERT or REPLACE query in the queue.
   * @param strQuery The query to queue.
   * @return True if the query was added successfully, false otherwise.
   */
  bool QueueInsertQuery(const std::string& strQuery);

  /*!
   * @brief Commit all queries in the queue.
   * @return True if all queries were executed successfully, false otherwise.
   */
  bool CommitInsertQueries();

  /*!
   * @brief Get the number of INSERT queries in the queue.
   * @return The number of queries.
   */
  size_t GetInsertQueriesCount() const;

  /*!
   * @brief Put a DELETE query in the queue.
   * @param strQuery The query to queue.
   * @return True if the query was added successfully, false otherwise.
   */
  bool QueueDeleteQuery(const std::string& strQuery);

  /*!
   * @brief Commit all queued DELETE queries.
   * @return True if all queries were executed successfully, false otherwise.
   */
  bool CommitDeleteQueries();

  /*!
   * @brief Get the number of DELETE queries in the queue.
   * @return The number of queries.
   */
  size_t GetDeleteQueriesCount() const;

  virtual bool GetFilter(CDbUrl& dbUrl, Filter& filter, SortDescription& sorting) { return true; }
  virtual bool BuildSQL(const std::string& strBaseDir,
                        const std::string& strQuery,
                        Filter& filter,
                        std::string& strSQL,
                        CDbUrl& dbUrl);
  virtual bool BuildSQL(const std::string& strBaseDir,
                        const std::string& strQuery,
                        Filter& filter,
                        std::string& strSQL,
                        CDbUrl& dbUrl,
                        SortDescription& sorting);

  enum class ConnectionState
  {
    STATE_ERROR,
    STATE_DATABASE_NOT_FOUND,
    STATE_CONNECTED,
  };

  ConnectionState Connect(const std::string& dbName, const DatabaseSettings& db, bool create);

protected:
  friend class CDatabaseManager;

  void Split(const std::string& strFileNameAndPath,
             std::string& strPath,
             std::string& strFileName) const;

  virtual bool Open();

  /*! \brief Create database tables and analytics as needed.
   Calls CreateTables() and CreateAnalytics() on child classes.
   */
  bool CreateDatabase();

  /* \brief Create tables for the current database schema.
   Will be called on database creation.
   */
  virtual void CreateTables() = 0;

  /* \brief Create views, indices and triggers for the current database schema.
   Will be called on database creation and database update.
   */
  virtual void CreateAnalytics() = 0;

  /* \brief Update database tables to the current version.
   Note that analytics (views, indices, triggers) are not present during this
   function, so don't rely on them.
   */
  virtual void UpdateTables(int version) {}

  /* \brief The minimum schema version that we support updating from.
   */
  virtual int GetMinSchemaVersion() const { return 0; }

  /* \brief The current schema version.
   */
  virtual int GetSchemaVersion() const = 0;
  virtual const char* GetBaseDBName() const = 0;

  int GetDBVersion();

  bool BuildSQL(std::string_view strQuery, const Filter& filter, std::string& strSQL) const;

  bool m_sqlite{true}; ///< \brief whether we use sqlite (defaults to true)

  std::unique_ptr<dbiplus::Database> m_pDB;
  std::unique_ptr<dbiplus::Dataset> m_pDS;
  std::unique_ptr<dbiplus::Dataset> m_pDS2;

  // Construction parameters
  const CProfileManager& m_profileManager;

private:
  void InitSettings(DatabaseSettings& dbSettings);
  void UpdateVersionNumber();

  bool m_bMultiInsert{
      false}; /*!< True if there are any queries in the insert queue, false otherwise */
  bool m_bMultiDelete{
      false}; /*!< True if there are any queries in the delete queue, false otherwise */
  unsigned int m_openCount{0};

  bool m_multipleExecute{false};
  std::vector<std::string> m_multipleQueries;
};
