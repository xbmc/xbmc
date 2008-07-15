#include "All.h"
#include "CircleBuffer.h"

CCircleBuffer::CCircleBuffer()
{
    m_pBuffer = NULL;
    m_nTotal = 0;
    m_nHead = 0;
    m_nTail = 0;
    m_nEndCap = 0;
    m_nMaxDirectWriteBytes = 0;
}

CCircleBuffer::~CCircleBuffer()
{
    SAFE_ARRAY_DELETE(m_pBuffer)
}

void CCircleBuffer::CreateBuffer(int nBytes, int nMaxDirectWriteBytes)
{
    SAFE_ARRAY_DELETE(m_pBuffer)
    
    m_nMaxDirectWriteBytes = nMaxDirectWriteBytes;
    m_nTotal = nBytes + 1 + nMaxDirectWriteBytes;
    m_pBuffer = new unsigned char [m_nTotal];
    m_nHead = 0;
    m_nTail = 0;
    m_nEndCap = m_nTotal;
}

int CCircleBuffer::MaxAdd()
{
    int nMaxAdd = (m_nTail >= m_nHead) ? (m_nTotal - 1 - m_nMaxDirectWriteBytes) - (m_nTail - m_nHead) : m_nHead - m_nTail - 1;
    return nMaxAdd;
}

int CCircleBuffer::MaxGet()
{
    return (m_nTail >= m_nHead) ? m_nTail - m_nHead : (m_nEndCap - m_nHead) + m_nTail;
}

int CCircleBuffer::Get(unsigned char * pBuffer, int nBytes)
{
    int nTotalGetBytes = 0;

    if (pBuffer != NULL && nBytes > 0)
    {
        int nHeadBytes = min(m_nEndCap - m_nHead, nBytes);
        int nFrontBytes = nBytes - nHeadBytes;

        memcpy(&pBuffer[0], &m_pBuffer[m_nHead], nHeadBytes);
        nTotalGetBytes = nHeadBytes;

        if (nFrontBytes > 0)
        {
            memcpy(&pBuffer[nHeadBytes], &m_pBuffer[0], nFrontBytes);
            nTotalGetBytes += nFrontBytes;
        }

        RemoveHead(nBytes);
    }

    return nTotalGetBytes;
}

void CCircleBuffer::Empty()
{
    m_nHead = 0;
    m_nTail = 0;
    m_nEndCap = m_nTotal;
}

int CCircleBuffer::RemoveHead(int nBytes)
{
    nBytes = min(MaxGet(), nBytes);
    m_nHead += nBytes;
    if (m_nHead >= m_nEndCap)
        m_nHead -= m_nEndCap;
    return nBytes;
}

int CCircleBuffer::RemoveTail(int nBytes)
{
    nBytes = min(MaxGet(), nBytes);
    m_nTail -= nBytes;
    if (m_nTail < 0)
        m_nTail += m_nEndCap;
    return nBytes;
}
