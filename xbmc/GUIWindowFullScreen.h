#pragma once
#include "GUIWindow.h"
#include "utils/CriticalSection.h"
#include "../guilib/GUIFontTTF.h"

class CGUIWindowFullScreen :
      public CGUIWindow
{
public:
  CGUIWindowFullScreen(void);
  virtual ~CGUIWindowFullScreen(void);
  virtual void AllocResources();
  virtual void FreeResources();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMouse();
  virtual void Render();
  virtual void OnWindowLoaded();
  void RenderFullScreen();
  bool NeedRenderFullScreen();
  bool HasProgressDisplay();
  void ChangetheTimeCode(DWORD remote);

private:
  void ShowOSD();
  void HideOSD();
  void RenderTTFSubtitles();
  void Seek(bool bPlus, bool bLargeStep);
  bool m_bShowTime;
  bool m_bShowCodecInfo;
  bool m_bShowViewModeInfo;
  DWORD m_dwShowViewModeTimeout;
  bool m_bShowCurrentTime;
  DWORD m_dwTimeCodeTimeout;
  DWORD m_dwFPSTime;
  float m_fFrameCounter;
  FLOAT m_fFPS;
  CCriticalSection m_section;
  CCriticalSection m_fontLock;
  bool m_bLastRender;
  int m_strTimeStamp[5];
  int m_iTimeCodePosition;
  int m_iCurrentBookmark;
  CGUIFontTTF* m_subtitleFont;
};
