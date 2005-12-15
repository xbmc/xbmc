#include "stdafx.h"
#include "GUIWindowHome.h"
#include "Util.h"
#include "Credits.h"
#include "GUIButtonScroller.h"
#include "utils/GUIInfoManager.h"
#include "GUIDialogContextMenu.h"
#include "utils/KaiClient.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "GUIConditionalButtonControl.h"
#include "SkinInfo.h"
#endif

#include "application.h"

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#define MENU_BUTTON_START 2    // normal buttons
#define MENU_BUTTON_END   20

#define MENU_BUTTON_IMAGE_BACKGROUND_START   101  // background images
#define MENU_BUTTON_IMAGE_BACKGROUND_END     120

#define MENU_BUTTON_IMAGE_NOFOCUS_START      121  // button images for button scroller
#define MENU_BUTTON_IMAGE_NOFOCUS_END        140
#define MENU_BUTTON_IMAGE_FOCUS_START        141  // focused button images for button scroller
#define MENU_BUTTON_IMAGE_FOCUS_END          160

#define CONTROL_BTN_XLINK_KAI  99
#endif
#define CONTROL_BTN_SCROLLER  300
#define CONTROL_DVDDRIVE_START  400


#define HOME_FADE_TIME 300
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#define HOME_HANDLES_FADES 1
#endif
CGUIWindowHome::CGUIWindowHome(void) : CGUIWindow(WINDOW_HOME, "Home.xml")
{
  m_iLastControl = -1;
  m_iLastMenuOption = -1;
  m_iSelectedItem = -1;
}

CGUIWindowHome::~CGUIWindowHome(void)
{}


bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      int iFocusControl = m_iLastControl;

      CGUIWindow::OnMessage(message);

#ifdef HOME_HANDLES_FADES
      // make controls 102-120 invisible...
      for (int iControl = MENU_BUTTON_IMAGE_BACKGROUND_START; iControl < MENU_BUTTON_IMAGE_BACKGROUND_END; iControl++)
      {
        SET_CONTROL_HIDDEN(iControl);
      }

      if (m_iLastMenuOption > 0)
      {
        SET_CONTROL_VISIBLE(m_iLastMenuOption + 100);
      }
#endif

      if (iFocusControl < 0)
      {
        iFocusControl = GetFocusedControl();
        m_iLastControl = iFocusControl;
      }

      if (m_iSelectedItem >= 0)
      {
        CGUIButtonScroller *pScroller = (CGUIButtonScroller *)GetControl(CONTROL_BTN_SCROLLER);
        if (pScroller) pScroller->SetActiveButton(m_iSelectedItem);
      }

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
    if (g_SkinInfo.GetVersion() < 1.8)
    {
      ON_POLL_BUTTON_CONDITION(CONTROL_BTN_XLINK_KAI, CGUIWindowHome, OnPollXLinkClient, 50);
    }
#endif
      SET_CONTROL_FOCUS(iFocusControl, 0);
#ifndef HOME_HANDLES_FADES
      SetControlVisibility();
#endif
      return true;
    }
    break;

  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIButtonScroller *pScroller = (CGUIButtonScroller *)GetControl(CONTROL_BTN_SCROLLER);
      if (pScroller)
        m_iSelectedItem = pScroller->GetActiveButton();
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      int iControl = message.GetControlId();
      m_iLastControl = iControl;
#ifdef HOME_HANDLES_FADES
      FadeBackgroundImages(message.GetControlId(), message.GetSenderId(), message.GetParam1());
#endif
      break;
    }

  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_DVDDRIVE_START)
      {
        //GeminiServer: to play a inserted media dvd
        CAutorun::PlayDisc();
      }
      m_iLastControl = message.GetSenderId();
      break;
    }
  }
  return CGUIWindow::OnMessage(message);
}

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
void CGUIWindowHome::OnClickOnlineGaming(CGUIMessage& aMessage)
{
  if (g_SkinInfo.GetVersion() < 1.8)
  	m_gWindowManager.ActivateWindow( WINDOW_BUDDIES );
}
#endif

bool CGUIWindowHome::OnAction(const CAction &action)
{
  return CGUIWindow::OnAction(action);
}

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
void CGUIWindowHome::Render()
{
  if (g_SkinInfo.GetVersion() < 1.8)
  { // set controls 121->160 invisible (these are the focus + nofocus buttons for the button scroller)
    for (int i = MENU_BUTTON_IMAGE_NOFOCUS_START; i < MENU_BUTTON_IMAGE_FOCUS_END; i++)
    {
      CGUIControl *pControl = (CGUIControl *)GetControl(i);
      if (pControl)
      {
        pControl->SetVisible(false);  // make invisible
        pControl->DynamicResourceAlloc(false);  // turn of dynamic allocation, as we render these in the
      }
    }
  }
#ifdef HOME_HANDLES_FADES
  // Hack for when music/video stops while on homepage (images will all display
  // in this case, whereas normally only 2 are allowed at the same time (during
  // fading)  We check for more than 5 images as a safety check from very fast
  // scrolling.  Note that this may well destroy skins designed purely using
  // the new visibility flags.  This needs removing once agreement is reached
  // that doing the fading both here and in the skin is not good.
  int numBackgroundsVisible = 0;
  for (int i = MENU_BUTTON_IMAGE_BACKGROUND_START; i <= MENU_BUTTON_IMAGE_BACKGROUND_END; ++i)
  {
    const CGUIControl *pControl = GetControl(i);
    if (pControl && pControl->IsVisible())
      numBackgroundsVisible++;
  }
  if (numBackgroundsVisible > 5)
    FadeBackgroundImages(GetFocusedControl(), GetFocusedControl(), 0);
#endif
  CGUIWindow::Render();
}
#endif

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
bool CGUIWindowHome::OnPollXLinkClient(CGUIConditionalButtonControl* pButton)
{
  if (g_guiSettings.GetBool("XLinkKai.Enabled"))
    return CKaiClient::GetInstance()->IsEngineConnected();
  return false;
}

void CGUIWindowHome::FadeBackgroundImages(int focusedControl, int messageSender, int associatedImage)
{
  const CGUIControl *pControl = GetControl(focusedControl);
  if (focusedControl >= MENU_BUTTON_START && focusedControl <= MENU_BUTTON_END && pControl && pControl->CanFocus())
  {
    m_iLastMenuOption = m_iLastControl;
    bool fade = ((messageSender == CONTROL_BTN_SCROLLER) || (messageSender >= MENU_BUTTON_START && messageSender < MENU_BUTTON_END)) && (messageSender != focusedControl);

    // make controls 102-120 invisible...
    for (int i = MENU_BUTTON_IMAGE_BACKGROUND_START; i < MENU_BUTTON_IMAGE_BACKGROUND_END; i++)
    {
      if (fade)
      {
        SET_CONTROL_FADE_OUT(i, HOME_FADE_TIME);
      }
      else
      {
        SET_CONTROL_HIDDEN(i);
      }
    }

    if (fade)
    {
      SET_CONTROL_FADE_IN(focusedControl + 100, HOME_FADE_TIME);
    }
    else
    {
      SET_CONTROL_VISIBLE(focusedControl + 100);
    }
  }
  else if (focusedControl == CONTROL_BTN_SCROLLER)
  {
    // the button scroller has changed focus...
    if (messageSender != CONTROL_BTN_SCROLLER && messageSender != GetID())
    {
      // button scroller is focused from some other control
      // we need to fire off a message to update the info
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_BTN_SCROLLER);
      OnMessage(msg);
      return;
    }
    bool fade = (messageSender == CONTROL_BTN_SCROLLER) || (messageSender >= MENU_BUTTON_START && messageSender < MENU_BUTTON_END);
    if (associatedImage >= MENU_BUTTON_IMAGE_BACKGROUND_START && associatedImage <= MENU_BUTTON_IMAGE_BACKGROUND_END)
    {
      // make controls 101-120 invisible...
      for (int i = MENU_BUTTON_IMAGE_BACKGROUND_START; i < MENU_BUTTON_IMAGE_BACKGROUND_END; i++)
      {
        if (fade)
        {
          SET_CONTROL_FADE_OUT(i, HOME_FADE_TIME);
        }
        else
        {
          SET_CONTROL_HIDDEN(i);
        }
      }

      if (fade)
      {
        SET_CONTROL_FADE_IN(associatedImage, HOME_FADE_TIME);
      }
      else
      {
        SET_CONTROL_VISIBLE(associatedImage);
      }
    }
  }
}
#endif
