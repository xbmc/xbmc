#pragma once

#include "lib/libMP4/mp4.h"
#include "lib/libMP4/mp4common.h"
#include "lib/libMP4/mp4file.h"

namespace MUSIC_INFO
{
// Only reading supported
class XMP4File : public MP4File
{
public:
  XMP4File(u_int32_t verbosity = 0);
  ~XMP4File();
  virtual void Close();
  virtual u_int32_t ReadBytes(u_int8_t* pBytes, u_int32_t numBytes, FILE* pFile = NULL);
  virtual void WriteBytes(u_int8_t* pBytes, u_int32_t numBytes, FILE* pFile = NULL);
  virtual void Open(const char* fmode);
  virtual void SetPosition(u_int64_t pos, FILE* pFile = NULL);
  virtual u_int64_t GetPosition(FILE* pFile = NULL);
};
};
