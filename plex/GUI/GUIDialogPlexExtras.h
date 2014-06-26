#ifndef GUIDIALOGPLEXEXTRAS_H
#define GUIDIALOGPLEXEXTRAS_H

#include "xbmc/dialogs/GUIDialogSelect.h"

class CGUIDialogPlexExtras : public CGUIDialogSelect
{
private:
public:
  CGUIDialogPlexExtras();

  virtual bool OnMessage(CGUIMessage& message);
};

#endif // GUIDIALOGPLEXEXTRAS_H
