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
//        Drop PROfile_START / PROfile_END macros at the begining/end of the functions
//        you want to trace.
//        To use steps within on function you can use the step macros
//        PROfile_STEP has to be included at the beginnig, once per scope
//        Then use PROfile_STEP_START(<your message>) / PROfile_STEP_END
//        The profiler can be enabled/disabled with g_profiler.Enable (true/false);
//        The profile results will be saved upon call of method PrintStats() into
//        a file called 'Profile.txt' in current directory.
/////////////////////////////////////////////////////////////////////////////////////////

#include "PlexProfiler.h"
#include "threads/Thread.h"
#include "system.h"
#include <boost/foreach.hpp>

typedef std::pair<ThreadIdentifier, CProfiledFunction*> FunctionMapPair;
typedef std::map<ThreadIdentifier, CProfiledFunction*>::iterator FunctionMapIterator;

/////////////////////////////////////////////////////////////////////////////////////////
// CProfiledFunction Class methods
/////////////////////////////////////////////////////////////////////////////////////////
CProfiledFunction *CProfiledFunction::FindChild(CStdString aName)
{
  //for (std::list<CProfiledFunction*>::iterator it = m_childFunctions.begin();it !=m_childFunctions.end();++it)
  BOOST_FOREACH(CProfiledFunction* f,m_childFunctions)
      if (f->GetName()==aName) return f;

  return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
CProfiledFunction *CProfiledFunction::AddChildFunction(CProfiledFunction *pFunc)
{
  // Update the parent
  pFunc->SetParent(this);

  // add it to child functions list
  m_childFunctions.push_back(pFunc);
  return pFunc;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CProfiledFunction::Start()
{
  m_startTime = GetTime();
  m_bStarted = true;
  m_numHits++;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CProfiledFunction::End()
{
  if (m_bStarted)
  {
    // End All Sub Functions
    BOOST_FOREACH(CProfiledFunction* f,m_childFunctions)
        f->End();

    // update the end time info
    m_endTime = GetTime();
    m_totalTime += (m_endTime - m_startTime);
    m_bStarted = false;
    PROFILE_DEBUG("ending start=%2.4f, end=%2.4f, total = %2.4f",m_startTime,m_endTime,m_totalTime);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
void CProfiledFunction::Clear()
{
  BOOST_FOREACH(CProfiledFunction* f,m_childFunctions)
      delete f;

  m_pParentFunction = NULL;
  m_childFunctions.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
void CProfiledFunction::PrintStats(FILE* file,int level)
{
  // Print the current function Stats
  CStdString sPad;
  for (int i=0;i<level;i++)
    sPad += "  | ";

  // try to get the parent total time
  float parentTime = 0;
  if (GetParent())
    parentTime = GetParent()->GetTotalTime();

  // compute our percentage compared to parent totaltime
  float percentage = 0;
  if (parentTime) percentage = (m_totalTime * 100.0f) / parentTime;

  // log information onto file
  CStdString sLine;
  sLine.Format("%s% 3d%%,%5d hit(s), % 2.3fs avg:%2.3fs- %s\n", sPad.c_str(),((int)percentage),m_numHits, m_totalTime,m_totalTime / m_numHits, m_name.c_str());
  fputs(sLine.c_str(),file);

  // iterate on childs
  BOOST_FOREACH(CProfiledFunction* f,m_childFunctions)
      f->PrintStats(file,level+1);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CPlexProfiler Class methods
/////////////////////////////////////////////////////////////////////////////////////////
CPlexProfiler::CPlexProfiler()
{ 
  m_enabled = false;
  m_bstopWatchStarted = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
CPlexProfiler::~CPlexProfiler()
{
  Clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
void CPlexProfiler::StartFunction(CStdString functionName)
{
  CSingleLock lk(m_lock);
  CProfiledFunction *currentFunction;

  PROFILE_DEBUG("Entering StartFunction for %s",functionName.c_str());
  if (!m_enabled)
    return;

  // Start Stopwatch if not already done
  if (!m_bstopWatchStarted)
  {
    m_stopWatch.StartZero();
    m_bstopWatchStarted = true;
  }

  ThreadIdentifier threadID = CThread::GetCurrentThreadId();

  // check if we have a root for this thread
  FunctionMapIterator it = m_root.find(threadID);
  if (it==m_root.end())
  {
    PROFILE_DEBUG("Creating Root for thread %X",threadID);
    CStdString RootName;
    RootName.Format("Root %X",threadID);
    m_root[threadID] = new CProfiledFunction(RootName);
    m_currentFunction[threadID] = m_root[threadID];
    currentFunction = m_currentFunction[threadID];
  }
  else
  {
    it = m_currentFunction.find(threadID);
    if (it==m_currentFunction.end())
      return;
    else currentFunction = it->second;
  }

  PROFILE_DEBUG("%s - Entering",functionName.c_str());

  //we look in child functions if we already have it
  PROFILE_DEBUG("%s - Looking for child %s in currentfunction ",functionName.c_str());
  CProfiledFunction *pChildFunction = currentFunction->FindChild(functionName);

  // if we don't have it, then create one as a child
  if (!pChildFunction)
  {
    PROFILE_DEBUG("Creating Child function of %s -> %s",currentFunction->GetName().c_str(),functionName.c_str());
    pChildFunction = currentFunction->AddChildFunction(functionName);
  }
  else PROFILE_DEBUG("Child function found","");

  // now current function is child function
  currentFunction = pChildFunction;

  // now we update its info
  currentFunction->Start();

  m_currentFunction[threadID] = currentFunction;
  PROFILE_DEBUG("%s - Exiting",functionName.c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////
void CPlexProfiler::EndFunction(CStdString functionName)
{
  CSingleLock lk(m_lock);
  CProfiledFunction *currentFunction;

  if (!m_enabled)
    return;

  ThreadIdentifier threadID = CThread::GetCurrentThreadId();

  FunctionMapIterator i = m_currentFunction.find(threadID);
  if (i==m_currentFunction.end())
    return;
  else currentFunction = i->second;

  PROFILE_DEBUG("%s - Entering",functionName.c_str());
  // We close eventually all subfunctions
  while (currentFunction->GetName()!=functionName)
  {
    PROFILE_DEBUG("Current function (%s) is not the one Ending (%s)",currentFunction->GetName().c_str(),functionName.c_str());
    // if the name is not matching, then we have exit the current function without calling endFunction
    // therfore we close current function and go up
    currentFunction->End();
    currentFunction = currentFunction->GetParent();

    if (!currentFunction)
      return;
  }

  // and close the current function
  if (currentFunction)
  {
    PROFILE_DEBUG("Ending function %s",currentFunction->GetName().c_str());
    currentFunction->End();

    // now pop back to the parent
    currentFunction = currentFunction->GetParent();

    if (!currentFunction)
      return;

    PROFILE_DEBUG("Current Function now is %s",currentFunction->GetName().c_str());
  }
  PROFILE_DEBUG("%s - Exiting",functionName.c_str());

  m_currentFunction[threadID] = currentFunction;
}

/////////////////////////////////////////////////////////////////////////////////////////
void CPlexProfiler::SaveProfile(CStdString fileName)
{
  CSingleLock lk(m_lock);
  FILE* file;
  CStdString sLine;
  CStdString OutfileName;

  if (!m_enabled)
    return;

  if (fileName!="")
    OutfileName = fileName;
  else
    OutfileName = "Profile.txt";

  file = fopen64_utf8(OutfileName.c_str(),"wb");
  if (file)
  {
    PROFILE_DEBUG("Outputing stats in %s",OutfileName.c_str());

    sLine.Format("Profiler results dump for %d threads :\n",m_root.size());
    fputs(sLine.c_str(),file);

    // now print individual theards results
    int iCount =1;
    //for (std::map<int,CProfiledFunction*>::iterator it=m_root.begin(); it!=m_root.end(); ++it)
    BOOST_FOREACH(FunctionMapPair p,m_root)
    {
      sLine.Format("-----------Thread %2d (%10X)-----------\n",iCount,p.first);
      fputs(sLine.c_str(),file);

      p.second->PrintStats(file,0);

      sLine = "--------------------------------------------\n";
      fputs(sLine.c_str(),file);

      iCount++;
    }

    fclose(file);
  }
  else PROFILE_DEBUG("Failed to open %s",OutfileName.c_str());

}

/////////////////////////////////////////////////////////////////////////////////////////
void CPlexProfiler::Clear()
{
  CSingleLock lk(m_lock);

  m_enabled = false;

  PROFILE_DEBUG("Clearing Profiler","");
  BOOST_FOREACH(FunctionMapPair p,m_root)
      delete p.second;

  m_root.clear();
  m_currentFunction.clear();
  m_bstopWatchStarted = false;
  PROFILE_DEBUG("Clearing Profiler Complete","");
}

