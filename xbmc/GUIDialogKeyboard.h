#pragma once
#include "guiDialog.h"

class CGUIDialogKeyboard: public CGUIDialog
{

public:
	CGUIDialogKeyboard(void);
	virtual ~CGUIDialogKeyboard(void);

	void SetText(CStdString& aTextString);
	CStdString GetText() const {return m_strEdit;};
	bool IsDirty() { return m_bDirty; };

	static bool ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult);

protected:
	
	virtual void OnInitWindow();
	virtual void OnAction(const CAction &action);
	virtual bool OnMessage(CGUIMessage& message);
private:

	void OnClickButton(int iButtonControl);
	void		UpdateButtons();
	WCHAR		GetCharacter(int iButton);

	void Character(WCHAR wch);
	void Backspace();

	CStdString	m_strEdit;
	bool m_bDirty;
	bool m_bCapsLock;
	bool m_bShift;
};
