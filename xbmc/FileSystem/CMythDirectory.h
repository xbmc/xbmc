#pragma once

#include "IDirectory.h"
#include "CMythSession.h"

namespace DIRECTORY
{


class CCMythDirectory
  : public IDirectory
{
public:
  CCMythDirectory();
  ~CCMythDirectory();

  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

private:
  void Release();
  bool GetRecordings(const CStdString& base, CFileItemList &items);
  bool GetChannels  (const CStdString& base, CFileItemList &items);
  bool GetChannelsDb(const CStdString& base, CFileItemList &items);

  CStdString GetString(char* str);

  XFILE::CCMythSession* m_session;
  DllLibCMyth*          m_dll;
  cmyth_database_t      m_database;
  cmyth_recorder_t      m_recorder;
  cmyth_proginfo_t      m_program;
};

}
