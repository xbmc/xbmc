
#include "../stdafx.h"
#include ".\osdoptionboolean.h"
#include "guifontmanager.h"

using namespace OSD;


COSDOptionBoolean::COSDOptionBoolean(int iAction,int iHeading)
:m_image(0, 1, 0, 0, 0, 0, "check-box.png","check-boxNF.png",16,16)
{
	m_bValue=false;
  m_iHeading=iHeading;
  m_iAction=iAction;
  m_image.SetShadow(true);
}

COSDOptionBoolean::COSDOptionBoolean(int iAction,int iHeading,bool bValue)
:m_image(0, 1, 0, 0, 0, 0, "check-box.png","check-boxNF.png",16,16)
{
  m_iHeading=iHeading;
	m_bValue=bValue;
  m_iAction=iAction;
  m_image.SetShadow(true);
}

COSDOptionBoolean::COSDOptionBoolean(const COSDOptionBoolean& option)
:m_image(0, 1, 0, 0, 0, 0, "check-box.png","check-boxNF.png",16,16)
{
	*this=option;
  m_image.SetShadow(true);
}

const OSD::COSDOptionBoolean& COSDOptionBoolean::operator = (const COSDOptionBoolean& option)
{
	if (this==&option) return *this;
	m_bValue=option.m_bValue;
  m_iHeading=option.m_iHeading;
  m_iAction=option.m_iAction;
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
                              2, 
                              2,
                              0xFF020202);
  }

  if (bSelected)
  {
    m_image.SetPosition(x+200,y);
    m_image.AllocResources();
    m_image.SetSelected(m_bValue);
    m_image.Render();
    m_image.FreeResources();
  }
}

bool COSDOptionBoolean::OnAction(IExecutor& executor, const CAction& action)
{
	if (action.wID==ACTION_OSD_SHOW_SELECT)
	{
		m_bValue=!m_bValue;
    executor.OnExecute(m_iAction,this);
		return true;
	}
  return false;
}

bool COSDOptionBoolean::GetValue() const
{
  return m_bValue;
}