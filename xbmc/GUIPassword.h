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
  bool IsItemUnlocked(CFileItem* pItem, const CStdString &strType);
  bool IsItemUnlocked(CShare* pItem, const CStdString &strType);
  bool IsMasterLockUnlocked(bool bPromptUser);
  void UpdateMasterLockRetryCount(bool bResetCount);
  bool GetSMBShareUserPassword();
  void SetSMBShare(const CStdString &strShare);
  CStdString GetSMBShare();
  bool CheckStartUpLock();
  bool CheckMenuLock(int iWindowID);
  bool SetMasterLockMode();
  CStdString GetSMBAuthFilename(const CStdString& strAuth);
  bool LockBookmark(const CStdString& strType, const CStdString& strName, bool bState);
  void LockBookmarks();
  void UnlockBookmarks();
  void RemoveBookmarkLocks();
  bool IsDatabasePathUnlocked(CStdString& strPath, VECSHARES& vecShares);

	MAPPASSWORDS			m_mapSMBPasswordCache; // SMB share password cache

  bool bMasterUser;
  int iMasterLockRetriesLeft;
protected:
  CStdString m_SMBShare;
};

extern CGUIPassword g_passwordManager;