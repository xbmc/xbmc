#pragma once
#ifdef HAS_GMYTH

#include "IDirectory.h"
#include "IFile.h"

namespace DIRECTORY
{


class CGMythDirectory
  : public IDirectory
{
public:
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
};

}
typedef struct _GMythLiveTV GMythLiveTV;
typedef struct _GMythFile GMythFile;
typedef struct _GByteArray GByteArray;

namespace XFILE
{

class CGMythFile : public IFile  
{
public:
  CGMythFile();
  virtual ~CGMythFile();
  virtual bool          Open(const CURL& url, bool binary = true);
  virtual bool          Exists(const CURL& url);
  virtual __int64       Seek(__int64 pos, int whence=SEEK_SET);
  virtual __int64       GetPosition();
  virtual __int64       GetLength();
  virtual int           Stat(const CURL& url, struct __stat64* buffer) { return -1; }
  virtual void          Close();
  virtual unsigned int  Read(void* buffer, __int64 size);
  virtual CStdString    GetContent() { return ""; }
  virtual bool          SkipNext();

  bool                  GetVideoInfoTag(CVideoInfoTag& tag);
  int                   GetTotalTime();
  int                   GetTime();
protected:
    GMythLiveTV    *m_livetv;
    GMythFile      *m_file;
    GByteArray     *m_array;
    unsigned int    m_used;
    char*           m_filename;
    char*           m_channel;
};

}

#endif
