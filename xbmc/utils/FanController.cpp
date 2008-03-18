#include "stdafx.h"
#include <ConIo.h>
#include "FanController.h"
#include "../xbox/Undocumented.h"
#include "../xbox/XKExports.h"


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
  unsigned long iDummy;
  bIs16Box = (HalReadSMBusValue(XCALIBUR_ADDRESS, 0, 0, (LPBYTE)&iDummy) == 0);
  cpuTempCount = 0;
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
  if (!g_guiSettings.GetBool("system.autotemperature")) return ;
  int interval = 500;
  tooHotLoopCount = 0;
  tooColdLoopCount = 0;
  while (!m_bStop)
  {
    GetGPUTempInternal();
    GetCPUTempInternal();
    GetFanSpeedInternal();

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

    if (cpuLastTemp.IsValid())
    {
      CalcSpeed(targetTemp);

      SetFanSpeed(calculatedFanSpeed, false);
    }

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
  if (!bIs16Box)
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
  if (m_ThreadHandle == NULL)
  {
    GetFanSpeedInternal();
  }
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

const CTemperature& CFanController::GetGPUTemp()
{
  if (m_ThreadHandle == NULL)
  {
    GetGPUTempInternal();
  }
  return gpuTemp;
}

void CFanController::GetGPUTempInternal()
{
  unsigned long temp;
  HalReadSMBusValue(PIC_ADDRESS, GPU_TEMP, 0, (LPBYTE)&temp);

  gpuTemp = CTemperature::CreateFromCelsius((double)temp);
  // The XBOX v1.6 shows the temp to high! Let's recalc it! It will only do ~minus 10 degress
  if (bIs16Box)
  {
    gpuTemp *= 0.8f;
  }

}

const CTemperature& CFanController::GetCPUTemp()
{
  if (m_ThreadHandle == NULL)
  {
    GetCPUTempInternal();
  }
  return cpuTemp;
}

void CFanController::GetCPUTempInternal()
{
  unsigned short cpu, cpudec;
  float temp1;

  
  if (!bIs16Box)
  { //if it is an old xbox, then do as we have always done
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

    cpuTemp = CTemperature::CreateFromCelsius((float)cpu + (float)cpudec / 256.0f);
  }
  else
  { // if its a 1.6 then we get the CPU temperature from the xcalibur
    _outp(0xc004, (0x70 << 1) | 0x01);  // address
    _outp(0xc008, 0xC1);                // command
    _outpw(0xc000, _inpw(0xc000));      // clear errors
    _outp(0xc002, 0x0d);                // start block transfer
    while ((_inp(0xc000) & 8));         // wait for response
   
    if (!(_inp(0xc000) & 0x23)) // if there was a error then just skip this read..
    {
      _inp(0xc004);                       // read out the data reg (no. bytes in block, will be 4)
      cpudec = _inp(0xc009);              // first byte
      cpu    = _inp(0xc009);              // second byte
      _inp(0xc009);                       // read out the two last bytes, dont' think its neccesary
      _inp(0xc009);                       // but done to be on the safe side

      /* the temperature recieved from the xcalibur is very jumpy, so we try and smooth it
          out by taking the average over 10 samples */
      temp1 = (float)cpu + (float)cpudec / 256;
      temp1 /= 10;
      cpuFrac += temp1;

      if (cpuTempCount == 9) // if we have taken 10 samples then commit the new temperature
      {
        cpuTemp = CTemperature::CreateFromCelsius(cpuFrac);
        cpuTempCount = 0;
        cpuFrac = 0;
      }
      else
      {
        cpuTempCount++;     // increse sample count
      }
    }

    /* the first time we read the temp sensor we set the temperature right away */
    if (cpuTemp == 0)
      cpuTemp = CTemperature::CreateFromCelsius((float)cpu + (float)cpudec / 256);
  }
}


void CFanController::CalcSpeed(int targetTemp)
{
  CTemperature temp;
  CTemperature tempOld;
  CTemperature targetTempFloor;
  CTemperature targetTempCeiling;

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
  targetTempFloor = CTemperature::CreateFromCelsius((float)targetTemp - 0.75f);
  targetTempCeiling = CTemperature::CreateFromCelsius((float)targetTemp + 0.75f);

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
      //CLog::Log(LOGDEBUG,"Lowering fanspeed to %i, tooHotLoopCount=%i tooColdLoopCount=%i", calculatedFanSpeed, tooHotLoopCount, tooColdLoopCount);
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
      //CLog::Log(LOGDEBUG,"Increasing fanspeed to %i, tooHotLoopCount=%i tooColdLoopCount=%i", calculatedFanSpeed, tooHotLoopCount, tooColdLoopCount);
      tooHotLoopCount = 0;
    }
  }

  if (calculatedFanSpeed < 1) {calculatedFanSpeed = 1;} // always keep the fan running
  if (calculatedFanSpeed > 50) {calculatedFanSpeed = 50;}
}
