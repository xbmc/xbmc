%{
/* Parse a string into an internal time stamp.
   Copyright (C) 1999, 2000, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* Originally written by Steven M. Bellovin <smb@research.att.com> while
   at the University of North Carolina at Chapel Hill.  Later tweaked by
   a couple of people on Usenet.  Completely overhauled by Rich $alz
   <rsalz@bbn.com> and Jim Berets <jberets@bbn.com> in August, 1990.

   Modified by Paul Eggert <eggert@twinsun.com> in August 1999 to do
   the right thing about local DST.  Unlike previous versions, this
   version is reentrant.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
# endif
#endif

/* Since the code of getdate.y is not included in the Emacs executable
   itself, there is no need to #define static in this file.  Even if
   the code were included in the Emacs executable, it probably
   wouldn't do any harm to #undef it here; this will only cause
   problems if we try to write to a static variable, which I don't
   think this code needs to do.  */
#ifdef emacs
# undef static
#endif

#include <ctype.h>
#include <string.h>

#if HAVE_STDLIB_H
# include <stdlib.h> /* for `free'; used by Bison 1.27 */
#endif

#if STDC_HEADERS || (! defined isascii && ! HAVE_ISASCII)
# define IN_CTYPE_DOMAIN(c) 1
#else
# define IN_CTYPE_DOMAIN(c) isascii (c)
#endif

#define ISSPACE(c) (IN_CTYPE_DOMAIN (c) && isspace (c))
#define ISALPHA(c) (IN_CTYPE_DOMAIN (c) && isalpha (c))
#define ISLOWER(c) (IN_CTYPE_DOMAIN (c) && islower (c))
#define ISDIGIT_LOCALE(c) (IN_CTYPE_DOMAIN (c) && isdigit (c))

/* ISDIGIT differs from ISDIGIT_LOCALE, as follows:
   - Its arg may be any int or unsigned int; it need not be an unsigned char.
   - It's guaranteed to evaluate its argument exactly once.
   - It's typically faster.
   POSIX says that only '0' through '9' are digits.  Prefer ISDIGIT to
   ISDIGIT_LOCALE unless it's important to use the locale's definition
   of `digit' even when the host does not conform to POSIX.  */
#define ISDIGIT(c) ((unsigned) (c) - '0' <= 9)

#if STDC_HEADERS || HAVE_STRING_H
# include <string.h>
#endif

#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8) || __STRICT_ANSI__
# define __attribute__(x)
#endif

#ifndef ATTRIBUTE_UNUSED
# define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#endif

#define EPOCH_YEAR 1970
#define TM_YEAR_BASE 1900

#define HOUR(x) ((x) * 60)

/* An integer value, and the number of digits in its textual
   representation.  */
typedef struct
{
  int value;
  int digits;
} textint;

/* An entry in the lexical lookup table.  */
typedef struct
{
  char const *name;
  int type;
  int value;
} table;

/* Meridian: am, pm, or 24-hour style.  */
enum { MERam, MERpm, MER24 };

/* Information passed to and from the parser.  */
typedef struct
{
  /* The input string remaining to be parsed. */
  const char *input;

  /* N, if this is the Nth Tuesday.  */
  int day_ordinal;

  /* Day of week; Sunday is 0.  */
  int day_number;

  /* tm_isdst flag for the local zone.  */
  int local_isdst;

  /* Time zone, in minutes east of UTC.  */
  int time_zone;

  /* Style used for time.  */
  int meridian;

  /* Gregorian year, month, day, hour, minutes, and seconds.  */
  textint year;
  int month;
  int day;
  int hour;
  int minutes;
  int seconds;

  /* Relative year, month, day, hour, minutes, and seconds.  */
  int rel_year;
  int rel_month;
  int rel_day;
  int rel_hour;
  int rel_minutes;
  int rel_seconds;

  /* Counts of nonterminals of various flavors parsed so far.  */
  int dates_seen;
  int days_seen;
  int local_zones_seen;
  int rels_seen;
  int times_seen;
  int zones_seen;

  /* Table of local time zone abbrevations, terminated by a null entry.  */
  table local_time_zone_table[3];
} parser_control;

#define PC (* (parser_control *) parm)
#define YYLEX_PARAM parm
#define YYPARSE_PARAM parm

static int yyerror ();
static int yylex ();

%}

/* We want a reentrant parser.  */
%pure_parser

/* This grammar has 13 shift/reduce conflicts. */
%expect 13

%union
{
  int intval;
  textint textintval;
}

%token tAGO tDST

%token <intval> tDAY tDAY_UNIT tDAYZONE tHOUR_UNIT tLOCAL_ZONE tMERIDIAN
%token <intval> tMINUTE_UNIT tMONTH tMONTH_UNIT tSEC_UNIT tYEAR_UNIT tZONE

%token <textintval> tSNUMBER tUNUMBER

%type <intval> o_merid

%%

spec:
    /* empty */
  | spec item
  ;

item:
    time
      { PC.times_seen++; }
  | local_zone
      { PC.local_zones_seen++; }
  | zone
      { PC.zones_seen++; }
  | date
      { PC.dates_seen++; }
  | day
      { PC.days_seen++; }
  | rel
      { PC.rels_seen++; }
  | number
  ;

time:
    tUNUMBER tMERIDIAN
      {
	PC.hour = $1.value;
	PC.minutes = 0;
	PC.seconds = 0;
	PC.meridian = $2;
      }
  | tUNUMBER ':' tUNUMBER o_merid
      {
	PC.hour = $1.value;
	PC.minutes = $3.value;
	PC.seconds = 0;
	PC.meridian = $4;
      }
  | tUNUMBER ':' tUNUMBER tSNUMBER
      {
	PC.hour = $1.value;
	PC.minutes = $3.value;
	PC.meridian = MER24;
	PC.zones_seen++;
	PC.time_zone = $4.value % 100 + ($4.value / 100) * 60;
      }
  | tUNUMBER ':' tUNUMBER ':' tUNUMBER o_merid
      {
	PC.hour = $1.value;
	PC.minutes = $3.value;
	PC.seconds = $5.value;
	PC.meridian = $6;
      }
  | tUNUMBER ':' tUNUMBER ':' tUNUMBER tSNUMBER
      {
	PC.hour = $1.value;
	PC.minutes = $3.value;
	PC.seconds = $5.value;
	PC.meridian = MER24;
	PC.zones_seen++;
	PC.time_zone = $6.value % 100 + ($6.value / 100) * 60;
      }
  ;

local_zone:
    tLOCAL_ZONE
      { PC.local_isdst = $1; }
  | tLOCAL_ZONE tDST
      { PC.local_isdst = $1 < 0 ? 1 : $1 + 1; }
  ;

zone:
    tZONE
      { PC.time_zone = $1; }
  | tDAYZONE
      { PC.time_zone = $1 + 60; }
  | tZONE tDST
      { PC.time_zone = $1 + 60; }
  ;

day:
    tDAY
      {
	PC.day_ordinal = 1;
	PC.day_number = $1;
      }
  | tDAY ','
      {
	PC.day_ordinal = 1;
	PC.day_number = $1;
      }
  | tUNUMBER tDAY
      {
	PC.day_ordinal = $1.value;
	PC.day_number = $2;
      }
  ;

date:
    tUNUMBER '/' tUNUMBER
      {
	PC.month = $1.value;
	PC.day = $3.value;
      }
  | tUNUMBER '/' tUNUMBER '/' tUNUMBER
      {
	/* Interpret as YYYY/MM/DD if the first value has 4 or more digits,
	   otherwise as MM/DD/YY.
	   The goal in recognizing YYYY/MM/DD is solely to support legacy
	   machine-generated dates like those in an RCS log listing.  If
	   you want portability, use the ISO 8601 format.  */
	if (4 <= $1.digits)
	  {
	    PC.year = $1;
	    PC.month = $3.value;
	    PC.day = $5.value;
	  }
	else
	  {
	    PC.month = $1.value;
	    PC.day = $3.value;
	    PC.year = $5;
	  }
      }
  | tUNUMBER tSNUMBER tSNUMBER
      {
	/* ISO 8601 format.  YYYY-MM-DD.  */
	PC.year = $1;
	PC.month = -$2.value;
	PC.day = -$3.value;
      }
  | tUNUMBER tMONTH tSNUMBER
      {
	/* e.g. 17-JUN-1992.  */
	PC.day = $1.value;
	PC.month = $2;
	PC.year.value = -$3.value;
	PC.year.digits = $3.digits;
      }
  | tMONTH tUNUMBER
      {
	PC.month = $1;
	PC.day = $2.value;
      }
  | tMONTH tUNUMBER ',' tUNUMBER
      {
	PC.month = $1;
	PC.day = $2.value;
	PC.year = $4;
      }
  | tUNUMBER tMONTH
      {
	PC.day = $1.value;
	PC.month = $2;
      }
  | tUNUMBER tMONTH tUNUMBER
      {
	PC.day = $1.value;
	PC.month = $2;
	PC.year = $3;
      }
  ;

rel:
    relunit tAGO
      {
	PC.rel_seconds = -PC.rel_seconds;
	PC.rel_minutes = -PC.rel_minutes;
	PC.rel_hour = -PC.rel_hour;
	PC.rel_day = -PC.rel_day;
	PC.rel_month = -PC.rel_month;
	PC.rel_year = -PC.rel_year;
      }
  | relunit
  ;

relunit:
    tUNUMBER tYEAR_UNIT
      { PC.rel_year += $1.value * $2; }
  | tSNUMBER tYEAR_UNIT
      { PC.rel_year += $1.value * $2; }
  | tYEAR_UNIT
      { PC.rel_year += $1; }
  | tUNUMBER tMONTH_UNIT
      { PC.rel_month += $1.value * $2; }
  | tSNUMBER tMONTH_UNIT
      { PC.rel_month += $1.value * $2; }
  | tMONTH_UNIT
      { PC.rel_month += $1; }
  | tUNUMBER tDAY_UNIT
      { PC.rel_day += $1.value * $2; }
  | tSNUMBER tDAY_UNIT
      { PC.rel_day += $1.value * $2; }
  | tDAY_UNIT
      { PC.rel_day += $1; }
  | tUNUMBER tHOUR_UNIT
      { PC.rel_hour += $1.value * $2; }
  | tSNUMBER tHOUR_UNIT
      { PC.rel_hour += $1.value * $2; }
  | tHOUR_UNIT
      { PC.rel_hour += $1; }
  | tUNUMBER tMINUTE_UNIT
      { PC.rel_minutes += $1.value * $2; }
  | tSNUMBER tMINUTE_UNIT
      { PC.rel_minutes += $1.value * $2; }
  | tMINUTE_UNIT
      { PC.rel_minutes += $1; }
  | tUNUMBER tSEC_UNIT
      { PC.rel_seconds += $1.value * $2; }
  | tSNUMBER tSEC_UNIT
      { PC.rel_seconds += $1.value * $2; }
  | tSEC_UNIT
      { PC.rel_seconds += $1; }
  ;

number:
    tUNUMBER
      {
	if (PC.dates_seen
	    && ! PC.rels_seen && (PC.times_seen || 2 < $1.digits))
	  PC.year = $1;
	else
	  {
	    if (4 < $1.digits)
	      {
		PC.dates_seen++;
		PC.day = $1.value % 100;
		PC.month = ($1.value / 100) % 100;
		PC.year.value = $1.value / 10000;
		PC.year.digits = $1.digits - 4;
	      }
	    else
	      {
		PC.times_seen++;
		if ($1.digits <= 2)
		  {
		    PC.hour = $1.value;
		    PC.minutes = 0;
		  }
		else
		  {
		    PC.hour = $1.value / 100;
		    PC.minutes = $1.value % 100;
		  }
		PC.seconds = 0;
		PC.meridian = MER24;
	      }
	  }
      }
  ;

o_merid:
    /* empty */
      { $$ = MER24; }
  | tMERIDIAN
      { $$ = $1; }
  ;

%%

/* Include this file down here because bison inserts code above which
   may define-away `const'.  We want the prototype for get_date to have
   the same signature as the function definition.  */
#include "modules/getdate.h"

#ifndef gmtime
struct tm *gmtime ();
#endif
#ifndef localtime
struct tm *localtime ();
#endif
#ifndef mktime
time_t mktime ();
#endif

static table const meridian_table[] =
{
  { "AM",   tMERIDIAN, MERam },
  { "A.M.", tMERIDIAN, MERam },
  { "PM",   tMERIDIAN, MERpm },
  { "P.M.", tMERIDIAN, MERpm },
  { 0, 0, 0 }
};

static table const dst_table[] =
{
  { "DST", tDST, 0 }
};

static table const month_and_day_table[] =
{
  { "JANUARY",	tMONTH,	 1 },
  { "FEBRUARY",	tMONTH,	 2 },
  { "MARCH",	tMONTH,	 3 },
  { "APRIL",	tMONTH,	 4 },
  { "MAY",	tMONTH,	 5 },
  { "JUNE",	tMONTH,	 6 },
  { "JULY",	tMONTH,	 7 },
  { "AUGUST",	tMONTH,	 8 },
  { "SEPTEMBER",tMONTH,	 9 },
  { "SEPT",	tMONTH,	 9 },
  { "OCTOBER",	tMONTH,	10 },
  { "NOVEMBER",	tMONTH,	11 },
  { "DECEMBER",	tMONTH,	12 },
  { "SUNDAY",	tDAY,	 0 },
  { "MONDAY",	tDAY,	 1 },
  { "TUESDAY",	tDAY,	 2 },
  { "TUES",	tDAY,	 2 },
  { "WEDNESDAY",tDAY,	 3 },
  { "WEDNES",	tDAY,	 3 },
  { "THURSDAY",	tDAY,	 4 },
  { "THUR",	tDAY,	 4 },
  { "THURS",	tDAY,	 4 },
  { "FRIDAY",	tDAY,	 5 },
  { "SATURDAY",	tDAY,	 6 },
  { 0, 0, 0 }
};

static table const time_units_table[] =
{
  { "YEAR",	tYEAR_UNIT,	 1 },
  { "MONTH",	tMONTH_UNIT,	 1 },
  { "FORTNIGHT",tDAY_UNIT,	14 },
  { "WEEK",	tDAY_UNIT,	 7 },
  { "DAY",	tDAY_UNIT,	 1 },
  { "HOUR",	tHOUR_UNIT,	 1 },
  { "MINUTE",	tMINUTE_UNIT,	 1 },
  { "MIN",	tMINUTE_UNIT,	 1 },
  { "SECOND",	tSEC_UNIT,	 1 },
  { "SEC",	tSEC_UNIT,	 1 },
  { 0, 0, 0 }
};

/* Assorted relative-time words. */
static table const relative_time_table[] =
{
  { "TOMORROW",	tMINUTE_UNIT,	24 * 60 },
  { "YESTERDAY",tMINUTE_UNIT,	- (24 * 60) },
  { "TODAY",	tMINUTE_UNIT,	 0 },
  { "NOW",	tMINUTE_UNIT,	 0 },
  { "LAST",	tUNUMBER,	-1 },
  { "THIS",	tUNUMBER,	 0 },
  { "NEXT",	tUNUMBER,	 1 },
  { "FIRST",	tUNUMBER,	 1 },
/*{ "SECOND",	tUNUMBER,	 2 }, */
  { "THIRD",	tUNUMBER,	 3 },
  { "FOURTH",	tUNUMBER,	 4 },
  { "FIFTH",	tUNUMBER,	 5 },
  { "SIXTH",	tUNUMBER,	 6 },
  { "SEVENTH",	tUNUMBER,	 7 },
  { "EIGHTH",	tUNUMBER,	 8 },
  { "NINTH",	tUNUMBER,	 9 },
  { "TENTH",	tUNUMBER,	10 },
  { "ELEVENTH",	tUNUMBER,	11 },
  { "TWELFTH",	tUNUMBER,	12 },
  { "AGO",	tAGO,		 1 },
  { 0, 0, 0 }
};

/* The time zone table.  This table is necessarily incomplete, as time
   zone abbreviations are ambiguous; e.g. Australians interpret "EST"
   as Eastern time in Australia, not as US Eastern Standard Time.
   You cannot rely on getdate to handle arbitrary time zone
   abbreviations; use numeric abbreviations like `-0500' instead.  */
static table const time_zone_table[] =
{
  { "GMT",	tZONE,     HOUR ( 0) },	/* Greenwich Mean */
  { "UT",	tZONE,     HOUR ( 0) },	/* Universal (Coordinated) */
  { "UTC",	tZONE,     HOUR ( 0) },
  { "WET",	tZONE,     HOUR ( 0) },	/* Western European */
  { "WEST",	tDAYZONE,  HOUR ( 0) },	/* Western European Summer */
  { "BST",	tDAYZONE,  HOUR ( 0) },	/* British Summer */
  { "ART",	tZONE,	  -HOUR ( 3) },	/* Argentina */
  { "BRT",	tZONE,	  -HOUR ( 3) },	/* Brazil */
  { "BRST",	tDAYZONE, -HOUR ( 3) },	/* Brazil Summer */
  { "NST",	tZONE,	 -(HOUR ( 3) + 30) },	/* Newfoundland Standard */
  { "NDT",	tDAYZONE,-(HOUR ( 3) + 30) },	/* Newfoundland Daylight */
  { "AST",	tZONE,    -HOUR ( 4) },	/* Atlantic Standard */
  { "ADT",	tDAYZONE, -HOUR ( 4) },	/* Atlantic Daylight */
  { "CLT",	tZONE,    -HOUR ( 4) },	/* Chile */
  { "CLST",	tDAYZONE, -HOUR ( 4) },	/* Chile Summer */
  { "EST",	tZONE,    -HOUR ( 5) },	/* Eastern Standard */
  { "EDT",	tDAYZONE, -HOUR ( 5) },	/* Eastern Daylight */
  { "CST",	tZONE,    -HOUR ( 6) },	/* Central Standard */
  { "CDT",	tDAYZONE, -HOUR ( 6) },	/* Central Daylight */
  { "MST",	tZONE,    -HOUR ( 7) },	/* Mountain Standard */
  { "MDT",	tDAYZONE, -HOUR ( 7) },	/* Mountain Daylight */
  { "PST",	tZONE,    -HOUR ( 8) },	/* Pacific Standard */
  { "PDT",	tDAYZONE, -HOUR ( 8) },	/* Pacific Daylight */
  { "AKST",	tZONE,    -HOUR ( 9) },	/* Alaska Standard */
  { "AKDT",	tDAYZONE, -HOUR ( 9) },	/* Alaska Daylight */
  { "HST",	tZONE,    -HOUR (10) },	/* Hawaii Standard */
  { "HAST",	tZONE,	  -HOUR (10) },	/* Hawaii-Aleutian Standard */
  { "HADT",	tDAYZONE, -HOUR (10) },	/* Hawaii-Aleutian Daylight */
  { "SST",	tZONE,    -HOUR (12) },	/* Samoa Standard */
  { "WAT",	tZONE,     HOUR ( 1) },	/* West Africa */
  { "CET",	tZONE,     HOUR ( 1) },	/* Central European */
  { "CEST",	tDAYZONE,  HOUR ( 1) },	/* Central European Summer */
  { "MET",	tZONE,     HOUR ( 1) },	/* Middle European */
  { "MEZ",	tZONE,     HOUR ( 1) },	/* Middle European */
  { "MEST",	tDAYZONE,  HOUR ( 1) },	/* Middle European Summer */
  { "MESZ",	tDAYZONE,  HOUR ( 1) },	/* Middle European Summer */
  { "EET",	tZONE,     HOUR ( 2) },	/* Eastern European */
  { "EEST",	tDAYZONE,  HOUR ( 2) },	/* Eastern European Summer */
  { "CAT",	tZONE,	   HOUR ( 2) },	/* Central Africa */
  { "SAST",	tZONE,	   HOUR ( 2) },	/* South Africa Standard */
  { "EAT",	tZONE,	   HOUR ( 3) },	/* East Africa */
  { "MSK",	tZONE,	   HOUR ( 3) },	/* Moscow */
  { "MSD",	tDAYZONE,  HOUR ( 3) },	/* Moscow Daylight */
  { "IST",	tZONE,	  (HOUR ( 5) + 30) },	/* India Standard */
  { "SGT",	tZONE,     HOUR ( 8) },	/* Singapore */
  { "KST",	tZONE,     HOUR ( 9) },	/* Korea Standard */
  { "JST",	tZONE,     HOUR ( 9) },	/* Japan Standard */
  { "GST",	tZONE,     HOUR (10) },	/* Guam Standard */
  { "NZST",	tZONE,     HOUR (12) },	/* New Zealand Standard */
  { "NZDT",	tDAYZONE,  HOUR (12) },	/* New Zealand Daylight */
  { 0, 0, 0  }
};

/* Military time zone table. */
static table const military_table[] =
{
  { "A", tZONE,	-HOUR ( 1) },
  { "B", tZONE,	-HOUR ( 2) },
  { "C", tZONE,	-HOUR ( 3) },
  { "D", tZONE,	-HOUR ( 4) },
  { "E", tZONE,	-HOUR ( 5) },
  { "F", tZONE,	-HOUR ( 6) },
  { "G", tZONE,	-HOUR ( 7) },
  { "H", tZONE,	-HOUR ( 8) },
  { "I", tZONE,	-HOUR ( 9) },
  { "K", tZONE,	-HOUR (10) },
  { "L", tZONE,	-HOUR (11) },
  { "M", tZONE,	-HOUR (12) },
  { "N", tZONE,	 HOUR ( 1) },
  { "O", tZONE,	 HOUR ( 2) },
  { "P", tZONE,	 HOUR ( 3) },
  { "Q", tZONE,	 HOUR ( 4) },
  { "R", tZONE,	 HOUR ( 5) },
  { "S", tZONE,	 HOUR ( 6) },
  { "T", tZONE,	 HOUR ( 7) },
  { "U", tZONE,	 HOUR ( 8) },
  { "V", tZONE,	 HOUR ( 9) },
  { "W", tZONE,	 HOUR (10) },
  { "X", tZONE,	 HOUR (11) },
  { "Y", tZONE,	 HOUR (12) },
  { "Z", tZONE,	 HOUR ( 0) },
  { 0, 0, 0 }
};



static int
to_hour (int hours, int meridian)
{
  switch (meridian)
    {
    case MER24:
      return 0 <= hours && hours < 24 ? hours : -1;
    case MERam:
      return 0 < hours && hours < 12 ? hours : hours == 12 ? 0 : -1;
    case MERpm:
      return 0 < hours && hours < 12 ? hours + 12 : hours == 12 ? 12 : -1;
    default:
      abort ();
    }
  /* NOTREACHED */
    return 0;
}

static int
to_year (textint textyear)
{
  int year = textyear.value;

  if (year < 0)
    year = -year;

  /* XPG4 suggests that years 00-68 map to 2000-2068, and
     years 69-99 map to 1969-1999.  */
  if (textyear.digits == 2)
    year += year < 69 ? 2000 : 1900;

  return year;
}

static table const *
lookup_zone (parser_control const *pc, char const *name)
{
  table const *tp;

  /* Try local zone abbreviations first; they're more likely to be right.  */
  for (tp = pc->local_time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  for (tp = time_zone_table; tp->name; tp++)
    if (strcmp (name, tp->name) == 0)
      return tp;

  return 0;
}

#if ! HAVE_TM_GMTOFF
/* Yield the difference between *A and *B,
   measured in seconds, ignoring leap seconds.
   The body of this function is taken directly from the GNU C Library;
   see src/strftime.c.  */
static int
tm_diff (struct tm const *a, struct tm const *b)
{
  /* Compute intervening leap days correctly even if year is negative.
     Take care to avoid int overflow in leap day calculations,
     but it's OK to assume that A and B are close to each other.  */
  int a4 = (a->tm_year >> 2) + (TM_YEAR_BASE >> 2) - ! (a->tm_year & 3);
  int b4 = (b->tm_year >> 2) + (TM_YEAR_BASE >> 2) - ! (b->tm_year & 3);
  int a100 = a4 / 25 - (a4 % 25 < 0);
  int b100 = b4 / 25 - (b4 % 25 < 0);
  int a400 = a100 >> 2;
  int b400 = b100 >> 2;
  int intervening_leap_days = (a4 - b4) - (a100 - b100) + (a400 - b400);
  int years = a->tm_year - b->tm_year;
  int days = (365 * years + intervening_leap_days
	      + (a->tm_yday - b->tm_yday));
  return (60 * (60 * (24 * days + (a->tm_hour - b->tm_hour))
		+ (a->tm_min - b->tm_min))
	  + (a->tm_sec - b->tm_sec));
}
#endif /* ! HAVE_TM_GMTOFF */

static table const *
lookup_word (parser_control const *pc, char *word)
{
  char *p;
  char *q;
  size_t wordlen;
  table const *tp;
  int i;
  int abbrev;

  /* Make it uppercase.  */
  for (p = word; *p; p++)
    if (ISLOWER ((unsigned char) *p))
      *p = toupper ((unsigned char) *p);

  for (tp = meridian_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* See if we have an abbreviation for a month. */
  wordlen = strlen (word);
  abbrev = wordlen == 3 || (wordlen == 4 && word[3] == '.');

  for (tp = month_and_day_table; tp->name; tp++)
    if ((abbrev ? strncmp (word, tp->name, 3) : strcmp (word, tp->name)) == 0)
      return tp;

  if ((tp = lookup_zone (pc, word)))
    return tp;

  if (strcmp (word, dst_table[0].name) == 0)
    return dst_table;

  for (tp = time_units_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* Strip off any plural and try the units table again. */
  if (word[wordlen - 1] == 'S')
    {
      word[wordlen - 1] = '\0';
      for (tp = time_units_table; tp->name; tp++)
	if (strcmp (word, tp->name) == 0)
	  return tp;
      word[wordlen - 1] = 'S';	/* For "this" in relative_time_table.  */
    }

  for (tp = relative_time_table; tp->name; tp++)
    if (strcmp (word, tp->name) == 0)
      return tp;

  /* Military time zones. */
  if (wordlen == 1)
    for (tp = military_table; tp->name; tp++)
      if (word[0] == tp->name[0])
	return tp;

  /* Drop out any periods and try the time zone table again. */
  for (i = 0, p = q = word; (*p = *q); q++)
    if (*q == '.')
      i = 1;
    else
      p++;
  if (i && (tp = lookup_zone (pc, word)))
    return tp;

  return 0;
}

static int
yylex (YYSTYPE *lvalp, parser_control *pc)
{
  unsigned char c;
  int count;

  for (;;)
    {
      while (c = *pc->input, ISSPACE (c))
	pc->input++;

      if (ISDIGIT (c) || c == '-' || c == '+')
	{
	  char const *p;
	  int sign;
	  int value;
	  if (c == '-' || c == '+')
	    {
	      sign = c == '-' ? -1 : 1;
	      c = *++pc->input;
	      if (! ISDIGIT (c))
		/* skip the '-' sign */
		continue;
	    }
	  else
	    sign = 0;
	  p = pc->input;
	  value = 0;
	  do
	    {
	      value = 10 * value + c - '0';
	      c = *++p;
	    }
	  while (ISDIGIT (c));
	  lvalp->textintval.value = sign < 0 ? -value : value;
	  lvalp->textintval.digits = p - pc->input;
	  pc->input = p;
	  return sign ? tSNUMBER : tUNUMBER;
	}

      if (ISALPHA (c))
	{
	  char buff[20];
	  char *p = buff;
	  table const *tp;

	  do
	    {
	      if (p < buff + sizeof buff - 1)
		*p++ = c;
	      c = *++pc->input;
	    }
	  while (ISALPHA (c) || c == '.');

	  *p = '\0';
	  tp = lookup_word (pc, buff);
	  if (! tp)
	    return '?';
	  lvalp->intval = tp->value;
	  return tp->type;
	}

      if (c != '(')
	return *pc->input++;
      count = 0;
      do
	{
	  c = *pc->input++;
	  if (c == '\0')
	    return c;
	  if (c == '(')
	    count++;
	  else if (c == ')')
	    count--;
	}
      while (count > 0);
    }
}

/* Do nothing if the parser reports an error.  */
static int
yyerror (char *s ATTRIBUTE_UNUSED)
{
  return 0;
}

/* Parse a date/time string P.  Return the corresponding time_t value,
   or (time_t) -1 if there is an error.  P can be an incomplete or
   relative time specification; if so, use *NOW as the basis for the
   returned time.  */
time_t
get_date (const char *p, const time_t *now)
{
  time_t Start = now ? *now : time (0);
  struct tm *tmp = localtime (&Start);
  struct tm tm;
  struct tm tm0;
  parser_control pc;

  if (! tmp)
    return -1;

  pc.input = p;
  pc.year.value = tmp->tm_year + TM_YEAR_BASE;
  pc.year.digits = 4;
  pc.month = tmp->tm_mon + 1;
  pc.day = tmp->tm_mday;
  pc.hour = tmp->tm_hour;
  pc.minutes = tmp->tm_min;
  pc.seconds = tmp->tm_sec;
  tm.tm_isdst = tmp->tm_isdst;

  pc.meridian = MER24;
  pc.rel_seconds = 0;
  pc.rel_minutes = 0;
  pc.rel_hour = 0;
  pc.rel_day = 0;
  pc.rel_month = 0;
  pc.rel_year = 0;
  pc.dates_seen = 0;
  pc.days_seen = 0;
  pc.rels_seen = 0;
  pc.times_seen = 0;
  pc.local_zones_seen = 0;
  pc.zones_seen = 0;

#if HAVE_STRUCT_TM_TM_ZONE
  pc.local_time_zone_table[0].name = tmp->tm_zone;
  pc.local_time_zone_table[0].type = tLOCAL_ZONE;
  pc.local_time_zone_table[0].value = tmp->tm_isdst;
  pc.local_time_zone_table[1].name = 0;

  /* Probe the names used in the next three calendar quarters, looking
     for a tm_isdst different from the one we already have.  */
  {
    int quarter;
    for (quarter = 1; quarter <= 3; quarter++)
      {
	time_t probe = Start + quarter * (90 * 24 * 60 * 60);
	struct tm *probe_tm = localtime (&probe);
	if (probe_tm && probe_tm->tm_zone
	    && probe_tm->tm_isdst != pc.local_time_zone_table[0].value)
	  {
	      {
		pc.local_time_zone_table[1].name = probe_tm->tm_zone;
		pc.local_time_zone_table[1].type = tLOCAL_ZONE;
		pc.local_time_zone_table[1].value = probe_tm->tm_isdst;
		pc.local_time_zone_table[2].name = 0;
	      }
	    break;
	  }
      }
  }
#else
#if HAVE_TZNAME
  {
# ifndef tzname
    extern char *tzname[];
# endif
    int i;
    for (i = 0; i < 2; i++)
      {
	pc.local_time_zone_table[i].name = tzname[i];
	pc.local_time_zone_table[i].type = tLOCAL_ZONE;
	pc.local_time_zone_table[i].value = i;
      }
    pc.local_time_zone_table[i].name = 0;
  }
#else
  pc.local_time_zone_table[0].name = 0;
#endif
#endif

  if (pc.local_time_zone_table[0].name && pc.local_time_zone_table[1].name
      && ! strcmp (pc.local_time_zone_table[0].name,
		   pc.local_time_zone_table[1].name))
    {
      /* This locale uses the same abbrevation for standard and
	 daylight times.  So if we see that abbreviation, we don't
	 know whether it's daylight time.  */
      pc.local_time_zone_table[0].value = -1;
      pc.local_time_zone_table[1].name = 0;
    }

  if (yyparse (&pc) != 0
      || 1 < pc.times_seen || 1 < pc.dates_seen || 1 < pc.days_seen
      || 1 < (pc.local_zones_seen + pc.zones_seen)
      || (pc.local_zones_seen && 1 < pc.local_isdst))
    return -1;

  tm.tm_year = to_year (pc.year) - TM_YEAR_BASE + pc.rel_year;
  tm.tm_mon = pc.month - 1 + pc.rel_month;
  tm.tm_mday = pc.day + pc.rel_day;
  if (pc.times_seen || (pc.rels_seen && ! pc.dates_seen && ! pc.days_seen))
    {
      tm.tm_hour = to_hour (pc.hour, pc.meridian);
      if (tm.tm_hour < 0)
	return -1;
      tm.tm_min = pc.minutes;
      tm.tm_sec = pc.seconds;
    }
  else
    {
      tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
    }

  /* Let mktime deduce tm_isdst if we have an absolute time stamp,
     or if the relative time stamp mentions days, months, or years.  */
  if (pc.dates_seen | pc.days_seen | pc.times_seen | pc.rel_day
      | pc.rel_month | pc.rel_year)
    tm.tm_isdst = -1;

  /* But if the input explicitly specifies local time with or without
     DST, give mktime that information.  */
  if (pc.local_zones_seen)
    tm.tm_isdst = pc.local_isdst;

  tm0 = tm;

  Start = mktime (&tm);

  if (Start == (time_t) -1)
    {

      /* Guard against falsely reporting errors near the time_t boundaries
         when parsing times in other time zones.  For example, if the min
         time_t value is 1970-01-01 00:00:00 UTC and we are 8 hours ahead
         of UTC, then the min localtime value is 1970-01-01 08:00:00; if
         we apply mktime to 1970-01-01 00:00:00 we will get an error, so
         we apply mktime to 1970-01-02 08:00:00 instead and adjust the time
         zone by 24 hours to compensate.  This algorithm assumes that
         there is no DST transition within a day of the time_t boundaries.  */
      if (pc.zones_seen)
	{
	  tm = tm0;
	  if (tm.tm_year <= EPOCH_YEAR - TM_YEAR_BASE)
	    {
	      tm.tm_mday++;
	      pc.time_zone += 24 * 60;
	    }
	  else
	    {
	      tm.tm_mday--;
	      pc.time_zone -= 24 * 60;
	    }
	  Start = mktime (&tm);
	}

      if (Start == (time_t) -1)
	return Start;
    }

  if (pc.days_seen && ! pc.dates_seen)
    {
      tm.tm_mday += ((pc.day_number - tm.tm_wday + 7) % 7
		     + 7 * (pc.day_ordinal - (0 < pc.day_ordinal)));
      tm.tm_isdst = -1;
      Start = mktime (&tm);
      if (Start == (time_t) -1)
	return Start;
    }

  if (pc.zones_seen)
    {
      int delta = pc.time_zone * 60;
#ifdef HAVE_TM_GMTOFF
      delta -= tm.tm_gmtoff;
#else
      struct tm *gmt = gmtime (&Start);
      if (! gmt)
	return -1;
      delta -= tm_diff (&tm, gmt);
#endif
      if ((Start < Start - delta) != (delta < 0))
	return -1;	/* time_t overflow */
      Start -= delta;
    }

  /* Add relative hours, minutes, and seconds.  Ignore leap seconds;
     i.e. "+ 10 minutes" means 600 seconds, even if one of them is a
     leap second.  Typically this is not what the user wants, but it's
     too hard to do it the other way, because the time zone indicator
     must be applied before relative times, and if mktime is applied
     again the time zone will be lost.  */
  {
    time_t t0 = Start;
    long d1 = 60 * 60 * (long) pc.rel_hour;
    time_t t1 = t0 + d1;
    long d2 = 60 * (long) pc.rel_minutes;
    time_t t2 = t1 + d2;
    int d3 = pc.rel_seconds;
    time_t t3 = t2 + d3;
    if ((d1 / (60 * 60) ^ pc.rel_hour)
	| (d2 / 60 ^ pc.rel_minutes)
	| ((t0 + d1 < t0) ^ (d1 < 0))
	| ((t1 + d2 < t1) ^ (d2 < 0))
	| ((t2 + d3 < t2) ^ (d3 < 0)))
      return -1;
    Start = t3;
  }

  return Start;
}

#if TEST

#include <stdio.h>

int
main (int ac, char **av)
{
  char buff[BUFSIZ];
  time_t d;

  printf ("Enter date, or blank line to exit.\n\t> ");
  fflush (stdout);

  buff[BUFSIZ - 1] = 0;
  while (fgets (buff, BUFSIZ - 1, stdin) && buff[0])
    {
      d = get_date (buff, 0);
      if (d == (time_t) -1)
	printf ("Bad format - couldn't convert.\n");
      else
	printf ("%s", ctime (&d));
      printf ("\t> ");
      fflush (stdout);
    }
  return 0;
}
#endif /* defined TEST */
