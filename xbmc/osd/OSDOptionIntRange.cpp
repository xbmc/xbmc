
#include ".\OSDOptionIntRange.h"

#include "localizestrings.h"
#include "guifont.h"
#include "guifontmanager.h"
using namespace OSD;
COSDOptionIntRange::COSDOptionIntRange(int iAction,int iHeading)
{
	m_iMin=0;
  m_iMax=10;
  m_iValue=0;
  m_iInterval=1;  
  m_iHeading=iHeading;
  m_iAction=iAction;
}

COSDOptionIntRange::COSDOptionIntRange(int iAction,int iHeading,int iStart, int iEnd, int iInterval, int iValue)
{
	m_iMin=iStart;
  m_iMax=iEnd;
  m_iValue=iValue;
  m_iInterval=iInterval;  
  m_iHeading=iHeading;
  m_iAction=iAction;
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
  m_iHeading=option.m_iHeading;
  m_iAction=option.m_iAction;
	return *this;
}

COSDOptionIntRange::~COSDOptionIntRange(void)
{
}

IOSDOption* COSDOptionIntRange::Clone() const
{
	return new COSDOptionIntRange(*this);
}


void COSDOptionIntRange::Draw(int x, int y, bool bFocus,bool bSelected)
{
  DWORD dwColor=0xff999999;
  if (bFocus)
    dwColor=0xffffffff;
  CGUIFont* pFont13=g_fontManager.GetFont("font13");
  if (pFont13)
  {
    wstring strHeading=g_localizeStrings.Get(m_iHeading);
    pFont13->DrawShadowText( (float)x,(float)y, dwColor,
                              strHeading.c_str(), 0,
                              0, 
                              5, 
                              5,
                              0xFF020202);
  }
}

bool COSDOptionIntRange::OnAction(IExecutor& executor, const CAction& action)
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