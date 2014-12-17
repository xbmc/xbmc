#ifndef TimeKeeper_HPP
#define TimeKeeper_HPP

#ifndef WIN32
#include <sys/time.h>
#endif

#include "timer.h"
#include <pthread.h>

#define HARD_CUT_DELAY 3

class TimeKeeper
{

public:

  TimeKeeper(double presetDuration, double smoothDuration, double easterEgg);
  ~TimeKeeper();

  void UpdateTimers();

  void StartPreset();
  void StartSmoothing();
  void EndSmoothing();
 
  bool CanHardCut();

  double SmoothRatio();
  bool IsSmoothing();

  double GetRunningTime(); 

  double PresetProgressA();
  double PresetProgressB();

  int PresetFrameA();
  int PresetFrameB();

  double sampledPresetDuration();

#ifndef WIN32
  /* The first ticks value of the application */
  struct timeval startTime;
#else  
  long startTime;
#endif /** !WIN32 */

private:

  double _easterEgg;
  double _presetDuration;
  double _presetDurationA;
  double _presetDurationB;
  double _smoothDuration;

  double _currentTime;
  double _presetTimeA;
  double _presetTimeB;
  int _presetFrameA;
  int _presetFrameB;

  bool _isSmoothing;

  pthread_mutex_t _tk_lock;

};
#endif
