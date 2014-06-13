
#ifndef _PLEXGLOBALCACHER_H_
#define _PLEXGLOBALCACHER_H_

#include "VideoThumbLoader.h"
#include "threads/Thread.h"
#include "threads/Event.h"
#include "dialogs/GUIDialogProgress.h"
#include "threads/CriticalSection.h"

// maximum number of caching threads
#define MAX_CACHE_WORKERS 6

class CPlexGlobalCacherWorker;

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexGlobalCacher : public CThread
{
public:
  static CPlexGlobalCacher* GetInstance();
  static void DeleteInstance();
  void Start();
  void Process();
  void OnExit();
  CFileItemPtr PickItem();

  inline void SetSections(CFileItemListPtr Sections) { m_Sections = Sections; }

private:
  CPlexGlobalCacher();
  void SetProgress(CStdString& Line1, CStdString& Line2, int percentage);
  void ProcessSection(CFileItemPtr Section, int iSection, int TotalSections);

  static CPlexGlobalCacher* m_globalCacher;
  CPlexGlobalCacherWorker* m_pWorkers[MAX_CACHE_WORKERS];

  bool m_continue;
  CFileItemList m_listToCache;
  CGUIDialogProgress* m_dlgProgress;
  CCriticalSection m_picklock;
  CFileItemListPtr m_Sections;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class CPlexGlobalCacherWorker : public CThread
{
private:
  CPlexGlobalCacher* m_pCacher;

public:
  CPlexGlobalCacherWorker(CPlexGlobalCacher* pCacher) : CThread("CPlexGlobalCacherWorker"), m_pCacher(pCacher) {}
  void Process();
};

#endif /* _PLEXGLOBALCACHER_H_*/
