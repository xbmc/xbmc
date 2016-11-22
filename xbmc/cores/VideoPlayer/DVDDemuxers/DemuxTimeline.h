#pragma once

#include "DVDDemux.h"

#include <list>
#include <map>
#include <memory>
#include <vector>

class CDemuxTimeline : public CDVDDemux
{

public:
  CDemuxTimeline();
  virtual ~CDemuxTimeline();

  static CDemuxTimeline* CreateTimeline(CDVDDemux *primaryDemuxer);
  static CDemuxTimeline* CreateTimelineFromEbml(CDVDDemux *primaryDemuxer);
  static CDemuxTimeline* CreateTimelineFromMatroskaParser(CDVDDemux *primaryDemuxer);

  void Reset() override;
  void Abort() override;
  void Flush() override;

  DemuxPacket* Read() override;

  bool SeekTime(double time, bool backwords = false, double* startpts = NULL) override;
  bool SeekChapter(int chapter, double* startpts = NULL) override;

  int GetChapterCount() override;
  int GetChapter() override;
  void GetChapterName(std::string& strChapterName, int chapterIdx=-1) override;
  int64_t GetChapterPos(int chapterIdx=-1) override;

  void SetSpeed(int iSpeed) override;
  int GetStreamLength() override;
  std::string GetFileName() override;

  int GetNrOfStreams() const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  CDemuxStream* GetStream(int iStreamId) const override;
  std::string GetStreamCodecName(int iStreamId) override;
  void EnableStream(int id, bool enable) override;

private:
  bool SwitchToNextDemuxer();

  struct DemuxerInfo {};

  struct ChapterInfo
  {
    CDVDDemux *demuxer;
    int startSrcTime;  // in MSEC
    int startDispTime; // in MSEC
    int duration; // in MSEC
    int stopSrcTime() {return startSrcTime + duration;}
    int stopDispTime() {return startDispTime + duration;}
    int shiftTime() {return startDispTime - startSrcTime;}
    size_t index;
    std::string title;

    ChapterInfo(CDVDDemux *demuxer = nullptr,
      int startSrcTime = 0, int startDispTime = 0,
      int duration = 0, size_t index = 0,
      std::string title = std::string()
    ) :
      demuxer(demuxer),
      startSrcTime(startSrcTime), startDispTime(startDispTime),
      duration(duration), index(index),
      title(title)
    {}
  };

  CDVDDemux *m_primaryDemuxer;

  std::list<std::shared_ptr<CDVDDemux>> m_demuxers;
  std::list<std::shared_ptr<CDVDInputStream>> m_inputStreams;

  std::vector<ChapterInfo> m_chapters;
  std::map<int,ChapterInfo*> m_chapterMap;  // maps chapter end display time in msec to chapter info

  ChapterInfo *m_curChapter;
};

// vim: ts=2 sw=2 expandtab
