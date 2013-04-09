#pragma once

#include "filesystem/CurlFile.h"
#include "URL.h"

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

  private:
    bool BuildHTTPURL(CURL& url);
  };
}
