/*!
	\file GUIDialog.h
	\brief 
	*/

#pragma once
#include "guiwindow.h"

/*!
	\ingroup winmsg
	\brief 
	*/
class CGUIDialog :
	public CGUIWindow
{
public:
	CGUIDialog(DWORD dwID);
	virtual ~CGUIDialog(void);

	virtual bool    OnMessage(CGUIMessage& message);
	void			DoModal(DWORD dwParentId); // modal
	void			Show(DWORD dwParentId); // modeless
	virtual void	Close();
	virtual bool    Load(const CStdString& strFileName, bool bContainsPath = false);
	virtual bool	IsRunning() const { return m_bRunning; }
	virtual bool	IsDialog() { return true;};

protected:
	DWORD			m_dwParentWindowID;
	CGUIWindow* 	m_pParentWindow;
	bool			m_bRunning;
	bool			m_bModal;

private:
	bool            m_bPrevOverlayAllowed;
};
