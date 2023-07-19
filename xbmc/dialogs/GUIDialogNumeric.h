/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "utils/XTimeUtils.h"

#include <cstdint>

enum class InputVerificationResult
{
  CANCELED,
  FAILED,
  SUCCESS
};

class CGUIDialogNumeric :
      public CGUIDialog
{
public:
  enum INPUT_MODE { INPUT_TIME = 1, INPUT_DATE, INPUT_IP_ADDRESS, INPUT_PASSWORD, INPUT_NUMBER, INPUT_TIME_SECONDS };
  CGUIDialogNumeric(void);
  ~CGUIDialogNumeric(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;
  void FrameMove() override;

  bool IsConfirmed() const;
  bool IsCanceled() const;
  bool IsInputHidden() const { return m_mode == INPUT_PASSWORD; }

  static bool ShowAndVerifyNewPassword(std::string& strNewPassword);
  static int ShowAndVerifyPassword(std::string& strPassword, const std::string& strHeading, int iRetries);
  static InputVerificationResult ShowAndVerifyInput(std::string& strPassword, const std::string& strHeading, bool bGetUserInput);

  void SetHeading(const std::string &strHeading);
  void SetMode(INPUT_MODE mode, const KODI::TIME::SystemTime& initial);
  void SetMode(INPUT_MODE mode, const std::string &initial);
  KODI::TIME::SystemTime GetOutput() const;
  std::string GetOutputString() const;

  static bool ShowAndGetTime(KODI::TIME::SystemTime& time, const std::string& heading);
  static bool ShowAndGetDate(KODI::TIME::SystemTime& date, const std::string& heading);
  static bool ShowAndGetIPAddress(std::string &IPAddress, const std::string &heading);
  static bool ShowAndGetNumber(std::string& strInput, const std::string &strHeading, unsigned int iAutoCloseTimeoutMs = 0, bool bSetHidden = false);
  static bool ShowAndGetSeconds(std::string& timeString, const std::string &heading);

protected:
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

  void OnNumber(uint32_t num);
  void VerifyDate(bool checkYear);
  void OnNext();
  void OnPrevious();
  void OnBackSpace();
  void OnOK();
  void OnCancel();

  void HandleInputIP(uint32_t num);
  void HandleInputDate(uint32_t num);
  void HandleInputSeconds(uint32_t num);
  void HandleInputTime(uint32_t num);

  bool m_bConfirmed = false;
  bool m_bCanceled = false;

  INPUT_MODE m_mode = INPUT_PASSWORD; // the current input mode
  KODI::TIME::SystemTime m_datetime; // for time and date modes
  uint8_t m_ip[4];                  // for ip address mode
  uint32_t m_block;             // for time, date, and IP methods.
  uint32_t m_lastblock;
  bool m_dirty = false; // true if the current block has been changed.
  std::string m_number;              ///< for number or password input
};
