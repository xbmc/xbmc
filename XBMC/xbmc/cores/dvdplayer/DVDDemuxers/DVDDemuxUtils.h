
#pragma once

#include "DVDDemux.h"

// forward declarations
void fast_memcpy(void* d, const void* s, unsigned n);
void fast_memset(void* d, int c, unsigned int n);

class CDVDDemuxUtils
{
public:
  static void FreeDemuxPacket(CDVDDemux::DemuxPacket* pPacket);
  static CDVDDemux::DemuxPacket* AllocateDemuxPacket(int iDataSize = 0);
};

template<typename _Ty>
class CMovingAverage
{
public:
  CMovingAverage(int iSize, _Ty InitalValue)
  {
    m_iSize = iSize;
    m_iPos = 0;

    m_pData = new _Ty[iSize];
    Reset(InitalValue);    
  }
  ~CMovingAverage()
  {
    delete[] m_pData;
  }

  void AddValue(_Ty Value)
  {    
    m_pData[m_iPos] = Value;
    m_iPos = (m_iPos  + 1) % m_iSize;
  }

  void Reset( _Ty Value )
  {
    for( int i = 0; i<m_iSize; i++)
    {
      m_pData[i] = Value;
    }
    m_iPos = 0;
  }

  _Ty GetAverage()
  {
    _Ty Value = 0;

    for( int i = 0; i<m_iSize; i++)
    {
      Value += m_pData[i];
    }
    return Value / m_iSize;
  }

  operator _Ty()
  {
    return GetAverage();
  }

private:
  _Ty *m_pData;
  int m_iSize;
  int m_iPos;
};
