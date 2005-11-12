
#include "..\stdafx.h"
#include "Archive.h"


#define BUFFER_MAX 4096

CArchive::CArchive(CFile* pFile, int mode)
{
  m_pFile = pFile;
  m_iMode = mode;

  m_pBuffer = new BYTE[BUFFER_MAX];
  m_BufferPos = 0;
}

CArchive::~CArchive()
{
  FlushBuffer();
  if (m_pBuffer != NULL)
    delete[] m_pBuffer;

  m_BufferPos = 0;
}

void CArchive::Close()
{
  FlushBuffer();
}

bool CArchive::IsLoading()
{
  return (m_iMode == load);
}

bool CArchive::IsStoring()
{
  return (m_iMode == store);
}

CArchive& CArchive::operator<<(float f)
{
  int size = sizeof(float);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &f, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(int i)
{
  int size = sizeof(int);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &i, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(__int64 i64)
{
  int size = sizeof(__int64);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &i64, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(long l)
{
  int size = sizeof(long);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &l, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(bool b)
{
  int size = sizeof(bool);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &b, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(const CStdString& str)
{
  *this << str.GetLength();

  int size = str.GetLength();
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  int iBufferMaxParts=size/BUFFER_MAX;
  for (int i=0; i<iBufferMaxParts; i++)
  {
    memcpy(&m_pBuffer[m_BufferPos], str.c_str()+(i*BUFFER_MAX), BUFFER_MAX);
    m_BufferPos+=BUFFER_MAX;
    FlushBuffer();
  }

  int iPos=iBufferMaxParts*BUFFER_MAX;
  int iSizeLeft=size-iPos;
  memcpy(&m_pBuffer[m_BufferPos], str.c_str()+iPos, iSizeLeft);
  m_BufferPos+=iSizeLeft;

  return *this;
}

CArchive& CArchive::operator<<(const CStdStringW& str)
{
  *this << str.GetLength();

  int size = str.GetLength();
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  int iBufferMaxParts=size/BUFFER_MAX;
  for (int i=0; i<iBufferMaxParts; ++i)
  {
    memcpy(&m_pBuffer[m_BufferPos], str.c_str()+(i*BUFFER_MAX), BUFFER_MAX);
    m_BufferPos+=BUFFER_MAX;
    FlushBuffer();
  }

  int iPos=iBufferMaxParts*BUFFER_MAX;
  int iSizeLeft=size-iPos;
  memcpy(&m_pBuffer[m_BufferPos], str.c_str()+iPos, iSizeLeft);
  m_BufferPos+=iSizeLeft;

  return *this;
}

CArchive& CArchive::operator<<(const SYSTEMTIME& time)
{
  int size = sizeof(SYSTEMTIME);
  if (m_BufferPos + size >= BUFFER_MAX)
    FlushBuffer();

  memcpy(&m_pBuffer[m_BufferPos], &time, size);
  m_BufferPos += size;

  return *this;
}

CArchive& CArchive::operator<<(ISerializable& obj)
{
  obj.Serialize(*this);

  return *this;
}

CArchive& CArchive::operator>>(float& f)
{
  m_pFile->Read((void*)&f, sizeof(float));

  return *this;
}

CArchive& CArchive::operator>>(int& i)
{
  m_pFile->Read((void*)&i, sizeof(int));

  return *this;
}

CArchive& CArchive::operator>>(__int64& i64)
{
  m_pFile->Read((void*)&i64, sizeof(__int64));

  return *this;
}

CArchive& CArchive::operator>>(long& l)
{
  m_pFile->Read((void*)&l, sizeof(long));

  return *this;
}

CArchive& CArchive::operator>>(bool& b)
{
  m_pFile->Read((void*)&b, sizeof(bool));

  return *this;
}

CArchive& CArchive::operator>>(CStdString& str)
{
  int iLength = 0;
  *this >> iLength;

  m_pFile->Read((void*)str.GetBufferSetLength(iLength), iLength);
  str.ReleaseBuffer();


  return *this;
}

CArchive& CArchive::operator>>(CStdStringW& str)
{
  int iLength = 0;
  *this >> iLength;

  m_pFile->Read((void*)str.GetBufferSetLength(iLength), iLength);
  str.ReleaseBuffer();


  return *this;
}

CArchive& CArchive::operator>>(SYSTEMTIME& time)
{
  m_pFile->Read((void*)&time, sizeof(SYSTEMTIME));

  return *this;
}

CArchive& CArchive::operator>>(ISerializable& obj)
{
  obj.Serialize(*this);

  return *this;
}

void CArchive::FlushBuffer()
{
  if (m_BufferPos > 0)
  {
    m_pFile->Write(m_pBuffer, m_BufferPos);
    m_BufferPos = 0;
  }
}
