#pragma once

#include "thread.h"

class CFanController : public CThread
{
public:

  void Start(int targetTemperature);
  void Stop();

  int   GetFanSpeed();
  void  SetFanSpeed(const int fanspeed);
  float GetGPUTemp();
  float GetCPUTemp();
  void  SetTargetTemperature(int targetTemperature);
  void  RestoreStartupSpeed();

  static CFanController* Instance();
	virtual ~CFanController();
private:
  enum SensorType {
    ST_GPU = 0,
    ST_CPU = 1
  };

  static CFanController* _Instance;
  
  int   targetTemp;
  int   systemFanSpeed;
  int   currentFanSpeed;
  int   calculatedFanSpeed;
  int   tooHotLoopCount;
  int   tooColdLoopCount;
  float cpuTemp;
  float cpuLastTemp;
  unsigned short gpuTemp;
  unsigned short gpuLastTemp;

  SensorType     sensor;

  CFanController();
  
  void GetFanSpeedInternal();
  void GetGPUTempInternal();
  void GetCPUTempInternal();

  void CalcSpeed(int targetTemp);

  virtual void		OnStartup();
	virtual void		OnExit();
	virtual void		Process();
};