#pragma once
#include "GUIDialog.h"

class CGUIDialogVolumeBar : public CGUIDialog
{
public:
  CGUIDialogVolumeBar(void);
  virtual ~CGUIDialogVolumeBar(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  void ResetTimer();
protected:
  DWORD m_dwTimer;
};
