///
///	@file 	date.cpp
/// @brief 	Date management routines
///	@overview Date management routines for Appweb
///
/////////////////////////////////// Copyright //////////////////////////////////
//
//	@copy	default.g
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	Portions Copyright (c) GoAhead Software, 1995-2000. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"shared.h"

/////////////////////////////// Forward Definitions ////////////////////////////

#if BLD_FEATURE_IF_MODIFIED
static int parseWeekday(char *buf, int *index);
static time_t parseDate1or2(char *buf, int *index);
static int parseTime(char *buf, int *index);
static time_t parseDate3Time(char *buf, int *index);
#endif

///////////////////////////////////// Code /////////////////////////////////////
//
//	Build an ASCII time string.  If sbuf is NULL we use the current time,
//	else we use the last modified time of sbuf;
//

char *maGetDateString(MprFileInfo *sbuf)
{
	char		*dateStr;
	struct tm	tm;
	time_t		when;

	if (sbuf == 0) {
		time(&when);
	} else {
		when = (time_t) sbuf->mtime;
	}
	dateStr = (char*) mprMalloc(64);
	mprGmtime(&when, &tm);
	mprRfcTime(dateStr, 64, &tm);
	return dateStr;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_IF_MODIFIED

//
//	Parse the a date/time string. FUTURE -- return time_t
// 

int maDateParse(char *cmd)
{
	int index, tmpIndex, weekday, timeValue;
	time_t parsedValue, dateValue;

	parsedValue = (time_t) 0;
	index =	timeValue = 0;
	weekday = parseWeekday(cmd, &index);

	if (weekday >= 0) {
		tmpIndex = index;
		dateValue = parseDate1or2(cmd, &tmpIndex);
		if (dateValue >= 0) {
			index = tmpIndex + 1;
			//
			//	One of these two forms is being used
			//	wkday "," SP date1 SP time SP "GMT"
			//	weekday "," SP date2 SP time SP "GMT"
 			//
			timeValue = parseTime(cmd, &index);
			if (timeValue >= 0) {
				//				
				//	Now match up that "GMT" string for completeness
				//	Compute the final value if there were no problems in 
				//	the parse
				//
				if ((weekday >= 0) &&
					(dateValue >= 0) &&
					(timeValue >= 0)) {
					parsedValue = dateValue + timeValue;
				}
			}
		} else {
			// 
			//	Try the other form - wkday SP date3 SP time SP 4DIGIT
			//
 
			tmpIndex = index;
			parsedValue = parseDate3Time(cmd, &tmpIndex);
		}
	}

	return (int) parsedValue;
}

////////////////////////////////////////////////////////////////////////////////

//	
//	These functions are intended to closely mirror the syntax for HTTP-date 
//	from RFC 2616 (MPR_HTTP/1.1 spec).  This code was submitted by 
//	Pete Bergstrom.
//
//	
//	RFC1123Date	= wkday "," SP date1 SP time SP "GMT"
//	RFC850Date	= weekday "," SP date2 SP time SP "GMT"
//	ASCTimeDate	= wkday SP date3 SP time SP 4DIGIT
//
//	Each of these functions tries to parse the value and update the index to 
//	the point it leaves off parsing.
//

typedef enum { JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC } MonthEnumeration;
typedef enum { SUN, MON, TUE, WED, THU, FRI, SAT } WeekdayEnumeration;

////////////////////////////////////////////////////////////////////////////////
//	
//	Parse an N-digit value
//

static int parseNDIGIT(char *buf, int digits, int *index) 
{
	int tmpIndex, returnValue;

	returnValue = 0;

	for (tmpIndex = *index; tmpIndex < *index+digits; tmpIndex++) {
		if (isdigit(buf[tmpIndex])) {
			returnValue = returnValue * 10 + (buf[tmpIndex] - '0');
		}
	}
	*index = tmpIndex;
	
	return returnValue;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return an index into the month array
// 

static int parseMonth(char *buf, int *index) 
{
	//	"Jan" | "Feb" | "Mar" | "Apr" | "May" | "Jun" | 
	//	"Jul" | "Aug" | "Sep" | "Oct" | "Nov" | "Dec"
 
	int tmpIndex, returnValue;

	returnValue = -1;
	tmpIndex = *index;

	switch (buf[tmpIndex]) {
		case 'A':
			switch (buf[tmpIndex+1]) {
				case 'p':
					returnValue = APR;
					break;
				case 'u':
					returnValue = AUG;
					break;
			}
			break;
		case 'D':
			returnValue = DEC;
			break;
		case 'F':
			returnValue = FEB;
			break;
		case 'J':
			switch (buf[tmpIndex+1]) {
				case 'a':
					returnValue = JAN;
					break;
				case 'u':
					switch (buf[tmpIndex+2]) {
						case 'l':
							returnValue = JUL;
							break;
						case 'n':
							returnValue = JUN;
							break;
					}
					break;
			}
			break;
		case 'M':
			switch (buf[tmpIndex+1]) {
				case 'a':
					switch (buf[tmpIndex+2]) {
						case 'r':
							returnValue = MAR;
							break;
						case 'y':
							returnValue = MAY;
							break;
					}
					break;
			}
			break;
		case 'N':
			returnValue = NOV;
			break;
		case 'O':
			returnValue = OCT;
			break;
		case 'S':
			returnValue = SEP;
			break;
	}

	if (returnValue >= 0) {
		*index += 3;
	}

	return returnValue;
}

////////////////////////////////////////////////////////////////////////////////
// 
//	Parse a year value (either 2 or 4 digits)
// 

static int parseYear(char *buf, int *index) 
{
	int tmpIndex, returnValue;

	tmpIndex = *index;
	returnValue = parseNDIGIT(buf, 4, &tmpIndex);

	if (returnValue >= 0) {
		*index = tmpIndex;
	} else {
		returnValue = parseNDIGIT(buf, 2, &tmpIndex);
		if (returnValue >= 0) {
			//
			//	Assume that any year earlier than the start of the 
			//	epoch for time_t (1970) specifies 20xx
			//
			if (returnValue < 70) {
				returnValue += 2000;
			} else {
				returnValue += 1900;
			}

			*index = tmpIndex;
		}
	}

	return returnValue;
}

////////////////////////////////////////////////////////////////////////////////
//
//	The formulas used to build these functions are from "Calendrical 
//	Calculations", by Nachum Dershowitz, Edward M. Reingold, Cambridge 
//	University Press, 1997.
//

#include <math.h>

const int GregorianEpoch = 1;

////////////////////////////////////////////////////////////////////////////////
//
//  Determine if year is a leap year
//

int GregorianLeapYearP(long year) 
{
	int		result;
	long	tmp;
	
	tmp = year % 400;

	if ((year % 4 == 0) &&
		(tmp != 100) &&
		(tmp != 200) &&
		(tmp != 300)) {
		result = 1;
	} else {
		result = 0;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Return the fixed date from the gregorian date
//

long FixedFromGregorian(long month, long day, long year) 
{
	long fixedDate;

	fixedDate = (long) (GregorianEpoch - 1 + 365 * (year - 1) + 
		((year - 1 + 2) / 4) -
		((year - 1 + 50) / 100) + 
		((year - 1 + 200) / 400) + 
		((367 * month - 362 + 6) / 12));

	if (month <= 2) {
		fixedDate += 0;
	} else if (1 == GregorianLeapYearP(year)) {
		fixedDate += -1;
	} else {
		fixedDate += -2;
	}

	fixedDate += day - 1;

	return fixedDate;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Return the gregorian year from a fixed date
 

long GregorianYearFromFixed(long fixedDate) 
{
	long result, d0, n400, d1, n100, d2, n4, d3, n1, d4, year;

	d0 =	fixedDate - GregorianEpoch;
	n400 =	(d0 + 146097/2) / 146097;
	d1 =	d0 % 146097;
	n100 =	(d1 + 36524/2) / 36524;
	d2 =	d1 % 36524;
	n4 =	(d2 + 1461/2) / 1461;
	d3 =	d2 % 1461;
	n1 =	(d3 + 365/2) / 365;
	d4 =	(d3 % 365) + 1;
	year =	400 * n400 + 100 * n100 + 4 * n4 + n1;

	if ((n100 == 4) || (n1 == 4)) {
		result = year;
	} else {
		result = year + 1;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
// 
//	Returns the Gregorian date from a fixed date
//	(not needed for this use, but included for completeness
 

#if 0
GregorianFromFixed(long fixedDate, long *month, long *day, long *year) 
{
	long priorDays, correction;

	*year =			GregorianYearFromFixed(fixedDate);
	priorDays =		fixedDate - FixedFromGregorian(1, 1, *year);

	if (fixedDate < FixedFromGregorian(3,1,*year) {
		correction = 0;
	} else if (true == GregorianLeapYearP(*year) {
		correction = 1;
	} else {
		correction = 2;
	}

	*month = (long)(floor((12.0 * (double)(priorDays + correction) + 373.0) / 367.0);
	*day = fixedDate - FixedFromGregorian(*month, 1, *year);
}
#endif

////////////////////////////////////////////////////////////////////////////////
// 
//	Returns the difference between two Gregorian dates
// 

long GregorianDateDifference(long month1, long day1, long year1,
								long month2, long day2, long year2) 
{
	return FixedFromGregorian(month2, day2, year2) - 
		FixedFromGregorian(month1, day1, year1);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Return the number of seconds into the current day
 

#define SECONDS_PER_DAY 24*60*60

static int parseTime(char *buf, int *index) 
{
	//	Format of buf is - 2DIGIT ":" 2DIGIT ":" 2DIGIT
 
	int returnValue, tmpIndex, hourValue, minuteValue, secondValue;

	hourValue = minuteValue = secondValue = -1;
	returnValue = -1;
	tmpIndex = *index;

	hourValue = parseNDIGIT(buf, 2, &tmpIndex);

	if (hourValue >= 0) {
		tmpIndex++;
		minuteValue = parseNDIGIT(buf, 2, &tmpIndex);
		if (minuteValue >= 0) {
			tmpIndex++;
			secondValue = parseNDIGIT(buf, 2, &tmpIndex);
		}
	}

	if ((hourValue >= 0) &&
		(minuteValue >= 0) &&
		(secondValue >= 0)) {
		returnValue = (((hourValue * 60) + minuteValue) * 60) + secondValue;
		*index = tmpIndex;
	}

	return returnValue;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the equivalent of time() given a gregorian date
 

static time_t dateToTimet(int year, int month, int day) 
{
	long dayDifference;

	//
     // Bug fix by Jeff Reeder (Jun 14, 2002): The 'month' parameter is 
     // numbered from  0 (Jan == 0), but FixedFromGregorian() takes 
     // months numbered from 1 (January == 1). We need to add 1 
     // to the month 
     //
	dayDifference = FixedFromGregorian(month + 1, day, year) - 
		FixedFromGregorian(1, 1, 1970);

	return dayDifference * SECONDS_PER_DAY;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the number of seconds between Jan 1, 1970 and the parsed date
//	(corresponds to documentation for time() function)
//

static time_t parseDate1or2(char *buf, int *index) 
{
	//	Format of buf is either
	//	2DIGIT SP month SP 4DIGIT
	//	or
	//	2DIGIT "-" month "-" 2DIGIT
 
	int		dayValue, monthValue, yearValue, tmpIndex;
	time_t	returnValue;

	returnValue = (time_t) -1;
	tmpIndex = *index;

	dayValue = monthValue = yearValue = -1;

	if (buf[tmpIndex] == ',') {
		// 
		//	Skip over the ", " 
		//
 
		tmpIndex += 2; 

		dayValue = parseNDIGIT(buf, 2, &tmpIndex);
		if (dayValue >= 0) {
			//
			//	Skip over the space or hyphen
			//
			tmpIndex++; 
			monthValue = parseMonth(buf, &tmpIndex);
			if (monthValue >= 0) {
				//
				//	Skip over the space or hyphen
				//
				tmpIndex++; 
				yearValue = parseYear(buf, &tmpIndex);
			}
		}

		if ((dayValue >= 0) &&
			(monthValue >= 0) &&
			(yearValue >= 0)) {
			if (yearValue < 1970) {
				//
				//	Allow for Microsoft IE's year 1601 dates 
				//
				returnValue = 0; 

			} else {
				returnValue = dateToTimet(yearValue, monthValue, dayValue);
			}
			*index = tmpIndex;
		}
	}
	
	return returnValue;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the number of seconds between Jan 1, 1970 and the parsed date
//

static time_t parseDate3Time(char *buf, int *index) 
{
	//	Format of buf is month SP ( 2DIGIT | ( SP 1DIGIT )
 
	int		dayValue, monthValue, yearValue, timeValue, tmpIndex;
	time_t	returnValue;

	returnValue = (time_t) -1;
	tmpIndex = *index;

	dayValue = monthValue = yearValue = timeValue = -1;

	monthValue = parseMonth(buf, &tmpIndex);
	if (monthValue >= 0) {
		//		
		//	Skip over the space 
		//
		tmpIndex++; 
		if (buf[tmpIndex] == ' ') {
			//
			//	Skip over this space too 
 			//
			tmpIndex++; 
			dayValue = parseNDIGIT(buf, 1, &tmpIndex);
		} else {
			dayValue = parseNDIGIT(buf, 2, &tmpIndex);
		}
		//		
		//	Now get the time and time SP 4DIGIT
 		//
		timeValue = parseTime(buf, &tmpIndex);
		if (timeValue >= 0) {
			//			
			//	Now grab the 4DIGIT year value
 			//
			yearValue = parseYear(buf, &tmpIndex);
		}
	}

	if ((dayValue >= 0) &&
		(monthValue >= 0) &&
		(yearValue >= 0)) {
		returnValue = dateToTimet(yearValue, monthValue, dayValue);
		returnValue += timeValue;
		*index = tmpIndex;
	}
	
	return returnValue;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Although this looks like a trivial function, I found I was replicating 
//	the implementation seven times in the parseWeekday function. In the 
//	interests of minimizing code size and redundancy, it is broken out 
//	into a separate function. The cost of an extra function call I can 
//	live with given that it should only be called once per MPR_HTTP request.
// 

static int bufferIndexIncrementGivenNTest(char *buf, int testIndex, 
	char testChar, int foundIncrement, int notfoundIncrement) 
{
	if (buf[testIndex] == testChar) {
		return foundIncrement;
	}
	return notfoundIncrement;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return an index into a logical weekday array
 

static int parseWeekday(char *buf, int *index) 
{
	//
	//	Format of buf is either
	//	"Mon" | "Tue" | "Wed" | "Thu" | "Fri" | "Sat" | "Sun"
	//	or
	//	"Monday" | "Tuesday" | "Wednesday" | "Thursday" | "Friday" | 
	//		"Saturday" | "Sunday"
 	//
	int tmpIndex, returnValue;

	returnValue = -1;
	tmpIndex = *index;

	switch (buf[tmpIndex]) {
		case 'F':
			returnValue = FRI;
			*index += bufferIndexIncrementGivenNTest(buf, tmpIndex+3, 
				'd', sizeof("Friday"), 3);
			break;
		case 'M':
			returnValue = MON;
			*index += bufferIndexIncrementGivenNTest(buf, tmpIndex+3, 
				'd', sizeof("Monday"), 3);
			break;
		case 'S':
			switch (buf[tmpIndex+1]) {
				case 'a':
					returnValue = SAT;
					*index += bufferIndexIncrementGivenNTest(buf, tmpIndex+3, 
						'u', sizeof("Saturday"), 3);
					break;
				case 'u':
					returnValue = SUN;
					*index += bufferIndexIncrementGivenNTest(buf, tmpIndex+3, 
						'd', sizeof("Sunday"), 3);
					break;
			}
			break;
		case 'T':
			switch (buf[tmpIndex+1]) {
				case 'h':
					returnValue = THU;
					*index += bufferIndexIncrementGivenNTest(buf, tmpIndex+3, 
						'r', sizeof("Thursday"), 3);
					break;
				case 'u':
					returnValue = TUE;
					*index += bufferIndexIncrementGivenNTest(buf, tmpIndex+3, 
						's', sizeof("Tuesday"), 3);
					break;
			}
			break;
		case 'W':
			returnValue = WED;
			*index += bufferIndexIncrementGivenNTest(buf, tmpIndex+3, 'n', 
				sizeof("Wednesday"), 3);
			break;
	}
	return returnValue;
}

////////////////////////////////////////////////////////////////////////////////
#endif // BLD_FEATURE_HTTP_IF_MODIFIED 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
