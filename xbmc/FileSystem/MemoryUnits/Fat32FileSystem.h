#pragma once

#include "IFileSystem.h"
#include "dosfs.h"

namespace XFILE
{
  class CFat32FileSystem : public IFileSystem
  {
    enum OPEN_STATE { CLOSED = 0, OPEN_FOR_READ, OPEN_FOR_WRITE };

  public:
    CFat32FileSystem(unsigned char unit);

    virtual bool Open(const CStdString &file);
    virtual bool OpenForWrite(const CStdString &file, bool overWrite);
    unsigned int Read(void *buffer, __int64 size);
    unsigned int Write(const void *buffer, __int64 size);
    virtual __int64 Seek(__int64 iFilePosition);
    virtual void Close();
    virtual __int64 GetLength();
    virtual __int64 GetPosition();
    virtual bool GetDirectory(const CStdString &directory, CFileItemList &items);
    virtual bool Delete(const CStdString &file);
    virtual bool Rename(const CStdString &oldFile, const CStdString &newFile);
    virtual bool MakeDir(const CStdString &path);
    virtual bool RemoveDir(const CStdString &path);
  protected:
    bool GetDirectoryWithShortPaths(const CStdString &directory, CFileItemList &items);
    bool GetShortFilePath(const CStdString &longPath, CStdString &shortPath);
    FILEINFO m_file;
    OPEN_STATE m_opened;
  };
};
