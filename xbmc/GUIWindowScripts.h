#pragma once
#include "GUIMediaWindow.h"

class CGUIWindowScripts : public CGUIMediaWindow
{
public:
  CGUIWindowScripts(void);
  virtual ~CGUIWindowScripts(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void OnWindowLoaded();

protected:
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnPlayMedia(int iItem);

  void OnInfo();

  bool m_bViewOutput;
  int m_scriptSize;
  VECSHARES m_shares;
};
