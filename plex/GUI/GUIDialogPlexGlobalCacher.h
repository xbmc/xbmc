#ifndef GUIDIALOGPLEXGLOBALCACHER_H
#define GUIDIALOGPLEXGLOBALCACHER_H

#include "FileItem.h"
#include "dialogs/GUIDialogSelect.h"

class CGUIDialogPlexGlobalCacher : public CGUIDialogSelect
{
private:
  CFileItemListPtr m_Sections;
  CFileItemListPtr m_SelectedSections;
  void LoadSections();
  void ProcessSelection();

public:
  CGUIDialogPlexGlobalCacher();
  virtual bool OnMessage(CGUIMessage& message);
  inline CFileItemListPtr GetSections() { return m_SelectedSections; }
};

#endif // GUIDIALOGPLEXGLOBALCACHER_H
