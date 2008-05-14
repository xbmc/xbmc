#pragma once
#include "GUIDialog.h"
#include "SmartPlaylist.h"

class CFileItemList;

class CGUIDialogSmartPlaylistEditor :
      public CGUIDialog
{
public:
  enum PLAYLIST_TYPE { TYPE_SONGS = 1, TYPE_MIXED, TYPE_MUSICVIDEOS, TYPE_MOVIES, TYPE_TVSHOWS, TYPE_EPISODES };

  CGUIDialogSmartPlaylistEditor(void);
  virtual ~CGUIDialogSmartPlaylistEditor(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();

  static bool EditPlaylist(const CStdString &path, const CStdString &type = "");
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
  PLAYLIST_TYPE ConvertType(const CStdString &type);
  CStdString ConvertType(PLAYLIST_TYPE type);
  int GetLocalizedType(PLAYLIST_TYPE type);

  CSmartPlaylist m_playlist;

  // our list of rules for display purposes
  CFileItemList* m_ruleLabels;

  CStdString m_path;
  bool m_cancelled;
  CStdString m_mode;  // mode we're in (partymode etc.)
};
