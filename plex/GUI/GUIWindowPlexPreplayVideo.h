#pragma once

#include "GUI/GUIPlexMediaWindow.h"
#include "JobManager.h"
#include "threads/Event.h"
#include "PlexNavigationHelper.h"
#include "FileSystem/PlexExtraDataLoader.h"
#include "GUIPlexWindowFocusSaver.h"

class CGUIWindowPlexPreplayVideo : public CGUIPlexMediaWindow
{
public:
  CGUIWindowPlexPreplayVideo(void);
  virtual ~CGUIWindowPlexPreplayVideo();

  virtual bool OnMessage(CGUIMessage& message);
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  virtual bool OnAction(const CAction &action);

  void Recommend();
  void Share();
  CFileItemPtr getSelectedExtraItem();

  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  bool Update(const CStdString &strDirectory, bool updateFilterPath);
  void UpdateItem();
  bool OnBack(int actionID);

  std::string m_parentPath;
  CFileItemList m_friends;
  CFileItemList m_networks;
  void MoveToItem(int idx);

  CCriticalSection m_navigationLock;
  bool m_navigating;
  CPlexExtraDataLoader m_extraDataLoader;

  CPlexNavigationHelper m_navHelper;
  CGUIPlexWindowFocusSaver m_focusSaver;
};
