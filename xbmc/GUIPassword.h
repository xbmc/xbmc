#pragma once
#include "guidialog.h"
#include "guiwindow.h"
#include "FileItem.h"

class CGUIPassword :
	public CGUIDialog
{
public:
	CGUIPassword(void);
	virtual ~CGUIPassword(void);
	bool						IsConfirmed() const;
	bool						IsCanceled() const;
	void						GetUserInput(CStdString* strOut);
	CStdStringW			m_strUserInput;
	CStdStringW			m_strPassword;
	int							m_iRetries;
	bool						m_bUserInputCleanup;
	static bool     IsItemUnlocked(CFileItem* pItem, const CStdString &strType);
  static bool     IsMasterLockUnlocked(bool bPromptUser);
  static void            UpdateMasterLockRetryCount(bool bResetCount);
protected:
	bool m_bConfirmed;
    bool m_bCanceled;
    wchar_t m_cHideInputChar;
};
