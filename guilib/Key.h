#ifndef GUILIB_KEY
#define GUILIB_KEY

#pragma once
#include "gui3d.h"

#define KEY_BUTTON_A  1
#define KEY_BUTTON_B  2
#define KEY_BUTTON_X  3
#define KEY_BUTTON_Y  4
#define KEY_BUTTON_BACK  5
#define KEY_BUTTON_START 6
#define KEY_BUTTON_WHITE 7
#define KEY_BUTTON_BLACK 8

#define KEY_BUTTON_DPAD_DOWN  10
#define KEY_BUTTON_DPAD_UP    11
#define KEY_BUTTON_DPAD_LEFT  12
#define KEY_BUTTON_DPAD_RIGHT 13
class CKey
{
public:
  CKey(void);
  CKey(bool bButton, DWORD dwButtonCode);
  CKey(const CKey& key);

  virtual ~CKey(void);
  bool IsButton() const;
  DWORD GetButtonCode() const;
  const CKey& operator=(const CKey& key);

private:
  bool  m_bIsButton;
  DWORD m_dwButtonCode;
};
#endif