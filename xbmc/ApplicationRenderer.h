#pragma once

#include "utils/Thread.h"
#include "utils/CriticalSection.h"
#include "Application.h"
#include "GUIDialogBusy.h"

class CApplicationRenderer : public CThread
{
public:
  CApplicationRenderer(void);
  ~CApplicationRenderer(void);

  bool Start();
  void Stop();
  void Render(bool bFullscreen = false);
  void Disable();
  void Enable();
  bool IsBusy() const;
  void SetBusy(bool bBusy);
private:
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  void UpdateBusyCount();
  bool CopySurface(LPDIRECT3DSURFACE8 pSurfaceSource, const RECT* rcSource, LPDIRECT3DSURFACE8 pSurfaceDest, const RECT* rcDest);

  DWORD m_time;
  bool m_enabled;
  int m_explicitbusy;
  int m_busycount;
  int m_prevbusycount;
  bool m_busyShown;
  CCriticalSection m_criticalSection;
  LPDIRECT3DSURFACE8 m_lpSurface;
  CGUIDialogBusy* m_pWindow;
  RESOLUTION m_Resolution;
};
extern CApplicationRenderer g_ApplicationRenderer;

