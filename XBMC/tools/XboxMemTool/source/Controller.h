
#pragma once

#include "Log.h"
#include "DataManager.h"
#include "FormManager.h"

//class CXboxMemoryTrace;
class CSymbolLookupHelper;
class CSettings;
class CCallerTree;
class CTreeCall;
class CSnapShot;

namespace XboxMemoryUsage
{
  using namespace Lyquidity::Controls::ExtendedListViews;
  using namespace System;

  public __gc class CController
  {
  public:
    CController(DataManager* dataManager, FormManager* formManager);
    ~CController();

    void Log(System::String* message);
    
    void LoadPdbFile();
    
    void OnStartup();
    //void OnApplicationExit();
    
    void OnStart();
    void OnExit();

    void LockGuiControls();
    void UnlockGuiControls();
    
    void OnTest();
    void OnDataChanged(int type);
    void OnSelectSnapShot(int id);
    void OnShowSourceFile(String* sourceFile, int lineNumber);
    
    void ProcessSnapShotById(int id);
    void TakeSnapshot();
    int DiffSnapShots(int firstId, int secondId);
    
    void SetCurrentSnapShotId(int id);
    
  private:

    CSymbolLookupHelper* m_pSymbolLookupHelper;
    CSettings* m_pSettings;

    DataManager* m_dataManager;
    FormManager* m_formManager;
  };
}