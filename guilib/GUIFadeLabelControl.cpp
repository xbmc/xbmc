#include "include.h"
#include "GUIFadeLabelControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUIFadeLabelControl::CGUIFadeLabelControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool scrollOut, DWORD timeToDelayAtEnd)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height), m_scrollInfo(50, labelInfo.offsetX)
    , m_textLayout(labelInfo.font, false)
{
  m_label = labelInfo;
  m_currentLabel = 0;
  ControlType = GUICONTROL_FADELABEL;
  m_scrollOut = scrollOut;
  m_fadeAnim = CAnimation::CreateFader(100, 0, timeToDelayAtEnd, 200);
  if (m_fadeAnim)
    m_fadeAnim->ApplyAnimation();
  m_renderTime = 0;
  m_lastLabel = -1;
}

CGUIFadeLabelControl::~CGUIFadeLabelControl(void)
{
  if (m_fadeAnim)
    delete m_fadeAnim;
}

void CGUIFadeLabelControl::SetInfo(const vector<int> &vecInfo)
{
  m_lastLabel = -1;
  if (vecInfo.size())
    m_infoLabels.clear();
  for (unsigned int i = 0; i < vecInfo.size(); i++)
  {
    vector<CInfoPortion> info;
    info.push_back(CInfoPortion(vecInfo[i], "", ""));
    m_infoLabels.push_back(info);
  }
}

void CGUIFadeLabelControl::SetLabel(const vector<string> &vecLabel)
{
  m_lastLabel = -1;
  m_infoLabels.clear();
  for (unsigned int i = 0; i < vecLabel.size(); i++)
    AddLabel(vecLabel[i]);
}

void CGUIFadeLabelControl::AddLabel(const string &label)
{
  vector<CInfoPortion> info;
  g_infoManager.ParseLabel(label, info);
  m_infoLabels.push_back(info);
}

void CGUIFadeLabelControl::DoRender(DWORD currentTime)
{
  m_renderTime = currentTime;
  CGUIControl::DoRender(currentTime);
}

void CGUIFadeLabelControl::Render()
{
	if (m_infoLabels.size() == 0)
  { // nothing to render
    CGUIControl::Render();
    return ;
  }

	if (m_currentLabel >= m_infoLabels.size() )
		m_currentLabel = 0;

  if (m_textLayout.Update(g_infoManager.GetMultiInfo(m_infoLabels[m_currentLabel], m_dwParentID)))
  { // changed label - update our suffix based on length of available text
    float width, height;
    m_textLayout.GetTextExtent(width, height);
    float spaceWidth = m_label.font->GetCharWidth(L' ');
    unsigned int numSpaces = (unsigned int)(m_width / spaceWidth) + 1;
    if (width < m_width) // append spaces for scrolling
      numSpaces += (unsigned int)((m_width - width) / spaceWidth) + 1;
    m_shortText = width + m_label.offsetX < m_width;
    m_scrollInfo.suffix.assign(numSpaces, L' ');
  }
  if (m_currentLabel != m_lastLabel)
  { // new label - reset scrolling
    m_scrollInfo.Reset();
    m_fadeAnim->QueueAnimation(ANIM_PROCESS_REVERSE);
    m_lastLabel = m_currentLabel;
  }

  if (m_infoLabels.size() == 1 && m_shortText) 
  { // single label set and no scrolling required - just display
    m_textLayout.Render(m_posX + m_label.offsetX, m_posY, 0, m_label.textColor, m_label.shadowColor, 0, m_width - m_label.offsetX);
    CGUIControl::Render();
    return;
  }

  bool moveToNextLabel = false;
  if (!m_scrollOut)
  {
    vector<DWORD> text;
    m_textLayout.GetFirstText(text);
    if (m_scrollInfo.characterPos)
      text.erase(text.begin(), text.begin() + m_scrollInfo.characterPos - 1);
    if (m_label.font->GetTextWidth(text) < m_width)
    {
      if (m_fadeAnim->GetProcess() != ANIM_PROCESS_NORMAL)
        m_fadeAnim->QueueAnimation(ANIM_PROCESS_NORMAL);
      moveToNextLabel = true;
    }
  }
  else if (m_scrollInfo.characterPos > m_textLayout.GetTextLength())
    moveToNextLabel = true;

  // apply the fading animation
  TransformMatrix matrix;
  m_fadeAnim->Animate(m_renderTime, true);
  m_fadeAnim->RenderAnimation(matrix);
  g_graphicsContext.AddTransform(matrix);

  if (m_fadeAnim->GetState() == ANIM_STATE_APPLIED)
    m_fadeAnim->ResetAnimation();

  m_scrollInfo.pixelSpeed = (m_fadeAnim->GetProcess() == ANIM_PROCESS_NONE) ? 1.0f : 0.0f;

  if (!m_scrollOut && m_shortText)
  {
    float posX = m_posX + m_label.offsetX;
    if (m_label.align & XBFONT_CENTER_X)
      posX = m_posX + m_width * 0.5f;
    else if (m_label.align & XBFONT_RIGHT)
      posX = m_posX + m_width;
    m_textLayout.Render(posX, m_posY, 0, m_label.textColor, m_label.shadowColor, m_label.align & 3, m_width);
  }
  else
    m_textLayout.RenderScrolling(m_posX, m_posY, 0, m_label.textColor, m_label.shadowColor, 0, m_width, m_scrollInfo);

  if (moveToNextLabel)
  { // increment the label and reset scrolling
    if (m_fadeAnim->GetProcess() != ANIM_PROCESS_NORMAL)
    {
      m_currentLabel++;
      m_scrollInfo.Reset();
      m_fadeAnim->QueueAnimation(ANIM_PROCESS_REVERSE);
    }
  }

  g_graphicsContext.RemoveTransform();

  CGUIControl::Render();
}


bool CGUIFadeLabelControl::CanFocus() const
{
  return false;
}


bool CGUIFadeLabelControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      AddLabel(message.GetLabel());
    }
    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_lastLabel = -1;
      m_infoLabels.clear();
      m_scrollInfo.Reset();
    }
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_lastLabel = -1;
      m_infoLabels.clear();
      m_scrollInfo.Reset();
      AddLabel(message.GetLabel());
    }
  }
  return CGUIControl::OnMessage(message);
}

