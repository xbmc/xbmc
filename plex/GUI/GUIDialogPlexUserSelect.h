#ifndef GUIDIALOGPLEXUSERSELECT_H
#define GUIDIALOGPLEXUSERSELECT_H

#include "dialogs/GUIDialogSelect.h"
#include "Job.h"

class CGUIDialogPlexUserSelect : public CGUIDialogSelect, public IJobCallback
{
public:
  CGUIDialogPlexUserSelect();
  bool OnMessage(CGUIMessage &message);
  void OnSelected();
  bool OnAction(const CAction &action);

private:
  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
};

#endif // GUIDIALOGPLEXUSERSELECT_H
