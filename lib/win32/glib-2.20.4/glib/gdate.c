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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#define DEBUG_MSG(x)	/* */
#ifdef G_ENABLE_DEBUG
/* #define DEBUG_MSG(args)	g_message args ; */
#endif

#include "glib.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "galias.h"

GDate*
g_date_new (void)
{
  GDate *d = g_new0 (GDate, 1); /* happily, 0 is the invalid flag for everything. */
  
  return d;
}

GDate*
g_date_new_dmy (GDateDay   day, 
                GDateMonth m, 
                GDateYear  y)
{
  GDate *d;
  g_return_val_if_fail (g_date_valid_dmy (day, m, y), NULL);
  
  d = g_new (GDate, 1);
  
  d->julian = FALSE;
  d->dmy    = TRUE;
  
  d->month = m;
  d->day   = day;
  d->year  = y;
  
  g_assert (g_date_valid (d));
  
  return d;
}

GDate*
g_date_new_julian (guint32 j)
{
  GDate *d;
  g_return_val_if_fail (g_date_valid_julian (j), NULL);
  
  d = g_new (GDate, 1);
  
  d->julian = TRUE;
  d->dmy    = FALSE;
  
  d->julian_days = j;
  
  g_assert (g_date_valid (d));
  
  return d;
}

void
g_date_free (GDate *d)
{
  g_return_if_fail (d != NULL);
  
  g_free (d);
}

gboolean     
g_date_valid (const GDate *d)
{
  g_return_val_if_fail (d != NULL, FALSE);
  
  return (d->julian || d->dmy);
}

static const guint8 days_in_months[2][13] = 
{  /* error, jan feb mar apr may jun jul aug sep oct nov dec */
  {  0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }, 
  {  0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } /* leap year */
};

static const guint16 days_in_year[2][14] = 
{  /* 0, jan feb mar apr may  jun  jul  aug  sep  oct  nov  dec */
  {  0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 }, 
  {  0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

gboolean     
g_date_valid_month (GDateMonth m)
{ 
  return ( (m > G_DATE_BAD_MONTH) && (m < 13) );
}

gboolean     
g_date_valid_year (GDateYear y)
{
  return ( y > G_DATE_BAD_YEAR );
}

gboolean     
g_date_valid_day (GDateDay d)
{
  return ( (d > G_DATE_BAD_DAY) && (d < 32) );
}

gboolean     
g_date_valid_weekday (GDateWeekday w)
{
  return ( (w > G_DATE_BAD_WEEKDAY) && (w < 8) );
}

gboolean     
g_date_valid_julian (guint32 j)
{
  return (j > G_DATE_BAD_JULIAN);
}

gboolean     
g_date_valid_dmy (GDateDay   d, 
                  GDateMonth m, 
		  GDateYear  y)
{
  return ( (m > G_DATE_BAD_MONTH) &&
           (m < 13)               && 
           (d > G_DATE_BAD_DAY)   && 
           (y > G_DATE_BAD_YEAR)  &&   /* must check before using g_date_is_leap_year */
           (d <=  (g_date_is_leap_year (y) ? 
		   days_in_months[1][m] : days_in_months[0][m])) );
}


/* "Julian days" just means an absolute number of days, where Day 1 ==
 *   Jan 1, Year 1
 */
static void
g_date_update_julian (const GDate *const_d)
{
  GDate *d = (GDate *) const_d;
  GDateYear year;
  gint idx;
  
  g_return_if_fail (d != NULL);
  g_return_if_fail (d->dmy);
  g_return_if_fail (!d->julian);
  g_return_if_fail (g_date_valid_dmy (d->day, d->month, d->year));
  
  /* What we actually do is: multiply years * 365 days in the year,
   * add the number of years divided by 4, subtract the number of
   * years divided by 100 and add the number of years divided by 400,
   * which accounts for leap year stuff. Code from Steffen Beyer's
   * DateCalc. 
   */
  
  year = d->year - 1; /* we know d->year > 0 since it's valid */
  
  d->julian_days = year * 365U;
  d->julian_days += (year >>= 2); /* divide by 4 and add */
  d->julian_days -= (year /= 25); /* divides original # years by 100 */
  d->julian_days += year >> 2;    /* divides by 4, which divides original by 400 */
  
  idx = g_date_is_leap_year (d->year) ? 1 : 0;
  
  d->julian_days += days_in_year[idx][d->month] + d->day;
  
  g_return_if_fail (g_date_valid_julian (d->julian_days));
  
  d->julian = TRUE;
}

static void 
g_date_update_dmy (const GDate *const_d)
{
  GDate *d = (GDate *) const_d;
  GDateYear y;
  GDateMonth m;
  GDateDay day;
  
  guint32 A, B, C, D, E, M;
  
  g_return_if_fail (d != NULL);
  g_return_if_fail (d->julian);
  g_return_if_fail (!d->dmy);
  g_return_if_fail (g_date_valid_julian (d->julian_days));
  
  /* Formula taken from the Calendar FAQ; the formula was for the
   *  Julian Period which starts on 1 January 4713 BC, so we add
   *  1,721,425 to the number of days before doing the formula.
   *
   * I'm sure this can be simplified for our 1 January 1 AD period
   * start, but I can't figure out how to unpack the formula.  
   */
  
  A = d->julian_days + 1721425 + 32045;
  B = ( 4 *(A + 36524) )/ 146097 - 1;
  C = A - (146097 * B)/4;
  D = ( 4 * (C + 365) ) / 1461 - 1;
  E = C - ((1461*D) / 4);
  M = (5 * (E - 1) + 2)/153;
  
  m = M + 3 - (12*(M/10));
  day = E - (153*M + 2)/5;
  y = 100 * B + D - 4800 + (M/10);
  
#ifdef G_ENABLE_DEBUG
  if (!g_date_valid_dmy (day, m, y)) 
    g_warning ("\nOOPS julian: %u  computed dmy: %u %u %u\n", 
	       d->julian_days, day, m, y);
#endif
  
  d->month = m;
  d->day   = day;
  d->year  = y;
  
  d->dmy = TRUE;
}

GDateWeekday 
g_date_get_weekday (const GDate *d)
{
  g_return_val_if_fail (g_date_valid (d), G_DATE_BAD_WEEKDAY);
  
  if (!d->julian) 
    g_date_update_julian (d);

  g_return_val_if_fail (d->julian, G_DATE_BAD_WEEKDAY);
  
  return ((d->julian_days - 1) % 7) + 1;
}

GDateMonth   
g_date_get_month (const GDate *d)
{
  g_return_val_if_fail (g_date_valid (d), G_DATE_BAD_MONTH);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_val_if_fail (d->dmy, G_DATE_BAD_MONTH);
  
  return d->month;
}

GDateYear    
g_date_get_year (const GDate *d)
{
  g_return_val_if_fail (g_date_valid (d), G_DATE_BAD_YEAR);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_val_if_fail (d->dmy, G_DATE_BAD_YEAR);  
  
  return d->year;
}

GDateDay     
g_date_get_day (const GDate *d)
{
  g_return_val_if_fail (g_date_valid (d), G_DATE_BAD_DAY);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_val_if_fail (d->dmy, G_DATE_BAD_DAY);  
  
  return d->day;
}

guint32      
g_date_get_julian (const GDate *d)
{
  g_return_val_if_fail (g_date_valid (d), G_DATE_BAD_JULIAN);
  
  if (!d->julian) 
    g_date_update_julian (d);

  g_return_val_if_fail (d->julian, G_DATE_BAD_JULIAN);  
  
  return d->julian_days;
}

guint        
g_date_get_day_of_year (const GDate *d)
{
  gint idx;
  
  g_return_val_if_fail (g_date_valid (d), 0);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_val_if_fail (d->dmy, 0);  
  
  idx = g_date_is_leap_year (d->year) ? 1 : 0;
  
  return (days_in_year[idx][d->month] + d->day);
}

guint        
g_date_get_monday_week_of_year (const GDate *d)
{
  GDateWeekday wd;
  guint day;
  GDate first;
  
  g_return_val_if_fail (g_date_valid (d), 0);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_val_if_fail (d->dmy, 0);  
  
  g_date_clear (&first, 1);
  
  g_date_set_dmy (&first, 1, 1, d->year);
  
  wd = g_date_get_weekday (&first) - 1; /* make Monday day 0 */
  day = g_date_get_day_of_year (d) - 1;
  
  return ((day + wd)/7U + (wd == 0 ? 1 : 0));
}

guint        
g_date_get_sunday_week_of_year (const GDate *d)
{
  GDateWeekday wd;
  guint day;
  GDate first;
  
  g_return_val_if_fail (g_date_valid (d), 0);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_val_if_fail (d->dmy, 0);  
  
  g_date_clear (&first, 1);
  
  g_date_set_dmy (&first, 1, 1, d->year);
  
  wd = g_date_get_weekday (&first);
  if (wd == 7) wd = 0; /* make Sunday day 0 */
  day = g_date_get_day_of_year (d) - 1;
  
  return ((day + wd)/7U + (wd == 0 ? 1 : 0));
}

/**
 * g_date_get_iso8601_week_of_year:
 * @date: a valid #GDate
 *
 * Returns the week of the year, where weeks are interpreted according
 * to ISO 8601. 
 * 
 * Returns: ISO 8601 week number of the year.
 *
 * Since: 2.6
 **/
guint
g_date_get_iso8601_week_of_year (const GDate *d)
{
  guint j, d4, L, d1, w;

  g_return_val_if_fail (g_date_valid (d), 0);
  
  if (!d->julian)
    g_date_update_julian (d);

  g_return_val_if_fail (d->julian, 0);

  /* Formula taken from the Calendar FAQ; the formula was for the
   * Julian Period which starts on 1 January 4713 BC, so we add
   * 1,721,425 to the number of days before doing the formula. 
   */
  j  = d->julian_days + 1721425;
  d4 = (j + 31741 - (j % 7)) % 146097 % 36524 % 1461;
  L  = d4 / 1460;
  d1 = ((d4 - L) % 365) + L;
  w  = d1 / 7 + 1;

  return w;
}

gint
g_date_days_between (const GDate *d1,
		     const GDate *d2)
{
  g_return_val_if_fail (g_date_valid (d1), 0);
  g_return_val_if_fail (g_date_valid (d2), 0);

  return (gint)g_date_get_julian (d2) - (gint)g_date_get_julian (d1);
}

void         
g_date_clear (GDate *d, guint ndates)
{
  g_return_if_fail (d != NULL);
  g_return_if_fail (ndates != 0);
  
  memset (d, 0x0, ndates*sizeof (GDate)); 
}

G_LOCK_DEFINE_STATIC (g_date_global);

/* These are for the parser, output to the user should use *
 * g_date_strftime () - this creates more never-freed memory to annoy
 * all those memory debugger users. :-) 
 */

static gchar *long_month_names[13] = 
{ 
  NULL,
};

static gchar *short_month_names[13] = 
{
  NULL, 
};

/* This tells us if we need to update the parse info */
static gchar *current_locale = NULL;

/* order of these in the current locale */
static GDateDMY dmy_order[3] = 
{
   G_DATE_DAY, G_DATE_MONTH, G_DATE_YEAR
};

/* Where to chop two-digit years: i.e., for the 1930 default, numbers
 * 29 and below are counted as in the year 2000, numbers 30 and above
 * are counted as in the year 1900.  
 */

static const GDateYear twodigit_start_year = 1930;

/* It is impossible to enter a year between 1 AD and 99 AD with this
 * in effect.  
 */
static gboolean using_twodigit_years = FALSE;

/* Adjustment of locale era to AD, non-zero means using locale era
 */
static gint locale_era_adjust = 0;

struct _GDateParseTokens {
  gint num_ints;
  gint n[3];
  guint month;
};

typedef struct _GDateParseTokens GDateParseTokens;

#define NUM_LEN 10

/* HOLDS: g_date_global_lock */
static void
g_date_fill_parse_tokens (const gchar *str, GDateParseTokens *pt)
{
  gchar num[4][NUM_LEN+1];
  gint i;
  const guchar *s;
  
  /* We count 4, but store 3; so we can give an error
   * if there are 4.
   */
  num[0][0] = num[1][0] = num[2][0] = num[3][0] = '\0';
  
  s = (const guchar *) str;
  pt->num_ints = 0;
  while (*s && pt->num_ints < 4) 
    {
      
      i = 0;
      while (*s && g_ascii_isdigit (*s) && i < NUM_LEN)
        {
          num[pt->num_ints][i] = *s;
          ++s; 
          ++i;
        }
      
      if (i > 0) 
        {
          num[pt->num_ints][i] = '\0';
          ++(pt->num_ints);
        }
      
      if (*s == '\0') break;
      
      ++s;
    }
  
  pt->n[0] = pt->num_ints > 0 ? atoi (num[0]) : 0;
  pt->n[1] = pt->num_ints > 1 ? atoi (num[1]) : 0;
  pt->n[2] = pt->num_ints > 2 ? atoi (num[2]) : 0;
  
  pt->month = G_DATE_BAD_MONTH;
  
  if (pt->num_ints < 3)
    {
      gchar *casefold;
      gchar *normalized;
      
      casefold = g_utf8_casefold (str, -1);
      normalized = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
      g_free (casefold);

      i = 1;
      while (i < 13)
        {
          if (long_month_names[i] != NULL) 
            {
              const gchar *found = strstr (normalized, long_month_names[i]);
	      
              if (found != NULL)
                {
                  pt->month = i;
		  break;
                }
            }
	  
          if (short_month_names[i] != NULL) 
            {
              const gchar *found = strstr (normalized, short_month_names[i]);
	      
              if (found != NULL)
                {
                  pt->month = i;
		  break;
                }
            }

          ++i;
        }

      g_free (normalized);
    }
}

/* HOLDS: g_date_global_lock */
static void
g_date_prepare_to_parse (const gchar      *str, 
                         GDateParseTokens *pt)
{
  const gchar *locale = setlocale (LC_TIME, NULL);
  gboolean recompute_localeinfo = FALSE;
  GDate d;
  
  g_return_if_fail (locale != NULL); /* should not happen */
  
  g_date_clear (&d, 1);              /* clear for scratch use */
  
  if ( (current_locale == NULL) || (strcmp (locale, current_locale) != 0) ) 
    recompute_localeinfo = TRUE;  /* Uh, there used to be a reason for the temporary */
  
  if (recompute_localeinfo)
    {
      int i = 1;
      GDateParseTokens testpt;
      gchar buf[128];
      
      g_free (current_locale); /* still works if current_locale == NULL */
      
      current_locale = g_strdup (locale);
      
      short_month_names[0] = "Error";
      long_month_names[0] = "Error";

      while (i < 13) 
        {
	  gchar *casefold;
	  
          g_date_set_dmy (&d, 1, i, 1);
	  
          g_return_if_fail (g_date_valid (&d));
	  
          g_date_strftime (buf, 127, "%b", &d);

	  casefold = g_utf8_casefold (buf, -1);
          g_free (short_month_names[i]);
          short_month_names[i] = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	  g_free (casefold);
	  
          g_date_strftime (buf, 127, "%B", &d);
	  casefold = g_utf8_casefold (buf, -1);
          g_free (long_month_names[i]);
          long_month_names[i] = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	  g_free (casefold);
          
          ++i;
        }
      
      /* Determine DMY order */
      
      /* had to pick a random day - don't change this, some strftimes
       * are broken on some days, and this one is good so far. */
      g_date_set_dmy (&d, 4, 7, 1976);
      
      g_date_strftime (buf, 127, "%x", &d);
      
      g_date_fill_parse_tokens (buf, &testpt);
      
      i = 0;
      while (i < testpt.num_ints)
        {
          switch (testpt.n[i])
            {
            case 7:
              dmy_order[i] = G_DATE_MONTH;
              break;
            case 4:
              dmy_order[i] = G_DATE_DAY;
              break;
            case 76:
              using_twodigit_years = TRUE; /* FALL THRU */
            case 1976:
              dmy_order[i] = G_DATE_YEAR;
              break;
            default:
              /* assume locale era */
              locale_era_adjust = 1976 - testpt.n[i];
              dmy_order[i] = G_DATE_YEAR;
              break;
            }
          ++i;
        }
      
#ifdef G_ENABLE_DEBUG
      DEBUG_MSG (("**GDate prepared a new set of locale-specific parse rules."));
      i = 1;
      while (i < 13) 
        {
          DEBUG_MSG (("  %s   %s", long_month_names[i], short_month_names[i]));
          ++i;
        }
      if (using_twodigit_years)
        {
	  DEBUG_MSG (("**Using twodigit years with cutoff year: %u", twodigit_start_year));
        }
      { 
        gchar *strings[3];
        i = 0;
        while (i < 3)
          {
            switch (dmy_order[i])
              {
              case G_DATE_MONTH:
                strings[i] = "Month";
                break;
              case G_DATE_YEAR:
                strings[i] = "Year";
                break;
              case G_DATE_DAY:
                strings[i] = "Day";
                break;
              default:
                strings[i] = NULL;
                break;
              }
            ++i;
          }
        DEBUG_MSG (("**Order: %s, %s, %s", strings[0], strings[1], strings[2]));
        DEBUG_MSG (("**Sample date in this locale: `%s'", buf));
      }
#endif
    }
  
  g_date_fill_parse_tokens (str, pt);
}

void         
g_date_set_parse (GDate       *d, 
                  const gchar *str)
{
  GDateParseTokens pt;
  guint m = G_DATE_BAD_MONTH, day = G_DATE_BAD_DAY, y = G_DATE_BAD_YEAR;
  
  g_return_if_fail (d != NULL);
  
  /* set invalid */
  g_date_clear (d, 1);
  
  G_LOCK (g_date_global);

  g_date_prepare_to_parse (str, &pt);
  
  DEBUG_MSG (("Found %d ints, `%d' `%d' `%d' and written out month %d", 
	      pt.num_ints, pt.n[0], pt.n[1], pt.n[2], pt.month));
  
  
  if (pt.num_ints == 4) 
    {
      G_UNLOCK (g_date_global);
      return; /* presumably a typo; bail out. */
    }
  
  if (pt.num_ints > 1)
    {
      int i = 0;
      int j = 0;
      
      g_assert (pt.num_ints < 4); /* i.e., it is 2 or 3 */
      
      while (i < pt.num_ints && j < 3) 
        {
          switch (dmy_order[j])
            {
            case G_DATE_MONTH:
	    {
	      if (pt.num_ints == 2 && pt.month != G_DATE_BAD_MONTH)
		{
		  m = pt.month;
		  ++j;      /* skip months, but don't skip this number */
		  continue;
		}
	      else 
		m = pt.n[i];
	    }
	    break;
            case G_DATE_DAY:
	    {
	      if (pt.num_ints == 2 && pt.month == G_DATE_BAD_MONTH)
		{
		  day = 1;
		  ++j;      /* skip days, since we may have month/year */
		  continue;
		}
	      day = pt.n[i];
	    }
	    break;
            case G_DATE_YEAR:
	    {
	      y  = pt.n[i];
	      
	      if (locale_era_adjust != 0)
	        {
		  y += locale_era_adjust;
	        }
	      else if (using_twodigit_years && y < 100)
		{
		  guint two     =  twodigit_start_year % 100;
		  guint century = (twodigit_start_year / 100) * 100;
		  
		  if (y < two)
		    century += 100;
		  
		  y += century;
		}
	    }
	    break;
            default:
              break;
            }
	  
          ++i;
          ++j;
        }
      
      
      if (pt.num_ints == 3 && !g_date_valid_dmy (day, m, y))
        {
          /* Try YYYY MM DD */
          y   = pt.n[0];
          m   = pt.n[1];
          day = pt.n[2];
          
          if (using_twodigit_years && y < 100) 
            y = G_DATE_BAD_YEAR; /* avoids ambiguity */
        }
      else if (pt.num_ints == 2)
	{
	  if (m == G_DATE_BAD_MONTH && pt.month != G_DATE_BAD_MONTH)
	    m = pt.month;
	}
    }
  else if (pt.num_ints == 1) 
    {
      if (pt.month != G_DATE_BAD_MONTH)
        {
          /* Month name and year? */
          m    = pt.month;
          day  = 1;
          y = pt.n[0];
        }
      else
        {
          /* Try yyyymmdd and yymmdd */
	  
          m   = (pt.n[0]/100) % 100;
          day = pt.n[0] % 100;
          y   = pt.n[0]/10000;
	  
          /* FIXME move this into a separate function */
          if (using_twodigit_years && y < 100)
            {
              guint two     =  twodigit_start_year % 100;
              guint century = (twodigit_start_year / 100) * 100;
              
              if (y < two)
                century += 100;
              
              y += century;
            }
        }
    }
  
  /* See if we got anything valid out of all this. */
  /* y < 8000 is to catch 19998 style typos; the library is OK up to 65535 or so */
  if (y < 8000 && g_date_valid_dmy (day, m, y)) 
    {
      d->month = m;
      d->day   = day;
      d->year  = y;
      d->dmy   = TRUE;
    }
#ifdef G_ENABLE_DEBUG
  else 
    {
      DEBUG_MSG (("Rejected DMY %u %u %u", day, m, y));
    }
#endif
  G_UNLOCK (g_date_global);
}

/**
 * g_date_set_time_t:
 * @date: a #GDate 
 * @timet: <type>time_t</type> value to set
 *
 * Sets the value of a date to the date corresponding to a time 
 * specified as a time_t. The time to date conversion is done using 
 * the user's current timezone.
 *
 * To set the value of a date to the current day, you could write:
 * |[
 *  g_date_set_time_t (date, time (NULL)); 
 * ]|
 *
 * Since: 2.10
 */
void         
g_date_set_time_t (GDate *date,
		   time_t timet)
{
  struct tm tm;
  
  g_return_if_fail (date != NULL);
  
#ifdef HAVE_LOCALTIME_R
  localtime_r (&timet, &tm);
#else
  {
    struct tm *ptm = localtime (&timet);

    if (ptm == NULL)
      {
	/* Happens at least in Microsoft's C library if you pass a
	 * negative time_t. Use 2000-01-01 as default date.
	 */
#ifndef G_DISABLE_CHECKS
	g_return_if_fail_warning (G_LOG_DOMAIN, "g_date_set_time", "ptm != NULL");
#endif

	tm.tm_mon = 0;
	tm.tm_mday = 1;
	tm.tm_year = 100;
      }
    else
      memcpy ((void *) &tm, (void *) ptm, sizeof(struct tm));
  }
#endif
  
  date->julian = FALSE;
  
  date->month = tm.tm_mon + 1;
  date->day   = tm.tm_mday;
  date->year  = tm.tm_year + 1900;
  
  g_return_if_fail (g_date_valid_dmy (date->day, date->month, date->year));
  
  date->dmy    = TRUE;
}


/**
 * g_date_set_time:
 * @date: a #GDate.
 * @time_: #GTime value to set.
 *
 * Sets the value of a date from a #GTime value.
 * The time to date conversion is done using the user's current timezone.
 *
 * @Deprecated:2.10: Use g_date_set_time_t() instead.
 */
void
g_date_set_time (GDate *date,
		 GTime  time_)
{
  g_date_set_time_t (date, (time_t) time_);
}

/**
 * g_date_set_time_val:
 * @date: a #GDate 
 * @timeval: #GTimeVal value to set
 *
 * Sets the value of a date from a #GTimeVal value.  Note that the
 * @tv_usec member is ignored, because #GDate can't make use of the
 * additional precision.
 *
 * Since: 2.10
 */
void
g_date_set_time_val (GDate    *date,
		     GTimeVal *timeval)
{
  g_date_set_time_t (date, (time_t) timeval->tv_sec);
}

void         
g_date_set_month (GDate     *d, 
                  GDateMonth m)
{
  g_return_if_fail (d != NULL);
  g_return_if_fail (g_date_valid_month (m));

  if (d->julian && !d->dmy) g_date_update_dmy(d);
  d->julian = FALSE;
  
  d->month = m;
  
  if (g_date_valid_dmy (d->day, d->month, d->year))
    d->dmy = TRUE;
  else 
    d->dmy = FALSE;
}

void         
g_date_set_day (GDate    *d, 
                GDateDay  day)
{
  g_return_if_fail (d != NULL);
  g_return_if_fail (g_date_valid_day (day));
  
  if (d->julian && !d->dmy) g_date_update_dmy(d);
  d->julian = FALSE;
  
  d->day = day;
  
  if (g_date_valid_dmy (d->day, d->month, d->year))
    d->dmy = TRUE;
  else 
    d->dmy = FALSE;
}

void         
g_date_set_year (GDate     *d, 
                 GDateYear  y)
{
  g_return_if_fail (d != NULL);
  g_return_if_fail (g_date_valid_year (y));
  
  if (d->julian && !d->dmy) g_date_update_dmy(d);
  d->julian = FALSE;
  
  d->year = y;
  
  if (g_date_valid_dmy (d->day, d->month, d->year))
    d->dmy = TRUE;
  else 
    d->dmy = FALSE;
}

void         
g_date_set_dmy (GDate      *d, 
                GDateDay    day, 
                GDateMonth  m, 
                GDateYear   y)
{
  g_return_if_fail (d != NULL);
  g_return_if_fail (g_date_valid_dmy (day, m, y));
  
  d->julian = FALSE;
  
  d->month = m;
  d->day   = day;
  d->year  = y;
  
  d->dmy = TRUE;
}

void         
g_date_set_julian (GDate   *d, 
                   guint32  j)
{
  g_return_if_fail (d != NULL);
  g_return_if_fail (g_date_valid_julian (j));
  
  d->julian_days = j;
  d->julian = TRUE;
  d->dmy = FALSE;
}


gboolean     
g_date_is_first_of_month (const GDate *d)
{
  g_return_val_if_fail (g_date_valid (d), FALSE);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_val_if_fail (d->dmy, FALSE);  
  
  if (d->day == 1) return TRUE;
  else return FALSE;
}

gboolean     
g_date_is_last_of_month (const GDate *d)
{
  gint idx;
  
  g_return_val_if_fail (g_date_valid (d), FALSE);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_val_if_fail (d->dmy, FALSE);  
  
  idx = g_date_is_leap_year (d->year) ? 1 : 0;
  
  if (d->day == days_in_months[idx][d->month]) return TRUE;
  else return FALSE;
}

void         
g_date_add_days (GDate *d, 
                 guint  ndays)
{
  g_return_if_fail (g_date_valid (d));
  
  if (!d->julian)
    g_date_update_julian (d);

  g_return_if_fail (d->julian);
  
  d->julian_days += ndays;
  d->dmy = FALSE;
}

void         
g_date_subtract_days (GDate *d, 
                      guint  ndays)
{
  g_return_if_fail (g_date_valid (d));
  
  if (!d->julian)
    g_date_update_julian (d);

  g_return_if_fail (d->julian);
  g_return_if_fail (d->julian_days > ndays);
  
  d->julian_days -= ndays;
  d->dmy = FALSE;
}

void         
g_date_add_months (GDate *d, 
                   guint  nmonths)
{
  guint years, months;
  gint idx;
  
  g_return_if_fail (g_date_valid (d));
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_if_fail (d->dmy);  
  
  nmonths += d->month - 1;
  
  years  = nmonths/12;
  months = nmonths%12;
  
  d->month = months + 1;
  d->year  += years;
  
  idx = g_date_is_leap_year (d->year) ? 1 : 0;
  
  if (d->day > days_in_months[idx][d->month])
    d->day = days_in_months[idx][d->month];
  
  d->julian = FALSE;
  
  g_return_if_fail (g_date_valid (d));
}

void         
g_date_subtract_months (GDate *d, 
                        guint  nmonths)
{
  guint years, months;
  gint idx;
  
  g_return_if_fail (g_date_valid (d));
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_if_fail (d->dmy);  
  
  years  = nmonths/12;
  months = nmonths%12;
  
  g_return_if_fail (d->year > years);
  
  d->year  -= years;
  
  if (d->month > months) d->month -= months;
  else 
    {
      months -= d->month;
      d->month = 12 - months;
      d->year -= 1;
    }
  
  idx = g_date_is_leap_year (d->year) ? 1 : 0;
  
  if (d->day > days_in_months[idx][d->month])
    d->day = days_in_months[idx][d->month];
  
  d->julian = FALSE;
  
  g_return_if_fail (g_date_valid (d));
}

void         
g_date_add_years (GDate *d, 
                  guint  nyears)
{
  g_return_if_fail (g_date_valid (d));
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_if_fail (d->dmy);  
  
  d->year += nyears;
  
  if (d->month == 2 && d->day == 29)
    {
      if (!g_date_is_leap_year (d->year))
        d->day = 28;
    }
  
  d->julian = FALSE;
}

void         
g_date_subtract_years (GDate *d, 
                       guint  nyears)
{
  g_return_if_fail (g_date_valid (d));
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_if_fail (d->dmy);  
  g_return_if_fail (d->year > nyears);
  
  d->year -= nyears;
  
  if (d->month == 2 && d->day == 29)
    {
      if (!g_date_is_leap_year (d->year))
        d->day = 28;
    }
  
  d->julian = FALSE;
}

gboolean     
g_date_is_leap_year (GDateYear year)
{
  g_return_val_if_fail (g_date_valid_year (year), FALSE);
  
  return ( (((year % 4) == 0) && ((year % 100) != 0)) ||
           (year % 400) == 0 );
}

guint8         
g_date_get_days_in_month (GDateMonth month, 
                          GDateYear  year)
{
  gint idx;
  
  g_return_val_if_fail (g_date_valid_year (year), 0);
  g_return_val_if_fail (g_date_valid_month (month), 0);
  
  idx = g_date_is_leap_year (year) ? 1 : 0;
  
  return days_in_months[idx][month];
}

guint8       
g_date_get_monday_weeks_in_year (GDateYear year)
{
  GDate d;
  
  g_return_val_if_fail (g_date_valid_year (year), 0);
  
  g_date_clear (&d, 1);
  g_date_set_dmy (&d, 1, 1, year);
  if (g_date_get_weekday (&d) == G_DATE_MONDAY) return 53;
  g_date_set_dmy (&d, 31, 12, year);
  if (g_date_get_weekday (&d) == G_DATE_MONDAY) return 53;
  if (g_date_is_leap_year (year)) 
    {
      g_date_set_dmy (&d, 2, 1, year);
      if (g_date_get_weekday (&d) == G_DATE_MONDAY) return 53;
      g_date_set_dmy (&d, 30, 12, year);
      if (g_date_get_weekday (&d) == G_DATE_MONDAY) return 53;
    }
  return 52;
}

guint8       
g_date_get_sunday_weeks_in_year (GDateYear year)
{
  GDate d;
  
  g_return_val_if_fail (g_date_valid_year (year), 0);
  
  g_date_clear (&d, 1);
  g_date_set_dmy (&d, 1, 1, year);
  if (g_date_get_weekday (&d) == G_DATE_SUNDAY) return 53;
  g_date_set_dmy (&d, 31, 12, year);
  if (g_date_get_weekday (&d) == G_DATE_SUNDAY) return 53;
  if (g_date_is_leap_year (year)) 
    {
      g_date_set_dmy (&d, 2, 1, year);
      if (g_date_get_weekday (&d) == G_DATE_SUNDAY) return 53;
      g_date_set_dmy (&d, 30, 12, year);
      if (g_date_get_weekday (&d) == G_DATE_SUNDAY) return 53;
    }
  return 52;
}

gint         
g_date_compare (const GDate *lhs, 
                const GDate *rhs)
{
  g_return_val_if_fail (lhs != NULL, 0);
  g_return_val_if_fail (rhs != NULL, 0);
  g_return_val_if_fail (g_date_valid (lhs), 0);
  g_return_val_if_fail (g_date_valid (rhs), 0);
  
  /* Remember the self-comparison case! I think it works right now. */
  
  while (TRUE)
    {
      if (lhs->julian && rhs->julian) 
        {
          if (lhs->julian_days < rhs->julian_days) return -1;
          else if (lhs->julian_days > rhs->julian_days) return 1;
          else                                          return 0;
        }
      else if (lhs->dmy && rhs->dmy) 
        {
          if (lhs->year < rhs->year)               return -1;
          else if (lhs->year > rhs->year)               return 1;
          else 
            {
              if (lhs->month < rhs->month)         return -1;
              else if (lhs->month > rhs->month)         return 1;
              else 
                {
                  if (lhs->day < rhs->day)              return -1;
                  else if (lhs->day > rhs->day)              return 1;
                  else                                       return 0;
                }
              
            }
          
        }
      else
        {
          if (!lhs->julian) g_date_update_julian (lhs);
          if (!rhs->julian) g_date_update_julian (rhs);
          g_return_val_if_fail (lhs->julian, 0);
          g_return_val_if_fail (rhs->julian, 0);
        }
      
    }
  return 0; /* warnings */
}


void        
g_date_to_struct_tm (const GDate *d, 
                     struct tm   *tm)
{
  GDateWeekday day;
     
  g_return_if_fail (g_date_valid (d));
  g_return_if_fail (tm != NULL);
  
  if (!d->dmy) 
    g_date_update_dmy (d);

  g_return_if_fail (d->dmy);
  
  /* zero all the irrelevant fields to be sure they're valid */
  
  /* On Linux and maybe other systems, there are weird non-POSIX
   * fields on the end of struct tm that choke strftime if they
   * contain garbage.  So we need to 0 the entire struct, not just the
   * fields we know to exist. 
   */
  
  memset (tm, 0x0, sizeof (struct tm));
  
  tm->tm_mday = d->day;
  tm->tm_mon  = d->month - 1; /* 0-11 goes in tm */
  tm->tm_year = ((int)d->year) - 1900; /* X/Open says tm_year can be negative */
  
  day = g_date_get_weekday (d);
  if (day == 7) day = 0; /* struct tm wants days since Sunday, so Sunday is 0 */
  
  tm->tm_wday = (int)day;
  
  tm->tm_yday = g_date_get_day_of_year (d) - 1; /* 0 to 365 */
  tm->tm_isdst = -1; /* -1 means "information not available" */
}

void
g_date_clamp (GDate       *date,
	      const GDate *min_date,
	      const GDate *max_date)
{
  g_return_if_fail (g_date_valid (date));

  if (min_date != NULL)
    g_return_if_fail (g_date_valid (min_date));

  if (max_date != NULL)
    g_return_if_fail (g_date_valid (max_date));

  if (min_date != NULL && max_date != NULL)
    g_return_if_fail (g_date_compare (min_date, max_date) <= 0);

  if (min_date && g_date_compare (date, min_date) < 0)
    *date = *min_date;

  if (max_date && g_date_compare (max_date, date) < 0)
    *date = *max_date;
}

void
g_date_order (GDate *date1,
              GDate *date2)
{
  g_return_if_fail (g_date_valid (date1));
  g_return_if_fail (g_date_valid (date2));

  if (g_date_compare (date1, date2) > 0)
    {
      GDate tmp = *date1;
      *date1 = *date2;
      *date2 = tmp;
    }
}

#ifdef G_OS_WIN32
static gsize
win32_strftime_helper (const GDate     *d,
		       const gchar     *format,
		       const struct tm *tm,
		       gchar           *s,
		       gsize	        slen)
{
  SYSTEMTIME systemtime;
  TIME_ZONE_INFORMATION tzinfo;
  LCID lcid;
  int n, k;
  GArray *result;
  const gchar *p;
  gunichar c;
  const wchar_t digits[] = L"0123456789";
  gchar *convbuf;
  glong convlen = 0;
  gsize retval;

  systemtime.wYear = tm->tm_year + 1900;
  systemtime.wMonth = tm->tm_mon + 1;
  systemtime.wDayOfWeek = tm->tm_wday;
  systemtime.wDay = tm->tm_mday;
  systemtime.wHour = tm->tm_hour;
  systemtime.wMinute = tm->tm_min;
  systemtime.wSecond = tm->tm_sec;
  systemtime.wMilliseconds = 0;
  
  lcid = GetThreadLocale ();
  result = g_array_sized_new (FALSE, FALSE, sizeof (wchar_t), MAX (128, strlen (format) * 2));

  p = format;
  while (*p)
    {
      c = g_utf8_get_char (p);
      if (c == '%')
	{
	  p = g_utf8_next_char (p);
	  if (!*p)
	    {
	      s[0] = '\0';
	      g_array_free (result, TRUE);

	      return 0;
	    }
	  
	  c = g_utf8_get_char (p);
	  if (c == 'E' || c == 'O')
	    {
	      /* Ignore modified conversion specifiers for now. */
	      p = g_utf8_next_char (p);
	      if (!*p)
		{
		  s[0] = '\0';
		  g_array_free (result, TRUE);
		  
		  return 0;
		}

	      c = g_utf8_get_char (p);
	    }

	  switch (c)
	    {
	    case 'a':
	      if (systemtime.wDayOfWeek == 0)
		k = 6;
	      else
		k = systemtime.wDayOfWeek - 1;
	      n = GetLocaleInfoW (lcid, LOCALE_SABBREVDAYNAME1+k, NULL, 0);
	      g_array_set_size (result, result->len + n);
	      GetLocaleInfoW (lcid, LOCALE_SABBREVDAYNAME1+k, ((wchar_t *) result->data) + result->len - n, n);
	      g_array_set_size (result, result->len - 1);
	      break;
	    case 'A':
	      if (systemtime.wDayOfWeek == 0)
		k = 6;
	      else
		k = systemtime.wDayOfWeek - 1;
	      n = GetLocaleInfoW (lcid, LOCALE_SDAYNAME1+k, NULL, 0);
	      g_array_set_size (result, result->len + n);
	      GetLocaleInfoW (lcid, LOCALE_SDAYNAME1+k, ((wchar_t *) result->data) + result->len - n, n);
	      g_array_set_size (result, result->len - 1);
	      break;
	    case 'b':
	    case 'h':
	      n = GetLocaleInfoW (lcid, LOCALE_SABBREVMONTHNAME1+systemtime.wMonth-1, NULL, 0);
	      g_array_set_size (result, result->len + n);
	      GetLocaleInfoW (lcid, LOCALE_SABBREVMONTHNAME1+systemtime.wMonth-1, ((wchar_t *) result->data) + result->len - n, n);
	      g_array_set_size (result, result->len - 1);
	      break;
	    case 'B':
	      n = GetLocaleInfoW (lcid, LOCALE_SMONTHNAME1+systemtime.wMonth-1, NULL, 0);
	      g_array_set_size (result, result->len + n);
	      GetLocaleInfoW (lcid, LOCALE_SMONTHNAME1+systemtime.wMonth-1, ((wchar_t *) result->data) + result->len - n, n);
	      g_array_set_size (result, result->len - 1);
	      break;
	    case 'c':
	      n = GetDateFormatW (lcid, 0, &systemtime, NULL, NULL, 0);
	      if (n > 0)
		{
		  g_array_set_size (result, result->len + n);
		  GetDateFormatW (lcid, 0, &systemtime, NULL, ((wchar_t *) result->data) + result->len - n, n);
		  g_array_set_size (result, result->len - 1);
		}
	      g_array_append_vals (result, L" ", 1);
	      n = GetTimeFormatW (lcid, 0, &systemtime, NULL, NULL, 0);
	      if (n > 0)
		{
		  g_array_set_size (result, result->len + n);
		  GetTimeFormatW (lcid, 0, &systemtime, NULL, ((wchar_t *) result->data) + result->len - n, n);
		  g_array_set_size (result, result->len - 1);
		}
	      break;
	    case 'C':
	      g_array_append_vals (result, digits + systemtime.wYear/1000, 1);
	      g_array_append_vals (result, digits + (systemtime.wYear/1000)%10, 1);
	      break;
	    case 'd':
	      g_array_append_vals (result, digits + systemtime.wDay/10, 1);
	      g_array_append_vals (result, digits + systemtime.wDay%10, 1);
	      break;
	    case 'D':
	      g_array_append_vals (result, digits + systemtime.wMonth/10, 1);
	      g_array_append_vals (result, digits + systemtime.wMonth%10, 1);
	      g_array_append_vals (result, L"/", 1);
	      g_array_append_vals (result, digits + systemtime.wDay/10, 1);
	      g_array_append_vals (result, digits + systemtime.wDay%10, 1);
	      g_array_append_vals (result, L"/", 1);
	      g_array_append_vals (result, digits + (systemtime.wYear/10)%10, 1);
	      g_array_append_vals (result, digits + systemtime.wYear%10, 1);
	      break;
	    case 'e':
	      if (systemtime.wDay >= 10)
		g_array_append_vals (result, digits + systemtime.wDay/10, 1);
	      else
		g_array_append_vals (result, L" ", 1);
	      g_array_append_vals (result, digits + systemtime.wDay%10, 1);
	      break;

	      /* A GDate has no time fields, so for now we can
	       * hardcode all time conversions into zeros (or 12 for
	       * %I). The alternative code snippets in the #else
	       * branches are here ready to be taken into use when
	       * needed by a g_strftime() or g_date_and_time_format()
	       * or whatever.
	       */
	    case 'H':
#if 1
	      g_array_append_vals (result, L"00", 2);
#else
	      g_array_append_vals (result, digits + systemtime.wHour/10, 1);
	      g_array_append_vals (result, digits + systemtime.wHour%10, 1);
#endif
	      break;
	    case 'I':
#if 1
	      g_array_append_vals (result, L"12", 2);
#else
	      if (systemtime.wHour == 0)
		g_array_append_vals (result, L"12", 2);
	      else
		{
		  g_array_append_vals (result, digits + (systemtime.wHour%12)/10, 1);
		  g_array_append_vals (result, digits + (systemtime.wHour%12)%10, 1);
		}
#endif
	      break;
	    case  'j':
	      g_array_append_vals (result, digits + (tm->tm_yday+1)/100, 1);
	      g_array_append_vals (result, digits + ((tm->tm_yday+1)/10)%10, 1);
	      g_array_append_vals (result, digits + (tm->tm_yday+1)%10, 1);
	      break;
	    case 'm':
	      g_array_append_vals (result, digits + systemtime.wMonth/10, 1);
	      g_array_append_vals (result, digits + systemtime.wMonth%10, 1);
	      break;
	    case 'M':
#if 1
	      g_array_append_vals (result, L"00", 2);
#else
	      g_array_append_vals (result, digits + systemtime.wMinute/10, 1);
	      g_array_append_vals (result, digits + systemtime.wMinute%10, 1);
#endif
	      break;
	    case 'n':
	      g_array_append_vals (result, L"\n", 1);
	      break;
	    case 'p':
	      n = GetTimeFormatW (lcid, 0, &systemtime, L"tt", NULL, 0);
	      if (n > 0)
		{
		  g_array_set_size (result, result->len + n);
		  GetTimeFormatW (lcid, 0, &systemtime, L"tt", ((wchar_t *) result->data) + result->len - n, n);
		  g_array_set_size (result, result->len - 1);
		}
	      break;
	    case 'r':
	      /* This is a rather odd format. Hard to say what to do.
	       * Let's always use the POSIX %I:%M:%S %p
	       */
#if 1
	      g_array_append_vals (result, L"12:00:00", 8);
#else
	      if (systemtime.wHour == 0)
		g_array_append_vals (result, L"12", 2);
	      else
		{
		  g_array_append_vals (result, digits + (systemtime.wHour%12)/10, 1);
		  g_array_append_vals (result, digits + (systemtime.wHour%12)%10, 1);
		}
	      g_array_append_vals (result, L":", 1);
	      g_array_append_vals (result, digits + systemtime.wMinute/10, 1);
	      g_array_append_vals (result, digits + systemtime.wMinute%10, 1);
	      g_array_append_vals (result, L":", 1);
	      g_array_append_vals (result, digits + systemtime.wSecond/10, 1);
	      g_array_append_vals (result, digits + systemtime.wSecond%10, 1);
	      g_array_append_vals (result, L" ", 1);
#endif
	      n = GetTimeFormatW (lcid, 0, &systemtime, L"tt", NULL, 0);
	      if (n > 0)
		{
		  g_array_set_size (result, result->len + n);
		  GetTimeFormatW (lcid, 0, &systemtime, L"tt", ((wchar_t *) result->data) + result->len - n, n);
		  g_array_set_size (result, result->len - 1);
		}
	      break;
	    case 'R':
#if 1
	      g_array_append_vals (result, L"00:00", 5);
#else
	      g_array_append_vals (result, digits + systemtime.wHour/10, 1);
	      g_array_append_vals (result, digits + systemtime.wHour%10, 1);
	      g_array_append_vals (result, L":", 1);
	      g_array_append_vals (result, digits + systemtime.wMinute/10, 1);
	      g_array_append_vals (result, digits + systemtime.wMinute%10, 1);
#endif
	      break;
	    case 'S':
#if 1
	      g_array_append_vals (result, L"00", 2);
#else
	      g_array_append_vals (result, digits + systemtime.wSecond/10, 1);
	      g_array_append_vals (result, digits + systemtime.wSecond%10, 1);
#endif
	      break;
	    case 't':
	      g_array_append_vals (result, L"\t", 1);
	      break;
	    case 'T':
#if 1
	      g_array_append_vals (result, L"00:00:00", 8);
#else
	      g_array_append_vals (result, digits + systemtime.wHour/10, 1);
	      g_array_append_vals (result, digits + systemtime.wHour%10, 1);
	      g_array_append_vals (result, L":", 1);
	      g_array_append_vals (result, digits + systemtime.wMinute/10, 1);
	      g_array_append_vals (result, digits + systemtime.wMinute%10, 1);
	      g_array_append_vals (result, L":", 1);
	      g_array_append_vals (result, digits + systemtime.wSecond/10, 1);
	      g_array_append_vals (result, digits + systemtime.wSecond%10, 1);
#endif
	      break;
	    case 'u':
	      if (systemtime.wDayOfWeek == 0)
		g_array_append_vals (result, L"7", 1);
	      else
		g_array_append_vals (result, digits + systemtime.wDayOfWeek, 1);
	      break;
	    case 'U':
	      n = g_date_get_sunday_week_of_year (d);
	      g_array_append_vals (result, digits + n/10, 1);
	      g_array_append_vals (result, digits + n%10, 1);
	      break;
	    case 'V':
	      n = g_date_get_iso8601_week_of_year (d);
	      g_array_append_vals (result, digits + n/10, 1);
	      g_array_append_vals (result, digits + n%10, 1);
	      break;
	    case 'w':
	      g_array_append_vals (result, digits + systemtime.wDayOfWeek, 1);
	      break;
	    case 'W':
	      n = g_date_get_monday_week_of_year (d);
	      g_array_append_vals (result, digits + n/10, 1);
	      g_array_append_vals (result, digits + n%10, 1);
	      break;
	    case 'x':
	      n = GetDateFormatW (lcid, 0, &systemtime, NULL, NULL, 0);
	      if (n > 0)
		{
		  g_array_set_size (result, result->len + n);
		  GetDateFormatW (lcid, 0, &systemtime, NULL, ((wchar_t *) result->data) + result->len - n, n);
		  g_array_set_size (result, result->len - 1);
		}
	      break;
	    case 'X':
	      n = GetTimeFormatW (lcid, 0, &systemtime, NULL, NULL, 0);
	      if (n > 0)
		{
		  g_array_set_size (result, result->len + n);
		  GetTimeFormatW (lcid, 0, &systemtime, NULL, ((wchar_t *) result->data) + result->len - n, n);
		  g_array_set_size (result, result->len - 1);
		}
	      break;
	    case 'y':
	      g_array_append_vals (result, digits + (systemtime.wYear/10)%10, 1);
	      g_array_append_vals (result, digits + systemtime.wYear%10, 1);
	      break;
	    case 'Y':
	      g_array_append_vals (result, digits + systemtime.wYear/1000, 1);
	      g_array_append_vals (result, digits + (systemtime.wYear/100)%10, 1);
	      g_array_append_vals (result, digits + (systemtime.wYear/10)%10, 1);
	      g_array_append_vals (result, digits + systemtime.wYear%10, 1);
	      break;
	    case 'Z':
	      n = GetTimeZoneInformation (&tzinfo);
	      if (n == TIME_ZONE_ID_UNKNOWN)
		;
	      else if (n == TIME_ZONE_ID_STANDARD)
		g_array_append_vals (result, tzinfo.StandardName, wcslen (tzinfo.StandardName));
	      else if (n == TIME_ZONE_ID_DAYLIGHT)
		g_array_append_vals (result, tzinfo.DaylightName, wcslen (tzinfo.DaylightName));
	      break;
	    case '%':
	      g_array_append_vals (result, L"%", 1);
	      break;
	    }      
	} 
      else if (c <= 0xFFFF)
	{
	  wchar_t wc = c;
	  g_array_append_vals (result, &wc, 1);
	}
      else
	{
	  glong nwc;
	  wchar_t *ws;

	  ws = g_ucs4_to_utf16 (&c, 1, NULL, &nwc, NULL);
	  g_array_append_vals (result, ws, nwc);
	  g_free (ws);
	}
      p = g_utf8_next_char (p);
    }
  
  convbuf = g_utf16_to_utf8 ((wchar_t *) result->data, result->len, NULL, &convlen, NULL);
  g_array_free (result, TRUE);

  if (!convbuf)
    {
      s[0] = '\0';
      return 0;
    }
  
  if (slen <= convlen)
    {
      /* Ensure only whole characters are copied into the buffer. */
      gchar *end = g_utf8_find_prev_char (convbuf, convbuf + slen);
      g_assert (end != NULL);
      convlen = end - convbuf;

      /* Return 0 because the buffer isn't large enough. */
      retval = 0;
    }
  else
    retval = convlen;

  memcpy (s, convbuf, convlen);
  s[convlen] = '\0';
  g_free (convbuf);

  return retval;
}

#endif

gsize     
g_date_strftime (gchar       *s, 
                 gsize        slen, 
                 const gchar *format, 
                 const GDate *d)
{
  struct tm tm;
#ifndef G_OS_WIN32
  gsize locale_format_len = 0;
  gchar *locale_format;
  gsize tmplen;
  gchar *tmpbuf;
  gsize tmpbufsize;
  gsize convlen = 0;
  gchar *convbuf;
  GError *error = NULL;
  gsize retval;
#endif

  g_return_val_if_fail (g_date_valid (d), 0);
  g_return_val_if_fail (slen > 0, 0); 
  g_return_val_if_fail (format != NULL, 0);
  g_return_val_if_fail (s != NULL, 0);

  g_date_to_struct_tm (d, &tm);

#ifdef G_OS_WIN32
  if (!g_utf8_validate (format, -1, NULL))
    {
      s[0] = '\0';
      return 0;
    }
  return win32_strftime_helper (d, format, &tm, s, slen);
#else

  locale_format = g_locale_from_utf8 (format, -1, NULL, &locale_format_len, &error);

  if (error)
    {
      g_warning (G_STRLOC "Error converting format to locale encoding: %s\n", error->message);
      g_error_free (error);

      s[0] = '\0';
      return 0;
    }

  tmpbufsize = MAX (128, locale_format_len * 2);
  while (TRUE)
    {
      tmpbuf = g_malloc (tmpbufsize);

      /* Set the first byte to something other than '\0', to be able to
       * recognize whether strftime actually failed or just returned "".
       */
      tmpbuf[0] = '\1';
      tmplen = strftime (tmpbuf, tmpbufsize, locale_format, &tm);

      if (tmplen == 0 && tmpbuf[0] != '\0')
        {
          g_free (tmpbuf);
          tmpbufsize *= 2;

          if (tmpbufsize > 65536)
            {
              g_warning (G_STRLOC "Maximum buffer size for g_date_strftime exceeded: giving up\n");
              g_free (locale_format);

              s[0] = '\0';
              return 0;
            }
        }
      else
        break;
    }
  g_free (locale_format);

  convbuf = g_locale_to_utf8 (tmpbuf, tmplen, NULL, &convlen, &error);
  g_free (tmpbuf);

  if (error)
    {
      g_warning (G_STRLOC "Error converting results of strftime to UTF-8: %s\n", error->message);
      g_error_free (error);

      s[0] = '\0';
      return 0;
    }

  if (slen <= convlen)
    {
      /* Ensure only whole characters are copied into the buffer.
       */
      gchar *end = g_utf8_find_prev_char (convbuf, convbuf + slen);
      g_assert (end != NULL);
      convlen = end - convbuf;

      /* Return 0 because the buffer isn't large enough.
       */
      retval = 0;
    }
  else
    retval = convlen;

  memcpy (s, convbuf, convlen);
  s[convlen] = '\0';
  g_free (convbuf);

  return retval;
#endif
}

#define __G_DATE_C__
#include "galiasdef.c"

