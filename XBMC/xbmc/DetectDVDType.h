//  CDetectDVDMedia		 -  Thread running in the background to detect a CD change
//							and the filesystem
//
//	by Bobbin007 in 2003
//	
//	
//

#pragma once
#include <xtl.h>
#include "xbox/iosupport.h"
#include "StdString.h"
#include "utils/Mutex.h"
#include "filesystem/cdiosupport.h"
#include "utils/thread.h"
#include "guimessage.h"
#include "guiwindowmanager.h"

#define GUI_MSG_DVDDRIVE_EJECTED_CD	GUI_MSG_USER + 1
#define GUI_MSG_DVDDRIVE_CHANGED_CD	GUI_MSG_USER + 2


using namespace XISO9660;

class CDetectDVDMedia : public CThread
{
public:
	CDetectDVDMedia();
	~CDetectDVDMedia();

	virtual void		OnStartup();
	virtual void		OnExit();
	virtual void		Process();

	static void WaitMediaReady();
	static bool	IsDiscInDrive();

	static CCdInfo* GetCdInfo();

protected:
	void UpdateDvdrom();
	DWORD GetTrayState();
	
	void DetectMediaType();
	void SetNewDVDShareUrl( CStdString strNewUrl );


private:
	CIoSupport				m_helper;
	
	static CMutex m_muReadingMedia;

	static CStdString	m_DVDUrl;
	static int m_DriveState;

	static CCdInfo* m_pCdInfo;

	DWORD m_dwTrayState;
	DWORD m_dwTrayCount;
	DWORD m_dwLastTrayState;
};