#pragma once
#include "GUIDialog.h"
#include "SmartPlaylist.h"

class CGUIDialogSmartPlaylistEditor :
      public CGUIDialog
{
public:
  CGUIDialogSmartPlaylistEditor(void);
  virtual ~CGUIDialogSmartPlaylistEditor(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();

  static bool EditPlaylist(const CStdString &path);
  static bool NewPlaylist(const CStdString &type);

protected:
  void OnRuleList(int item);
  void OnRuleAdd();
  void OnRuleRemove(int item);
  void OnName();
  void OnMatch();
  void OnLimit();
  void OnType();
  void OnOrder();
  void OnOrderDirection();
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  int GetSelectedItem();
  void HighlightItem(int item);

  CSmartPlaylist m_playlist;

  // our list of rules for display purposes
  CFileItemList m_ruleLabels;

  CStdString m_path;
  bool m_cancelled;
  int m_isPartyMode; // 0 - no, 1 - music, 2 - videos
};
