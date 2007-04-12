
#include "..\stdafx.h"


#include "Controller.h"
#include <shellapi.h>
#include "Xbmemdump.h"
#include "Log.h"
//#include "XboxMemoryTrace.h"
#include "SymbolLookupHelper.h"
#include "Settings.h"
#include "CallerTree.h"
#include "..\FormSnapShot.h"

namespace XboxMemoryUsage
{
  CController::CController(DataManager* dataManager, FormManager* formManager)
  {
    m_dataManager = dataManager;
    m_formManager = formManager;

    m_pSymbolLookupHelper = new CSymbolLookupHelper();
    m_pSettings = new CSettings();
    OnStartup();
  }

  CController::~CController()
  {
    //delete m_pTrace;
    delete m_pSymbolLookupHelper;
  }
  
  void CController::Log(System::String* message)
  {
    m_formManager->Log(message);
  }

  void CController::LoadPdbFile()
  {
    //char* pdbFile = "H:\\programeren\\xbmc\\XBMC\\Debug\\xbmc.pdb";
    //
    //if (!m_pSymbolLookupHelper->RegisterPdbFile(pdbFile))
    //{
    //  String* message = pdbFile;
    //  String* error = " not loaded, no corresponding module found the the same signature (make sure the pdb file is correct)";
    //  message = message->Concat(error);
    //  Log(message);
    //}
  }
  
  void CController::OnStartup()
  {

    Log("Loading settings.xml");
    if (m_pSettings->Load("settings.xml"))
    {
      Log("Loaded settings.xml");
      Log("Configured pdb files:");
      std::vector<std::string>& files = m_pSettings->GetRegisteredPdbFiles();
      int size = files.size();
      for (int i = 0; i < size; i++)
      {
        std::string& file = files[i];
        Log(file.c_str());
      }
      
      Log("Registering symbol files");
      m_pSymbolLookupHelper->LoadModules(files);
    }
    else Log("Error loading settings.xml");
  }
  
  void CController::OnStart()
  {
    Log("begin: OnStart()");
    

    Log("done: OnStart()");
  }
  
  void CController::OnExit()
  {
    Log("begin: OnExit()");
    
    // CoUninitialize();
    
    Application::Exit();
  }

  void CController::LockGuiControls()
  {
    m_formManager->LockControls();
  }
  
  void CController::UnlockGuiControls()
  {
    m_formManager->UnlockControls();
  }
  
  void CController::OnTest()
  {
    m_formManager->OnTest();
  }
  
  void CController::OnDataChanged(int type)
  {
    m_formManager->OnDataChanged(type);
  }
    
  void CController::TakeSnapshot()
  {
    Log("begin: TakeSnapshot()");

    CXbmemdump xbmemdump;
    
    std::vector<std::string> result;
    if (xbmemdump.Execute(result))
    {
      // the first 25 lines are extra infomation
      int size = result.size();
      if (size > 25) size = 25;
      for (int i = 0; i < size; i++)
      {
        const char* line = result[i].c_str();
        Log(line);
      }
      Log("Stacktrace removed from log (to big)....");
      Log("");
      
      CSnapShot* pSnapShot = CXbmemdump::CreateSnapShotFromVectorData(result);
      if (pSnapShot != NULL)
      {
        DWORD loadAddress = m_pSymbolLookupHelper->GetLoadAddress();
        pSnapShot->m_loadAddress = loadAddress;

        int id = m_dataManager->AddSnapShot(0, pSnapShot);
        SetCurrentSnapShotId(id);
        OnDataChanged(FM_NEWSNAPSHOT);
      }
      else
      {
        Log("CXbmemdump::CreateSnapShotFromVectorData: failed");
      }
    }
    else
    {
      Log("xbmemdump.Execute: failure");
    }
    Log("done: TakeSnapshot()");

  }
  
  int CController::DiffSnapShots(int firstId, int secondId)
  {
    int newId = -1;
    
    CSnapShotElement* firstElement = m_dataManager->GetSnapShotById(firstId);
    CSnapShotElement* secondElement = m_dataManager->GetSnapShotById(secondId);
    
    // always subtract from the latest snapshot
    if (DateTime::Compare(firstElement->dateTime, secondElement->dateTime) > 0)
    {
      CSnapShotElement* t = firstElement;
      firstElement = secondElement;
      secondElement = t;
    }
    
    if (firstElement != NULL && secondElement != NULL)
    {
      CSnapShot* firstSnapshot = firstElement->snapShot;
      CSnapShot* secondSnapshot = secondElement->snapShot;
      
      if (firstSnapshot != NULL && secondSnapshot != NULL)
      {
        CSnapShot* diff = new CSnapShot();
        *diff = *secondSnapshot;
        *diff -= *firstSnapshot;
        
        String* comment = S"Diff between snapshot ";
        comment = String::Concat(comment, Convert::ToString(firstElement->id));
        comment = String::Concat(comment, S" and ");
        comment = String::Concat(comment, Convert::ToString(secondElement->id));

        newId = m_dataManager->AddSnapShot(0, diff, comment);
        SetCurrentSnapShotId(newId);
        OnDataChanged(FM_NEWSNAPSHOT);
      }
    }

    return newId;
  }
  
  void CController::SetCurrentSnapShotId(int id)
  {
    m_dataManager->SetCurrentSnapShotId(id);
  }
  
  void CController::OnSelectSnapShot(int id)
  {
    ProcessSnapShotById(id);
    m_dataManager->SetCurrentSnapShotId(id);
    OnDataChanged(FM_SELECTSNAPSHOT);
  }
  
  void CController::ProcessSnapShotById(int id)
  {
    CSnapShotElement* element = m_dataManager->GetSnapShotById(id);
    if (element != NULL && element->treeCall == NULL)
    {
      CCallerTree* pCallerTree = new CCallerTree(m_pSymbolLookupHelper);
      pCallerTree->Clear();
      pCallerTree->ProcessSnapShot(element->snapShot);
      element->treeCall = pCallerTree;
    }
  }
  
  void CController::OnShowSourceFile(String* sourceFile, int lineNumber)
  {
    m_dataManager->SetSourceFile(sourceFile);
    m_dataManager->SetSourceFileLineNumber(lineNumber);
    OnDataChanged(FM_SHOWSOURCEFILE);
  }
}