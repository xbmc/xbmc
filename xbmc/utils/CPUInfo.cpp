#include "CPUInfo.h"

// In seconds
#define MINIMUM_TIME_BETWEEN_READS 3

CCPUInfo::CCPUInfo(void)
{
  m_fProcStat = fopen("/proc/stat", "r");
  m_fProcTemperature = NULL; // until we implement it
  m_lastUsedPercentage = 0;
  readProcStat(m_userTicks, m_niceTicks, m_systemTicks, m_idleTicks);  
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

int CCPUInfo::getTemperatureC()
{
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
    
  int num = sscanf(buf, "cpu %Lu %Lu %Lu %Lu %*s", &user, &nice, &system, &idle);
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
