#pragma once
#include "GUIDialog.h"
#include "guiwindowmanager.h"
#include "guilistitem.h"
#include "stdstring.h"
#include <vector>
#include <string>
using namespace std;

class CGUIDialogSelect :
  public CGUIDialog
{
public:
  CGUIDialogSelect(void);
  virtual ~CGUIDialogSelect(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  
	virtual void		Close();
	void						Reset();
	void						Add(const CStdString& strLabel);
	int							GetSelectedLabel() const;
	const CStdString& GetSelectedLabelText();
	void						SetHeading(const wstring& strLine);
	void						SetHeading(const string& strLine);
	void						SetHeading(int iString);
  void            EnableButton(bool bOnOff);
  void            SetButtonLabel(int iString);
  bool            IsButtonPressed();
	void						Sort(bool bSortAscending=true);
protected:
  bool            m_bButtonEnabled;
  bool            m_bButtonPressed;
	int							m_iSelected;
	CStdString			m_strSelected;
	vector<CGUIListItem*> m_vecList;
};
