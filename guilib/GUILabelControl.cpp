#include "include.h"
#include "GUILabelControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"

CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const string& strLabel, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height), m_textLayout(labelInfo.font, wrapMultiLine)
{
  m_bHasPath = bHasPath;
  m_iCursorPos = 0; 
  SetLabel(strLabel);
  m_label = labelInfo;
  m_bShowCursor = false;
  m_dwCounter = 0;
  ControlType = GUICONTROL_LABEL;
  m_ScrollInsteadOfTruncate = false;
  m_singleInfo = 0;
  m_startHighlight = m_endHighlight = 0;
}

CGUILabelControl::~CGUILabelControl(void)
{
}

void CGUILabelControl::ShowCursor(bool bShow)
{
  m_bShowCursor = bShow;
}

void CGUILabelControl::SetCursorPos(int iPos)
{
  if (iPos > (int)m_strLabel.length()) iPos = m_strLabel.length();
  if (iPos < 0) iPos = 0;
  m_iCursorPos = iPos;
}

void CGUILabelControl::SetInfo(int singleInfo)
{
  m_singleInfo = singleInfo;
}

void CGUILabelControl::Render()
{
  CStdString label;
	if (m_singleInfo)
	{ 
		label = g_infoManager.GetLabel(m_singleInfo, m_dwParentID);
	}
	else
	{
    label = g_infoManager.GetMultiInfo(m_multiInfo, m_dwParentID);
	}

  if (m_bShowCursor)
  { // cursor location assumes utf16 text, so deal with that (inefficient, but it's not as if it's a high-use area
    // virtual keyboard only)
    CStdStringW utf16;  
    g_charsetConverter.utf8ToUTF16(label, utf16);
    CStdStringW col;
    if ((++m_dwCounter % 50) > 25)
      col.Format(L"|");
    else
      col.Format(L"[COLOR %x]|[/COLOR]", 0x1000000);
    utf16.Insert(m_iCursorPos, col);
    g_charsetConverter.utf16toUTF8(utf16, label);
  }
  else if (m_startHighlight || m_endHighlight)
  { // this is only used for times/dates, so working in ascii (utf8) is fine
    CStdString colorLabel;
    colorLabel.Format("[COLOR %x]%s[/COLOR]%s[COLOR %x]%s[/COLOR]", m_label.disabledColor, label.Left(m_startHighlight),
                 label.Mid(m_startHighlight, m_endHighlight - m_startHighlight), m_label.disabledColor, label.Mid(m_endHighlight));
    label = colorLabel;
  }

  if (m_textLayout.Update(label, m_width))
  { // reset the scrolling as we have a new label
    m_ScrollInfo.Reset();
  }

  // check for scrolling
  bool bNormalDraw = true;
  if (m_ScrollInsteadOfTruncate && m_width > 0 && !IsDisabled())
  { // ignore align center - just use align left/right
    float width, height;
    m_textLayout.GetTextExtent(width, height);
    if (width > m_width)
    { // need to scroll - set the viewport.  Should be set just using the height of the text
      bNormalDraw = false;
      float fPosX = m_posX;
      if (m_label.align & XBFONT_RIGHT)
        fPosX -= m_width;
      float fPosY = m_posY;
      if (m_label.align & XBFONT_CENTER_Y)
        fPosY += m_height * 0.5f;

      m_textLayout.RenderScrolling(fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, (m_label.align & ~3), m_width, m_ScrollInfo);
    }
  }
  if (bNormalDraw)
  {
    float fPosX = m_posX;
    if (m_label.align & XBFONT_CENTER_X)
      fPosX += m_width * 0.5f;

    float fPosY = m_posY;
    if (m_label.align & XBFONT_CENTER_Y)
      fPosY += m_height * 0.5f;

    if (IsDisabled())
      m_textLayout.Render(fPosX, fPosY, m_label.angle, m_label.disabledColor, m_label.shadowColor, m_label.align | XBFONT_TRUNCATED, m_width, true);
    else
      m_textLayout.Render(fPosX, fPosY, m_label.angle, m_label.textColor, m_label.shadowColor, m_label.align | XBFONT_TRUNCATED, m_width);
  }
  CGUIControl::Render();
}


bool CGUILabelControl::CanFocus() const
{
  return false;
}

void CGUILabelControl::SetLabel(const string &strLabel)
{
  if (m_strLabel.compare(strLabel) == 0)
    return;
  m_strLabel = strLabel;

  // shorten the path label
  if ( m_bHasPath )
  {
    m_multiInfo.clear();
    m_multiInfo.push_back(CInfoPortion(0, ShortenPath(strLabel), ""));
  }
  else // parse the label for info tags
    g_infoManager.ParseLabel(strLabel, m_multiInfo);
  SetWidthControl(m_ScrollInsteadOfTruncate);
  if (m_iCursorPos > (int)m_strLabel.size())
    m_iCursorPos = m_strLabel.size();
}

void CGUILabelControl::SetWidthControl(bool bScroll)
{
  m_ScrollInsteadOfTruncate = bScroll;
  m_ScrollInfo.Reset();
}

void CGUILabelControl::SetAlignment(DWORD align)
{
  m_label.align = align;
}

bool CGUILabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      SetLabel(message.GetLabel());
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

CStdString CGUILabelControl::ShortenPath(const CStdString &path)
{
  if (!m_label.font || m_width == 0 || path.IsEmpty())
    return path;

  char cDelim = '\0';
  int nPos;

  nPos = path.find_last_of( '\\' );
  if ( nPos >= 0 )
    cDelim = '\\';
  else
  {
    nPos = path.find_last_of( '/' );
    if ( nPos >= 0 )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return path;

  CStdString workPath(path);
  // remove trailing slashes
  if (workPath.size() > 3)
    if (workPath.Right(3).Compare("://") != 0 && workPath.Right(2).Compare(":\\") != 0)
      if (nPos == workPath.size() - 1)
      {
        workPath.erase(workPath.size() - 1);
        nPos = workPath.find_last_of( cDelim );
      }

  float fTextHeight, fTextWidth;
  m_textLayout.Update(workPath);
  m_textLayout.GetTextExtent(fTextWidth, fTextHeight);

  while ( fTextWidth > m_width )
  {
    nPos = workPath.find_last_of( cDelim, nPos );
    int nGreaterDelim = nPos;
    if ( nPos >= 0 )
      nPos = workPath.find_last_of( cDelim, nPos - 1 );
    else
      break;

    if ( nPos < 0 )
      break;

    if ( nGreaterDelim > nPos )
      workPath.replace( nPos + 1, nGreaterDelim - nPos - 1, "..." );

    m_textLayout.Update(workPath);
    m_textLayout.GetTextExtent(fTextWidth, fTextHeight);
  }
  return workPath;
}

void CGUILabelControl::SetTruncate(bool bTruncate)
{
  if (bTruncate)
    m_label.align |= XBFONT_TRUNCATED;
  else
    m_label.align &= ~XBFONT_TRUNCATED;
}

void CGUILabelControl::SetHighlight(unsigned int start, unsigned int end)
{
  m_startHighlight = start;
  m_endHighlight = end;
}