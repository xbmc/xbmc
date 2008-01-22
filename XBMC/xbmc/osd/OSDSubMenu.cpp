
#include "../stdafx.h"
#include "OSDSubMenu.h"
#include "GUIFontManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace OSD;

COSDSubMenu::COSDSubMenu()
{
  m_iHeading = 0;
  m_iCurrentOption = 0;
  m_iXPos = 0;
  m_iYPos = 0;
}

COSDSubMenu::COSDSubMenu(int iHeading, int iXpos, int iYpos)
{
  m_iHeading = iHeading;
  m_iCurrentOption = 0;
  m_iXPos = iXpos;
  m_iYPos = iYpos;
}

COSDSubMenu::COSDSubMenu(const COSDSubMenu& submenu)
{
  *this = submenu;
}
COSDSubMenu::~COSDSubMenu(void)
{
  Clear();
}

void COSDSubMenu::Clear()
{
  ivecOptions i = m_vecOptions.begin();
  while (i != m_vecOptions.end())
  {
    IOSDOption* pOption = *i;
    delete pOption;
    i = m_vecOptions.erase(i);
  }
  m_iCurrentOption = 0;
}


const COSDSubMenu& COSDSubMenu::operator = (const COSDSubMenu& submenu)
{
  if (&submenu == this) return * this;
  Clear();
  icvecOptions i = submenu.m_vecOptions.begin();
  while (i != submenu.m_vecOptions.end())
  {
    const IOSDOption* pOption = *i;
    m_vecOptions.push_back ( pOption->Clone() );
    ++i;
  }

  m_iCurrentOption = submenu.m_iCurrentOption;
  m_iXPos = submenu.m_iXPos;
  m_iYPos = submenu.m_iYPos;
  m_iHeading = submenu.m_iHeading;
  return *this;
}

COSDSubMenu* COSDSubMenu::Clone() const
{
  return new COSDSubMenu(*this);
}

void COSDSubMenu::Draw()
{
  CGUIFont* pFont14 = g_fontManager.GetFont("font14");
  if (pFont14)
  {
    wstring strHeading = g_localizeStrings.Get(m_iHeading);
    pFont14->DrawShadowText( (float)m_iXPos, (float)m_iYPos, 0xFFFFFFFF,
                             strHeading.c_str(), 0,
                             0,
                             5,
                             5,
                             0xFF020202);
    for (int i = 0; i < (int)m_vecOptions.size(); ++i)
    {
      IOSDOption* pOption = m_vecOptions[i];
      pOption->Draw(m_iXPos + 40, 40 + m_iYPos + i*34, i == m_iCurrentOption, true);
    }
  }
}

bool COSDSubMenu::OnAction(IExecutor& executor, const CAction& action)
{
  if (m_iCurrentOption < 0 || m_iCurrentOption >= (int)m_vecOptions.size())
  {
    m_iCurrentOption = 0;
    return false; // invalid choice.
  }
  IOSDOption* pOption = m_vecOptions[m_iCurrentOption];

  if (pOption->OnAction(executor, action)) return true;

  if (action.wID == ACTION_OSD_SHOW_UP)
  {

    if ( m_iCurrentOption > 0)
    {
      m_iCurrentOption--;
    }
    else
    {
      m_iCurrentOption = m_vecOptions.size() - 1;
    }
    return true;
  }

  if (action.wID == ACTION_OSD_SHOW_DOWN)
  {

    if ( m_iCurrentOption + 1 < (int)m_vecOptions.size() )
    {
      m_iCurrentOption++;
    }
    else
    {
      m_iCurrentOption = 0;
    }
    return true;
  }

  return false;
}

int COSDSubMenu::GetX() const
{
  return m_iXPos;
}

void COSDSubMenu::SetX(int X)
{
  m_iXPos = X;
}

int COSDSubMenu::GetY() const
{
  return m_iYPos;
}

void COSDSubMenu::SetY(int Y)
{
  m_iYPos = Y;
}


void COSDSubMenu::AddOption(const IOSDOption* option)
{
  m_vecOptions.push_back(option->Clone());
}


void COSDSubMenu::SetLabel(int iMessage, const CStdString& strLabel)
{
  for (int i = 0; i < (int)m_vecOptions.size(); ++i)
  {
    IOSDOption* pOption = m_vecOptions[i];
    if (pOption->GetMessage() == iMessage)
    {
      pOption->SetLabel(strLabel);
    }
  }
}
void COSDSubMenu::SetValue(int iMessage, int iValue)
{
  for (int i = 0; i < (int)m_vecOptions.size(); ++i)
  {
    IOSDOption* pOption = m_vecOptions[i];
    if (pOption->GetMessage() == iMessage)
    {
      pOption->SetValue(iValue);
    }
  }
}
