#ifndef MOUSE_H
#define MOUSE_H

#include "../Geometry.h"

enum MOUSE_STATE { MOUSE_STATE_NORMAL = 1, MOUSE_STATE_FOCUS, MOUSE_STATE_DRAG, MOUSE_STATE_CLICK };
enum MOUSE_BUTTON { MOUSE_LEFT_BUTTON = 0, MOUSE_RIGHT_BUTTON, MOUSE_MIDDLE_BUTTON, MOUSE_EXTRA_BUTTON1, MOUSE_EXTRA_BUTTON2 };

struct MouseState
{
  int x;            // x location
  int y;            // y location
  char dx;          // change in x
  char dy;          // change in y
  char dz;          // change in z (wheel)
  bool button[5];   // true if a button is down
  bool active;      // true if the mouse is active
};

class IMouseDevice
{
public:
  virtual void Initialize(void *appData = NULL)=0;
  virtual void Acquire()=0;
  virtual bool Update(MouseState &state)=0;
};

class CMouse
{
public:

  CMouse();
  ~CMouse();

  void Initialize(void *appData = NULL);
  void Update();
  void Acquire();
  void SetResolution(int maxX, int maxY, float speedX, float speedY);
  bool IsActive() const;
  bool HasMoved() const;
  void SetInactive();
  void SetExclusiveAccess(DWORD dwControlID, DWORD dwWindowID, const CPoint &point);
  void EndExclusiveAccess(DWORD dwControlID, DWORD dwWindowID);
  DWORD GetExclusiveWindowID() const { return m_exclusiveWindowID; };
  DWORD GetExclusiveControlID() const { return m_exclusiveControlID; };
  const CPoint &GetExclusiveOffset() const { return m_exclusiveOffset; };
  void SetState(DWORD state) { m_pointerState = state; };
  DWORD GetState() const { return m_pointerState; };
  CPoint GetLocation() const;
  void SetLocation(CPoint &point);
  CPoint GetLastMove() const;
  char GetWheel() const;

private:
  // exclusive access to mouse from a control
  DWORD m_exclusiveWindowID;
  DWORD m_exclusiveControlID;
  CPoint m_exclusiveOffset;
  
  // state of the mouse
  DWORD m_pointerState;
  MouseState m_mouseState;
  bool m_lastDown[5];

  // mouse device
  IMouseDevice *m_mouseDevice;

  // mouse limits and speed
  int m_maxX;
  int m_maxY;
  float m_speedX;
  float m_speedY;

  // active/click timers
  DWORD m_lastActiveTime;
  DWORD m_lastClickTime[5];

public:
  // public access variables to button clicks etc.
  bool bClick[5];
  bool bDoubleClick[5];
  bool bHold[5];
};

extern CMouse g_Mouse;

#endif

