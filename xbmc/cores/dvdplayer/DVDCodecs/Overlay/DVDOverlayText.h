
#pragma once
#include "DVDOverlay.h"

class CDVDOverlayText : public CDVDOverlay
{
public:

  enum ElementType
  {
    ELEMENT_TYPE_NONE     = -1,
    ELEMENT_TYPE_TEXT     = 1,
    ELEMENT_TYPE_PROPERTY = 2
  };

  class CElement
  {
  public:
    CElement(ElementType type)
    {
      m_type = type;
    }
    
    virtual ~CElement()
    {
    }
    
    bool IsElementType(ElementType type) { return (type == m_type); }
    
    CElement* pNext;
    ElementType m_type;
  };

  class CElementText : public CElement
  {
  public:
    CElementText(const char* strText) : CElement(ELEMENT_TYPE_TEXT)
    {
      m_text = strdup(strText);
    }
    
    virtual ~CElementText()
    {
      if (m_text) free(m_text);
    }
    
    char* m_text;
  };
  
  class CElementProperty : public CElement
  {
    CElementProperty(const wchar_t* wszText) : CElement(ELEMENT_TYPE_PROPERTY)
    {
      bItalic = false;
      bBold = false;
    }
    
  public:
    bool bItalic;
    bool bBold;
    // color
  };
  
  CDVDOverlayText() : CDVDOverlay(DVDOVERLAY_TYPE_TEXT)
  {
    m_pHead = NULL;
    m_pEnd = NULL;
  }

  virtual ~CDVDOverlayText()
  {
    CElement* pTemp;
    while (m_pHead)
    {
      pTemp = m_pHead;
      m_pHead = m_pHead->pNext;
      delete pTemp;
    }
  }
  
  void AddElement(CDVDOverlayText::CElement* pElement)
  {
    pElement->pNext = NULL;
    
    if (!m_pHead)
    {
      m_pEnd = pElement;
      m_pHead = m_pEnd;
    }
    else
    {
      m_pEnd->pNext = pElement;
    }
  }
  
  CElement* m_pHead;
  CElement* m_pEnd;
};

