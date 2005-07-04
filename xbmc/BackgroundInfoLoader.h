#include "utils/Thread.h"

#pragma once

class IBackgroundLoaderObserver
{
public:
  virtual void OnItemLoaded(CFileItem* pItem) = 0;
};

class CBackgroundInfoLoader : public CThread
{
public:
  CBackgroundInfoLoader();
  virtual ~CBackgroundInfoLoader();

  void Load(CFileItemList& items);
  bool IsLoading();
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
  void SetObserver(IBackgroundLoaderObserver* pObserver);
  virtual bool LoadItem(CFileItem* pItem) { return false; };

protected:
  virtual void OnLoaderStart() {};
  virtual void OnLoaderFinish() {};
protected:
  CFileItemList* m_pVecItems;
  IBackgroundLoaderObserver* m_pObserver;
  bool m_bRunning;
};
