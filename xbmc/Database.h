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

#include "StdString.h"
#include "lib/sqLite/mysqldataset.h"
#include "lib/sqLite/sqlitedataset.h"

#include <memory>

struct DatabaseSettings; // forward

class CDatabase
{
public:
  CDatabase(void);
  virtual ~CDatabase(void);
  bool IsOpen();
  void Close();
  bool Compress(bool bForce=true);
  void Interupt();

  bool Open(DatabaseSettings &db);

  void BeginTransaction();
  virtual bool CommitTransaction();
  void RollbackTransaction();
  bool InTransaction();

  static CStdString FormatSQL(CStdString strStmt, ...);
  CStdString PrepareSQL(CStdString strStmt, ...) const;

protected:
  void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  uint32_t ComputeCRC(const CStdString &text);

  virtual bool Open();
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version) { return true; };

  virtual int GetMinVersion() const=0;
  virtual const char *GetDefaultDBName() const=0;

  bool m_bOpen;
  bool m_sqlite; ///< \brief whether we use sqlite (defaults to true)

  std::auto_ptr<dbiplus::Database> m_pDB;
  std::auto_ptr<dbiplus::Dataset> m_pDS;
  std::auto_ptr<dbiplus::Dataset> m_pDS2;

private:
  bool UpdateVersionNumber();

  int m_iRefCount;
};
