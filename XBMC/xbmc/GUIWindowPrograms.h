#pragma once
#include "GUIMediaWindow.h"
#include "ProgramDatabase.h"
#include "GUIDialogProgress.h"
#include "ThumbLoader.h"

class CGUIWindowPrograms :
      public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool OnMessage(CGUIMessage& message);

#ifdef HAS_TRAINER
  void PopulateTrainersList();
#endif
protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual bool Update(const CStdString& strDirectory);
  virtual bool OnPlayMedia(int iItem);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnWindowLoaded();
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

  int GetRegion(int iItem, bool bReload=false);
  bool OnChooseVideoModeAndLaunch(int iItem);

  CGUIDialogProgress* m_dlgProgress;

  CProgramDatabase m_database;

  int m_iRegionSet; // for cd stuff

  CProgramThumbLoader m_thumbLoader;
};
