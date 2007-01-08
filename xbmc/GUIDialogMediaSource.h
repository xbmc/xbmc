#pragma once
#include "GUIDialog.h"

class CGUIDialogMediaSource :
      public CGUIDialog
{
public:
  CGUIDialogMediaSource(void);
  virtual ~CGUIDialogMediaSource(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();
  static bool ShowAndAddMediaSource(const CStdString &type);
  static bool ShowAndEditMediaSource(const CStdString &type, const CShare &share);

  bool IsConfirmed() const { return m_confirmed; };

  void SetShare(const CShare &share);
  void SetTypeOfMedia(const CStdString &type, bool editNotAdd = false);
protected:
  void OnPathBrowse(int item);
  void OnPath(int item);
  void OnPathAdd();
  void OnPathRemove(int item);
  void OnName();
  void OnOK();
  void OnCancel();
  void UpdateButtons();
  int GetSelectedItem();
  void HighlightItem(int item);

  vector<CStdString> GetPaths();

  CStdString m_type;
  CStdString m_name;
  CFileItemList m_paths;
  bool m_confirmed;

  bool m_hasMultiPath;
};
