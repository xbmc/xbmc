#pragma once
#include "lib/sqLite/sqlitedataset.h"

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

  static CStdString FormatSQL(CStdString strStmt, ...);
protected:
  void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  DWORD ComputeCRC(const CStdString &text);

  virtual bool CreateTables();

  bool m_bOpen;
  int m_version;
//#ifdef PRE_2_1_DATABASE_COMPATIBILITY
  float m_preV2version;
//#endif
  CStdString m_strDatabaseFile;
  std::auto_ptr<dbiplus::SqliteDatabase> m_pDB;
  std::auto_ptr<dbiplus::Dataset> m_pDS;
  std::auto_ptr<dbiplus::Dataset> m_pDS2;

private:
  virtual bool UpdateOldVersion(int version);
  bool UpdateVersionNumber();

  int m_iRefCount;
};
