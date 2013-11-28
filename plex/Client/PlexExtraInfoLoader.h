#ifndef CPLEXEXTRAINFOLOADER_H
#define CPLEXEXTRAINFOLOADER_H

#include <vector>

#include "threads/Thread.h"
#include "threads/CriticalSection.h"

#include "PlexTypes.h"
#include "FileItem.h"

#include "PlexJobs.h"
#include "JobManager.h"

#include <map>

class CPlexExtraInfoLoader : public IJobCallback
{
  public:
    CPlexExtraInfoLoader();
    ~CPlexExtraInfoLoader();
    void LoadExtraInfoForItem(CFileItemList *list, CFileItemPtr extraItem=CFileItemPtr());

  private:
    void CopyProperties(CFileItemList *item, CFileItemPtr extraItem);
    void OnJobComplete(unsigned int jobID, bool success, CJob *job);

    CCriticalSection m_lock;
    std::map<int, CFileItemList*> m_jobMap;
};

#endif // CPLEXEXTRAINFOLOADER_H
