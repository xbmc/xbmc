#ifndef GUIDIALOGPLEXPLAYQUEUE_H
#define GUIDIALOGPLEXPLAYQUEUE_H

#include "dialogs/GUIDialogSelect.h"

class CGUIDialogPlexPlayQueue : public CGUIDialogSelect
{
public:
  CGUIDialogPlexPlayQueue();
  bool OnMessage(CGUIMessage& message);
  void LoadPlayQueue();
private:
  void ItemSelected();
};

#endif // GUIDIALOGPLEXPLAYQUEUE_H
