#pragma once
#include "utils/Thread.h"

namespace MUSIC_INFO
{
	enum SCAN_STATE { PREPARING=0, REMOVING_OLD, CLEANING_UP_DATABASE, READING_MUSIC_INFO, COMPRESSING_DATABASE, WRITING_CHANGES };

	class IMusicInfoScannerObserver
	{
	public:
		virtual void OnStateChanged(SCAN_STATE state)=0;
		virtual void OnDirectoryChanged(const CStdString& strDirectory)=0;
		virtual void OnDirectoryScanned(const CStdString& strDirectory)=0;
		virtual void OnFinished()=0;
	};

	class CMusicInfoScanner : public CThread
	{
	public:
		CMusicInfoScanner();
		virtual ~CMusicInfoScanner();


		virtual void		OnStartup();
		virtual void		OnExit();
		virtual void		Process();

						void		Start(const CStdString& strDirectory, bool bUpdateAll);
						bool		IsScanning();
						void		Stop();
						void		SetObserver(IMusicInfoScannerObserver* pObserver);


	protected:
						int			RetrieveMusicInfo(VECFILEITEMS& items, const CStdString& strDirectory);
						bool		DoScan(const CStdString& strDirectory);

	protected:
		IMusicInfoScannerObserver*	m_pObserver;
		bool												m_bRunning;
		bool												m_bUpdateAll;
		bool												m_bCanInterrupt;
		CStdString									m_strStartDir;
		CMusicDatabase							m_musicDatabase;
	};
};
