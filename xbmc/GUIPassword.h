#pragma once
#include "StdString.h"

typedef map<CStdString, CStdString> MAPPASSWORDS;
typedef map<CStdString, CStdString>::iterator IMAPPASSWORDS;

class CGUIPassword
{
public:
  CGUIPassword(void);
  virtual ~CGUIPassword(void);
  bool IsConfirmed() const;
  bool IsCanceled() const;
  bool IsItemUnlocked(CFileItem* pItem, const CStdString &strType);
  bool IsItemUnlocked(CShare* pItem, const CStdString &strType);
  bool IsMasterLockUnlocked(bool bPromptUser);
  bool IsMasterLockLocked(bool bPromptUser);  // This one does not ask for saving the MasterCode!
  void UpdateMasterLockRetryCount(bool bResetCount);
  void GetSMBShareUserPassword();
  void SetSMBShare(const CStdString &strShare);
  CStdString GetSMBShare();
  bool CheckStartUpLock();
  bool CheckMenuLock(int iWindowID);
  bool MasterUser();

  CStdString CGUIPassword::GetSMBAuthFilename(const CStdString& strAuth);

	MAPPASSWORDS			m_mapSMBPasswordCache; // SMB share password cache
protected:
  bool m_bConfirmed;
  bool m_bCanceled;
  CStdString m_SMBShare;
};

extern CGUIPassword g_passwordManager;

