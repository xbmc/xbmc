#pragma once
#include <windows.h>
#include <XInput.h>                       // Defines XBOX controller API
#pragma comment(lib, "XInput.lib")        // Library containing necessary 360
                                          // functions

class Xbox360Controller
{
private:
  XINPUT_STATE state;
  int num;
  bool button_down[14];
  bool button_pressed[14];
  bool button_released[14];

  XINPUT_STATE getState();
  void updateButton(int num, int button);
public:
  Xbox360Controller(int num);
  void updateState();
  bool isConnected();
  bool buttonPressed(int num);
  bool buttonReleased(int num);
  bool thumbMoved(int num);
  SHORT getThumb(int num);
  bool triggerMoved(int num);
  BYTE getTrigger(int num);
};
