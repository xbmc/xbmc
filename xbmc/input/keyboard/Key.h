/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>

/*!
 * \ingroup keyboard
 */
class CKey
{
public:
  CKey(void);
  CKey(uint32_t buttonCode,
       uint8_t leftTrigger = 0,
       uint8_t rightTrigger = 0,
       float leftThumbX = 0.0f,
       float leftThumbY = 0.0f,
       float rightThumbX = 0.0f,
       float rightThumbY = 0.0f,
       float repeat = 0.0f);
  CKey(uint32_t buttonCode, unsigned int held);
  CKey(uint32_t keycode,
       uint8_t vkey,
       wchar_t unicode,
       char ascii,
       uint32_t modifiers,
       uint32_t lockingModifiers,
       unsigned int held);
  CKey(const CKey& key);
  void Reset();

  virtual ~CKey(void);
  CKey& operator=(const CKey& key);
  uint8_t GetLeftTrigger() const;
  uint8_t GetRightTrigger() const;
  float GetLeftThumbX() const;
  float GetLeftThumbY() const;
  float GetRightThumbX() const;
  float GetRightThumbY() const;
  float GetRepeat() const;
  bool FromKeyboard() const;
  bool IsAnalogButton() const;
  bool IsIRRemote() const;
  void SetFromService(bool fromService);
  bool GetFromService() const { return m_fromService; }

  inline uint32_t GetButtonCode() const { return m_buttonCode; }
  inline uint32_t GetKeycode() const { return m_keycode; } // XBMCKey enum in XBMC_keysym.h
  inline uint8_t GetVKey() const { return m_vkey; }
  inline wchar_t GetUnicode() const { return m_unicode; }
  inline char GetAscii() const { return m_ascii; }
  inline uint32_t GetModifiers() const { return m_modifiers; }
  inline uint32_t GetLockingModifiers() const { return m_lockingModifiers; }
  inline unsigned int GetHeld() const { return m_held; }

  enum Modifier
  {
    MODIFIER_CTRL = 0x00010000,
    MODIFIER_SHIFT = 0x00020000,
    MODIFIER_ALT = 0x00040000,
    MODIFIER_RALT = 0x00080000,
    MODIFIER_SUPER = 0x00100000,
    MODIFIER_META = 0X00200000,
    MODIFIER_LONG = 0X01000000,
    MODIFIER_NUMLOCK = 0X02000000,
    MODIFIER_CAPSLOCK = 0X04000000,
    MODIFIER_SCROLLLOCK = 0X08000000,
  };

private:
  uint32_t m_buttonCode;
  uint32_t m_keycode;
  uint8_t m_vkey;
  wchar_t m_unicode;
  char m_ascii;
  uint32_t m_modifiers;
  uint32_t m_lockingModifiers;
  unsigned int m_held;

  uint8_t m_leftTrigger;
  uint8_t m_rightTrigger;
  float m_leftThumbX;
  float m_leftThumbY;
  float m_rightThumbX;
  float m_rightThumbY;
  float m_repeat; // time since last keypress
  bool m_fromService;
};
