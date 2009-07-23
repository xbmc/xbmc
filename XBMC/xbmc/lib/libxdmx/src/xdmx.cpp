#include "common.h"
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
XdmxLogFuncPtr g_xdmxlog = printf;
int g_xdmxlevel = XDMX_LOG_LEVEL_ERROR;

void XdmxSetLogFunc(XdmxLogFuncPtr func)
{
  if (func)
    g_xdmxlog = func;
}

void XdmxSetLogLevel(int level)
{
  g_xdmxlevel = level;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__GNUC__)
#define ALIGN(value, alignment) (((value)+(alignment-1))&~(alignment-1))
void* xdmx_aligned_malloc(size_t s, size_t boundary)
{
  char *pFull = (char*)malloc(s + boundary + sizeof(char *));
  char *pAlligned = (char *)ALIGN(((unsigned long)pFull + sizeof(char *)), boundary);

  *(char **)(pAlligned - sizeof(char*)) = pFull;

  return(pAlligned);
}

void xdmx_aligned_free(void *p)
{
  if (!p)
    return;

  char *pFull = *(char **)(((char *)p) - sizeof(char *));
  free(pFull);
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////

CElementaryStream::CElementaryStream(unsigned int id, unsigned char elementType) :
  m_Id(id),
  m_ElementType(elementType)
{
}

CElementaryStream::~CElementaryStream()
{
}

unsigned char CElementaryStream::GetElementType()
{
  return m_ElementType;
}

unsigned int CElementaryStream::GetId()
{
  return m_Id;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

CParserPayload::CParserPayload(CElementaryStream* pStream) :
  m_pStream(pStream),
  m_pData(NULL),
  m_Size(0),
  m_Pts(0),
  m_Dts(0)
{

}

CParserPayload::CParserPayload(CElementaryStream* pStream, unsigned char* pData, unsigned int size) :
  m_pStream(pStream),
  m_pData(NULL),
  m_Size(0),
  m_Pts(0),
  m_Dts(0)
{
  Attach(pData, size);
}

CParserPayload::~CParserPayload()
{
  delete[] m_pData;
}

void CParserPayload::Attach(unsigned char* pData, unsigned int size)
{
  if (m_pData)
    delete m_pData;
  m_pData = pData;
  m_Size = size;
}

unsigned char* CParserPayload::GetData(unsigned int size /*= 0*/)
{
  if (m_Size < size)
    return NULL;
  return m_pData;
}

unsigned char* CParserPayload::Detach()
{
  unsigned char* pData = m_pData;
  m_pData = NULL;
  m_Size = 0;
  return pData;
}

unsigned int CParserPayload::GetSize()
{
  return m_Size;
}

void CParserPayload::SetPts(double pts)
{
  m_Pts = pts;
}

void CParserPayload::SetDts(double dts)
{
  m_Dts = dts;
}

double CParserPayload::GetPts()
{
  return m_Pts;
}

double CParserPayload::GetDts()
{
  return m_Dts;
}

CElementaryStream* CParserPayload::GetStream()
{
  return m_pStream;
}
