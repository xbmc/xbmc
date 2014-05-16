#ifndef GUIDIALOGPLEXPLAYQUEUE_H
#define GUIDIALOGPLEXPLAYQUEUE_H

#include "dialogs/GUIDialogSelect.h"

class CGUIDialogPlexPlayQueue : public CGUIDialogSelect
{
public:
  CGUIDialogPlexPlayQueue();
  bool OnMessage(CGUIMessage& message);
  bool OnAction(const CAction &action);
  void LoadPlayQueue();
  bool IsMediaWindow() { return true; }
private:
  void ItemSelected();
};

#endif // GUIDIALOGPLEXPLAYQUEUE_H
