

#include "Owned/PlexGlobalCacher.h"
#include "FileSystem/PlexDirectory.h"
#include "Client/PlexServerDataLoader.h"
#include "PlexApplication.h"
#include "utils/Stopwatch.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "TextureCache.h"

using namespace XFILE;

#define GLOBAL_CACHING_DESC "Global Sections Caching"

CPlexGlobalCacher::CPlexGlobalCacher() : CPlexThumbCacher() , CThread("Plex Global Cacher")
{
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

    // Here we have the file list, just process the items
    for (int i = 0; i < list.Size(); i++)
    {
      CFileItemPtr item = list.Get(i);
      if (item->IsPlexMediaServer())
      {
          if (item->HasArt("thumb") &&
              !CTextureCache::Get().HasCachedImage(item->GetArt("thumb")))
            CTextureCache::Get().CacheImage(item->GetArt("thumb"));

          if (item->HasArt("fanart") &&
              !CTextureCache::Get().HasCachedImage(item->GetArt("fanart")))
            CTextureCache::Get().CacheImage(item->GetArt("fanart"));


          if (item->HasArt("grandParentThumb") &&
              !CTextureCache::Get().HasCachedImage(item->GetArt("grandParentThumb")))
            CTextureCache::Get().CacheImage(item->GetArt("grandParentThumb"));


          if (item->HasArt("bigPoster") &&
              !CTextureCache::Get().HasCachedImage(item->GetArt("bigPoster")))
            CTextureCache::Get().CacheImage(item->GetArt("bigPoster"));

      }

      CStdString Message;
      Message.Format("Processing %0.0f%%..." ,float(i * 100.0f) / (float)list.Size());
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, Message, GLOBAL_CACHING_DESC, 8000, false,10);

    }

	// now write the file to mark that caching has been done once
	CFile CacheFile;
	if (CacheFile.OpenForWrite("special://masterprofile/plex.cached"))
	{
		CacheFile.Close();
	}
	else CLog::Log(LOGERROR,"Global Cache : Cannot Create %s","special://masterprofile/plex.cached");

	CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, "Processing Complete.", GLOBAL_CACHING_DESC, 5000, false,500);
}


