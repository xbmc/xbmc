#pragma once

#include "IFileSystem.h"
#include "../FileHD.h"

class CFatXFileSystem : public IFileSystem
{
public:
  CFatXFileSystem(unsigned char unit);

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
  CStdString GetLocal(const CStdString &file);
  XFILE::CFileHD m_file;
};
