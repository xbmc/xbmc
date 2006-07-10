/*****************************************************************
|
|   Neptune - Time
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_TIME_H_
#define _NPT_TIME_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"

/*----------------------------------------------------------------------
|   NPT_TimeStamp
+---------------------------------------------------------------------*/
class NPT_TimeStamp
{
 public:
    // methods
    NPT_TimeStamp(unsigned long seconds = 0, unsigned long nano_seconds = 0) :
        m_Seconds(seconds), m_NanoSeconds(nano_seconds) {}
    NPT_TimeStamp(float seconds);
    operator float() const;
    NPT_TimeStamp& operator+=(const NPT_TimeStamp& time_stamp);
    NPT_TimeStamp& operator-=(const NPT_TimeStamp& time_stamp);
    bool operator==(const NPT_TimeStamp& time_stamp) const;
    bool operator!=(const NPT_TimeStamp& time_stamp) const;
    bool operator>(const NPT_TimeStamp& time_stamp) const;
    bool operator<(const NPT_TimeStamp& time_stamp) const;
    bool operator>=(const NPT_TimeStamp& time_stamp) const;
    bool operator<=(const NPT_TimeStamp& time_stamp) const;

    // friend operators
    friend NPT_TimeStamp operator+(const NPT_TimeStamp& timestamp, long seconds);
    friend NPT_TimeStamp operator-(const NPT_TimeStamp& timestamp, long seconds);

    // members
    long m_Seconds;
    long m_NanoSeconds;
};

/*----------------------------------------------------------------------
|   operator+
+---------------------------------------------------------------------*/
inline 
NPT_TimeStamp 
operator+(const NPT_TimeStamp& t1, const NPT_TimeStamp& t2) 
{
    NPT_TimeStamp t = t1;
    return t += t2;
}

/*----------------------------------------------------------------------
|   operator-
+---------------------------------------------------------------------*/
inline 
NPT_TimeStamp 
operator-(const NPT_TimeStamp& t1, const NPT_TimeStamp& t2) 
{
    NPT_TimeStamp t = t1;
    return t -= t2;
}

/*----------------------------------------------------------------------
|   NPT_TimeInterval
+---------------------------------------------------------------------*/
typedef NPT_TimeStamp NPT_TimeInterval;

#endif // _NPT_TIME_H_
