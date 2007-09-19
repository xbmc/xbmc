#ifndef MOUSE_H
#define MOUSE_H

#define MOUSE_DOUBLE_CLICK_LENGTH 500L
#define MOUSE_ACTIVE_LENGTH   5000L

// states
#define MOUSE_STATE_NORMAL  1
#define MOUSE_STATE_FOCUS  2
#define MOUSE_STATE_DRAG  3
#define MOUSE_STATE_CLICK  4

// button ids
#define MOUSE_LEFT_BUTTON 0
#define MOUSE_RIGHT_BUTTON 1
#define MOUSE_MIDDLE_BUTTON 2
#define MOUSE_EXTRA_BUTTON1 3
#define MOUSE_EXTRA_BUTTON2 4

#include "../Geometry.h"

class CMouse
{
public:
  CMouse();
  ~CMouse();

  void Initialize(HWND hWnd);
  bool IsActive() const;
  bool HasMoved() const;
  void Update();
  void SetResolution(int nXMax, int nYMax, float fXSpeed, float fYSpeed);
  void SetInactive();
  void SetExclusiveAccess(DWORD dwControlID, DWORD dwWindowID, const CPoint &point);
  void EndExclusiveAccess(DWORD dwControlID, DWORD dwWindowID);
  DWORD GetExclusiveWindowID() const { return m_dwExclusiveWindowID;};
  DWORD GetExclusiveControlID() const { return m_dwExclusiveControlID;};
  const CPoint &GetExclusiveOffset() const { return m_exclusiveOffset; };
  void SetState(DWORD dwState) {m_dwState = dwState;};
  DWORD GetState() const { return m_dwState;};

private:
  // variables for mouse state
  XINPUT_STATE m_MouseState[4*2];   // one for each port
  HANDLE m_hMouseDevice[4*2];  // handle to each device
  DWORD m_dwLastMousePacket[4*2]; // last packet received from mouse
  DWORD m_dwMousePort; // mask of ports that currently hold a mouse
  XINPUT_STATE m_CurrentState;

  // variables for resolution bounding + speed setting
  float m_fSpeedX;
  float m_fSpeedY;
  int m_iMaxX;
  int m_iMaxY;

  // stuff to hold the clicks
  DWORD dwLastClickTime[5];
  DWORD dwLastActiveTime;
  bool m_bActive;
  bool bDown[5];  // is the button down?
  bool bLastDown[5]; // was it down last frame?

  // exclusive access to mouse from a control
  DWORD m_dwExclusiveWindowID;
  DWORD m_dwExclusiveControlID;
  CPoint m_exclusiveOffset;

  // state of the mouse
  DWORD m_dwState;

public:
  bool bClick[5];
  bool bDoubleClick[5];
  bool bHold[5];
  char cMickeyX;
  char cMickeyY;
  char cWheel;
  float posX;
  float posY;
};

extern CMouse g_Mouse;

#endif

