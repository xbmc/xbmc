#pragma once

#include "dshowutil/dshowutil.h"
#include "DShowUtil/DshowCommon.h"
#include <initguid.h>
#include "moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>

#include "DSGraph.h"
#include "log.h"
#include "CharsetConverter.h"
#include "RegExp.h"

struct SAudioStreamInfos
{
  CStdString name;
  DWORD flags;
  IUnknown *pObj; // Output pin of the splitter
  IUnknown *pUnk; // Intput pin of the filter
  LCID  lcid;
  DWORD group;
};

class CAudioStreamsManager
{
public:
  static CAudioStreamsManager *m_pSingleton;
  static CAudioStreamsManager *getSingleton();
  static void Destroy();

  std::map<long, SAudioStreamInfos *> Get();
  int  GetAudioStreamCount();
  int  GetAudioStream();
  void GetAudioStreamName(int iStream, CStdString &strStreamName);
  void SetAudioStream(int iStream);
  void LoadAudioStreams();
  IAMStreamSelect *GetStreamSelector() { return m_pIAMStreamSelect; }
  bool IsChangingAudioStream();

  bool InitManager(IBaseFilter *Splitter, IFilterGraph2 *graphBuilder, CDSGraph *DSGraph);

private:
  CAudioStreamsManager(void);
  ~CAudioStreamsManager(void);

  std::map<long, SAudioStreamInfos *> m_audioStreams;
  IAMStreamSelect *m_pIAMStreamSelect;
  IBaseFilter* m_pSplitter;
  IFilterGraph2* m_pGraphBuilder;
  CDSGraph* m_pGraph;

  bool m_init;
  bool m_bChangingAudioStream;

};

