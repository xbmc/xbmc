#pragma once
#include "utils/Thread.h"
#include "IProgressCallback.h"

class IBackgroundLoaderObserver
{
public:
  virtual ~IBackgroundLoaderObserver() {}
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
  void SetProgressCallback(IProgressCallback* pCallback);
  virtual bool LoadItem(CFileItem* pItem) { return false; };

protected:
  virtual void OnLoaderStart() {};
  virtual void OnLoaderFinish() {};
protected:
  CFileItemList* m_pVecItems;
  IBackgroundLoaderObserver* m_pObserver;
  IProgressCallback* m_pProgressCallback;
  bool m_bRunning;
};
