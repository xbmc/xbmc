#pragma once
#include "GUIMediaWindow.h"
#include "programdatabase.h"
#include "GUIDialogProgress.h"
#include "ThumbLoader.h"

class CGUIWindowPrograms :
      public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnPopupMenu(int iItem, bool bContextDriven = true);

  void PopulateTrainersList();
protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual bool Update(const CStdString& strDirectory);
  virtual bool OnPlayMedia(int iItem);
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnWindowLoaded();

  int GetRegion(int iItem, bool bReload=false);

  CGUIDialogProgress* m_dlgProgress;

  CProgramDatabase m_database;

  int m_iRegionSet; // for cd stuff

  CProgramThumbLoader m_thumbLoader;
};
