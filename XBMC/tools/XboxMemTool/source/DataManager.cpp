
#include "..\stdafx.h"

#include "DataManager.h"


namespace XboxMemoryUsage
{
  DataManager::DataManager()
  {
    m_snapShots = new ArrayList();
    m_diff = NULL;
    
    m_currentSnapShotElementId = 0;
    
  }

  DataManager::~DataManager()
  {
  }
  
  void DataManager::SetCurrentSnapShotId(int id)
  {
    m_currentSnapShotElementId = id;
    m_diff = NULL;
  }
  
  int DataManager::AddSnapShot(DateTime dateTime, CSnapShot* pSnapShot, String* comment)
  {
    CSnapShotElement* element = new CSnapShotElement(pSnapShot, NULL);
    element->id = GetNewSnapShotId();
    element->comment = comment;
        
    m_snapShots->Add(element);
    return element->id;
  }

  int DataManager::AddSnapShot(DateTime dateTime, CSnapShot* pSnapShot)
  {
    return AddSnapShot(dateTime, pSnapShot, "");
  }
  
  int DataManager::GetNewSnapShotId()
  {
    int id = 1;
    int size = m_snapShots->Count;
    for (int i = 0; i < size; i++)
    {
      CSnapShotElement* element = static_cast<CSnapShotElement*>(m_snapShots->Item[i]);
      if (id <= element->id)
      {
        id = element->id + 1;
      }
    }
    return id;
  }
  
  CSnapShotElement* DataManager::GetCurrentSnapShot()
  {
    if (m_diff != NULL)
    {
      return m_diff;
    }
    else
    {
      return GetSnapShotById(m_currentSnapShotElementId);
    }
  }
  
  CSnapShotElement* DataManager::GetSnapShotById(int id)
  {
    int size = m_snapShots->Count;
    for (int i = 0; i < size; i++)
    {
      CSnapShotElement* element = static_cast<CSnapShotElement*>(m_snapShots->Item[i]);
      if (element->id == id)
      {
        return element;
      }
    }
    return NULL;
  }

  CSnapShotElement* DataManager::GetSnapShotByIndex(int index)
  {
    int size = m_snapShots->Count;
    if (index >= 0 && index < size)
    {
      CSnapShotElement* element = static_cast<CSnapShotElement*>(m_snapShots->Item[index]);
      return element;
    }
    return NULL;
  }
  
  int DataManager::getNrOfSnapShots()
  {
    int size = m_snapShots->Count;
    return size;
  }
  
  void DataManager::SetSourceFileLineNumber(int lineNumber)
  {
    m_sourceFileLineNumber = lineNumber;
  }
  
  int DataManager::GetSourceFileLineNumber()
  {
    return m_sourceFileLineNumber;
  }

  void DataManager::SetSourceFile(String* sourceFile)
  {
    m_sourceFile = sourceFile;
  }
  
  String* DataManager::GetSourceFile()
  {
    return m_sourceFile;
  }
}