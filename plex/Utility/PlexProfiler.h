/////////////////////////////////////////////////////////////////////////////////////////
// CPlexProfiler Class Definition
/////////////////////////////////////////////////////////////////////////////////////////
// Author :
//        LongChair - 2013
//
// Description :
//        When trying to profile Plex on RPi, the lack of proper tools to
//        efficiently get some figures on timing functions led me to create
//				this utility class. It will just time functions and subfunctions
//				when the macros are used and will then report the timing into
//				an hierachically organised text file
//
// Usage :
//        Drop PROFILE_START / PROFILE_END macros at the begining/end of the functions
//        you want to trace.
//        To use steps within on function you can use the step macros
//        PROFILE_STEP has to be included at the beginnig, once per scope
//        Then use PROFILE_STEP_START(<your message>) / PROFILE_STEP_END
//        The profiler can be enabled/disabled with g_profiler.Enable (true/false);
//        The profile results will be saved upon call of method PrintStats() into
//        a file called 'Profile.txt' in current directory.
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CPLEXPROFILER_H_
#define _CPLEXPROFILER_H_

#include <list>
#include <map>
#include "StdString.h"
#include "Stopwatch.h"
#include "log.h"
#include "stdio_utf8.h"
#include <boost/enable_shared_from_this.hpp>
#include "PlexApplication.h"
#include "threads/CriticalSection.h"

class CProfiledFunction;

/////////////////////////////////////////////////////////////////////////////////////////
// Profiler class, will handle the functions called within the code through macros
class CPlexProfiler : public boost::enable_shared_from_this<CPlexProfiler>
{
  protected:
    std::map<ThreadIdentifier,CProfiledFunction*> m_root;
    std::map<ThreadIdentifier,CProfiledFunction*> m_currentFunction;
    bool m_enabled;
    bool m_bstopWatchStarted;
    CStopWatch m_stopWatch;
    CCriticalSection m_lock;

  public:
    CPlexProfiler();
    ~CPlexProfiler();

    void Clear();
    void StartFunction(CStdString functionName);
    void EndFunction(CStdString functionName);
    void SaveProfile(CStdString fileName="");
    inline void Enable(bool state) { m_enabled = state; }
    inline CStopWatch GetStopWatch() { return m_stopWatch; }
};

/////////////////////////////////////////////////////////////////////////////////////////
// This class grabs information about a function call.
// will be created by the CPlexProfiler class and maintained under a tree form
// to get some proper profiling output.
class CProfiledFunction
{
  protected:
    CStdString  m_name;
    float m_startTime;
    float m_endTime;
    float m_totalTime;
    bool  m_bStarted;
    int   m_numHits;

    std::list<CProfiledFunction*> m_childFunctions;
    CProfiledFunction *m_pParentFunction;

  public:
    CProfiledFunction(CStdString aName)
    {
      m_name = aName;
      m_startTime = m_endTime =  m_totalTime = 0;
      m_pParentFunction = NULL;
      m_bStarted = false;
      m_numHits = 0;

    }

    ~CProfiledFunction() { Clear(); }

    inline float GetStartTime()       { return m_startTime; }
    inline float GetEndTime()         { return m_endTime; }
    inline float GetTotalTime()       { return m_totalTime; }
    inline float GetElapsedTime()			{ return (m_endTime - m_startTime); }
    inline CStdString GetName()				{ return m_name; }
    inline CProfiledFunction* GetParent()             { return m_pParentFunction; }
    inline void SetParent(CProfiledFunction *pParent) { m_pParentFunction = pParent; }

    inline static float GetTime()     { return g_plexApplication.profiler->GetStopWatch().GetElapsedSeconds();}

    CProfiledFunction *FindChild(CStdString aName);
    CProfiledFunction *AddChildFunction(CProfiledFunction *pFunc);
    inline CProfiledFunction *AddChildFunction(CStdString aName) { return AddChildFunction(new CProfiledFunction(aName));}

    void Start();
    void End();
    void Clear();
    void PrintStats(FILE* file,int Level);
};

inline CStdString GetClassMethod(const char *text)
{
  CStdString strName = text;
  size_t pos1 = strName.find("::");
  if (pos1!=CStdString::npos)
  {
    size_t pos2 = strName.rfind(" ",pos1);
    if (pos2!=CStdString::npos)
    {
      size_t pos3 = strName.find("(",pos1);
      if (pos3!=CStdString::npos)
      {
        return strName.substr(pos2,pos3-pos2);
      }
    }
  }
  return strName;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Profiling Macros functions
#define PROFILER_ACTIVE 0

#if PROFILER_ACTIVE
#define PROFILE_RESET         if (g_plexApplication.profiler) { g_plexApplication.profiler->Clear(); g_plexApplication.profiler->Enable(true);}
#define PROFILE_START         if (g_plexApplication.profiler) g_plexApplication.profiler->StartFunction(GetClassMethod(__PRETTY_FUNCTION__)+"()");
#define PROFILE_END           if (g_plexApplication.profiler) g_plexApplication.profiler->EndFunction(GetClassMethod(__PRETTY_FUNCTION__)+"()");
#define PROFILE_STEP          CStdString fName;
#define PROFILE_STEP_START(format,...)	fName.Format("%s() - " format, GetClassMethod(__PRETTY_FUNCTION__).c_str(), __VA_ARGS__); if (g_plexApplication.profiler) g_plexApplication.profiler->StartFunction(fName);
#define PROFILE_STEP_END      if (g_plexApplication.profiler) g_plexApplication.profiler->.EndFunction(fName);
#define PROFILE_SAVE          if (g_plexApplication.profiler) g_plexApplication.profiler->SaveProfile();
#else
#define PROFILE_RESET                   ;
#define PROFILE_START                   ;
#define PROFILE_END                     ;
#define PROFILE_STEP                    ;
#define PROFILE_STEP_START(format,...)	;
#define PROFILE_STEP_END                ;
#define PROFILE_SAVE                    ;
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// internal debug macros
#if 0
#define PROFILE_DEBUG(format,...)		CLog::Log(LOGDEBUG,"PROFILE - %20s - %10X - " format, __PRETTY_FUNCTION__,(unsigned int)CThread::GetCurrentThreadId(), __VA_ARGS__);
#else
#define PROFILE_DEBUG(format,...) ;
#endif


#endif /* _CPLEXPROFILER_H_ */
