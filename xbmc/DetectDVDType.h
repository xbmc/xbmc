//  CDetectDVDMedia		 -  Thread running in the background to detect a CD change
//							and the filesystem
//
//	by Bobbin007 in 2003
//	
//	
//

#pragma once
#include "xbox/iosupport.h"
#include "filesystem/cdiosupport.h"

namespace MEDIA_DETECT 
{

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
		static CEvent			m_evAutorun;

	protected:
		void							UpdateDvdrom();
		DWORD							GetTrayState();
		
		void							DetectMediaType();
		void							SetNewDVDShareUrl( const CStdString& strNewUrl, bool bCDDA, const CStdString& strDiscLabel );
		void							SetShareName(CStdString& strShareName, CStdString& strStatus);

	private:
		CIoSupport				m_helper;
		
		static CCriticalSection	m_muReadingMedia;

		static int				m_DriveState;
		static time_t			m_LastPoll;
		static CDetectDVDMedia* m_pInstance;

		static CCdInfo*		m_pCdInfo;

		bool							m_bStartup;
		bool							m_bAutorun;
		DWORD							m_dwTrayState;
		DWORD							m_dwTrayCount;
		DWORD							m_dwLastTrayState;
		
	};
}