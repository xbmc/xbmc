
#pragma once

#include "../DVDCodecs/Overlay/DVDOverlay.h"

typedef struct stListElement
{
  CDVDOverlay* pOverlay;
  struct stListElement* pNext;
  
} ListElement;

class CDVDSubtitleLineCollection
{
public:
  CDVDSubtitleLineCollection();
  virtual ~CDVDSubtitleLineCollection();

  //void Lock()   { EnterCriticalSection(&m_critSection); }
  //void Unlock() { LeaveCriticalSection(&m_critSection); }

  void Add(CDVDOverlay* pSubtitle);
  
  CDVDOverlay* Get(double iPts = 0LL); // get the first overlay in this fifo

  void Reset();
  
  void Remove();
  void Clear();
  int GetSize() { return m_iSize; }
  
private:
  ListElement* m_pHead;
  ListElement* m_pCurrent;
  
  int m_iSize;
  //CRITICAL_SECTION m_critSection;
};

