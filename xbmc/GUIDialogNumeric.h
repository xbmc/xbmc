#pragma once
#include "GUIDialog.h"

class CGUIDialogNumeric :
      public CGUIDialog
{
public:
  enum INPUT_MODE { INPUT_TIME = 1, INPUT_DATE, INPUT_IP_ADDRESS, INPUT_PASSWORD, INPUT_NUMBER };
  CGUIDialogNumeric(void);
  virtual ~CGUIDialogNumeric(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

  bool IsConfirmed() const;
  bool IsCanceled() const;

  static bool ShowAndGetNewPassword(CStdString& strNewPassword);
  static int ShowAndVerifyPassword(CStdString& strPassword, const CStdStringW& strHeading, int iRetries);
  static bool ShowAndVerifyInput(CStdString& strPassword, const CStdStringW& strHeading, bool bGetUserInput);

  void SetHeading(const CStdStringW &strHeading);
  void SetMode(INPUT_MODE mode, void *initial);
  void GetOutput(void *output);

  static bool ShowAndGetTime(SYSTEMTIME &time, const CStdStringW &heading);
  static bool ShowAndGetDate(SYSTEMTIME &date, const CStdStringW &heading);
  static bool ShowAndGetIPAddress(CStdString &IPAddress, const CStdStringW &heading);
  static bool ShowAndGetNumber(CStdString& strInput, const CStdStringW &strHeading);

protected:
  void OnNumber(unsigned int num);
  void VerifyDate(bool checkYear);
  void OnNext();
  void OnPrevious();
  void OnBackSpace();

  bool m_bConfirmed;
  bool m_bCanceled;
  wchar_t m_cHideInputChar;

  INPUT_MODE m_mode;                // the current input mode
  SYSTEMTIME m_datetime;            // for time and date modes
  WORD m_ip[4];                     // for ip address mode
  __int64 m_integer;                // for password/numeric input modes
  unsigned int m_block;             // for time, date, and IP methods.
  unsigned int m_lastblock;
  bool m_dirty;                     // true if the current block has been changed.
};
