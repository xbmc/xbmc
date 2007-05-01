#pragma once

#include "Thread.h"
#include "../Temperature.h"

class CFanController : public CThread
{
public:

  void Start(int targetTemperature);
  void Stop();

  int GetFanSpeed();
  void SetFanSpeed(const int fanspeed, const bool force = true);
  const CTemperature& GetGPUTemp();
  const CTemperature& GetCPUTemp();
  void SetTargetTemperature(int targetTemperature);
  void RestoreStartupSpeed();

  static CFanController* Instance();
  static void RemoveInstance();
  virtual ~CFanController();
private:
  enum SensorType {
    ST_GPU = 0,
    ST_CPU = 1
  };

  static CFanController* _Instance;

  int targetTemp;
  int systemFanSpeed;
  unsigned long currentFanSpeed;
  int calculatedFanSpeed;
  int tooHotLoopCount;
  int tooColdLoopCount;
  bool inCustomMode;
  bool bIs16Box;
  CTemperature cpuTemp;
  float cpuFrac;
  int   cpuTempCount;
  CTemperature cpuLastTemp;
  CTemperature gpuTemp;
  CTemperature gpuLastTemp;

  SensorType sensor;

  CFanController();

  void GetFanSpeedInternal();
  void GetGPUTempInternal();
  void GetCPUTempInternal();

  void CalcSpeed(int targetTemp);

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
};
