/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* 
 * MT safe
 */

#include "config.h"
#include "glibconfig.h"

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifndef G_OS_WIN32
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#endif /* G_OS_WIN32 */

#ifdef G_OS_WIN32
#include <windows.h>
#endif /* G_OS_WIN32 */

#include "glib.h"
#include "gthread.h"
#include "galias.h"

#define G_NSEC_PER_SEC 1000000000

#define GETTIME(v) (v = g_thread_gettime ())

struct _GTimer
{
  guint64 start;
  guint64 end;

  guint active : 1;
};


GTimer*
g_timer_new (void)
{
  GTimer *timer;

  timer = g_new (GTimer, 1);
  timer->active = TRUE;

  GETTIME (timer->start);

  return timer;
}

void
g_timer_destroy (GTimer *timer)
{
  g_return_if_fail (timer != NULL);

  g_free (timer);
}

void
g_timer_start (GTimer *timer)
{
  g_return_if_fail (timer != NULL);

  timer->active = TRUE;

  GETTIME (timer->start);
}

void
g_timer_stop (GTimer *timer)
{
  g_return_if_fail (timer != NULL);

  timer->active = FALSE;

  GETTIME (timer->end);
}

void
g_timer_reset (GTimer *timer)
{
  g_return_if_fail (timer != NULL);

  GETTIME (timer->start);
}

void
g_timer_continue (GTimer *timer)
{
  guint64 elapsed;

  g_return_if_fail (timer != NULL);
  g_return_if_fail (timer->active == FALSE);

  /* Get elapsed time and reset timer start time
   *  to the current time minus the previously
   *  elapsed interval.
   */

  elapsed = timer->end - timer->start;

  GETTIME (timer->start);

  timer->start -= elapsed;

  timer->active = TRUE;
}

gdouble
g_timer_elapsed (GTimer *timer,
		 gulong *microseconds)
{
  gdouble total;
  gint64 elapsed;

  g_return_val_if_fail (timer != NULL, 0);

  if (timer->active)
    GETTIME (timer->end);

  elapsed = timer->end - timer->start;

  total = elapsed / 1e9;

  if (microseconds)
    *microseconds = (elapsed / 1000) % 1000000;

  return total;
}

void
g_usleep (gulong microseconds)
{
#ifdef G_OS_WIN32
  Sleep (microseconds / 1000);
#else /* !G_OS_WIN32 */
# ifdef HAVE_NANOSLEEP
  struct timespec request, remaining;
  request.tv_sec = microseconds / G_USEC_PER_SEC;
  request.tv_nsec = 1000 * (microseconds % G_USEC_PER_SEC);
  while (nanosleep (&request, &remaining) == -1 && errno == EINTR)
    request = remaining;
# else /* !HAVE_NANOSLEEP */
#  ifdef HAVE_NSLEEP
  /* on AIX, nsleep is analogous to nanosleep */
  struct timespec request, remaining;
  request.tv_sec = microseconds / G_USEC_PER_SEC;
  request.tv_nsec = 1000 * (microseconds % G_USEC_PER_SEC);
  while (nsleep (&request, &remaining) == -1 && errno == EINTR)
    request = remaining;
#  else /* !HAVE_NSLEEP */
  if (g_thread_supported ())
    {
      static GStaticMutex mutex = G_STATIC_MUTEX_INIT;
      static GCond* cond = NULL;
      GTimeVal end_time;
      
      g_get_current_time (&end_time);
      if (microseconds > G_MAXLONG)
	{
	  microseconds -= G_MAXLONG;
	  g_time_val_add (&end_time, G_MAXLONG);
	}
      g_time_val_add (&end_time, microseconds);

      g_static_mutex_lock (&mutex);
      
      if (!cond)
	cond = g_cond_new ();
      
      while (g_cond_timed_wait (cond, g_static_mutex_get_mutex (&mutex), 
				&end_time))
	/* do nothing */;
      
      g_static_mutex_unlock (&mutex);
    }
  else
    {
      struct timeval tv;
      tv.tv_sec = microseconds / G_USEC_PER_SEC;
      tv.tv_usec = microseconds % G_USEC_PER_SEC;
      select(0, NULL, NULL, NULL, &tv);
    }
#  endif /* !HAVE_NSLEEP */
# endif /* !HAVE_NANOSLEEP */
#endif /* !G_OS_WIN32 */
}

/**
 * g_time_val_add:
 * @time_: a #GTimeVal
 * @microseconds: number of microseconds to add to @time
 *
 * Adds the given number of microseconds to @time_. @microseconds can
 * also be negative to decrease the value of @time_.
 **/
void 
g_time_val_add (GTimeVal *time_, glong microseconds)
{
  g_return_if_fail (time_->tv_usec >= 0 && time_->tv_usec < G_USEC_PER_SEC);

  if (microseconds >= 0)
    {
      time_->tv_usec += microseconds % G_USEC_PER_SEC;
      time_->tv_sec += microseconds / G_USEC_PER_SEC;
      if (time_->tv_usec >= G_USEC_PER_SEC)
       {
         time_->tv_usec -= G_USEC_PER_SEC;
         time_->tv_sec++;
       }
    }
  else
    {
      microseconds *= -1;
      time_->tv_usec -= microseconds % G_USEC_PER_SEC;
      time_->tv_sec -= microseconds / G_USEC_PER_SEC;
      if (time_->tv_usec < 0)
       {
         time_->tv_usec += G_USEC_PER_SEC;
         time_->tv_sec--;
       }      
    }
}

/* converts a broken down date representation, relative to UTC, to
 * a timestamp; it uses timegm() if it's available.
 */
static time_t
mktime_utc (struct tm *tm)
{
  time_t retval;
  
#ifndef HAVE_TIMEGM
  static const gint days_before[] =
  {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
  };
#endif

#ifndef HAVE_TIMEGM
  if (tm->tm_mon < 0 || tm->tm_mon > 11)
    return (time_t) -1;

  retval = (tm->tm_year - 70) * 365;
  retval += (tm->tm_year - 68) / 4;
  retval += days_before[tm->tm_mon] + tm->tm_mday - 1;
  
  if (tm->tm_year % 4 == 0 && tm->tm_mon < 2)
    retval -= 1;
  
  retval = ((((retval * 24) + tm->tm_hour) * 60) + tm->tm_min) * 60 + tm->tm_sec;
#else
  retval = timegm (tm);
#endif /* !HAVE_TIMEGM */
  
  return retval;
}

/**
 * g_time_val_from_iso8601:
 * @iso_date: an ISO 8601 encoded date string
 * @time_: a #GTimeVal
 *
 * Converts a string containing an ISO 8601 encoded date and time
 * to a #GTimeVal and puts it into @time_.
 *
 * Return value: %TRUE if the conversion was successful.
 *
 * Since: 2.12
 */
gboolean
g_time_val_from_iso8601 (const gchar *iso_date,
			 GTimeVal    *time_)
{
  struct tm tm;
  long val;

  g_return_val_if_fail (iso_date != NULL, FALSE);
  g_return_val_if_fail (time_ != NULL, FALSE);

  /* Ensure that the first character is a digit,
   * the first digit of the date, otherwise we don't
   * have an ISO 8601 date */
  while (g_ascii_isspace (*iso_date))
    iso_date++;

  if (*iso_date == '\0')
    return FALSE;

  if (!g_ascii_isdigit (*iso_date) && *iso_date != '-' && *iso_date != '+')
    return FALSE;

  val = strtoul (iso_date, (char **)&iso_date, 10);
  if (*iso_date == '-')
    {
      /* YYYY-MM-DD */
      tm.tm_year = val - 1900;
      iso_date++;
      tm.tm_mon = strtoul (iso_date, (char **)&iso_date, 10) - 1;
      
      if (*iso_date++ != '-')
       	return FALSE;
      
      tm.tm_mday = strtoul (iso_date, (char **)&iso_date, 10);
    }
  else
    {
      /* YYYYMMDD */
      tm.tm_mday = val % 100;
      tm.tm_mon = (val % 10000) / 100 - 1;
      tm.tm_year = val / 10000 - 1900;
    }

  if (*iso_date++ != 'T')
    return FALSE;
  
  val = strtoul (iso_date, (char **)&iso_date, 10);
  if (*iso_date == ':')
    {
      /* hh:mm:ss */
      tm.tm_hour = val;
      iso_date++;
      tm.tm_min = strtoul (iso_date, (char **)&iso_date, 10);
      
      if (*iso_date++ != ':')
        return FALSE;
      
      tm.tm_sec = strtoul (iso_date, (char **)&iso_date, 10);
    }
  else
    {
      /* hhmmss */
      tm.tm_sec = val % 100;
      tm.tm_min = (val % 10000) / 100;
      tm.tm_hour = val / 10000;
    }

  time_->tv_sec = mktime_utc (&tm);
  time_->tv_usec = 0;
  
  if (*iso_date == ',' || *iso_date == '.')
    {
      glong mul = 100000;

      while (g_ascii_isdigit (*++iso_date))
        {
          time_->tv_usec += (*iso_date - '0') * mul;
          mul /= 10;
        }
    }
    
  if (*iso_date == '+' || *iso_date == '-')
    {
      gint sign = (*iso_date == '+') ? -1 : 1;
      
      val = strtoul (iso_date + 1, (char **)&iso_date, 10);
      
      if (*iso_date == ':')
	val = 60 * val + strtoul (iso_date + 1, (char **)&iso_date, 10);
      else
        val = 60 * (val / 100) + (val % 100);

      time_->tv_sec += (time_t) (60 * val * sign);
    }
  else if (*iso_date++ != 'Z')
    return FALSE;

  while (g_ascii_isspace (*iso_date))
    iso_date++;

  return *iso_date == '\0';
}

/**
 * g_time_val_to_iso8601:
 * @time_: a #GTimeVal
 * 
 * Converts @time_ into an ISO 8601 encoded string, relative to the
 * Coordinated Universal Time (UTC).
 *
 * Return value: a newly allocated string containing an ISO 8601 date
 *
 * Since: 2.12
 */
gchar *
g_time_val_to_iso8601 (GTimeVal *time_)
{
  gchar *retval;
  struct tm *tm;
#ifdef HAVE_GMTIME_R
  struct tm tm_;
#endif
  time_t secs;
  
  g_return_val_if_fail (time_->tv_usec >= 0 && time_->tv_usec < G_USEC_PER_SEC, NULL);

 secs = time_->tv_sec;
#ifdef _WIN32
 tm = gmtime (&secs);
#else
#ifdef HAVE_GMTIME_R
  tm = gmtime_r (&secs, &tm_);
#else
  tm = gmtime (&secs);
#endif
#endif

  if (time_->tv_usec != 0)
    {
      /* ISO 8601 date and time format, with fractionary seconds:
       *   YYYY-MM-DDTHH:MM:SS.MMMMMMZ
       */
      retval = g_strdup_printf ("%4d-%02d-%02dT%02d:%02d:%02d.%06ldZ",
                                tm->tm_year + 1900,
                                tm->tm_mon + 1,
                                tm->tm_mday,
                                tm->tm_hour,
                                tm->tm_min,
                                tm->tm_sec,
                                time_->tv_usec);
    }
  else
    {
      /* ISO 8601 date and time format:
       *   YYYY-MM-DDTHH:MM:SSZ
       */
      retval = g_strdup_printf ("%4d-%02d-%02dT%02d:%02d:%02dZ",
                                tm->tm_year + 1900,
                                tm->tm_mon + 1,
                                tm->tm_mday,
                                tm->tm_hour,
                                tm->tm_min,
                                tm->tm_sec);
    }
  
  return retval;
}

#define __G_TIMER_C__
#include "galiasdef.c"
