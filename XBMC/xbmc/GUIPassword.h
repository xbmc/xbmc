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
  bool GetSettings();
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
  bool IsDatabasePathUnlocked(CStdString& strPath, VECSHARES& vecShares);
  bool CheckMasterLockCode();
  bool CheckMasterLock(bool bDisalogYesNo);

	MAPPASSWORDS			m_mapSMBPasswordCache; // SMB share password cache

  bool bMasterLockFilemanager;      // prompts user for MasterLockCode on Click Filemanager! 
  bool bMasterUser;                 // Master user can access to all share with one pass: availible on Advanced mode
  bool bMasterLockSettings;         // prompts user for MasterLockCode on Click Settings!
  bool bMasterLockEnableShutdown;   // allows XBMC Master Lock to shut off XBOX if true
  bool bMasterLockProtectShares;    // prompts for mastercode when editing shares with context menu if true
  bool bMasterLockStartupLock;      // prompts user for szMasterLockCode on startup if true
  bool bMasterNormalUserMode;        // true->0:Normal ; false->1:Advanced - User mode
  CStdString strMasterLockCode;     // password to check for on startup
  int iMasterLockMaxRetry;          // maximum # of password retries a user gets for all locked shares
  int iMasterLockMode;              // determines the type of master lock UI to present to the user, if any
  int iMasterLockHomeMedia;         // prompts user for MasterLockCode on Click Media [Video/Picture/Musik/Programs]!
  int iMasterLockSetFile;
  

protected:
  bool m_bConfirmed;
  bool m_bCanceled;
  CStdString m_SMBShare;
};

extern CGUIPassword g_passwordManager;

