#pragma once
#include "lib/sqlLite/sqlitedataset.h"
#include "StdString.h"
#include <vector>
#include <memory>
using namespace std;


class CVideoDatabase
{
public:
  CVideoDatabase(void);
  virtual ~CVideoDatabase(void);
	bool		Open() ;
	void		Close() ;
  void    AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles);
protected:
  auto_ptr<SqliteDatabase>  m_pDB;
	auto_ptr<Dataset>				  m_pDS;
  void                      RemoveInvalidChars(CStdString& strTxt);
  void                      Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
	bool						          CreateTables();
  long    AddPath(const CStdString& strPath, const CStdString& strFilename, const CStdString& strCdLabel);
};
