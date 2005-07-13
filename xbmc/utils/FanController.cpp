
#include "../stdafx.h"
#include <ConIo.h>
#include "FanController.h"
#include "../xbox/undocumented.h"
#include "../xbox/XKExports.h"
#include "SystemInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PIC_ADDRESS      0x20
#define XCALIBUR_ADDRESS 0xE0 // XCalibur/1.6 videochip
#define FAN_MODE         0x05 // Enable/ disable the custom fan speeds (0/1)
#define FAN_REGISTER     0x06 // Set custom fan speeds (0-50)
#define FAN_READBACK     0x10 // Current fan speed (0-50)
#define GPU_TEMP         0x0A // GPU Temperature
#define CPU_TEMP         0x09 // CPU Temperature

CFanController* CFanController::_Instance = NULL;

CFanController* CFanController::Instance()
{
  if (_Instance == NULL)
  {
    _Instance = new CFanController();
  }
  return _Instance;
}

void CFanController::RemoveInstance()
{
  if (_Instance)
  {
    _Instance->Stop();
    delete _Instance;
    _Instance=NULL;
  }
}

CFanController::CFanController()
{
  inCustomMode = false;
  systemFanSpeed = GetFanSpeed();
  currentFanSpeed = systemFanSpeed;
  calculatedFanSpeed = systemFanSpeed;
}


CFanController::~CFanController()
{
  _Instance = NULL;
}

void CFanController::OnStartup()
{}

void CFanController::OnExit()
{}

void CFanController::Process()
{
  if (!g_guiSettings.GetBool("System.AutoTemperature")) return ;
  int interval = 500;
  tooHotLoopCount = 0;
  tooColdLoopCount = 0;
  while (!m_bStop)
  {
    GetGPUTempInternal();
    GetCPUTempInternal();

    // Use the highest temperature, if the temperatures are
    // equal, go with the CPU temperature.
    if (cpuTemp >= gpuTemp)
    {
      sensor = ST_CPU;
    }
    else
    {
      sensor = ST_GPU;
    }

    CalcSpeed(targetTemp);

    SetFanSpeed(calculatedFanSpeed, false);

    cpuLastTemp = cpuTemp;
    gpuLastTemp = gpuTemp;

    Sleep(interval);
  }
}

void CFanController::SetTargetTemperature(int targetTemperature)
{
  targetTemp = targetTemperature;
}

void CFanController::RestoreStartupSpeed()
{
  SetFanSpeed(systemFanSpeed);
  Sleep(100);
  //if it's not a 1.6 box disable custom fanmode
  int iDummy;
  if (HalReadSMBusValue(XCALIBUR_ADDRESS, 0, 0, (LPBYTE)&iDummy) != 0)
  {
    //disable custom fanmode
    HalWriteSMBusValue(PIC_ADDRESS, FAN_MODE, 0, 0);
  }
  inCustomMode = false;
}

void CFanController::Start(int targetTemperature)
{
  StopThread();
  targetTemp = targetTemperature;
  Create();
}

void CFanController::Stop()
{
  StopThread();
  if (inCustomMode)
  {
    RestoreStartupSpeed();
  }
}

int CFanController::GetFanSpeed()
{
  GetFanSpeedInternal();
  return currentFanSpeed;
}

void CFanController::GetFanSpeedInternal()
{
  HalReadSMBusValue(PIC_ADDRESS, FAN_READBACK, 0, (LPBYTE)&currentFanSpeed);
}

void CFanController::SetFanSpeed(const int fanspeed, const bool force)
{
  if (fanspeed < 0) return ;
  if (fanspeed > 50) return ;
  if ((currentFanSpeed == fanspeed) && (!force)) return ;
  if (force)
  {
    //on boot or first time set it needs a kickstart in releasemode for some reason
    //it works fine without this block in debugmode...
    HalWriteSMBusValue(PIC_ADDRESS, FAN_MODE, 0, 1);
    Sleep(10);
    HalWriteSMBusValue(PIC_ADDRESS, FAN_REGISTER, 0, fanspeed);
  }
  //enable custom fanspeeds
  HalWriteSMBusValue(PIC_ADDRESS, FAN_MODE, 0, 1);
  Sleep(10);
  HalWriteSMBusValue(PIC_ADDRESS, FAN_REGISTER, 0, fanspeed);
  Sleep(10);
  currentFanSpeed = fanspeed;
  inCustomMode = true;
}

float CFanController::GetGPUTemp()
{
  if (m_ThreadHandle == NULL)
  {
    GetGPUTempInternal();
  }
  return gpuTemp;
}

void CFanController::GetGPUTempInternal()
{
  HalReadSMBusValue(PIC_ADDRESS, GPU_TEMP, 0, (LPBYTE)&fGPUTemp);

  CStdString strXboxVer;
  SYSINFO::GetXBOXVersionDetected(strXboxVer);
  if (strXboxVer == "v1.6") 
  { // The XBOX v1.6 shows the temp to high! Let's recalc it! It will only do ~minus 10 degress
     gpuTemp = ((float)fGPUTemp * 0.8f);
  }
  else gpuTemp = fGPUTemp;
}

float CFanController::GetCPUTemp()
{
  if (m_ThreadHandle == NULL)
  {
    GetCPUTempInternal();
  }
  return cpuTemp;
}

void CFanController::GetCPUTempInternal()
{
  //HalReadSMBusValue(PIC_ADDRESS, CPU_TEMP, 0, (LPBYTE)&cpuTemp);

  unsigned short cpu, cpudec;

  _outp(0xc004, (0x4c << 1) | 0x01);
  _outp(0xc008, 0x01);
  _outpw(0xc000, _inpw(0xc000));
  _outp(0xc002, (0) ? 0x0b : 0x0a);
  while ((_inp(0xc000) & 8));
  cpu = _inpw(0xc006);

  _outp(0xc004, (0x4c << 1) | 0x01);
  _outp(0xc008, 0x10);
  _outpw(0xc000, _inpw(0xc000));
  _outp(0xc002, (0) ? 0x0b : 0x0a);
  while ((_inp(0xc000) & 8));
  cpudec = _inpw(0xc006);

  CStdString strXboxVer;
  SYSINFO::GetXBOXVersionDetected(strXboxVer);
  if (strXboxVer == "v1.6") 
  { // The XBOX v1.6 shows the temp to high! Let's recalc it! it will only do ~minus 10 degress
    cpuTemp = ((float)cpu + (float)cpudec / 256.0f )* 0.8f;
  }
  else
  { // else let's show then the normal Temp!
    cpuTemp = (float)cpu + (float)cpudec / 256.0f;
  }

}


void CFanController::CalcSpeed(int targetTemp)
{
  float temp;
  float tempOld;
  float targetTempFloor;
  float targetTempCeiling;

  if (sensor == ST_GPU)
  {
    temp = gpuTemp;
    tempOld = gpuLastTemp;
  }
  else
  {
    temp = cpuTemp;
    tempOld = cpuLastTemp;
  }
  targetTempFloor = (float)targetTemp - 0.75f;
  targetTempCeiling = (float)targetTemp + 0.75f;

  if ((temp >= targetTempFloor) && (temp <= targetTempCeiling))
  {
    //within range, try to keep it steady
    tooHotLoopCount = 0;
    tooColdLoopCount = 0;
    if (temp > tempOld)
    {
      calculatedFanSpeed++;
    }
    else if (temp < tempOld)
    {
      calculatedFanSpeed--;
    }
  }

  else if (temp < targetTempFloor)
  {
    //cool, lower speed unless it's getting hotter
    if (temp == tempOld)
    {
      tooColdLoopCount++;
    }
    else if (temp > tempOld)
    {
      tooColdLoopCount--;
    }
    if ((temp < tempOld) || (tooColdLoopCount == 12))
    {
      calculatedFanSpeed--;
      //CLog::DebugLog("Lowering fanspeed to %i, tooHotLoopCount=%i tooColdLoopCount=%i", calculatedFanSpeed, tooHotLoopCount, tooColdLoopCount);
      tooColdLoopCount = 0;
    }
  }

  else if (temp > targetTempCeiling)
  {
    //hot, increase fanspeed if it's still getting hotter or not getting any cooler for at leat loopcount*sleepvalue
    if (temp == tempOld)
    {
      tooHotLoopCount++;
    }
    else if (temp < tempOld)
    {
      tooHotLoopCount--;
    }
    if ((temp > tempOld) || (tooHotLoopCount == 12))
    {
      calculatedFanSpeed++;
      //CLog::DebugLog("Increasing fanspeed to %i, tooHotLoopCount=%i tooColdLoopCount=%i", calculatedFanSpeed, tooHotLoopCount, tooColdLoopCount);
      tooHotLoopCount = 0;
    }
  }

  if (calculatedFanSpeed < 1) {calculatedFanSpeed = 1;} // always keep the fan running
  if (calculatedFanSpeed > 50) {calculatedFanSpeed = 50;}
}
