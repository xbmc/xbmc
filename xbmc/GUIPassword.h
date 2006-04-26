#pragma once
#include "StdString.h"
#include "FileItem.h"

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
  bool IsDatabasePathUnlocked(CStdString& strPath, VECSHARES& vecShares);

  void GetSMBShareUserPassword();
  void SetSMBShare(const CStdString &strShare);
  CStdString GetSMBShare();
  CStdString GetSMBAuthFilename(const CStdString& strAuth);

	MAPPASSWORDS			m_mapSMBPasswordCache; // SMB share password cache

  bool IsMasterUser();
  bool MasterUserOverridesPasswords();
  bool MasterLockDisabled();
  void LogoutMasterUser();

  bool CheckMasterLock(bool shutdownOnFailure = false);
  bool CheckStartUpLock();
  bool CheckMenuLock(int iWindowID);

protected:
  bool m_bConfirmed;
  bool m_bCanceled;
  CStdString m_SMBShare;

  int  m_masterLockRetriesRemaining;
};

extern CGUIPassword g_passwordManager;

