#pragma once
#include "guilib/GUIDialog.h"

class CGUIDialogRating: public CGUIDialog
{
private:
  int m_iRating;

public:
  CGUIDialogRating(void);
  virtual ~CGUIDialogRating(void);
  
  static int ShowAndGetInput(int heading, const CStdString& title, int rating);
  
  virtual void SetHeading(int iHeading);
  virtual void SetTitle(const CStdString& strHeading);
  virtual int GetRating() { return m_iRating; }
  virtual void SetRating(int iRating);
  
  virtual bool OnAction(const CAction &action);
  
  bool m_bConfirmed;
};
