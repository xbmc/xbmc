#ifndef GUIDIALOGPLEXUSERSELECT_H
#define GUIDIALOGPLEXUSERSELECT_H

#include "dialogs/GUIDialogSelect.h"

class CGUIDialogPlexUserSelect : public CGUIDialogSelect
{
public:
  CGUIDialogPlexUserSelect();
  bool OnMessage(CGUIMessage &message);
};

#endif // GUIDIALOGPLEXUSERSELECT_H
