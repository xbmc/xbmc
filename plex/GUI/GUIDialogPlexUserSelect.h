#ifndef GUIDIALOGPLEXUSERSELECT_H
#define GUIDIALOGPLEXUSERSELECT_H

#include "dialogs/GUIDialogSelect.h"
#include "Job.h"

class CGUIDialogPlexUserSelect : public CGUIDialogSelect, public IJobCallback
{
public:
  CGUIDialogPlexUserSelect();
  bool OnMessage(CGUIMessage &message);

private:
  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
};

#endif // GUIDIALOGPLEXUSERSELECT_H
