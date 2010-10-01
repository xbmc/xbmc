#include "utils.h"
#include <math.h>

#include <windows.h>
#include <mmsystem.h>

double ticksPerSecond = 0;
bool timerInitialized = false;
bool usePerformance   = false;
double performanceBase = 0;
double GetSystemTimer()
{
	if( !timerInitialized )
	{
		// We need to know how often the clock is updated
		__int64 tps;
		if( !QueryPerformanceFrequency((LARGE_INTEGER *)&tps) )
			usePerformance = false;
		else
		{
			usePerformance = true;
			ticksPerSecond = (double)tps;

			__int64 ticks;
			QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
			performanceBase = (double)ticks/ticksPerSecond;
		}

		timerInitialized = true;
	}

	if( usePerformance )
	{
		__int64 ticks;
		QueryPerformanceCounter((LARGE_INTEGER *)&ticks);

		double t = (double)ticks/ticksPerSecond - performanceBase;

		// We need to calibrate the performance timer as it is known to jump from time to time
		double t2 = (double)timeGetTime()/1000.0;
		if( fabs(t-t2) > 0.1 )
		{
			performanceBase += t - t2;
			t = t2;
		}

		return t;
	}
	else
        return (double)timeGetTime()/1000.0;
}

