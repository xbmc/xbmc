
#include ".\OSDOptionIntRange.h"
using namespace OSD;
COSDOptionIntRange::COSDOptionIntRange(void)
{
	m_iMin=0;
  m_iMax=10;
  m_iValue=0;
  m_iInterval=1;
}

COSDOptionIntRange::COSDOptionIntRange(int iStart, int iEnd, int iInterval, int iValue)
{
	m_iMin=iStart;
  m_iMax=iEnd;
  m_iValue=iValue;
  m_iInterval=iInterval;
}

COSDOptionIntRange::COSDOptionIntRange(const COSDOptionIntRange& option)
{
	*this=option;
}

const OSD::COSDOptionIntRange& COSDOptionIntRange::operator = (const COSDOptionIntRange& option)
{
	if (this==&option) return *this;

	m_iMin=option.m_iMin;
  m_iMax=option.m_iMax;
  m_iValue=option.m_iValue;
  m_iInterval=option.m_iInterval;
	return *this;
}

COSDOptionIntRange::~COSDOptionIntRange(void)
{
}

IOSDOption* COSDOptionIntRange::Clone() const
{
	return new COSDOptionIntRange(*this);
}


void COSDOptionIntRange::Draw(int x, int y, bool bFocus)
{
}

bool COSDOptionIntRange::OnAction(const CAction& action)
{
	if (action.wID==ACTION_MOVE_UP)
	{
    if (m_iValue+m_iInterval <=m_iMax)
    {
      m_iValue+=m_iInterval ;
    }
    else
    {
      m_iValue=m_iMin;
    }
    return true;
	}
	if (action.wID==ACTION_MOVE_DOWN)
	{
    if (m_iValue-m_iInterval >=m_iMin)
    {
      m_iValue-=m_iInterval ;
    }
    else
    {
      m_iValue=m_iMax;
    }
		return true;
	}
  return false;
}