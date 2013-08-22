#pragma once

#include <string>
#include "threads/Thread.h"

class CrashSubmitter : public CThread
{
  public:
    CrashSubmitter() : CThread("CrashSubmitter") { Create(true); }

    void Process();

    static void UploadCrashReports();
    static bool UploadFile(const CStdString& path);

  private:
    static void Upload();
    static CStdString GetDumpData(const CStdString &path);
    static std::string ExtractVersionFromCrashDump(const CStdString& path);
};
