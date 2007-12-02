#pragma once
#include "GUIWindow.h"
#include "utils/CriticalSection.h"

class CGUITextLayout; // forward

class CGUIWindowFullScreen :
      public CGUIWindow
{
public:
  CGUIWindowFullScreen(void);
  virtual ~CGUIWindowFullScreen(void);
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMouse(const CPoint &point);
  virtual void Render();
  virtual void OnWindowLoaded();
  void RenderFullScreen();
  bool NeedRenderFullScreen();
  void ChangetheTimeCode(DWORD remote);

protected:
  virtual void OnDeinitWindow(int nextWindow) {}; // no out window animation for fullscreen video

private:
  void RenderTTFSubtitles();
  void Seek(bool bPlus, bool bLargeStep);
  void PreloadDialog(unsigned int windowID);
  void UnloadDialog(unsigned int windowID);

  bool m_bShowViewModeInfo;
  DWORD m_dwShowViewModeTimeout;

  bool m_bShowCurrentTime;
  bool m_bLastRender;

  bool m_timeCodeShow;
  DWORD m_timeCodeTimeout;
  int m_timeCodeStamp[5];
  int m_timeCodePosition;

  CCriticalSection m_fontLock;
  CGUITextLayout* m_subsLayout;
};
