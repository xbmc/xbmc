#include "CPUInfo.h"
#include <string>
#include <string.h>

using namespace std;

// In seconds
#define MINIMUM_TIME_BETWEEN_READS 3

CCPUInfo::CCPUInfo(void)
{
  m_fProcStat = fopen("/proc/stat", "r");
  m_fProcTemperature = fopen("/proc/acpi/thermal_zone/THRM/temperature", "r");
  if (m_fProcTemperature == NULL)
    m_fProcTemperature = fopen("/proc/acpi/thermal_zone/THR1/temperature", "r");
  m_lastUsedPercentage = 0;
  readProcStat(m_userTicks, m_niceTicks, m_systemTicks, m_idleTicks);  

  FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
  m_cpuCount = 0;
  m_cpuFreq = 0;
  if (cpuinfo)
  {
    char buffer[512];

    while (fgets(buffer, sizeof(buffer), cpuinfo))
    {
      if (strncmp(buffer, "processor", strlen("processor"))==0)
      {
        m_cpuCount++;
      }
      else if (strncmp(buffer, "cpu MHz", strlen("cpu MHz"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          sscanf(needle, "%f", &m_cpuFreq);
        }
      }
      else if (strncmp(buffer, "model name", strlen("model name"))==0)
      {
        char *needle = strstr(buffer, ":");
        if (needle && strlen(needle)>3)
        {
          needle+=2;
          m_cpuModel = needle;
        }
      }
    }
    fclose(cpuinfo);
  }
  else
  {
    m_cpuCount = 1;
    m_cpuModel = "Unknown";
  }
}

CCPUInfo::~CCPUInfo()
{
  if (m_fProcStat != NULL)
    fclose(m_fProcStat);
    
  if (m_fProcTemperature != NULL)
    fclose(m_fProcTemperature);
}
  
int CCPUInfo::getUsedPercentage()
{
  if (m_lastReadTime + MINIMUM_TIME_BETWEEN_READS > time(NULL))
  {
    return m_lastUsedPercentage;
  }

  unsigned long long userTicks;
  unsigned long long niceTicks;
  unsigned long long systemTicks;
  unsigned long long idleTicks;
       
  if (!readProcStat(userTicks, niceTicks, systemTicks, idleTicks))
  {
    return 0;
  }
  
  userTicks -= m_userTicks;
  niceTicks -= m_niceTicks;
  systemTicks -= m_systemTicks;
  idleTicks -= m_idleTicks;
  
  int result = (int) ((userTicks + niceTicks + systemTicks) * 100 / (userTicks + niceTicks + systemTicks + idleTicks));
  
  m_userTicks += userTicks;
  m_niceTicks += niceTicks;
  m_systemTicks += systemTicks;
  m_idleTicks += idleTicks;
  
  m_lastUsedPercentage = result;
  
  return result;
}

CTemperature CCPUInfo::getTemperature()
{
  int value;
  char scale;
 
  if (m_fProcTemperature == NULL)
    return CTemperature();

  rewind(m_fProcTemperature);
  fflush(m_fProcTemperature);
  
  char buf[256];
  if (!fgets(buf, sizeof(buf), m_fProcTemperature))
    return CTemperature();

  int num = sscanf(buf, "temperature: %d %c", &value, &scale);
  if (num != 2)
    return CTemperature();

  if (scale == 'C' || scale == 'c')
    return CTemperature::CreateFromCelsius(value);
  if (scale == 'F' || scale == 'f')
    return CTemperature::CreateFromFahrenheit(value);
  else
    return CTemperature();
}

bool CCPUInfo::readProcStat(unsigned long long& user, unsigned long long& nice, 
    unsigned long long& system, unsigned long long& idle)
{
  if (m_fProcStat == NULL)
    return false;
    
  rewind(m_fProcStat);
  fflush(m_fProcStat);
  
  char buf[256];
  if (!fgets(buf, sizeof(buf), m_fProcStat))
    return false;
    
  int num = sscanf(buf, "cpu %llu %llu %llu %llu %*s", &user, &nice, &system, &idle);
  if (num < 4)
    return false;
   
  m_lastReadTime = time(NULL);
  
  return true;
}

CCPUInfo g_cpuInfo;

/*
int main()
{
  CCPUInfo c;
  usleep(...);
  int r = c.getUsedPercentage();
  printf("%d\n", r);
}
*/
