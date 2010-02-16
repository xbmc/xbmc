#pragma once

#include "dshowutil/dshowutil.h"

// IAMExtendedSeeking
#include <qnetwork.h>

#include "DSGraph.h"
#include "log.h"
#include "CharsetConverter.h"

struct SChapterInfos
{
  CStdString name;
  double time; // in ms
};


class CChaptersManager
{
public:
  static CChaptersManager *m_pSingleton;
  static CChaptersManager *getSingleton();
  static void Destroy();

  int GetChapterCount();
  int GetChapter();
  void GetChapterName(CStdString& strChapterName);
  void UpdateChapters();
  int SeekChapter(int iChapter);
  bool LoadChapters();

  bool InitManager(IBaseFilter *Splitter, CDSGraph *Graph);

private:
  CChaptersManager(void);
  ~CChaptersManager(void);

  std::map<long, SChapterInfos *> m_chapters;
  long m_currentChapter;
  IAMExtendedSeeking* m_pIAMExtendedSeeking;
  IBaseFilter* m_pSplitter;
  CDSGraph *m_pGraph;

  bool m_init;
};

