#include "include.h"
#include "GUIListExItem.h"
#include "../xbmc/utils/CharsetConverter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

void CGUIListExItem::SetIcon(INT aWidth, INT aHeight, const CStdString& aTexture)
{
  if (m_pIcon)
  {
    m_pIcon->FreeResources();
    delete m_pIcon;
  }

  m_pIcon = new CGUIImage(0, 0, 0, 0, aWidth, aHeight, aTexture, 0x0);
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

    int iPosX = pDC->m_iPositionX;
    int iPosY = pDC->m_iPositionY;

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
      pDC->m_pButton->SetPosition(iPosX, iPosY);
      pDC->m_pButton->Render();
      iPosX += 8;
    }

    if (m_pIcon)
    {
      // render the icon (centered vertically)
      m_pIcon->SetPosition(iPosX, iPosY + ((int)pDC->m_pButton->GetHeight() - (int)m_pIcon->GetHeight()) / 2);
      m_pIcon->Render();
    }

    iPosX += m_pIcon->GetWidth();

    if (pDC->m_pFont)
    {
      // render the text
      DWORD dwColor = pDC->m_bFocused ? pDC->m_dwTextSelectedColour : pDC->m_dwTextNormalColour;

      CStdString strDisplayText;
      GetDisplayText(strDisplayText);

      CStdStringW strUnicode;
      g_charsetConverter.stringCharsetToFontCharset(strDisplayText, strUnicode);

      float fPosX = (float)iPosX + pDC->m_pButton->GetTextOffsetX();
      float fPosY = (float)iPosY + pDC->m_pButton->GetTextOffsetY();
      if (pDC->m_pButton->GetTextAlign() & XBFONT_CENTER_Y)
      {
        float fTextHeight, fTextWidth;
        pDC->m_pFont->GetTextExtent( strUnicode.c_str(), &fTextWidth, &fTextHeight);
        fPosY = (float)iPosY + ((float)pDC->m_pButton->GetHeight() - fTextHeight) / 2;
      }
      RenderText(fPosX, fPosY, (FLOAT)pDC->m_pButton->GetWidth(), dwColor, (WCHAR*) strUnicode.c_str(), pDC->m_pFont);
    }
  }
}

void CGUIListExItem::RenderText(float fPosX, float fPosY, float fMaxWidth, DWORD dwTextColor, WCHAR* wszText, CGUIFont* pFont )
{
  if (!pFont)
    return ;
  static int scroll_pos = 0;
  static int iScrollX = 0;
  static int iLastItem = -1;
  static int iFrames = 0;
  static int iStartFrame = 0;

  float fTextHeight, fTextWidth;
  pFont->GetTextExtent( wszText, &fTextWidth, &fTextHeight);

  float fPosCX = fPosX;
  float fPosCY = fPosY;
  g_graphicsContext.Correct(fPosCX, fPosCY);
  if (fPosCX < 0) fPosCX = 0.0f;
  if (fPosCY < 0) fPosCY = 0.0f;
  if (fPosCY > g_graphicsContext.GetHeight()) fPosCY = (float)g_graphicsContext.GetHeight();
  float fHeight = 60.0f;
  if (fHeight + fPosCY >= g_graphicsContext.GetHeight() )
    fHeight = g_graphicsContext.GetHeight() - fPosCY - 1;
  if (fHeight <= 0) return ;

  float fwidth = fMaxWidth - 5.0f;

  D3DVIEWPORT8 newviewport, oldviewport;
  g_graphicsContext.Get3DDevice()->GetViewport(&oldviewport);
  newviewport.X = (DWORD)fPosCX;
  newviewport.Y = (DWORD)fPosCY;
  newviewport.Width = (DWORD)(fwidth);
  newviewport.Height = (DWORD)(fHeight);
  newviewport.MinZ = 0.0f;
  newviewport.MaxZ = 1.0f;
  g_graphicsContext.Get3DDevice()->SetViewport(&newviewport);
  pFont->DrawTextWidth(fPosX, fPosY, dwTextColor, wszText, fMaxWidth);
  g_graphicsContext.Get3DDevice()->SetViewport(&oldviewport);
}
