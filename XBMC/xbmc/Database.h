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

#include "lib/sqLite/sqlitedataset.h"

class CDatabase
{
public:
  CDatabase(void);
  virtual ~CDatabase(void);
  bool Open();
  bool IsOpen();
  void Close();
  bool Compress(bool bForce=true);
  void Interupt();

  void BeginTransaction();
  virtual bool CommitTransaction();
  void RollbackTransaction();
  bool InTransaction();

  static CStdString FormatSQL(CStdString strStmt, ...);
protected:
  void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  DWORD ComputeCRC(const CStdString &text);

  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);

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
  bool UpdateVersionNumber();

  int m_iRefCount;
};
