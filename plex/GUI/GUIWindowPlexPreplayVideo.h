#pragma once

#include "windows/GUIMediaWindow.h"
#include "JobManager.h"
#include "threads/Event.h"
#include "PlexNavigationHelper.h"

class CGUIWindowPlexPreplayVideo : public CGUIMediaWindow, public IJobCallback
{
public:
  CGUIWindowPlexPreplayVideo(void);
  virtual ~CGUIWindowPlexPreplayVideo();

  virtual bool OnMessage(CGUIMessage& message);
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  virtual bool OnAction(const CAction &action);

  void Recommend();
  void Share();

  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  bool Update(const CStdString &strDirectory, bool updateFilterPath);
  void UpdateItem();
  bool OnBack(int actionID);

  std::string m_parentPath;
  CEvent m_dataLoaded;
  CFileItemList m_friends;
  CFileItemList m_networks;
  void MoveToItem(int idx);

  CCriticalSection m_navigationLock;
  bool m_navigating;

  CPlexNavigationHelper m_navHelper;
};
