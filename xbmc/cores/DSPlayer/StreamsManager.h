#pragma once

#include "dshowutil/dshowutil.h"
#include "DShowUtil/DshowCommon.h"
#include <initguid.h>
#include "moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>
#include "DShowUtil/MediaTypeEx.h"

#include "DSGraph.h"
#include "log.h"
#include "CharsetConverter.h"
#include "RegExp.h"

struct SStreamInfos
{
  CStdString name;
  CStdString codecname;
  DWORD flags;
  IUnknown *pObj; // Output pin of the splitter
  IUnknown *pUnk; // Input pin of the filter
  LCID  lcid;
  DWORD group;

  virtual void Clear()
  {
    flags = 0;
    pObj = 0;
    pUnk = 0;
    lcid = 0;
    group = 0;
    name = "";
    codecname = "";
  }
};

struct SAudioStreamInfos: SStreamInfos
{
  unsigned int channels;
  unsigned int bitrate;
  unsigned int samplerate;

  virtual void Clear()
  {
    SStreamInfos::Clear();

    channels = 0;
    bitrate = 0;
    samplerate = 0;
  }
};

struct SVideoStreamInfos: SStreamInfos
{
  unsigned int width;
  unsigned int height;

  virtual void Clear()
  {
    SStreamInfos::Clear();

    width = 0;
    height = 0;
  }
};

class CStreamsManager
{
public:
  static CStreamsManager *m_pSingleton;
  static CStreamsManager *getSingleton();
  static void Destroy();

  std::map<long, SAudioStreamInfos *> Get();
  int  GetAudioStreamCount();
  int  GetAudioStream();
  void GetAudioStreamName(int iStream, CStdString &strStreamName);
  void SetAudioStream(int iStream);
  void LoadStreams();
  IAMStreamSelect *GetStreamSelector() { return m_pIAMStreamSelect; }
  bool IsChangingAudioStream();

  int GetChannels();
  int GetBitsPerSample();
  int GetSampleRate();

  int GetPictureWidth();
  int GetPictureHeight();

  CStdString GetAudioCodecName();
  CStdString GetVideoCodecName();

  bool InitManager(IBaseFilter *Splitter, IFilterGraph2 *graphBuilder, CDSGraph *DSGraph);

private:
  CStreamsManager(void);
  ~CStreamsManager(void);

  void GetStreamInfos(AM_MEDIA_TYPE *mt, SStreamInfos *s);
  int InternalGetAudioStream();

  std::map<long, SAudioStreamInfos *> m_audioStreams;
  IAMStreamSelect *m_pIAMStreamSelect;
  IBaseFilter* m_pSplitter;
  IFilterGraph2* m_pGraphBuilder;
  CDSGraph* m_pGraph;

  bool m_init;
  bool m_bChangingAudioStream;

  SVideoStreamInfos m_videoStream;

};

