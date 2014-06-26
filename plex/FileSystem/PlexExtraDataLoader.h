#ifndef PLEXEXTRAINFOLOADER_H
#define PLEXEXTRAINFOLOADER_H

#include <string>
#include "xbmc/FileItem.h"
#include "JobManager.h"

using namespace std;

class CPlexExtraDataLoader : public IJobCallback
{
private:
  string m_path;
  CFileItemListPtr m_items;

public:
  enum ExtraDataType
  { TRAILER = 1 };

  CPlexExtraDataLoader();
  inline CFileItemListPtr getItems() { return m_items; }

  void loadDataForItem(CFileItemPtr pItem, ExtraDataType type);

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob* job);
};

#endif // PLEXEXTRAINFOLOADER_H
