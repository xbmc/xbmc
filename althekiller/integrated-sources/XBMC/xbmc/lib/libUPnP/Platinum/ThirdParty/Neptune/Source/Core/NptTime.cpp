/*****************************************************************
|
|   Neptune - Time
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTime.h"

/*----------------------------------------------------------------------
|   NPT_TimeStamp::NPT_TimeStamp
+---------------------------------------------------------------------*/
NPT_TimeStamp::NPT_TimeStamp(float seconds)
{
    m_Seconds     = (long)seconds;
    m_NanoSeconds = (long)(1E9f*(seconds - (float)m_Seconds));
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator float
+---------------------------------------------------------------------*/
NPT_TimeStamp::operator float() const
{       
    return ((float)m_Seconds) + ((float)m_NanoSeconds)/1E9f;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator+=
+---------------------------------------------------------------------*/
NPT_TimeStamp&
NPT_TimeStamp::operator+=(const NPT_TimeStamp& t)
{
    m_Seconds += t.m_Seconds;
    m_NanoSeconds += t.m_NanoSeconds;
    if (m_NanoSeconds <= -1000000000) {
        m_Seconds--;
        m_NanoSeconds += 1000000000;
    } else if (m_NanoSeconds >= 1000000000) {
        m_Seconds++;
        m_NanoSeconds -= 1000000000;
    }
    if (m_NanoSeconds < 0 && m_Seconds > 0) {
        m_Seconds--;
        m_NanoSeconds += 1000000000;
    }
    return *this;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator-=
+---------------------------------------------------------------------*/
NPT_TimeStamp&
NPT_TimeStamp::operator-=(const NPT_TimeStamp& t)
{
    m_Seconds -= t.m_Seconds;
    m_NanoSeconds -= t.m_NanoSeconds;
    if (m_NanoSeconds <= -1000000000) {
        m_Seconds--;
        m_NanoSeconds += 1000000000;
    } else if (m_NanoSeconds >= 1000000000) {
        m_Seconds++;
        m_NanoSeconds -= 1000000000;
    }
    if (m_NanoSeconds < 0 && m_Seconds > 0) {
        m_Seconds--;
        m_NanoSeconds += 1000000000;
    }
    return *this;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator==
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator==(const NPT_TimeStamp& t) const
{
    return 
        m_Seconds     == t.m_Seconds && 
        m_NanoSeconds == t.m_NanoSeconds;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator!=
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator!=(const NPT_TimeStamp& t) const
{
    return 
        m_Seconds     != t.m_Seconds || 
        m_NanoSeconds != t.m_NanoSeconds;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator>
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator>(const NPT_TimeStamp& t) const
{
    return 
        m_Seconds   > t.m_Seconds || 
        (m_Seconds == t.m_Seconds && m_NanoSeconds > t.m_NanoSeconds);
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator<
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator<(const NPT_TimeStamp& t) const
{
    return 
        m_Seconds   < t.m_Seconds || 
        (m_Seconds == t.m_Seconds && m_NanoSeconds < t.m_NanoSeconds);
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator>=
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator>=(const NPT_TimeStamp& t) const
{
    return *this > t || *this == t;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator<=
+---------------------------------------------------------------------*/
bool
NPT_TimeStamp::operator<=(const NPT_TimeStamp& t) const
{
    return *this < t || *this == t;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator+
+---------------------------------------------------------------------*/
NPT_TimeStamp
operator+(const NPT_TimeStamp& timestamp, long seconds)
{
    // shortcut
    if (seconds == 0) return NPT_TimeStamp(timestamp);

    NPT_TimeStamp result = timestamp;
    result.m_Seconds += seconds;

    return result;
}

/*----------------------------------------------------------------------
|   NPT_TimeStamp::operator-
+---------------------------------------------------------------------*/
NPT_TimeStamp
operator-(const NPT_TimeStamp& timestamp, long seconds)
{
    // shortcut
    if (seconds == 0) return NPT_TimeStamp(timestamp);

    NPT_TimeStamp result = timestamp;
    result.m_Seconds -= seconds;

    return result;
}

