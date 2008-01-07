#include "include.h"
#include "GUILabelControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"

#include "LocalizeStrings.h"  // for CGUIInfoLabel

CGUIInfoLabel::CGUIInfoLabel()
{
}

CGUIInfoLabel::CGUIInfoLabel(const CStdString &label, const CStdString &fallback)
{
  SetLabel(label, fallback);
}

void CGUIInfoLabel::SetLabel(const CStdString &label, const CStdString &fallback)
{
  m_fallback = fallback;
  Parse(label);
}

CStdString CGUIInfoLabel::GetLabel(DWORD contextWindow, bool preferImage) const
{
  CStdString label;
  for (unsigned int i = 0; i < m_info.size(); i++)
  {
    const CInfoPortion &portion = m_info[i];
    if (portion.m_info)
    {
      CStdString infoLabel;
      if (preferImage)
        infoLabel = g_infoManager.GetImage(portion.m_info, contextWindow);
      if (infoLabel.IsEmpty())
        infoLabel = g_infoManager.GetLabel(portion.m_info, contextWindow);
      if (!infoLabel.IsEmpty())
      {
        label += portion.m_prefix;
        label += infoLabel;
        label += portion.m_postfix;
      }
    }
    else
    { // no info, so just append the prefix
      label += portion.m_prefix;
    }
  }
  if (label.IsEmpty())  // empty label, use the fallback
    return m_fallback;
  return label;
}

CStdString CGUIInfoLabel::GetItemLabel(const CGUIListItem *item, bool preferImages) const
{
  if (!item->IsFileItem()) return "";
  CStdString label;
  for (unsigned int i = 0; i < m_info.size(); i++)
  {
    const CInfoPortion &portion = m_info[i];
    if (portion.m_info)
    {
      CStdString infoLabel;
      if (preferImages)
        infoLabel = g_infoManager.GetItemImage((const CFileItem *)item, portion.m_info);
      else
        infoLabel = g_infoManager.GetItemLabel((const CFileItem *)item, portion.m_info);
      if (!infoLabel.IsEmpty())
      {
        label += portion.m_prefix;
        label += infoLabel;
        label += portion.m_postfix;
      }
    }
    else
    { // no info, so just append the prefix
      label += portion.m_prefix;
    }
  }
  if (label.IsEmpty())
    return m_fallback;
  return label;
}

bool CGUIInfoLabel::IsEmpty() const
{
  return m_info.size() == 0;
}

bool CGUIInfoLabel::IsConstant() const
{
  return m_info.size() == 0 || (m_info.size() == 1 && m_info[0].m_info == 0);
}

void CGUIInfoLabel::Parse(const CStdString &label)
{
  m_info.clear();
  CStdString work(label);
  // Step 1: Replace all $LOCALIZE[number] with the real string
  int pos1 = work.Find("$LOCALIZE[");
  while (pos1 >= 0)
  {
    int pos2 = StringUtils::FindEndBracket(work, '[', ']', pos1 + 10);
    if (pos2 > pos1)
    {
      CStdString left = work.Left(pos1);
      CStdString right = work.Mid(pos2 + 1);
      CStdString replace = g_localizeStringsTemp.Get(atoi(work.Mid(pos1 + 10).c_str()));
      if (replace == "")
         replace = g_localizeStrings.Get(atoi(work.Mid(pos1 + 10).c_str()));
      work = left + replace + right;
    }
    else
    {
      CLog::Log(LOGERROR, "Error parsing label - missing ']'");
      return;
    }
    pos1 = work.Find("$LOCALIZE[", pos1);
  }
  // Step 2: Find all $INFO[info,prefix,postfix] blocks
  pos1 = work.Find("$INFO[");
  while (pos1 >= 0)
  {
    // output the first block (contents before first $INFO)
    if (pos1 > 0)
      m_info.push_back(CInfoPortion(0, work.Left(pos1), ""));

    // ok, now decipher the $INFO block
    int pos2 = StringUtils::FindEndBracket(work, '[', ']', pos1 + 6);
    if (pos2 > pos1)
    {
      // decipher the block
      CStdString block = work.Mid(pos1 + 6, pos2 - pos1 - 6);
      CStdStringArray params;
      StringUtils::SplitString(block, ",", params);
      int info = g_infoManager.TranslateString(params[0]);
      CStdString prefix, postfix;
      if (params.size() > 1)
        prefix = params[1];
      if (params.size() > 2)
        postfix = params[2];
      m_info.push_back(CInfoPortion(info, prefix, postfix));
      // and delete it from our work string
      work = work.Mid(pos2 + 1);
    }
    else
    {
      CLog::Log(LOGERROR, "Error parsing label - missing ']'");
      return;
    }
    pos1 = work.Find("$INFO[");
  }
  // add any last block
  if (!work.IsEmpty())
    m_info.push_back(CInfoPortion(0, work, ""));
}

CGUIInfoLabel::CInfoPortion::CInfoPortion(int info, const CStdString &prefix, const CStdString &postfix)
{
  m_info = info;
  m_prefix = prefix;
  m_postfix = postfix;
  // filter our prefix and postfix for comma's and $$
  m_prefix.Replace("$COMMA", ","); m_prefix.Replace("$$", "$");
  m_postfix.Replace("$COMMA", ","); m_postfix.Replace("$$", "$");
  m_prefix.Replace("$LBRACKET", "["); m_prefix.Replace("$RBRACKET", "]");
  m_postfix.Replace("$LBRACKET", "["); m_postfix.Replace("$RBRACKET", "]");
}

CGUILabelControl::CGUILabelControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height), m_textLayout(labelInfo.font, wrapMultiLine)
{
  m_bHasPath = bHasPath;
  m_iCursorPos = 0; 
  m_label = labelInfo;
  m_bShowCursor = false;
  m_dwCounter = 0;
  ControlType = GUICONTROL_LABEL;
  m_ScrollInsteadOfTruncate = false;
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
  CStdString label = m_infoLabel.GetLabel(m_dwParentID);
  if (iPos > (int)label.length()) iPos = label.length();
  if (iPos < 0) iPos = 0;
  m_iCursorPos = iPos;
}

void CGUILabelControl::SetInfo(const CGUIInfoLabel &infoLabel)
{
  m_infoLabel = infoLabel;
}

void CGUILabelControl::Render()
{
  CStdString label(m_infoLabel.GetLabel(m_dwParentID));

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
  // shorten the path label
  if ( m_bHasPath )
    m_infoLabel.SetLabel(ShortenPath(strLabel), "");
  else // parse the label for info tags
    m_infoLabel.SetLabel(strLabel, "");
  SetWidthControl(m_ScrollInsteadOfTruncate);
  if (m_iCursorPos > (int)strLabel.size())
    m_iCursorPos = strLabel.size();
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
  size_t nPos;

  nPos = path.find_last_of( '\\' );
  if ( nPos != std::string::npos )
    cDelim = '\\';
  else
  {
    nPos = path.find_last_of( '/' );
    if ( nPos != std::string::npos )
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
    size_t nGreaterDelim = workPath.find_last_of( cDelim, nPos );
    if (nGreaterDelim == std::string::npos)
      break;
    nPos = workPath.find_last_of( cDelim, nGreaterDelim - 1 );
    if ( nPos == std::string::npos )
      break;

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