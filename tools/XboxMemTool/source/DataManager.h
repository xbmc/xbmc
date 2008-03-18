
#pragma once

#include "SnapShot.h"
#include "CallerTree.h"

namespace XboxMemoryUsage
{
  using namespace System;
  using namespace System::Collections;

  public __gc class CSnapShotElement
  {
  public:
    
    CSnapShotElement(CSnapShot* pSnapShot, CCallerTree* pTreeCall)
    {
      id = 0;
      dateTime = DateTime::Now;
      snapShot = pSnapShot;
      treeCall = pTreeCall;
      comment = "";
    }
    
    ~CSnapShotElement()
    {
      if (snapShot != NULL)
      {
        delete snapShot;
      }
      if (treeCall != NULL)
      {
        delete treeCall;
      }
    }
    
    int id;
    DateTime dateTime;
    String* comment;
    CSnapShot* snapShot;
    CCallerTree* treeCall;
  };
  
  public __gc class DataManager
  {
  public:
    DataManager();
    ~DataManager();
    
    int AddSnapShot(DateTime dateTime, CSnapShot* snapShot, String* comment);
    int AddSnapShot(DateTime dateTime, CSnapShot* snapShot);
    
    CSnapShotElement* GetCurrentSnapShot();
    CSnapShotElement* GetSnapShotById(int id);
    CSnapShotElement* GetSnapShotByIndex(int index);
    int getNrOfSnapShots();

    void SetCurrentSnapShotId(int id);
    
    void SetSourceFileLineNumber(int lineNumber);
    int GetSourceFileLineNumber();

    void SetSourceFile(String* sourceFile);
    String* GetSourceFile();
    
  private:
    ArrayList* m_snapShots;
    CSnapShotElement* m_diff;
    int GetNewSnapShotId();
    
    int m_currentSnapShotElementId;
    
    int m_sourceFileLineNumber;
    String* m_sourceFile;
  };
}