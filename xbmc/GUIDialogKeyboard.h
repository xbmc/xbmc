#pragma once
#include "GUIDialog.h"

enum KEYBOARD {CAPS, LOWER, SYMBOLS };

class CGUIDialogKeyboard: public CGUIDialog
{

public:
  CGUIDialogKeyboard(void);
  virtual ~CGUIDialogKeyboard(void);

  virtual void Render();
  void SetHeading(const CStdString& strHeading) {m_strHeading = strHeading;} ;
  void SetText(CStdString& aTextString);
  CStdString GetText() const { return m_strEdit;};
  bool IsConfirmed() { return m_bIsConfirmed; };
  void SetHiddenInput(bool hiddenInput) { m_hiddenInput = hiddenInput; };

  static bool ShowAndGetInput(CStdString& aTextString, bool allowEmptyResult);
  static bool ShowAndGetInput(CStdString& aTextString, const CStdStringW &strHeading, bool allowEmptyResult, bool hiddenInput = false);
  static bool ShowAndGetNewPassword(CStdString& strNewPassword);
  static bool ShowAndGetNewPassword(CStdString& newPassword, const CStdStringW &heading, bool allowEmpty);
  static int ShowAndVerifyPassword(CStdString& strPassword, const CStdStringW& strHeading, int iRetries);

  virtual void Close(bool forceClose = false);

protected:

  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  void OnShift();
  void MoveCursor(int iAmount);
  int GetCursorPos() const;
  void OnSymbols();

private:

  void OnClickButton(int iButtonControl);
  void OnRemoteNumberClick(int key);
  void UpdateButtons();
  WCHAR GetCharacter(int iButton);
  void UpdateLabel();

  void Character(WCHAR wch);
  void Backspace();

  CStdString m_strEdit;
  bool m_bIsConfirmed;
  KEYBOARD m_keyType;
  int m_iMode;
  bool m_bShift;
  bool m_hiddenInput;

  DWORD m_lastRemoteClickTime;
  WORD m_lastRemoteKeyClicked;
  int m_indexInSeries;
  CStdString m_strHeading;
  static const char* s_charsSeries[10];
};
