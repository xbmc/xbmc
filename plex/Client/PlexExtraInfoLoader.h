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
  void LoadExtraInfoForItem(const CFileItemListPtr &list, const CFileItemPtr &extraItem = CFileItemPtr(), bool block = false);

private:
  void CopyProperties(const CFileItemListPtr &item, CFileItemPtr extraItem);
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
};

#endif // CPLEXEXTRAINFOLOADER_H
