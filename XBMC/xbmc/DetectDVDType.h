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
#include "utils/criticalsection.h"
#include "utils/singlelock.h"
#include "filesystem/cdiosupport.h"
#include "utils/thread.h"
#include "GuiUserMessages.h"
#include "guiwindowmanager.h"

using namespace XISO9660;

class CDetectDVDMedia : public CThread
{
public:
	CDetectDVDMedia();
	virtual ~CDetectDVDMedia();

	virtual void			OnStartup();
	virtual void			OnExit();
	virtual void			Process();

	static void				WaitMediaReady();
	static bool				IsDiscInDrive();

	static CCdInfo*		GetCdInfo();

protected:
	void							UpdateDvdrom();
	DWORD							GetTrayState();
	
	void							DetectMediaType();
	void							SetNewDVDShareUrl( CStdString strNewUrl );

private:
	CIoSupport				m_helper;
	
	static CCriticalSection	m_muReadingMedia;

	static int				m_DriveState;

	static CCdInfo*		m_pCdInfo;

	bool							m_bStartup;
	bool							m_bAutorun;
	DWORD							m_dwTrayState;
	DWORD							m_dwTrayCount;
	DWORD							m_dwLastTrayState;
public:
	static CEvent			m_evAutorun;
};