#pragma once
#include "GUIDialog.h"
#include "StringUtils.h"

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
  CStdString GetSeekTimeLabel(TIME_FORMAT format = TIME_FORMAT_GUESS);
protected:
  DWORD m_dwTimer;
  float m_fSeekPercentage;
  bool m_bRequireSeek;
};
