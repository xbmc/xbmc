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



#define KEY_REMOTE_REVERSE    100
#define KEY_REMOTE_FORWARD    101
#define KEY_REMOTE_PLAY       102
#define KEY_REMOTE_STOP       103
#define KEY_REMOTE_SKIPMINUS  104
#define KEY_REMOTE_SKIPPLUS   105
#define KEY_REMOTE_PAUSE      106
#define KEY_REMOTE_TITLE      107
#define KEY_REMOTE_SELECT     108
#define KEY_REMOTE_INFO       109
#define KEY_REMOTE_BACK       110
#define KEY_REMOTE_DISPLAY    111
#define KEY_REMOTE_MENU       112

#define KEY_REMOTE_DOWN       113
#define KEY_REMOTE_UP         114
#define KEY_REMOTE_LEFT       115
#define KEY_REMOTE_RIGHT      116

#define KEY_REMOTE_0          200
#define KEY_REMOTE_1          201
#define KEY_REMOTE_2          202
#define KEY_REMOTE_3          203
#define KEY_REMOTE_4          204
#define KEY_REMOTE_5          205
#define KEY_REMOTE_6          206
#define KEY_REMOTE_7          207
#define KEY_REMOTE_8          208
#define KEY_REMOTE_9          209


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