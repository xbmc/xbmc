#pragma once
#include "Database.h"

typedef vector<CStdString> VECPROGRAMPATHS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%

#define PROGRAM_DATABASE_NAME "MyPrograms6.db"

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase(void);
  long AddProgram(const CStdString& strFilenameAndPath, DWORD titleId, const CStdString& strDescription, const CStdString& strBookmark);
  bool AddTrainer(int iTitleId, const CStdString& strText);
  bool RemoveTrainer(const CStdString& strText);
  bool GetTrainers(unsigned int iTitleId, std::vector<CStdString>& vecTrainers);
  bool GetAllTrainers(std::vector<CStdString>& vecTrainers);
  bool SetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data);
  bool GetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data);
  void SetTrainerActive(const CStdString& strTrainerPath, unsigned int iTitleId, bool bActive);
  CStdString GetActiveTrainer(unsigned int iTitleId);
  long GetFile(const CStdString& strFilenameAndPath, CFileItemList& programs);
  void GetProgramsByBookmark(CStdString& strBookmark, CFileItemList& programs, int iDepth, bool bOnlyDefaultXBE);
  void GetPathsByBookmark(const CStdString& strBookmark, vector <CStdString>& vecPaths);
  void GetProgramsByPath(const CStdString& strPath, CFileItemList& programs, int idepth, bool bOnlyDefaultXBE);
  bool GetXBEPathByTitleId(const DWORD titleId, CStdString& strPathAndFilename);
  bool IncTimesPlayed(const CStdString& strFileName1);
  bool SetDescription(const CStdString& strFileName1, const CStdString& strDescription);
  int GetRegion(const CStdString& strFilenameAndPath);
  bool SetRegion(const CStdString& strFilenameAndPath, int iRegion=0);
  bool EntryExists(const CStdString& strPath, const CStdString& strBookmark);
  bool HasTrainer(const CStdString& strTrainerPath);
  bool ItemHasTrainer(unsigned int iTitleId);
  void DeleteProgram(const CStdString& strPath);
protected:
  static const unsigned __int64 Date_1601 = 0x0701CE1722770000i64;

  long AddPath(const CStdString& strPath);
  long AddFile(long lPathId, const CStdString& strFileName, DWORD titleId , const CStdString& strDescription, int iRegion);

  long AddBookMark(const CStdString& strBookmark);
  long GetProgram(long lPathId);
  long GetPath(const CStdString& strPath);

  virtual bool CreateTables();
  virtual bool UpdateOldVersion(float fVersion);
  void DeleteFile(long lFileId);
  unsigned __int64 LocalTimeToTimeStamp( const SYSTEMTIME & localTime );
  SYSTEMTIME TimeStampToLocalTime( unsigned __int64 timeStamp );
};
