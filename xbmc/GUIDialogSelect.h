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
  virtual void    OnKey(const CKey& key);
  
	void						Reset();
	void						Add(const CStdString& strLabel);
	int							GetSelectedLabel() const;
	void						SetHeading(const wstring& strLine);
	void						SetHeading(int iString);
protected:
	int							m_iSelected;
	vector<CGUIListItem*> m_vecList;
};
