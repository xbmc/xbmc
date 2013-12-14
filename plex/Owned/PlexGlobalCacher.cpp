

#include "Owned/PlexGlobalCacher.h"
#include "FileSystem/PlexDirectory.h"
#include "Client/PlexServerDataLoader.h"
#include "PlexApplication.h"
#include "utils/Stopwatch.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/log.h"
#include "filesystem/File.h"

using namespace XFILE;

#define GLOBAL_CACHING_DESC "Global Sections Caching"

CPlexGlobalCacher::CPlexGlobalCacher() : CPlexThumbCacher() , CThread("Plex Global Cacher")
{
	m_MediaCount = 0;
	m_MediaTotal = 0;
}

CPlexGlobalCacher::~CPlexGlobalCacher()
{
}

void CPlexGlobalCacher::Start()
{
	CThread::Create(true);
}

void CPlexGlobalCacher::Process()
{
	CPlexDirectory dir;
	CFileItemList list;
	CStopWatch timer;
	CStopWatch looptimer;
	CStdString Message;
	int Lastcount = 0;
	
	
	// first Check if we have already completed the global cache	
	if (XFILE::CFile::Exists("special://masterprofile/plex.cached")) 
	{
		CLog::Log(LOGNOTICE,"Global Cache : Will skip, global caching already done.");
		return;
	}
		
	// get all the sections names
	CFileItemListPtr pAllSections = g_plexApplication.dataLoader->GetAllSections();

	timer.StartZero();
	CLog::Log(LOGNOTICE,"Global Cache : found %d Sections",pAllSections->Size());
	for (int i = 0; i < pAllSections->Size(); i ++)
	{
		looptimer.StartZero();
		CFileItemPtr Section = pAllSections->Get(i);

		// Pop the notification
		Message.Format("Retrieving content from '%s'...", Section->GetLabel());
		CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, Message, GLOBAL_CACHING_DESC, 5000, false,500);

		// gets all the data from one section
		CURL url(Section->GetPath());
		PlexUtils::AppendPathToURL(url, "all");

		// store them into the list
		dir.GetDirectory(url, list);
		
		CLog::Log(LOGNOTICE,"Global Cache : Processed +%d items in '%s' (total %d), took %f (total %f)",list.Size() - Lastcount,Section->GetLabel().c_str(),list.Size(), looptimer.GetElapsedSeconds(),timer.GetElapsedSeconds());
		Lastcount = list.Size();
	}

	m_MediaTotal += list.Size();

	// load the list into thumbCaher
	CPlexThumbCacher::Load(list);

	// Start teh processing
	// this will spawn one job per item
	CPlexThumbCacher::Start();

	// wait for the proper number of threads to complete
	m_CompletedEvent.Wait();

	// now write the file to mark that caching has been done once
	CFile CacheFile;
	if (CacheFile.OpenForWrite("special://masterprofile/plex.cached"))
	{
		CacheFile.Close();
	}
	else CLog::Log(LOGERROR,"Global Cache : Cannot Create %s","special://masterprofile/plex.cached");

	CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Processing Complete.", GLOBAL_CACHING_DESC, 5000, false,500);
}

void  CPlexGlobalCacher::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
	m_MediaCount++;

	// Display progress
	CStdString Message;
	Message.Format("Processing %0.0f%%..." ,float(m_MediaCount * 100.0f) / (float)m_MediaTotal);
	CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, Message, GLOBAL_CACHING_DESC, 5000, false,10);

	// call base class processing
	CPlexThumbCacher::OnJobComplete(jobID, success, job);
	
	// if we are done, then release here so that main theard can exit
	if (m_MediaCount >= m_MediaTotal) m_CompletedEvent.Set();

}