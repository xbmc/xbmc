#pragma once
#include "GUIDialog.h"

class CGUIDialogSeekBar : public CGUIDialog
{
public:
  CGUIDialogSeekBar(void);
  virtual ~CGUIDialogSeekBar(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  void ResetTimer();
  float GetPercentage() {return m_fSeekPercentage;};
  CStdString GetSeekTimeLabel();
protected:
  DWORD m_dwTimer;
  float m_fSeekPercentage;
  bool m_bRequireSeek;
};
