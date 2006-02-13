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

  static bool ShowAndAddMediaSource(const CStdString &type);
  static bool ShowAndEditMediaSource(const CStdString &type, const CStdString &name, const CStdString &path);

  bool IsConfirmed() const { return m_confirmed; };

  void SetPathAndName(const CStdString &path, const CStdString &name);
  void SetTypeOfMedia(const CStdString &type, bool editNotAdd = false);
protected:
  void OnPathBrowse();
  void OnPath();
  void OnName();
  void OnOK();
  void OnCancel();
  void UpdateButtons();

  CStdString m_path;
  CStdString m_name;
  bool m_confirmed;
};
