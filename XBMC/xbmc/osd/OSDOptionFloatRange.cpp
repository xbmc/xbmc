
#include ".\OSDOptionFloatRange.h"

#include "localizestrings.h"
#include "guifont.h"
#include "guifontmanager.h"
using namespace OSD;
COSDOptionFloatRange::COSDOptionFloatRange(int iAction, int iHeading)
{
	m_fMin=0.0f;
  m_fMax=1.0f;
  m_fValue=0.0f;
  m_fInterval=0.1f;
  m_iHeading=iHeading;
  m_iAction=iAction;
}

COSDOptionFloatRange::COSDOptionFloatRange(int iAction, int iHeading,float fStart, float fEnd, float fInterval, float fValue)
{
	m_fMin=fStart;
  m_fMax=fEnd;
  m_fValue=fValue;
  m_fInterval=fInterval;
  m_iHeading=iHeading;
  m_iAction=iAction;
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
  m_iHeading=option.m_iHeading;
  m_iAction=option.m_iAction;
	return *this;
}

COSDOptionFloatRange::~COSDOptionFloatRange(void)
{
}

IOSDOption* COSDOptionFloatRange::Clone() const
{
	return new COSDOptionFloatRange(*this);
}


void COSDOptionFloatRange::Draw(int x, int y, bool bFocus,bool bSelected)
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

bool COSDOptionFloatRange::OnAction(IExecutor& executor, const CAction& action)
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