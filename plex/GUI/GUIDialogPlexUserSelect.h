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
  CStdString getSelectedUserThumb() { return m_selectedUserThumb; }

  bool fetchUsers();
  bool m_authed;
  bool m_userSwitched;
  CStdString m_selectedUser;
  CStdString m_selectedUserThumb;
};

#endif // GUIDIALOGPLEXUSERSELECT_H
