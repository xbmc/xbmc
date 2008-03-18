
#include "..\stdafx.h"

#include "FormManager.h"
#include "Controller.h"
#include "..\Form1.h"
#include "..\FormSnapShot.h"
#include "Log.h"
#include "Settings.h"
#include "CallerTree.h"

namespace XboxMemoryUsage
{
  FormManager::FormManager(DataManager* dataManager, Form1* formMain, FormSnapShot* formSnapShot)
  {
    m_dataManager = dataManager;
    m_formMain = formMain;
    m_formSnapShot = formSnapShot;
  }

  FormManager::~FormManager()
  {
  }

  void FormManager::Log(System::String* message)
  {
    m_formMain->listBoxDebug->Items->Add(message);
    m_formMain->listBoxDebug->Refresh();
  }
  
  void FormManager::OnStartup()
  {
    Log("FormManager::OnStartup()");

  }
  
  void FormManager::OnTest()
  {
  
      System::Windows::Forms::RichTextBox* textBoxSourceFile = m_formMain->textBoxSourceFile;
      
      int lineNumber = m_dataManager->GetSourceFileLineNumber();

      System::IO::StreamReader* reader = System::IO::File::OpenText("H:\\programeren\\xbmc\\XBMC\\xbmc\\Application.cpp");

      String* text = reader->ReadToEnd();

      textBoxSourceFile->Text = text;
      
      int selStart = SendMessage((HWND)m_formMain->textBoxSourceFile->Handle.ToInt32(), EM_LINEINDEX, 4228, 0);
      String* line = static_cast<String*>(m_formMain->textBoxSourceFile->Lines->get_Item(4228));
      
      int selLength = line->Length;//SendMessage((HWND)m_formMain->textBoxSourceFile->Handle.ToInt32(), EM_LINELENGTH, 4228, 0);
      textBoxSourceFile->SelectionStart = selStart;
      textBoxSourceFile->SelectionLength = selLength;
      textBoxSourceFile->Focus();
      
      //int charIndex = textBoxSourceFile->WndProc(

  }
  
  void FormManager::OnDataChanged(int type)
  {
    char temp[64]; // for text conversion
    
    if (type == FM_NEWSNAPSHOT)
    {
      ListView::ListViewItemCollection* listViewItems = m_formSnapShot->listViewSnapshots->Items;
      listViewItems->Clear();
      int size = m_dataManager->getNrOfSnapShots();
      for (int i = 0; i < size; i++)
      {
        CSnapShotElement* snapShotElement = m_dataManager->GetSnapShotByIndex(i);
        ListViewItem* listViewItem = new ListViewItem();

        listViewItem->Text = Convert::ToString(snapShotElement->id);

        listViewItem->SubItems->Add(snapShotElement->dateTime.ToLongTimeString());
        
        listViewItem->SubItems->Add(Convert::ToString(snapShotElement->snapShot->m_totalSize));
        
        sprintf(temp, "0x%x", snapShotElement->snapShot->m_loadAddress);
        listViewItem->SubItems->Add(temp);
        
        listViewItem->SubItems->Add(snapShotElement->comment);
        
        listViewItems->Add(listViewItem);
      }
    }
    else if (type == FM_SELECTSNAPSHOT)
    {
      RedrawTreeListView();
    }
    else if (type == FM_SHOWSOURCEFILE)
    {
      System::Windows::Forms::RichTextBox* textBoxSourceFile = m_formMain->textBoxSourceFile;
      String* sourceFile = m_dataManager->GetSourceFile();
      int lineNumber = m_dataManager->GetSourceFileLineNumber();

      textBoxSourceFile->Text = S"";
      
      System::IO::StreamReader* reader = NULL;
      
      if (System::IO::File::Exists(sourceFile))
      {
        try
        {
          reader = System::IO::File::OpenText(sourceFile);
          String* text = reader->ReadToEnd();

          textBoxSourceFile->Text = text;
          
          if (lineNumber > 0)
          {
            int selStart = SendMessage((HWND)m_formMain->textBoxSourceFile->Handle.ToInt32(), EM_LINEINDEX, lineNumber - 1, 0);
            String* line = static_cast<String*>(m_formMain->textBoxSourceFile->Lines->get_Item(lineNumber - 1));
            
            int selLength = line->Length;
            textBoxSourceFile->SelectionStart = selStart;
            textBoxSourceFile->SelectionLength = selLength;
            textBoxSourceFile->Focus();
          }
        } __finally {
            if (reader != NULL) reader->Close();
        }
      }
    }
  }

  void FormManager::LockControls()
  {
    m_formMain->Enabled = false;
    m_formSnapShot->Enabled = false;
  }
  
  void FormManager::UnlockControls()
  {
    m_formMain->Enabled = true;
    m_formSnapShot->Enabled = true;
  }
    
  void FormManager::RedrawTreeListView()
  {
    m_formMain->treeListView->Nodes->Clear();
    CCallerTree* pCallerTree = m_dataManager->GetCurrentSnapShot()->treeCall;
    if (pCallerTree != NULL)
    {
      m_formMain->treeListView->BeginUpdate();
      if (pCallerTree->m_pTopCall)
      {
        CTreeCall* pCall = pCallerTree->m_pTopCall;//->m_pChildHead;
        while (pCall != NULL)
        {
          AddTreeListNode(m_formMain->treeListView->Nodes, pCall);
          pCall = pCall->m_pChildNext;
        }
      }
      m_formMain->treeListView->EndUpdate();
    }
  }
  
  void FormManager::AddTreeListNode(TreeListNodeCollection* pNodeCollection, CTreeCall* pCall)
  {
    static char temp[64];
    if (pCall != NULL)
    {
      CTreeFunction* pFunction = pCall->m_pFunction;
      int allocatedSize = pCall->GetAllocatedSize();
      int nrOfAllocations = pCall->GetNrOffAllocations();
      String* text = "";
      String* moduleName = "unknown";
      if (pFunction != NULL)
      {
        text = pFunction->functionName.c_str();
        moduleName = pFunction->moduleName.c_str();
      } 

      TreeListNode* tln = new TreeListNode();
      if (pCall->m_pChildHead == NULL)
        tln->ImageIndex = -1; // no icon
      else
        tln->ImageIndex = 1;
      
      String* sourceFile = S"";
      if (pCall->m_pFromCall != NULL)
      {
        sourceFile = pCall->m_pFromCall->m_pFunction->sourceFileName.c_str();
      }

      TreeListNodeInfo* tag = new TreeListNodeInfo();
      tag->lineNumber = pCall->line;
      tag->sourceFile = sourceFile;
      
      tln->Tag = tag;
      tln->Text = text;
      sprintf(temp, "0x%x", pCall->address);
      tln->SubItems->Add(temp);
      tln->SubItems->Add(Convert::ToString(allocatedSize));
      tln->SubItems->Add(Convert::ToString(nrOfAllocations));
      tln->SubItems->Add(moduleName);
      
      sourceFile = String::Concat(sourceFile, S" (");
      sourceFile = String::Concat(sourceFile, Convert::ToString((int)pCall->line));
      sourceFile = String::Concat(sourceFile, S")");

      tln->SubItems->Add(sourceFile);
      
      pNodeCollection->Add(tln);
      
      // and do the same for each child
      pCall = pCall->m_pChildHead;
      while (pCall != NULL)
      {
        AddTreeListNode(tln->Nodes, pCall);
        pCall = pCall->m_pChildNext;
      }
    }
  }
  

}