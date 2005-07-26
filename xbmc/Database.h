#pragma once
#include "lib/sqlLite/sqlitedataset.h"

using namespace dbiplus;

class CDatabase
{
public:
  CDatabase(void);
  virtual ~CDatabase(void);
  bool Open();
  bool IsOpen();
  void Close();
  bool Compress();
  void Interupt();

  void BeginTransaction();
  bool CommitTransaction();
  void RollbackTransaction();
  bool InTransaction();

protected:
  void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  CStdString FormatSQL(CStdString strStmt, ...);

  virtual bool CreateTables();

  bool m_bOpen;
  float m_fVersion;
  CStdString m_strDatabaseFile;
  auto_ptr<SqliteDatabase> m_pDB;
  auto_ptr<Dataset> m_pDS;
  auto_ptr<Dataset> m_pDS2;

private:
  virtual bool UpdateOldVersion(float fVersion);
  bool UpdateVersionNumber();

  int m_iRefCount;
};
