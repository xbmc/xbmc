#include ".\osdmenu.h"
using namespace OSD;
COSDMenu::COSDMenu()
{		
	m_iCurrentSubMenu=0;
	m_iXPos=0;
	m_iYPos=0;
}

COSDMenu::COSDMenu(int iXpos, int iYpos)
{		
	m_iCurrentSubMenu=0;
	m_iXPos=iXpos;
	m_iYPos=iYpos;
}

COSDMenu::COSDMenu(const COSDMenu& menu)
{
	*this=menu;
}

COSDMenu::~COSDMenu(void)
{
	Clear();
}
void COSDMenu::Clear()
{
  ivecSubMenus i =m_vecSubMenus.begin();
	while (i != m_vecSubMenus.end())
	{
		COSDSubMenu* pSubMenu=*i;
		delete pSubMenu;
    i=m_vecSubMenus.erase(i);
	}
}


COSDMenu* COSDMenu::Clone()
{
	return new COSDMenu(*this);
}


const COSDMenu& COSDMenu::operator = (const COSDMenu& menu)
{
	if (&menu==this) return *this;
  Clear();
	ivecSubMenus i =m_vecSubMenus.begin();
	while (i != m_vecSubMenus.end())
	{
		COSDSubMenu* pSubMenu=*i;
		m_vecSubMenus.push_back ( pSubMenu->Clone() );
	}

	m_iCurrentSubMenu=menu.m_iCurrentSubMenu;
	m_iXPos=menu.m_iXPos;
	m_iYPos=menu.m_iYPos;
	return *this;
}


void COSDMenu::Draw()
{
	for (int i=0; i < (int)m_vecSubMenus.size(); ++i)
	{
		COSDSubMenu* pSubMenu=m_vecSubMenus[i];
		pSubMenu->Draw();
	}
}

bool COSDMenu::OnAction(const CAction& action)
{
	if (m_iCurrentSubMenu >= (int)m_vecSubMenus.size()) return false; // invalid choice.
	COSDSubMenu* pSubMenu=m_vecSubMenus[m_iCurrentSubMenu];

	
	if (pSubMenu->OnAction(action)) return true;

	if (action.wID==ACTION_MOVE_LEFT)
	{
		if ( m_iCurrentSubMenu > 0)
		{
			m_iCurrentSubMenu--;
		}
		else
		{
			m_iCurrentSubMenu=m_vecSubMenus.size()-1;
		}
		return true;
	}

	if (action.wID==ACTION_MOVE_RIGHT)
	{
		if ( m_iCurrentSubMenu+1 < (int)m_vecSubMenus.size() )
		{
			m_iCurrentSubMenu++;
		}
		else
		{
			m_iCurrentSubMenu=0;
		}
		return true;
	}

	return false;
}

int  COSDMenu::GetX() const
{
	return m_iXPos;
}

void COSDMenu::SetX(int X)
{
	m_iXPos=X;
}

int  COSDMenu::GetY() const
{
	return m_iYPos;
}

void COSDMenu::SetY(int Y)
{
	m_iYPos=Y;
}


void COSDMenu::AddSubMenu(const COSDSubMenu& submenu)
{
  m_vecSubMenus.push_back( submenu.Clone() );
}