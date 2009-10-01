#ifndef GUI_ACTION_DESCRIPTOR
#define GUI_ACTION_DESCRIPTOR

#include "StdString.h"
#include "system.h"

class CGUIActionDescriptor
{
public:
  typedef enum { LANG_XBMC = 0, LANG_PYTHON = 1 /*, LANG_JAVASCRIPT = 2 */ } ActionLang;

  CGUIActionDescriptor()
  {
    m_lang = LANG_XBMC;
    m_action = "";
    m_sourceWindowId = -1;
  }

  CGUIActionDescriptor(CStdString& action)
  {
    m_lang = LANG_XBMC;
    m_action = action;    
  }
  
  CGUIActionDescriptor(ActionLang lang, CStdString& action)
  {
    m_lang = lang;
    m_action = action;
  }
  
  CStdString m_action;
  ActionLang m_lang;
  int m_sourceWindowId; // the id of the window that was a source of an action
};

#endif
