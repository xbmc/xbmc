
#pragma once

#include "DataManager.h"

#define FM_NEWSNAPSHOT      1
#define FM_SELECTSNAPSHOT   2
#define FM_SHOWSOURCEFILE   3

namespace XboxMemoryUsage
{
  using namespace Lyquidity::Controls::ExtendedListViews;
  using namespace System;
  using namespace System::Collections;
  using namespace System::Windows::Forms;
  
  public __gc class Form1;
  public __gc class FormSnapShot;

  public __gc class TreeListNodeInfo
  {
  public:
    String* sourceFile;
    int lineNumber;
  };
  
  public __gc class FormManager
  {
  public:
    FormManager(DataManager* dataManager, Form1* formMain, FormSnapShot* formSnapShot);
    ~FormManager();

    void OnStartup();
    void OnTest();
    void OnDataChanged(int type);

    void Log(System::String* message);

    void LockControls();
    void UnlockControls();
    
  private:
    DataManager* m_dataManager;
    void AddTreeListNode(TreeListNodeCollection* pNodeCollection, CTreeCall* pCall);
    
    void RedrawTreeListView();
    
  public:
    Form1* m_formMain;
    FormSnapShot* m_formSnapShot;
  };
}