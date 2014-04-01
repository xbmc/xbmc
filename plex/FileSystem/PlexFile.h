#pragma once

#include "filesystem/CurlFile.h"
#include "URL.h"

#include <string>
#include <vector>

namespace XFILE
{
  class CPlexFile : public CCurlFile
  {
  public:
    CPlexFile(void);
    virtual ~CPlexFile() {};

    /* overloaded from CCurlFile */
    virtual bool Open(const CURL& url);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual int IoControl(EIoControl request, void* param);

    static std::vector<std::pair<std::string, std::string> > GetHeaderList();
    static bool BuildHTTPURL(CURL& url);

    /* Returns false if the server is missing or
     * there is something else wrong */
    static bool CanBeTranslated(const CURL &url);
    static CStdString GetMimeType(const CURL &url);
  };
}
