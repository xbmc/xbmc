#ifndef WIN32
#include <sys/time.h>
#else
#endif /** !WIN32 */
#include <stdlib.h>
#include "TimeKeeper.hpp"
#include "RandomNumberGenerators.hpp"
#include "SectionLock.h"

TimeKeeper::TimeKeeper(double presetDuration, double smoothDuration, double easterEgg)
  {    
    _smoothDuration = smoothDuration;
    _presetDuration = presetDuration;
    _easterEgg = easterEgg;
#ifdef _USE_THREADS
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_tk_lock, &attr);
    pthread_mutexattr_destroy(&attr);
#endif

#ifndef WIN32
	gettimeofday ( &this->startTime, NULL );
#else
	startTime = GetTickCount();
#endif /** !WIN32 */

	UpdateTimers();
  }

  TimeKeeper::~TimeKeeper()
  {
#ifdef _USE_THREADS
    pthread_mutex_destroy(&_tk_lock);
#endif
  }

  void TimeKeeper::UpdateTimers()
  {
        CSectionLock lock(&_tk_lock);
#ifndef WIN32
        _currentTime = getTicks ( &startTime ) * 0.001;
#else
	_currentTime = getTicks ( startTime ) * 0.001;
#endif /** !WIN32 */

	_presetFrameA++;
	_presetFrameB++;

  }

  void TimeKeeper::StartPreset()
  {
    CSectionLock lock(&_tk_lock);
    _isSmoothing = false;
    _presetTimeA = _currentTime;
    _presetFrameA = 1;
    _presetDurationA = sampledPresetDuration();
  }
  void TimeKeeper::StartSmoothing()
  {
    CSectionLock lock(&_tk_lock);
    _isSmoothing = true;
    _presetTimeB = _currentTime;
    _presetFrameB = 1;
    _presetDurationB = sampledPresetDuration();
  }
  void TimeKeeper::EndSmoothing()
  {
    CSectionLock lock(&_tk_lock);
    _isSmoothing = false;
    _presetTimeA = _presetTimeB;
    _presetFrameA = _presetFrameB;
    _presetDurationA = _presetDurationB;
  }
 
  bool TimeKeeper::CanHardCut()
  {
    CSectionLock lock(&_tk_lock);
    return ((_currentTime - _presetTimeA) > HARD_CUT_DELAY);      
  }

  double TimeKeeper::SmoothRatio()
  {
    CSectionLock lock(&_tk_lock);
    return (_currentTime - _presetTimeB) / _smoothDuration;
  }
  bool TimeKeeper::IsSmoothing()
  {  
    CSectionLock lock(&_tk_lock);
    return _isSmoothing;
  }

  double TimeKeeper::GetRunningTime()
  {
    CSectionLock lock(&_tk_lock);
    return _currentTime;
  } 

  double TimeKeeper::PresetProgressA()
  {
    CSectionLock lock(&_tk_lock);
    if (_isSmoothing) return 1.0;
    else return (_currentTime - _presetTimeA) / _presetDurationA;
  }
  double TimeKeeper::PresetProgressB()
  {
    CSectionLock lock(&_tk_lock);
    return (_currentTime - _presetTimeB) / _presetDurationB;
  }

int TimeKeeper::PresetFrameB()
  {
    CSectionLock lock(&_tk_lock);
    return _presetFrameB;
  }

int TimeKeeper::PresetFrameA()
  {
    CSectionLock lock(&_tk_lock);
    return _presetFrameA;
  }

double TimeKeeper::sampledPresetDuration() {
    CSectionLock lock(&_tk_lock);
#ifdef WIN32
	return  _presetDuration;
#else
		return fmax(1, fmin(60, RandomNumberGenerators::gaussian
			(_presetDuration, _easterEgg)));
#endif
}
