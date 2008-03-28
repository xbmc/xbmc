#pragma once

#include "../DVDDemuxers/DVDDemux.h"

class CDVDOverlayCodecFFmpeg;
class CDVDInputStream;
class CDVDDemuxFFmpeg;

class CDVDDemuxVobsub : public CDVDDemux
{
public:
  CDVDDemuxVobsub();
  virtual ~CDVDDemuxVobsub();
  
  virtual bool          Open(const std::string& filename);
  virtual void          Reset();
  virtual void          Abort() {};
  virtual void          Flush();
  virtual DemuxPacket*  Read();
  virtual bool          SeekTime(int time, bool backwords, double* startpts = NULL);
  virtual void          SetSpeed(int speed) {}
  virtual CDemuxStream* GetStream(int index) { return m_Streams[index]; }
  virtual int           GetNrOfStreams()     { return m_Streams.size(); }
  virtual int           GetStreamLenght()    { return 0; }
  virtual std::string   GetFileName()        { return m_Filename; }

private:
  class CStream 
    : public CDemuxStreamSubtitle
  {
  public:
    CStream(CDVDDemuxVobsub* parent)
      : m_parent(parent)
    {}
    virtual void      SetDiscard(AVDiscard discard);
    virtual AVDiscard GetDiscard()                  { return m_discard; }

    AVDiscard        m_discard;
    CDVDDemuxVobsub* m_parent;
  };

  typedef struct STimestamp
  {
    __int64 pos;
    double  pts;
    int     id;
  } STimestamp;

  std::string                        m_Filename;
  std::auto_ptr<CDVDInputStream>     m_Input;
  std::auto_ptr<CDVDDemuxFFmpeg>     m_Demuxer;
  std::vector<STimestamp>            m_Timestamps;
  std::vector<STimestamp>::iterator  m_Timestamp;
  std::vector<CStream*> m_Streams;

  int m_width;
  int m_height;
  int m_left;
  int m_top;

  typedef struct SState
  {
    int         id;
    double      delay;
    std::string extra;
  } SState;

  struct sorter
  {
    bool operator()(const STimestamp &p1, const STimestamp &p2)
    {
      return p1.pts < p2.pts || (p1.pts == p2.pts && p1.id < p2.id);
    }
  };

  bool ParseLangIdx(SState& state, char* line);
  bool ParseDelay(SState& state, char* line);
  bool ParseId(SState& state, char* line);
  bool ParseExtra(SState& state, char* line);
  bool ParseTimestamp(SState& state, char* line);
};
