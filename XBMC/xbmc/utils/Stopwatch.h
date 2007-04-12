#pragma once

class CStopWatch
{
public:
  CStopWatch();
  ~CStopWatch();

  bool IsRunning() const;
  void StartZero();
  void Stop();
  void Reset();

  float GetElapsedSeconds() const;
  float GetElapsedMilliseconds() const;
private:
  __int64 GetTicks() const;
  float m_timerPeriod;        // to save division in GetElapsed...()
  __int64 m_startTick;
  bool m_isRunning;
};
