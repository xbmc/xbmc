#include ".\osdsubmenu.h"
#include "localizestrings.h"
using namespace OSD;
COSDSubMenu::COSDSubMenu()
{		
  m_iHeading=0;
	m_iCurrentOption=0;
	m_iXPos=0;
	m_iYPos=0;
}

COSDSubMenu::COSDSubMenu(int iHeading,int iXpos, int iYpos)
{		
  m_iHeading=iHeading;
	m_iCurrentOption=0;
	m_iXPos=iXpos;
	m_iYPos=iYpos;
}

COSDSubMenu::COSDSubMenu(const COSDSubMenu& submenu)
{
	*this=submenu;
}
COSDSubMenu::~COSDSubMenu(void)
{
  Clear();
}

void COSDSubMenu::Clear()
{
	ivecOptions i =m_vecOptions.begin();
	while (i != m_vecOptions.end())
	{
		IOSDOption* pOption=*i;
		delete pOption;
	}
	m_vecOptions.erase(m_vecOptions.begin(),m_vecOptions.end());
}


COSDSubMenu& COSDSubMenu::operator = (const COSDSubMenu& submenu)
{
	if (&submenu==this) return *this;
  Clear();
	ivecOptions i =m_vecOptions.begin();
	while (i != m_vecOptions.end())
	{
		IOSDOption* pOption=*i;
		m_vecOptions.push_back ( pOption->Clone() );
	}

	m_iCurrentOption=submenu.m_iCurrentOption;
	m_iXPos=submenu.m_iXPos;
	m_iYPos=submenu.m_iYPos;
  m_iHeading=submenu.m_iHeading;
	return *this;
}

COSDSubMenu* COSDSubMenu::Clone() const
{
	return new COSDSubMenu(*this);
}

void COSDSubMenu::Draw()
{
	for (int i=0; i < (int)m_vecOptions.size(); ++i)
	{
		IOSDOption* pOption=m_vecOptions[i];
		pOption->Draw(m_iXPos, m_iYPos+i*34, i==m_iCurrentOption);
	}
}

bool COSDSubMenu::OnAction(const CAction& action)
{
	if (m_iCurrentOption >= (int)m_vecOptions.size()) return false; // invalid choice.
	IOSDOption* pOption=m_vecOptions[m_iCurrentOption];

	
	if (pOption->OnAction(action)) return true;

	if (action.wID==ACTION_MOVE_UP)
	{
		if ( m_iCurrentOption > 0)
		{
			m_iCurrentOption--;
		}
		else
		{
			m_iCurrentOption=m_vecOptions.size()-1;
		}
		return true;
	}

	if (action.wID==ACTION_MOVE_DOWN)
	{
		if ( m_iCurrentOption+1 < (int)m_vecOptions.size() )
		{
			m_iCurrentOption++;
		}
		else
		{
			m_iCurrentOption=0;
		}
		return true;
	}

	return false;
}

int  COSDSubMenu::GetX() const
{
	return m_iXPos;
}

void COSDSubMenu::SetX(int X)
{
	m_iXPos=X;
}

int  COSDSubMenu::GetY() const
{
	return m_iYPos;
}

void COSDSubMenu::SetY(int Y)
{
	m_iYPos=Y;
}


void COSDSubMenu::AddOption(const IOSDOption* option)
{
  m_vecOptions.push_back(option->Clone());
}