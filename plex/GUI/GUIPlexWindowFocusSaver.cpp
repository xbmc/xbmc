#include "GUIPlexWindowFocusSaver.h"
#include "GUIControlGroup.h"
#include "GUIBaseContainer.h"
#include "GUIWindowManager.h"
#include "log.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIPlexWindowFocusSaver::CGUIPlexWindowFocusSaver()
{
  m_lastFocusedControlID = -1;
  m_lastFocusedControlItem = -1;
  m_window = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexWindowFocusSaver::SaveFocus(CGUIWindow* window)
{
  if (!window)
    return;

  m_window = window;

  // save current focused controls
  m_lastFocusedControlID = m_window->GetFocusedControlID();
  const CGUIControl* control = m_window->GetControl(m_lastFocusedControlID);
  if (control->IsContainer())
  {
    CGUIBaseContainer* container = (CGUIBaseContainer*)control;
    if (container)
      m_lastFocusedControlItem = container->GetSelectedItem();
    else
      m_lastFocusedControlItem = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexWindowFocusSaver::RestoreFocus(bool reset)
{
  if (!m_window)
    return;

  // restore last focused controls
  if (m_lastFocusedControlID > 0)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, m_window->GetID(), m_lastFocusedControlID,
                    m_lastFocusedControlItem + 1);
    g_windowManager.SendThreadMessage(msg);

    if (reset)
    {
      m_lastFocusedControlID = -1;
      m_lastFocusedControlItem = -1;
    }
  }
}
