#pragma once

#include "IFile.h"
#include "ILiveTV.h"

typedef struct cmyth_ringbuf  *cmyth_ringbuf_t;
typedef struct cmyth_conn     *cmyth_conn_t;
typedef struct cmyth_recorder *cmyth_recorder_t;
typedef struct cmyth_proginfo *cmyth_proginfo_t;
typedef struct cmyth_proglist *cmyth_proglist_t;
typedef struct cmyth_file     *cmyth_file_t;

class DllLibCMyth;

namespace XFILE
{

class CCMythFile 
  : public IFile
  ,        ILiveTVInterface
{
public:
  CCMythFile();
  virtual ~CCMythFile();
  virtual bool          Open(const CURL& url, bool binary = true);
  virtual __int64       Seek(__int64 pos, int whence=SEEK_SET);
  virtual __int64       GetPosition();
  virtual __int64       GetLength();
  virtual int           Stat(const CURL& url, struct __stat64* buffer) { return -1; }
  virtual void          Close();
  virtual unsigned int  Read(void* buffer, __int64 size);
  virtual CStdString    GetContent() { return ""; }
  virtual bool          SkipNext();

  virtual bool          Delete(const CURL& url);
  virtual bool          Exists(const CURL& url);

  virtual ILiveTVInterface* GetLiveTV() {return (ILiveTVInterface*)this;}

  virtual bool           NextChannel();
  virtual bool           PrevChannel();

  virtual int            GetTotalTime();
  virtual int            GetStartTime();

  virtual CVideoInfoTag* GetVideoInfoTag();
protected:
  bool HandleEvents();
  bool ChangeChannel(int direction, const char* channel);

  bool SetupConnection(const CURL& url);
  bool SetupRecording(const CURL& url);
  bool SetupLiveTV(const CURL& url);

  DllLibCMyth     *m_dll;

  cmyth_conn_t      m_control;
  cmyth_conn_t      m_event;
  cmyth_recorder_t  m_recorder;
  cmyth_proginfo_t  m_program;
  cmyth_proglist_t  m_programlist;
  cmyth_file_t      m_file;
  CStdString        m_filename;
  int               m_remain;
  CVideoInfoTag     m_infotag;
};

}
