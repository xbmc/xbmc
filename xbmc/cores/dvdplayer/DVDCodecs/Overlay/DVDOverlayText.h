
#pragma once
#include "DVDOverlay.h"
#ifdef _LINUX
#include "../../utils/CharsetConverter.h"
#endif

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
    CElementText(const wchar_t* wszText) : CElement(ELEMENT_TYPE_TEXT)
    {
      m_wszText = wcsdup(wszText);
    }

    CElementText(const char* strText) : CElement(ELEMENT_TYPE_TEXT)
    {
#ifndef _LINUX
      // ansi to unicode
      int iNeeded = MultiByteToWideChar(CP_ACP, 0, strText, -1, NULL, 0) + 2;

      m_wszText = (WCHAR*)malloc(sizeof(WCHAR) * iNeeded);
      MultiByteToWideChar(CP_ACP, 0, strText, -1, m_wszText, iNeeded);
#else
      CStdStringW strTextW;
      g_charsetConverter.utf8ToUTF16(strText, strTextW, false); 
      m_wszText = wcsdup(strTextW.c_str());
#endif
    }
    
    virtual ~CElementText()
    {
      if (m_wszText) free(m_wszText);
    }
    
    wchar_t* m_wszText;
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

