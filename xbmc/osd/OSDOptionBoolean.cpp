
#include ".\osdoptionboolean.h"

#include "localizestrings.h"
#include "guifont.h"
#include "guifontmanager.h"

using namespace OSD;
COSDOptionBoolean::COSDOptionBoolean(int iHeading)
:m_image(0, 1, 0, 0, 0, 0, "check-box.png",0)
{
	m_bValue=false;
  m_iHeading=iHeading;
}

COSDOptionBoolean::COSDOptionBoolean(int iHeading,bool bValue)
:m_image(0, 1, 0, 0, 0, 0, "check-box.png",0)
{
  iHeading=iHeading;
	m_bValue=bValue;
}

COSDOptionBoolean::COSDOptionBoolean(const COSDOptionBoolean& option)
:m_image(0, 1, 0, 0, 0, 0, "check-box.png",0)
{
	*this=option;
}

const OSD::COSDOptionBoolean& COSDOptionBoolean::operator = (const COSDOptionBoolean& option)
{
	if (this==&option) return *this;
	m_bValue=option.m_bValue;
  m_iHeading=option.m_iHeading;
	return *this;
}

COSDOptionBoolean::~COSDOptionBoolean(void)
{
}

IOSDOption* COSDOptionBoolean::Clone() const
{
	return new COSDOptionBoolean(*this);
}


void COSDOptionBoolean::Draw(int x, int y, bool bFocus,bool bSelected)
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

  if (m_bValue && bSelected)
  {
    m_image.SetPosition(x+200,y);
    m_image.AllocResources();
    m_image.Render();
    m_image.FreeResources();
  }
}

bool COSDOptionBoolean::OnAction(const CAction& action)
{
	if (action.wID==ACTION_SELECT_ITEM)
	{
		m_bValue=!m_bValue;
		return true;
	}
  return false;
}