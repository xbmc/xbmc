#pragma once
#include "GUIDialog.h"
#include "SmartPlaylist.h"

class CGUIDialogSmartPlaylistRule :
      public CGUIDialog
{
public:
  CGUIDialogSmartPlaylistRule(void);
  virtual ~CGUIDialogSmartPlaylistRule(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnInitWindow();

  static bool EditRule(CSmartPlaylistRule &rule, const CStdString& type="songs");

protected:
  void OnValue();
  void OnField();
  void OnOperator();
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  void AddOperatorLabel(CSmartPlaylistRule::SEARCH_OPERATOR op);
  void OnBrowse();

  CSmartPlaylistRule m_rule;
  bool m_cancelled;
  CStdString m_type;
};
