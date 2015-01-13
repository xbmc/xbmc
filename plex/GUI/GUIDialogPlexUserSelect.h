#ifndef GUIDIALOGPLEXUSERSELECT_H
#define GUIDIALOGPLEXUSERSELECT_H

#include "dialogs/GUIDialogSelect.h"
#include "Job.h"

class CGUIDialogPlexUserSelect : public CGUIDialogSelect// , public IJobCallback
{
public:
  CGUIDialogPlexUserSelect();
  bool OnMessage(CGUIMessage &message);
  void OnSelected();
  bool OnAction(const CAction &action);
  bool DidAuth() { return m_authed; }
  bool DidSwitchUser() { return m_userSwitched; }
  CStdString getSelectedUser() { return m_selectedUser; }

private:
  void fetchUsers();
//  void OnJobComplete(unsigned int jobID, bool success, CJob *job);
  bool m_authed;
  bool m_userSwitched;
  CStdString m_selectedUser;
};

#endif // GUIDIALOGPLEXUSERSELECT_H
