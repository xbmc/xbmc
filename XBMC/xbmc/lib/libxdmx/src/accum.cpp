#include "common.h"
#include "accum.h"
#include <string.h>

CPayloadAccumulator::CPayloadAccumulator() :
  m_pData(NULL),
  m_DataLen(0),
  m_PayloadLen(0),
  m_BufferLen(0),
  m_Unbounded(false),
  m_MaxLen(0)
{
  
}

CPayloadAccumulator::~CPayloadAccumulator()
{
  FreeBuffer(m_pData);
}

void CPayloadAccumulator::StartPayload(unsigned int len)
{
  m_DataLen = 0;
  m_PayloadLen = len;
  m_Unbounded = false;
  if (m_BufferLen < m_PayloadLen)
  {
    FreeBuffer(m_pData);
    m_pData = CreateBuffer(m_PayloadLen);
    m_BufferLen = m_PayloadLen;
  }
}

void CPayloadAccumulator::StartPayloadUnbounded()
{
  m_DataLen = 0;
  m_PayloadLen = 0;
  m_Unbounded = true;
  if (m_BufferLen < 65535)
  {
    FreeBuffer(m_pData);
    m_pData = CreateBuffer(65535);
    m_BufferLen = 65535;
  }
}

bool CPayloadAccumulator::AddData(unsigned char* pData, unsigned int* bytes)
{
  unsigned int inLen = *bytes;
  if (m_Unbounded)
  {
    // Dynamically grow the buffer
    if (m_DataLen + inLen > m_BufferLen)
    {
      unsigned int newLen = 2 * m_BufferLen;
      unsigned char* pNewBuffer = CreateBuffer(newLen);
      memcpy(pNewBuffer, m_pData, m_DataLen);
      FreeBuffer(m_pData);
      m_pData = pNewBuffer;
      m_BufferLen = newLen;
    }
  }
  else if ((m_PayloadLen - m_DataLen) < inLen)
    inLen = m_PayloadLen - m_DataLen;

  if (inLen)
  {
    memcpy(&m_pData[m_DataLen], pData, inLen);
    m_DataLen += inLen;
    *bytes -= inLen;
  }

  return (m_DataLen == m_PayloadLen);
}

unsigned int CPayloadAccumulator::GetLen()
{
  return m_DataLen;
}

unsigned int CPayloadAccumulator::GetPayloadLen()
{
  return m_PayloadLen;
}

void CPayloadAccumulator::Reset()
{
  m_DataLen = 0;
  m_PayloadLen = 0;
  m_Unbounded = false;
}

unsigned char* CPayloadAccumulator::GetData(unsigned int len /*= 0*/)
{
  if (m_DataLen < len)
    return NULL;

  return m_pData;
}

bool CPayloadAccumulator::IsUnbounded()
{
  return m_Unbounded;
}

unsigned char* CPayloadAccumulator::Detach(bool release /*= false*/)
{
  Reset();
  unsigned char* pData = m_pData;
  if (m_BufferLen > 65535)
    m_BufferLen = 65535;
  m_pData = CreateBuffer(m_BufferLen);
  if (release)
  {
    FreeBuffer(pData);
    return NULL;
  }
  return pData;
}

unsigned char* CPayloadAccumulator::CreateBuffer(unsigned int len)
{
  unsigned char* pData = (unsigned char*)xdmx_aligned_malloc(len, 16);
  if (len > m_MaxLen)
    m_MaxLen = len;
  return pData;
}

void CPayloadAccumulator::FreeBuffer(unsigned char* pBuf)
{
  xdmx_aligned_free(pBuf);
}
