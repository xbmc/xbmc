#include "include.h"

#include "GUIWebBrowserControl.h"
#include "../xbmc/LinksBoksManager.h"
#include "../xbmc/GUIDialogKeyboard.h"
#include "GUIWindowManager.h"

#ifdef WITH_LINKS_BROWSER

#define SCROLL_THRESHOLD1   0.3
#define SCROLL_THRESHOLD2   0.9

CGUIWebBrowserControl::CGUIWebBrowserControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
{
  m_bKeepSession = FALSE;
  ControlType = GUICONTROL_WEBBROWSER;
}

CGUIWebBrowserControl::~CGUIWebBrowserControl(void)
{
}

void CGUIWebBrowserControl::PreAllocResources()
{
}

bool CGUIWebBrowserControl::OnMessage(class CGUIMessage &message)
{
  ILinksBoksWindow *pLB = g_browserManager.GetBrowserWindow();
  if (pLB && message.GetMessage() == GUI_MSG_SETFOCUS)
    pLB->SetFocus(true);
  return CGUIControl::OnMessage(message);
}

bool CGUIWebBrowserControl::OnAction(const CAction &action)
{
  ILinksBoksWindow *pLB = g_browserManager.GetBrowserWindow();
  if (!pLB) return CGUIControl::OnAction(action);
  switch (action.wID)
  {
    case ACTION_WEBBROWSER_PREVLINK:
        pLB->KeyboardAction(LINKSBOKS_KBD_UP, 0);
        return true;
    case ACTION_WEBBROWSER_NEXTLINK:
        pLB->KeyboardAction(LINKSBOKS_KBD_DOWN, 0);
        return true;
    case ACTION_MOVE_LEFT:
	      pLB->KeyboardAction(LINKSBOKS_KBD_LEFT, 0);
	      return true;
    case ACTION_MOVE_RIGHT:
	    pLB->KeyboardAction(LINKSBOKS_KBD_RIGHT, 0);
	    return true;
    case ACTION_MOVE_UP:
        pLB->KeyboardAction(LINKSBOKS_KBD_UP, 0);
	      return true;
    case ACTION_MOVE_DOWN:
	      pLB->KeyboardAction(LINKSBOKS_KBD_DOWN, 0);
	      return true;
    case ACTION_SELECT_ITEM:
      {
        char currentText[256];
        if (g_Mouse.IsActive())
          OnMouseClick(MOUSE_LEFT_BUTTON);
        
        // execute following code even after the mouse click succeeded
        // to bring up the keyboard even with the mouse pointer

        if(pLB->CanInputText((unsigned char *)currentText, 256))
        {
          CStdString strText = currentText;
          m_gWindowManager.m_bPointerNav = false;
          g_Mouse.SetInactive();
          if(CGUIDialogKeyboard::ShowAndGetInput(strText, "", true))
            pLB->SendText((unsigned char *)strText.c_str());
        }
        else
        {
          if (!g_Mouse.IsActive())
            pLB->KeyboardAction(LINKSBOKS_KBD_ENTER, 0);
        }
      }
      return true;
    case ACTION_ANALOG_MOVE:

      if(abs(action.fAmount1) > SCROLL_THRESHOLD1)
      {
        float posX = g_Mouse.posX - m_realPosX;
        float posY = g_Mouse.posY - m_realPosY;

        if(abs(action.fAmount1) > SCROLL_THRESHOLD2)
          pLB->MouseAction((int)posX, (int)posY, ((action.fAmount1 > 0) ? LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELRIGHT
                                                            : LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELLEFT));
        else
          pLB->MouseAction((int)posX, (int)posY, ((action.fAmount1 > 0) ? LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELRIGHT1
                                                            : LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELLEFT1));
      }

      if(abs(action.fAmount2) > SCROLL_THRESHOLD1)
      {
        float posX = g_Mouse.posX - m_realPosX;
        float posY = g_Mouse.posY - m_realPosY;

        if(abs(action.fAmount2) > SCROLL_THRESHOLD2)
          pLB->MouseAction((int)posX, (int)posY, ((action.fAmount2 > 0) ? LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELUP
                                                            : LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELDOWN));
        else
          pLB->MouseAction((int)posX, (int)posY, ((action.fAmount2 > 0) ? LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELUP1
                                                            : LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELDOWN1));
      }
      return true;
    case ACTION_WEBBROWSER_BACK:
      pLB->GoBack();
      return true;
    case ACTION_WEBBROWSER_FORWARD:
	  pLB->GoForward();
      return true;
    case ACTION_PARENT_DIR:
      pLB->KeyboardAction(LINKSBOKS_KBD_ESC, 0);
      return true;
  }
  return CGUIControl::OnAction(action);
}

bool CGUIWebBrowserControl::OnMouseOver()
{
  float posX = g_Mouse.posX - m_realPosX;
  float posY = g_Mouse.posY - m_realPosY;

  ILinksBoksWindow *pLB = g_browserManager.GetBrowserWindow();
  if (!pLB) return false;
  
  pLB->MouseAction((int)posX, (int)posY, LINKSBOKS_MOUSE_MOVE);

  return true;
}

bool CGUIWebBrowserControl::OnMouseWheel()
{
  ILinksBoksWindow *pLB = g_browserManager.GetBrowserWindow();
  if (!pLB) return false;
  
  float posX = g_Mouse.posX - m_realPosX;
  float posY = g_Mouse.posY - m_realPosY;

  if(g_Mouse.cWheel > 0)
	pLB->MouseAction((int)posX, (int)posY, LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELUP);
  else
	pLB->MouseAction((int)posX, (int)posY, LINKSBOKS_MOUSE_MOVE | LINKSBOKS_MOUSE_WHEELDOWN);

  return true;
}

bool CGUIWebBrowserControl::OnMouseClick(DWORD dwButton)
{
  float posX = g_Mouse.posX - m_realPosX;
  float posY = g_Mouse.posY - m_realPosY;

  ILinksBoksWindow *pLB = g_browserManager.GetBrowserWindow();
  if (!pLB) return false;
  
  if (dwButton != MOUSE_LEFT_BUTTON) return false;
  pLB->MouseAction((int)posX, (int)posY, LINKSBOKS_MOUSE_DOWN | LINKSBOKS_MOUSE_LEFT);
  pLB->MouseAction((int)posX, (int)posY, LINKSBOKS_MOUSE_UP | LINKSBOKS_MOUSE_LEFT);

  return true;
}

void CGUIWebBrowserControl::AllocResources()
{
  // LinksBoks doesn't scale so we have to make sure the XBMC control
  // is the same size as the LinksBoks viewport by scaling the latter
  CGUIWindow *pWindow = m_gWindowManager.GetWindow(GetParentID());
  g_graphicsContext.SetScalingResolution(pWindow->GetCoordsRes(), pWindow->GetPosX(), pWindow->GetPosY(), true);
  m_realPosX = (int)floor(g_graphicsContext.ScaleFinalXCoord(m_posX, m_posY));
  m_realPosY = (int)floor(g_graphicsContext.ScaleFinalYCoord(m_posX, m_posY));
  m_realWidth = (int)floor(g_graphicsContext.ScaleFinalXCoord(m_width, m_height));
  m_realHeight = (int)floor(g_graphicsContext.ScaleFinalYCoord(m_width, m_height));

  /* if the window already exists, we're recovering
  from freeze mode, so don't create it again */
  if (!g_browserManager.GetBrowserWindow())
    g_browserManager.CreateBrowserWindow(m_realWidth, m_realHeight);

  g_browserManager.GetBrowserWindow()->RedrawWindow();

  CGUIControl::AllocResources();
}

void CGUIWebBrowserControl::FreeResources()
{
  CGUIControl::FreeResources();
}

void CGUIWebBrowserControl::Render()
{
  LPDIRECT3DSURFACE8 pBackBuffer;
  ILinksBoksWindow *pWindow = g_browserManager.GetBrowserWindow();
  if (pWindow)
  {
    g_graphicsContext.SetScalingResolution(m_gWindowManager.GetWindow(GetParentID())->GetCoordsRes(), 0, 0, true);

    RECT srcRect = { 0, 0, m_realWidth, m_realHeight };
    POINT dstPoint = { (LONG)m_realPosX, (LONG)m_realPosY };
    srcRect.right = min(m_realPosX + srcRect.right, g_graphicsContext.GetWidth()) - m_realPosX;
    srcRect.bottom = min(m_realPosY + srcRect.bottom, g_graphicsContext.GetHeight()) - m_realPosY;
    g_graphicsContext.Get3DDevice()->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
    g_graphicsContext.Get3DDevice()->CopyRects( pWindow->GetSurface(), &srcRect, 1, pBackBuffer, &dstPoint );
    pBackBuffer->Release();
  }
}

#endif /* WITH_LINKS_BROWSER */