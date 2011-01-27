#ifndef CPUINFO_H
#define CPUINFO_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <stdio.h>
#include <time.h>
#include "Archive.h"
#include "Temperature.h"
#include <string>
#include <map>

#define CPU_FEATURE_MMX      1 << 0
#define CPU_FEATURE_MMX2     1 << 1
#define CPU_FEATURE_SSE      1 << 2
#define CPU_FEATURE_SSE2     1 << 3
#define CPU_FEATURE_SSE3     1 << 4
#define CPU_FEATURE_SSSE3    1 << 5
#define CPU_FEATURE_SSE4     1 << 6
#define CPU_FEATURE_SSE42    1 << 7
#define CPU_FEATURE_3DNOW    1 << 8
#define CPU_FEATURE_3DNOWEXT 1 << 9
#define CPU_FEATURE_ALTIVEC  1 << 10

struct CoreInfo
{
  int    m_id;
  double m_fSpeed;
  double m_fPct;
  unsigned long long m_user;
  unsigned long long m_nice;
  unsigned long long m_system;
  unsigned long long m_idle;
  unsigned long long m_io;
  CStdString m_strVendor;
  CStdString m_strModel;
  CoreInfo() : m_id(0), m_fSpeed(.0), m_fPct(.0), m_user(0LL), m_nice(0LL), m_system(0LL), m_idle(0LL), m_io(0LL) {}
};

class CCPUInfo
{
public:
  CCPUInfo(void);
  ~CCPUInfo();

  int getUsedPercentage();
  int getCPUCount() { return m_cpuCount; }
  float getCPUFrequency();
  CTemperature getTemperature();
  std::string& getCPUModel() { return m_cpuModel; }

  const CoreInfo &GetCoreInfo(int nCoreId);
  bool HasCoreId(int nCoreId) const;

  CStdString GetCoresUsageString() const;

  unsigned int GetCPUFeatures() { return m_cpuFeatures; }

private:
  bool readProcStat(unsigned long long& user, unsigned long long& nice, unsigned long long& system,
    unsigned long long& idle, unsigned long long& io);
  void ReadCPUFeatures();

  FILE* m_fProcStat;
  FILE* m_fProcTemperature;
  FILE* m_fCPUInfo;

  unsigned long long m_userTicks;
  unsigned long long m_niceTicks;
  unsigned long long m_systemTicks;
  unsigned long long m_idleTicks;
  unsigned long long m_ioTicks;

  int          m_lastUsedPercentage;
  time_t       m_lastReadTime;
  std::string  m_cpuModel;
  int          m_cpuCount;
  unsigned int m_cpuFeatures;

  std::map<int, CoreInfo> m_cores;

};

extern CCPUInfo g_cpuInfo;

#endif
