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

protected:
  virtual bool Update(const CStdString &strDirectory);
  virtual bool OnPlayMedia(int iItem);
  virtual bool GetDirectory(const CStdString& strDirectory, CFileItemList& items);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  void OnInfo();

  bool m_bViewOutput;
  int m_scriptSize;
  VECSOURCES m_shares;
};
