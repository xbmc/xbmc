
#include ".\OSDOptionFloatRange.h"
using namespace OSD;
COSDOptionFloatRange::COSDOptionFloatRange(void)
{
	m_fMin=0.0f;
  m_fMax=1.0f;
  m_fValue=0.0f;
  m_fInterval=0.1f;
}

COSDOptionFloatRange::COSDOptionFloatRange(float fStart, float fEnd, float fInterval, float fValue)
{
	m_fMin=fStart;
  m_fMax=fEnd;
  m_fValue=fValue;
  m_fInterval=fInterval;
}

COSDOptionFloatRange::COSDOptionFloatRange(const COSDOptionFloatRange& option)
{
	*this=option;
}

const OSD::COSDOptionFloatRange& COSDOptionFloatRange::operator = (const COSDOptionFloatRange& option)
{
	if (this==&option) return *this;

	m_fMin=option.m_fMin;
  m_fMax=option.m_fMax;
  m_fValue=option.m_fValue;
  m_fInterval=option.m_fInterval;
	return *this;
}

COSDOptionFloatRange::~COSDOptionFloatRange(void)
{
}

IOSDOption* COSDOptionFloatRange::Clone() const
{
	return new COSDOptionFloatRange(*this);
}


void COSDOptionFloatRange::Draw(int x, int y, bool bFocus)
{
}

bool COSDOptionFloatRange::OnAction(const CAction& action)
{
	if (action.wID==ACTION_MOVE_UP)
	{
    if (m_fValue+m_fInterval <=m_fMax)
    {
      m_fValue+=m_fInterval ;
    }
    else
    {
      m_fValue=m_fMin;
    }
    return true;
	}
	if (action.wID==ACTION_MOVE_DOWN)
	{
    if (m_fValue-m_fInterval >=m_fMin)
    {
      m_fValue-=m_fInterval ;
    }
    else
    {
      m_fValue=m_fMax;
    }
		return true;
	}
  return false;
}