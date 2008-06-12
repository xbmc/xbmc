/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "include.h"
#include "GUIListExItem.h"
#include "utils/CharsetConverter.h"

CGUIListExItem::CGUIListExItem(CStdString& aItemName) : CGUIItem(aItemName)
{
  m_pIcon = NULL;
  m_dwFocusedDuration = 0;
  m_dwFrameCounter = 0;
}

CGUIListExItem::~CGUIListExItem(void)
{
  if (m_pIcon)
  {
    m_pIcon->FreeResources();
    delete m_pIcon;
  }
}

void CGUIListExItem::SetIcon(CGUIImage* pImage)
{
  if (m_pIcon)
  {
    m_pIcon->FreeResources();
    delete m_pIcon;
  }

  m_pIcon = pImage;
}

void CGUIListExItem::AllocResources()
{
  CGUIItem::AllocResources();

  if (m_pIcon)
  {
    m_pIcon->AllocResources();
  }
}

void CGUIListExItem::FreeResources()
{
  CGUIItem::FreeResources();

  if (m_pIcon)
  {
    m_pIcon->FreeResources();
  }
}

void CGUIListExItem::SetIcon(float width, float height, const CStdString& aTexture)
{
  if (m_pIcon)
  {
    m_pIcon->FreeResources();
    delete m_pIcon;
  }

  m_pIcon = new CGUIImage(0, 0, 0, 0, width, height, aTexture, 0x0);
  m_pIcon->AllocResources();
}

void CGUIListExItem::OnPaint(CGUIItem::RenderContext* pContext)
{
  m_dwFrameCounter++;

  CGUIListExItem::RenderContext* pDC = (CGUIListExItem::RenderContext*)pContext;
  if (pDC)
  {
    // if focused increment the frame counter, otherwise reset it
    m_dwFocusedDuration = pDC->m_bFocused ? (m_dwFocusedDuration + 1) : 0;

    float posX = pDC->m_positionX;
    float posY = pDC->m_positionY;

    if (pDC->m_pButton)
    {
      // render control
      pDC->m_pButton->SetAlpha(0xFF);
      pDC->m_pButton->SetFocus(pDC->m_bFocused);
      if (!pDC->m_bFocused && pDC->m_bActive)
      { // listcontrolex is not focused, yet we have an active item, so render it focused
        // but around 55% transparent.
        pDC->m_pButton->SetAlpha(0x60);
        pDC->m_pButton->SetFocus(true);
      }
      pDC->m_pButton->SetPosition(posX, posY);
      pDC->m_pButton->Render();
      posX += 8;
    }

    if (m_pIcon)
    {
      // render the icon (centered vertically)
      m_pIcon->SetPosition(posX, posY + ((int)pDC->m_pButton->GetHeight() - (int)m_pIcon->GetHeight()) / 2);
      m_pIcon->Render();
    }

    posX += m_pIcon->GetWidth();

    if (pDC->m_label.font)
    {
      // render the text
      DWORD dwColor = pDC->m_bFocused ? pDC->m_label.selectedColor : pDC->m_label.textColor;

      CStdString strDisplayText;
      GetDisplayText(strDisplayText);

      CGUITextLayout layout(pDC->m_label.font, false);
      layout.Update(strDisplayText);

      float fPosX = posX + pDC->m_pButton->GetLabelInfo().offsetX;
      float fPosY = posY + pDC->m_pButton->GetLabelInfo().offsetY;
      if (pDC->m_pButton->GetLabelInfo().align & XBFONT_CENTER_Y)
        fPosY = posY + pDC->m_pButton->GetHeight() * 0.5f;
      layout.Render(fPosX, fPosY, 0, dwColor, 0, pDC->m_pButton->GetLabelInfo().align, pDC->m_pButton->GetWidth());
    }
  }
}

