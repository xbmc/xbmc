#pragma once

#include "GUIControl.h"
#include "GUIEmbeddedBrowserWindowObserver.h"

// #include "berkelium/Berkelium.hpp"
// #include "berkelium/Window.hpp"
// #include "berkelium/WindowDelegate.hpp"

// class GlDelegate;

class CGUIWebBrowserControl : public CGUIControl
{
public:
  CGUIWebBrowserControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height);
  virtual ~CGUIWebBrowserControl(void);
  virtual CGUIWebBrowserControl *Clone() const { return new CGUIWebBrowserControl(*this); };

  virtual void Render();
  //virtual bool OnMessage(CGUIMessage& message);
  //virtual bool OnAction(const CAction &action);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);

  void Back();
  void Forward();

#if 0
  bool m_bKeepSession;
  bool m_isZoomed;

protected:
  bool OnMouseOver(const CPoint &point);
  bool OnMouseWheel(char wheel, const CPoint &point);
  bool OnMouseClick(DWORD dwButton, const CPoint &point);

  int m_realPosX, m_realPosY, m_realHeight, m_realWidth;
  bool m_bDirty;    /* to prevent two operations in the same loop which cause bad things during scrolling */
#endif

private:
  unsigned int m_texture;
  GUIEmbeddedBrowserWindowObserver *m_observer;
//   Berkelium::Window *m_window;
//   GlDelegate *m_delegate;
};
