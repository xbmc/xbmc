#ifndef PLEXSECTIONFANOUT_H
#define PLEXSECTIONFANOUT_H

#include "Job.h"
#include "PlexTypes.h"
#include "URL.h"
#include "PlexApplication.h"
#include "PlexTimer.h"
#include "threads/CriticalSection.h"
#include "PlexJobs.h"

typedef std::pair<int, CFileItemList*> contentListPair;

#define CONTENT_LIST_RECENTLY_ADDED    11000
#define CONTENT_LIST_ON_DECK           11001
#define CONTENT_LIST_RECENTLY_ACCESSED 11002
#define CONTENT_LIST_QUEUE             11003
#define CONTENT_LIST_RECOMMENDATIONS   11004
#define CONTENT_LIST_PLAYQUEUE_VIDEO   11005
#define CONTENT_LIST_PLAYQUEUE_MUSIC   11006
#define CONTENT_LIST_PLAYQUEUE_PHOTO   11007
#define CONTENT_LIST_FANART            12000

class CPlexSectionFanout : public IJobCallback
{
public:
  enum SectionTypes
  {
    SECTION_TYPE_MOVIE,
    SECTION_TYPE_HOME_MOVIE,
    SECTION_TYPE_SHOW,
    SECTION_TYPE_ALBUM,
    SECTION_TYPE_PHOTOS,
    SECTION_TYPE_QUEUE,
    SECTION_TYPE_GLOBAL_FANART,
    SECTION_TYPE_CHANNELS,
    SECTION_TYPE_PLAYQUEUE
  };

  CPlexSectionFanout(const CStdString& url, SectionTypes sectionType, bool useGlobalSlideshow);

  void GetContentTypes(std::vector<int>& types);
  void GetContentList(int type, CFileItemList& list);
  void Refresh();
  void Show();
  static SectionTypes GetSectionTypeFromDirectoryType(EPlexDirectoryType dirType);

  bool NeedsRefresh();
  static CStdString GetBestServerUrl(const CStdString& extraUrl = "");

  SectionTypes m_sectionType;
  bool m_needsRefresh;
  void ShowPlayQueue();

  private:
  int LoadSection(const CURL& url, int contentType);
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);

  std::map<int, CFileItemList*> m_fileLists;
  CURL m_url;
  CPlexTimer m_age;
  CCriticalSection m_critical;
  std::vector<int> m_outstandingJobs;
  bool m_useGlobalSlideshow;
};

#endif // PLEXSECTIONFANOUT_H
