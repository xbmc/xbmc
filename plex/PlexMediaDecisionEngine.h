//
//  PlexMediaDecisionEngine.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-02.
//
//

#ifndef __Plex_Home_Theater__PlexMediaDecisionEngine__
#define __Plex_Home_Theater__PlexMediaDecisionEngine__

#include "FileItem.h"
#include "FileSystem/PlexDirectory.h"
#include "threads/Thread.h"
#include "filesystem/CurlFile.h"
#include "Job.h"

class CPlexMediaDecisionJob : public CJob
{
public:
  CPlexMediaDecisionJob(const CFileItem& item) : m_item(item), m_success(false), m_bStop(false)
  {
  }
  virtual void Cancel();
  virtual bool DoWork();
  CFileItem m_choosenMedia;

private:
  CStdString GetPartURL(CFileItemPtr mediaPart);
  CFileItemPtr ResolveIndirect(CFileItemPtr item);
  void AddHeaders();

  bool m_success;
  XFILE::CPlexDirectory m_dir;
  XFILE::CCurlFile m_http;

  CFileItem m_item;
  CEvent m_done;
  bool m_bStop;
};

class CPlexMediaDecisionEngine : public IJobCallback
{
public:
  bool resolveItem(const CFileItem& item, CFileItem& resolvedItem);
  static void ProcessStack(const CFileItem& item, const CFileItemList& stack);
  static CFileItemPtr getSelectedMediaItem(const CFileItem& item);
  static CFileItemPtr getMediaPart(const CFileItem& item, int partId = -1);

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob* job);

private:
  bool m_success;
  CFileItem m_resolvedItem;
};

#endif /* defined(__Plex_Home_Theater__PlexMediaDecisionEngine__) */
