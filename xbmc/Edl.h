#ifndef CEDL_H
#define CEDL_H

#include "StdString.h"
#include <vector>

#define CACHED_EDL_FILENAME "Z:\\xbmc.edl"

#define COMSKIPSTR "FILE PROCESSING COMPLETE"
#define VRSTR "<Version>2"
#define VRCUT "<Cut>"
#define VRSCENE "<SceneMarker "
#define BTVSTR "<cutlist>"
#define BTVCUT "<Region><start"
#define BTVSTREND "</cutlist>"


typedef enum 
  {
    CUT = 0,
    MUTE = 1,
    SCENE = 2,
  } Action;  

struct Cut
  {
    double CutStart;
    double CutEnd;
    Action CutAction;
  };

class CEdl
{
public:
  CEdl();
  virtual ~CEdl(void);

  bool ReadnCacheAny(const CStdString& strMovie);
  bool ReadEdl();
  bool ReadComskip();
  bool ReadVideoRedo();
  bool ReadBeyondTV();
  //bool ReadVDR();
  void Reset();
  
  bool AddCutpoint(const Cut& NewCut);
  bool AddScene(const Cut& NewCut);

  void SetMovie(const CStdString& strMovie);
  bool CacheEdl();
  CStdString GetCachedEdl();

  bool HaveCutpoints();
  bool HaveScenes();
  char GetEdlStatus();

  bool IsCached();
  bool InCutpoint(double dCurSeek, Cut *pCurCut = NULL);

  void CompensateSeek(bool bPlus,int *iSeek);
  bool SeekScene(bool bPlus,__int64 *iScenemarker);

  void AddBookmark(double dSceneMarker);
  
protected:
private:
  CStdString m_strCachedEdl;
  CStdString m_strMovie;
  CStdString m_strEdlFilename;
  bool m_bCutpoints;
  bool m_bCached;
  bool m_bScenes;
  char m_szBuffer[1024]; // Buffer for file reading
  vector<Cut> m_vecCutlist;
  vector<double> m_vecScenelist;
};

#endif // CEDL_H
