#pragma once
#include "../DynamicDll.h"

class DllHtmlScraperInterface
{
public:
  virtual int IMDbGetSearchResults(char *, const char *, const char *)=0;
  virtual int IMDbGetDetails(char *, const char *, const char *)=0;
};

class DllHtmlScraper : public DllDynamic, DllHtmlScraperInterface
{
  DECLARE_DLL_WRAPPER(DllHtmlScraper, Q:\\system\\HTMLScraper.dll)
  DEFINE_METHOD3(int, IMDbGetSearchResults, (char * p1, const char * p2, const char * p3))
  DEFINE_METHOD3(int, IMDbGetDetails, (char *p1, const char *p2, const char *p3))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(IMDbGetSearchResults)
    RESOLVE_METHOD(IMDbGetDetails)
  END_METHOD_RESOLVE()
};
