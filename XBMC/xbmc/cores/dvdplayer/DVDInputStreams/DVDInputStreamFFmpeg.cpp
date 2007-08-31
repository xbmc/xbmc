
#include "stdafx.h"
#include "DVDInputStreamFFmpeg.h"

using namespace XFILE;

CDVDInputStreamFFmpeg::CDVDInputStreamFFmpeg() 
  : CDVDInputStream(DVDSTREAM_TYPE_FFMPEG)
{

}

CDVDInputStreamFFmpeg::~CDVDInputStreamFFmpeg()
{
  Close();  
}

bool CDVDInputStreamFFmpeg::IsEOF()
{
  return false;
}

bool CDVDInputStreamFFmpeg::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content)) 
    return false;

  return true;
}

// close file and reset everyting
void CDVDInputStreamFFmpeg::Close()
{
  CDVDInputStream::Close();
}

int CDVDInputStreamFFmpeg::Read(BYTE* buf, int buf_size)
{
  return -1;
}

__int64 CDVDInputStreamFFmpeg::GetLength()
{
  return 0;
}

__int64 CDVDInputStreamFFmpeg::Seek(__int64 offset, int whence)
{
  return -1;
}

