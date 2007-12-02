/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "ArenaItem.h"
#include "Utils/KaiClient.h"

CGUIImage* CArenaItem::m_pPrivateIcon = NULL;

CArenaItem::CArenaItem(CStdString& strLabel) : CKaiItem(strLabel)
{
  SetCookie( CKaiClient::Item::Arena );
  m_strVector = "/";
  m_strDescription = "";
  m_strPassword = "";
  m_nPlayers = -1;
  m_nPlayerLimit = -1;
  m_bIsPrivate = false;
  m_bIsPersonal = false;
}

CArenaItem::~CArenaItem(void)
{}

void CArenaItem::SetIcons(float width, float height, const CStdString& aPrivateTexture)
{
  if (aPrivateTexture.length() > 0)
  {
    m_pPrivateIcon = new CGUIImage(0, 0, 0, 0, width, height, aPrivateTexture, 0x0);
    m_pPrivateIcon->AllocResources();
  }
}

void CArenaItem::FreeIcons()
{
  if (m_pPrivateIcon)
  {
    m_pPrivateIcon->FreeResources();
    delete m_pPrivateIcon;
    m_pPrivateIcon = NULL;
  }
}

CArenaItem::Tier CArenaItem::GetTier()
{
  int tier = 0;

  for (int characterIndex = 0; characterIndex < m_strVector.GetLength(); characterIndex++)
  {
    if (m_strVector[characterIndex] == '/')
    {
      tier++;
    }
  }

  if (tier > 4)
  {
    tier = 4;
  }

  return (CArenaItem::Tier) tier;
}

void CArenaItem::GetTier(CArenaItem::Tier aTier, CStdString& aTierName)
{
  GetTier(aTier, m_strVector, aTierName);
}

void CArenaItem::GetTier(Tier aTier, CStdString aVector, CStdString& aTierName)
{
  int tier = 0;
  int characterIndex = 0;
  CStdString name;

  for (; characterIndex < aVector.GetLength(); characterIndex++)
  {
    if (tier == (int)aTier)
    {
      int nextTier = aVector.Find('/', characterIndex + 1);
      if (nextTier >= characterIndex)
      {
        aTierName = aVector.Mid(characterIndex, nextTier - characterIndex);
      }
      else
      {
        aTierName = aVector.Mid(characterIndex);
      }
      break;
    }

    if (aVector[characterIndex] == '/')
    {
      tier++;
    }
  }
}

void CArenaItem::GetDisplayText(CStdString& aString)
{
  if (m_bIsPersonal)
  {
    char chLastLetter = m_strName[m_strName.length() - 1];
    bool bEndsInS = (chLastLetter == 's') || (chLastLetter == 'S');
    if (bEndsInS)
    {
      aString.Format("%s' arena", m_strName);
    }
    else
    {
      aString.Format("%s's arena", m_strName);
    }
  }
  else
  {
    aString = m_strName;
  }
}

void CArenaItem::OnPaint(CGUIItem::RenderContext* pContext)
{
  CKaiItem::OnPaint(pContext);

  CGUIListExItem::RenderContext* pDC = (CGUIListExItem::RenderContext*)pContext;

  if (pDC)
  {
    float baseLineY = pDC->m_positionY;
    float baseLineX = pDC->m_positionX + pDC->m_pButton->GetWidth();

    baseLineX -= 64;

    if (m_pPrivateIcon && m_bIsPrivate)
    {
      float iconPosX = baseLineX - (m_pPrivateIcon->GetWidth() + 4);
      float iconPosY = baseLineY;
      iconPosY += (pDC->m_pButton->GetHeight() - m_pPrivateIcon->GetHeight()) / 2;

      m_pPrivateIcon->SetPosition(iconPosX, iconPosY);
      m_pPrivateIcon->Render();
    }

    if (pDC->m_label.font)
    {
      float posX = baseLineX;
      float posY = baseLineY;

      // render the text
      DWORD dwColor = pDC->m_bFocused ? pDC->m_label.selectedColor : pDC->m_label.textColor;

      CStdString strInfo;
      if (m_nPlayers < 0)
      {
        // orbs remeshing: don't display anything
      }
      else if (m_nPlayerLimit > 0)
      {
        strInfo.Format("%d/%d", (m_nPlayers > 0) ? m_nPlayers : 0, m_nPlayerLimit);
      }
      else
      {
        strInfo.Format("%d", (m_nPlayers > 0) ? m_nPlayers : 0);
      }

      CGUITextLayout layout(pDC->m_label.font, false);
      layout.Update(strInfo);

      float fPosX = posX;
      float fPosY = posY + 2;
      if (pDC->m_pButton->GetLabelInfo().align & XBFONT_CENTER_Y)
        fPosY = posY + pDC->m_pButton->GetHeight() * 0.5f;
      layout.Render(fPosX, fPosY, 0, dwColor, 0, pDC->m_pButton->GetLabelInfo().align, pDC->m_pButton->GetWidth());
    }
  }
}