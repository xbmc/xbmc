#pragma once
#include "guidialog.h"
#include "GUIList.h"

class CGUIDialogHost :
	public CGUIDialog
{
public:
	CGUIDialogHost(void);
	virtual ~CGUIDialogHost(void);
	virtual bool	OnMessage(CGUIMessage& message);

	void			Update() const;
	bool			IsOK() const;
	bool			IsPrivate() const;
	void			GetConfiguration(CStdString& aPassword, CStdString& aDescription, INT& aPlayerLimit);

protected:
	virtual void OnInitWindow();
protected:
	bool		m_bPrivate;
	CStdString	m_strPassword;
	CStdString	m_strDescription;
	int			m_nPlayerLimit;
	bool		m_bOK;
};
