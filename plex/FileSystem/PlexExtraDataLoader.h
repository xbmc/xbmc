#ifndef PLEXEXTRAINFOLOADER_H
#define PLEXEXTRAINFOLOADER_H

#include <string>
#include "xbmc/FileItem.h"
#include "JobManager.h"

using namespace std;

class CPlexExtraDataLoader : public IJobCallback
{
public:
  enum ExtraDataType
  {
    NONE = 0,
    TRAILER = 1
  };

  CPlexExtraDataLoader();
  inline CFileItemListPtr getItems() { return m_items; }
  inline ExtraDataType getDataType() { return m_type; }

  void loadDataForItem(CFileItemPtr pItem, ExtraDataType type = NONE);
  bool getDataForItem(CFileItemPtr pItem, ExtraDataType type = NONE);

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob* job);

private:
  CURL getItemURL(CFileItemPtr pItem, ExtraDataType type);
  
  string m_path;
  ExtraDataType m_type;
  CFileItemListPtr m_items;
  CCriticalSection m_itemMapLock;
};

#endif // PLEXEXTRAINFOLOADER_H
