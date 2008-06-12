
#pragma once

#include "SnapShot.h"

class CXbmemdump
{
public:
  CXbmemdump();
  ~CXbmemdump();

  bool Execute(std::vector<std::string>& result);

  // parse the data into something that is easier to handle
  static CSnapShot* CreateSnapShotFromVectorData(std::vector<std::string>& result);
  static int ParseTraceElementFromVectorData(std::vector<std::string>& result, int offset, CStackTrace* pStackTrace);

private:
  bool Setup();
  bool CreateChildProcess();
  void Finish();
  
  HANDLE m_hOriginalStdout;
  HANDLE m_hChildStdoutRd;
  HANDLE m_hChildStdoutWr;
  HANDLE m_hChildStdoutRdDup;
};
