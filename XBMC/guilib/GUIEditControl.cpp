#include "include.h"
#include "GUIEditControl.h"
#include "../xbmc/Util.h"
#include "../xbmc/utils/CharsetConverter.h"

CGUIEditControl::CGUIEditControl(DWORD dwParentID, DWORD dwControlId,
                                 float posX, float posY, float width, float height,
                                 const CLabelInfo& labelInfo, const string& strLabel)
    : CGUILabelControl(dwParentID, dwControlId, posX, posY, width, height, labelInfo, false, false)
{
  ControlType = GUICONTROL_EDIT;
  m_pObserver = NULL;
  m_originalPosX = posX;
  SetLabel(strLabel);
  ShowCursor(true);
}

CGUIEditControl::~CGUIEditControl(void)
{}

void CGUIEditControl::SetObserver(IEditControlObserver* aObserver)
{
  m_pObserver = aObserver;
}

void CGUIEditControl::OnKeyPress(const CAction &action) // FIXME TESTME: NEW/CHANGED parameter and NOT tested CAN'T do it/DON'T know where (window 2700)/how exactly
{
  CStdString label(m_infoLabel.GetLabel(m_dwParentID));
  if (action.wID >= KEY_VKEY && action.wID < KEY_ASCII)
  {
    // input from the keyboard (vkey, not ascii)
    BYTE b = action.wID & 0xFF;
    if (b == 0x25 && m_iCursorPos > 0)
    {
      // left
      m_iCursorPos--;
    }
    if (b == 0x27 && m_iCursorPos < (int)label.length())
    {
      // right
      m_iCursorPos++;
    }
  }
  else if (action.wID >= KEY_ASCII)
  {
    // input from the keyboard
    // NOTE: The below code is from the unicode keyboard patch which isn't in trunk fully yet.
    //       I have included it as this class will eventually need it anyway.  Ideally it should
    //       be using action.unicode if/when the unicode keyboard patch is merged to trunk.
    switch (action.wID) 
    {
    case 27:
      { // escape
        label.clear();
        m_iCursorPos = 0;
        break;
      }
    case 10:
      {
        // enter
        if (m_pObserver)
        {
          CStdString strLineOfText = label;
          label.clear();
          m_iCursorPos = 0;
          m_pObserver->OnEditTextComplete(strLineOfText);
        }
        break;
      }
    case 8:
      {
        // backspace or delete??
        if (m_iCursorPos > 0)
        {
          label.erase(m_iCursorPos - 1, 1);
          m_iCursorPos--;
        }
        break;
      }
    default:
      {
        // use character input // FIXME TESTME: NEW/CHANGED and NOT tested CAN'T do it/DON'T know where/how exactly (conversion from utf8 to WCHAR and back) 
        CStdStringW wStrLabel;
        g_charsetConverter.utf8ToW(label, wStrLabel);
        wStrLabel.insert( wStrLabel.begin() + m_iCursorPos, (WCHAR)action.wID);
        g_charsetConverter.wToUTF8(wStrLabel, label);
        m_iCursorPos++;
        break;
      }
    }
  }
  SetLabel(label);
  RecalcLabelPosition();
}

void CGUIEditControl::RecalcLabelPosition()
{
  float maxWidth = m_width - 8;

  float fTextWidth, fTextHeight;

  // this is possibly not quite correct (utf8 issues)
  CStdString strTempLabel = m_infoLabel.GetLabel(m_dwParentID);
  CStdString strTempPart = strTempLabel.Mid(0, m_iCursorPos);

  m_textLayout.Update(strTempPart);
  m_textLayout.GetTextExtent(fTextWidth, fTextHeight);

  // if skinner forgot to set height :p
  if (m_height == 0)
  {
    // store font height
    m_height = fTextHeight;
  }

  // if text accumulated is greater than width allowed
  if (fTextWidth > maxWidth)
  {
    // move the position of the label to the left (outside of the viewport)
    m_posX = (m_originalPosX + maxWidth) - fTextWidth;
  }
  else
  {
    // otherwise use original position
    m_posX = m_originalPosX;
  }
}

void CGUIEditControl::Render()
{
  // we can only perform view port operations if we have an area to display
  if (m_height > 0 && m_width > 0)
  {
    if (g_graphicsContext.SetClipRegion(m_originalPosX, m_posY, m_width, m_height))
    {
      CGUILabelControl::Render();
      g_graphicsContext.RestoreClipRegion();
    }
  }
  else
  {
    // use default rendering until we have recalculated label position
    CGUILabelControl::Render();
  }
}
