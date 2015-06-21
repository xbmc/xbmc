#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDOverlay.h"
#include <stdlib.h>
#include <string>

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
      pNext = NULL;
      m_type = type;
    }

    CElement(CElement& src)
    {
      pNext  = NULL;
      m_type = src.m_type;
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
  private:
    std::string m_text;
  public:
    CElementText(const char* strText, int size = -1) : CElement(ELEMENT_TYPE_TEXT)
    {
      if (!strText)
        return;
      if (size == -1)
        m_text.assign(strText);
      else
        m_text.assign(strText, size);
    }
    CElementText(const std::string& text) : CElement(ELEMENT_TYPE_TEXT),
      m_text(text)
    {
    }

    CElementText(CElementText& src)
     : CElement(src),
       m_text(src.m_text)
    {
    }

    const std::string& GetText()
    { return m_text; }
    const char* GetTextPtr()
    { return m_text.c_str(); }

    virtual ~CElementText()
    {  }

  };

  class CElementProperty : public CElement
  {
  public:
    CElementProperty() : CElement(ELEMENT_TYPE_PROPERTY)
    {
      bItalic = false;
      bBold = false;
    }

    CElementProperty(CElementProperty& src)
    : CElement(src)
    {
      bItalic = src.bItalic;
      bBold   = src.bBold;
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

  CDVDOverlayText(CDVDOverlayText& src)
    : CDVDOverlay(src)
  {
    m_pHead = NULL;
    m_pEnd = NULL;
    for(CElement* e = src.m_pHead; e; e = e->pNext)
    {
      if(e->IsElementType(ELEMENT_TYPE_TEXT))
        AddElement(new CElementText(*static_cast<CElementText*>(e)));
      else if(e->IsElementType(ELEMENT_TYPE_PROPERTY))
        AddElement(new CElementProperty(*static_cast<CElementProperty*>(e)));
      else
        AddElement(new CElement(*static_cast<CElement*>(e)));
    }
  }

  virtual ~CDVDOverlayText()
  {
    while (m_pHead)
    {
      CElement* pTemp = m_pHead;
      m_pHead = m_pHead->pNext;
      delete pTemp;
    }
  }

  virtual CDVDOverlayText* Clone()
  {
    return new CDVDOverlayText(*this);
  }

  void AddElement(CDVDOverlayText::CElement* pElement)
  {
    pElement->pNext = NULL;

    if (!m_pHead)
    { // first element - set our head to this element, and update the end to the new element
      m_pHead = pElement;
      m_pEnd = pElement;
    }
    else
    { // extra element - add to the end and update the end to the new element
      m_pEnd->pNext = pElement;
      m_pEnd = pElement;
    }
  }

  CElement* m_pHead;
  CElement* m_pEnd;
};

