#ifndef __PERFORMANCE_SAMPLE__
#define __PERFORMANCE_SAMPLE__

#ifdef _LINUX
#include "linux/PlatformDefs.h"
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#endif

#include <string>

#ifndef NO_PERFORMANCE_MEASURE
#define MEASURE_FUNCTION CPerformanceSample aSample(__FUNCTION__,true);
#define BEGIN_MEASURE_BLOCK(n) { CPerformanceSample aSample(n,true);
#define END_MEASURE_BLOCK } 
#else
#define MEASURE_FUNCTION 
#define BEGIN_MEASURE_BLOCK(n) 
#define END_MEASURE_BLOCK  
#endif

class CPerformanceSample
{
public:
  CPerformanceSample(const std::string &statName, bool bCheckWhenDone=true);
  virtual ~CPerformanceSample();

  void Reset();
  void CheckPoint(); // will add a sample to stats and restart counting.

  static double GetEstimatedError();

protected:
  std::string m_statName;
  bool m_bCheckWhenDone;

#ifdef _LINUX
  struct rusage m_usage;
#endif

  LARGE_INTEGER m_tmStart;  
  static LARGE_INTEGER m_tmFreq;  
};

#endif
