#pragma once
#ifdef HAS_GMYTH

#include "IFile.h"

typedef struct _GMythBackendInfo GMythBackendInfo;
typedef struct _GMythLiveTV GMythLiveTV;
typedef struct _GMythFileTransfer GMythFileTransfer;
typedef struct _GByteArray GByteArray;
typedef struct _GMythProgramInfo GMythProgramInfo;
typedef struct _GMythScheduler GMythScheduler;

namespace XFILE
{

class CGMythFile : public IFile  
{
public:
  CGMythFile();
  virtual ~CGMythFile();
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

  bool                  NextChannel();
  bool                  PrevChannel();

  CVideoInfoTag*        GetVideoInfoTag();
  int                   GetTotalTime();
  int                   GetStartTime();
protected:
  bool SetupInfo(const CURL& url);
  bool SetupTransfer();
  bool SetupRecording(const CURL& url, const CStdString &base);

  GMythBackendInfo  *m_info;
  GMythLiveTV       *m_livetv;
  GMythFileTransfer *m_file;
  GByteArray        *m_array;
  GMythScheduler    *m_scheduler;
  void              *m_recording;
  unsigned int       m_used;
  char*              m_filename;
  char*              m_channel;
  bool               m_held;
  CVideoInfoTag      m_infotag;  
};

}

#endif
