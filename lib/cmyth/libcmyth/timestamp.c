/*
 *  Copyright (C) 2004-2010, Eric Lund
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * timestamp.c - functions to manage MythTV timestamps.  Primarily,
 *               these allocate timestamps and convert between string
 *               and cmyth_timestamp_t and between time_t and
 *               cmyth_timestamp_t.
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <cmyth_local.h>

/*
 * cmyth_timestamp_create(void)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a timestamp structure and return a pointer to the structure.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_timestamp_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_timestamp_t
 */
cmyth_timestamp_t
cmyth_timestamp_create(void)
{
	cmyth_timestamp_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ret) {
		return(NULL);
	}
	ret->timestamp_year = 0;
	ret->timestamp_month = 0;
	ret->timestamp_day = 0;
	ret->timestamp_hour = 0;
	ret->timestamp_minute = 0;
	ret->timestamp_second = 0;
	ret->timestamp_isdst = 0;
	return ret;
}

/*
 * cmyth_timestamp_from_string(char *str)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create and fill out a timestamp structure using the string 'str'.
 * The string must be a timestamp of the forn:
 *
 *    yyyy-mm-ddThh:mm:ss
 *
 * Return Value:
 *
 * Success: A timestamp structure (this is a pointer type)
 *
 * Failure: NULL
 */
cmyth_timestamp_t
cmyth_timestamp_from_string(char *str)
{
	cmyth_timestamp_t ret;
	unsigned int i;
	int datetime = 1;
	char *yyyy = &str[0];
	char *MM = &str[5];
	char *dd = &str[8];
	char *hh = &str[11];
	char *mm = &str[14];
	char *ss = &str[17];
	
	if (!str) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL string\n", __FUNCTION__);
		return NULL;
	}

	ret = cmyth_timestamp_create();
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL timestamp\n",
			  __FUNCTION__);
		return NULL;
	}
	if (strlen(str) != CMYTH_TIMESTAMP_LEN) {
		datetime = 0;
		if (strlen(str) != CMYTH_DATESTAMP_LEN) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: string is not a timestamp '%s'\n",
				  __FUNCTION__, str);
			goto err;
		}
	}

	if ((datetime == 1) &&
	    ((str[4] != '-') || (str[7] != '-') || (str[10] != 'T') ||
	     (str[13] != ':') || (str[16] != ':'))) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: string is badly formed '%s'\n",
			  __FUNCTION__, str);
		goto err;
	}
	if ((datetime == 0) &&
	    ((str[4] != '-') || (str[7] != '-'))) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: string is badly formed '%s'\n",
			  __FUNCTION__, str);
		goto err;
	}

	str[4] = '\0';
	str[7] = '\0';
	if (datetime) {
		str[10] = '\0';
		str[13] = '\0';
		str[16] = '\0';
	}
	for (i = 0;
	     i < (datetime ? CMYTH_TIMESTAMP_LEN : CMYTH_DATESTAMP_LEN);
	     ++i) {
		if (str[i] && !isdigit(str[i])) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: expected numeral at '%s'[%d]\n",
				  __FUNCTION__, str, i);
			goto err;
		}
	}
	ret->timestamp_year = atoi(yyyy);
	ret->timestamp_month = atoi(MM);
	if (ret->timestamp_month > 12) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: month value too big'%s'\n",
			  __FUNCTION__, str);
		goto err;
	}
	ret->timestamp_day = atoi(dd);
	if (ret->timestamp_day > 31) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: day value too big'%s'\n",
			  __FUNCTION__, str);
		goto err;
	}

	if (datetime == 0)
		return ret;

	ret->timestamp_hour = atoi(hh);
	if (ret->timestamp_hour > 23) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: hour value too big'%s'\n",
			  __FUNCTION__, str);
		goto err;
	}
	ret->timestamp_minute = atoi(mm);
	if (ret->timestamp_minute > 59) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: minute value too big'%s'\n",
			  __FUNCTION__, str);
		goto err;
	}
	ret->timestamp_second = atoi(ss);
	if (ret->timestamp_second > 59) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: second value too big'%s'\n",
			  __FUNCTION__, str);
		goto err;
	}
	return ret;

    err:
	ref_release(ret);
	return NULL;
}

cmyth_timestamp_t
cmyth_timestamp_from_tm(struct tm * tm_datetime)
{
    	cmyth_timestamp_t ret;
	ret = cmyth_timestamp_create();
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL timestamp\n",
			  __FUNCTION__);
		return NULL;
	}

	ret->timestamp_year = tm_datetime->tm_year + 1900;
	ret->timestamp_month = tm_datetime->tm_mon + 1;
	ret->timestamp_day = tm_datetime->tm_mday;
	ret->timestamp_hour = tm_datetime->tm_hour;
	ret->timestamp_minute = tm_datetime->tm_min;
	ret->timestamp_second = tm_datetime->tm_sec;
	ret->timestamp_isdst = tm_datetime->tm_isdst;
	return ret;
}

/*
 * cmyth_timestamp_from_unixtime(time_t l)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create and fill out a timestamp structure using the time_t 'l'.
 *
 * Return Value:
 *
 * Success: cmyth_timestamp_t object
 *
 * Failure: -(ERRNO)
 */
cmyth_timestamp_t
cmyth_timestamp_from_unixtime(time_t l)
{
	struct tm tm_datetime;
	localtime_r(&l,&tm_datetime);
	return cmyth_timestamp_from_tm(&tm_datetime);
}


/*
 * cmyth_timestamp_to_longlong( cmyth_timestamp_t ts)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a time_t value from the timestamp structure 'ts' and
 * return the result.
 * 
 *
 * Return Value:
 *
 * Success: time_t value > 0 (seconds from January 1, 1970)
 *
 * Failure: (time_t) -1
 */
time_t
cmyth_timestamp_to_unixtime(cmyth_timestamp_t ts)
{
    struct tm tm;
    tm.tm_sec = ts->timestamp_second;
    tm.tm_min = ts->timestamp_minute;
    tm.tm_hour = ts->timestamp_hour;
    tm.tm_mday = ts->timestamp_day;
    tm.tm_mon = ts->timestamp_month-1;
    tm.tm_year = ts->timestamp_year-1900;
    tm.tm_isdst = ts->timestamp_isdst;
    return mktime(&tm);
}

/*
 * cmyth_timestamp_to_string(char *str, cmyth_timestamp_t ts)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a string from the timestamp structure 'ts' and put it in the
 * user supplied buffer 'str'.  The size of 'str' must be
 * CMYTH_TIMESTAMP_LEN + 1 or this will overwrite beyond 'str'.
 * 
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_timestamp_to_string(char *str, cmyth_timestamp_t ts)
{
	if (!str) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL output string provided\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	if (!ts) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL timestamp provided\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	sprintf(str,
		"%4.4ld-%2.2ld-%2.2ldT%2.2ld:%2.2ld:%2.2ld",
		ts->timestamp_year,
		ts->timestamp_month,
		ts->timestamp_day,
		ts->timestamp_hour,
		ts->timestamp_minute,
		ts->timestamp_second);
	return 0;
}

/*
 * cmyth_timestamp_to_isostring(char *str, cmyth_timestamp_t ts)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a string from the timestamp structure 'ts' and put it in the
 * user supplied buffer 'str'.  The size of 'str' must be
 * CMYTH_TIMESTAMP_LEN + 1 or this will overwrite beyond 'str'.
 * 
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_timestamp_to_isostring(char *str, cmyth_timestamp_t ts)
{
	if (!str) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL output string provided\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	if (!ts) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL timestamp provided\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	sprintf(str,
		"%4.4ld-%2.2ld-%2.2ld",
		ts->timestamp_year,
		ts->timestamp_month,
		ts->timestamp_day);
	return 0;
}

int
cmyth_timestamp_to_display_string(char *str, cmyth_timestamp_t ts,
																	int time_format_12)
{
	if (!str) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL output string provided\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	if (!ts) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL timestamp provided\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	if (time_format_12)
	{
		unsigned long hour = ts->timestamp_hour;
		int pm = 0;
		if (hour > 11)
		{
			pm = 1;
			hour -= 12;
		}
		if (hour == 0)
			hour = 12;

		sprintf(str,
			"%4.4ld-%2.2ld-%2.2ldT%2.2ld:%2.2ld:%2.2ld %s",
			ts->timestamp_year,
			ts->timestamp_month,
			ts->timestamp_day,
			hour,
			ts->timestamp_minute,
			ts->timestamp_second,
			pm ? "PM" : "AM");
	}
	else
	{
		sprintf(str,
			"%4.4ld-%2.2ld-%2.2ldT%2.2ld:%2.2ld:%2.2ld",
			ts->timestamp_year,
			ts->timestamp_month,
			ts->timestamp_day,
			ts->timestamp_hour,
			ts->timestamp_minute,
			ts->timestamp_second);
	}
	return 0;
}

/*
 * cmyth_datetime_to_string(char *str, cmyth_timestamp_t ts)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a string from the timestamp structure 'ts' and put it in the
 * user supplied buffer 'str'.  The size of 'str' must be
 * CMYTH_DATETIME_LEN + 1 or this will overwrite beyond 'str'.
 * 
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_datetime_to_string(char *str, cmyth_timestamp_t ts)
{
	struct tm tm_datetime;
	time_t t_datetime;

	if (!str) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL output string provided\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	if (!ts) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL timestamp provided\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	memset(&tm_datetime, 0, sizeof(tm_datetime));
	tm_datetime.tm_year = ts->timestamp_year - 1900;
	tm_datetime.tm_mon = ts->timestamp_month - 1;
	tm_datetime.tm_mday = ts->timestamp_day;
	tm_datetime.tm_hour = ts->timestamp_hour;
	tm_datetime.tm_min = ts->timestamp_minute;
	tm_datetime.tm_sec = ts->timestamp_second;
	tm_datetime.tm_isdst = ts->timestamp_isdst;
	t_datetime = mktime(&tm_datetime);
	sprintf(str,
		"%4.4ld-%2.2ld-%2.2ldT%2.2ld:%2.2ld:%2.2ld",
		ts->timestamp_year,
		ts->timestamp_month,
		ts->timestamp_day,
		ts->timestamp_hour,
		ts->timestamp_minute,
		ts->timestamp_second);
	cmyth_dbg(CMYTH_DBG_ERROR, "original timestamp string: %s \n",str);
	sprintf(str,"%lu",(unsigned long) t_datetime);
	cmyth_dbg(CMYTH_DBG_ERROR, "time in seconds: %s \n",str);
	
	return 0;
}



/*
 * cmyth_timestamp_compare(cmyth_timestamp_t ts1, cmyth_timestamp_t ts2)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Compare ts1 to ts2 and indicate whether ts1 is less than, equal to
 * or greater than ts1.
 * 
 *
 * Return Value:
 *
 * -1: ts1 is less than (erlier than) ts2
 *  0: ts1 is the same as ts2
 *  1: ts1 is greater than (later than) ts2
 */
int
cmyth_timestamp_compare(cmyth_timestamp_t ts1, cmyth_timestamp_t ts2)
{
	/*
	 * If either timestamp is NULL it is 'less than' the non-NULL one
	 * (this is a stretch, but it shouldn't happen).  If they are both
	 * NULL, they are equal.
	 */
	if (!ts1) {
		if (!ts2) {
			return 0;
		}
		return -1;
	}
	if (!ts2) {
		return 1;
	}
	if (ts1->timestamp_year != ts2->timestamp_year) {
		return (ts1->timestamp_year > ts2->timestamp_year) ? 1 : -1;
	}
	if (ts1->timestamp_month != ts2->timestamp_month) {
		return (ts1->timestamp_month > ts2->timestamp_month) ? 1 : -1;
	}
	if (ts1->timestamp_day != ts2->timestamp_day) {
		return (ts1->timestamp_day > ts2->timestamp_day) ? 1 : -1;
	}
	if (ts1->timestamp_hour != ts2->timestamp_hour) {
		return (ts1->timestamp_hour > ts2->timestamp_hour) ? 1 : -1;
	}
	if (ts1->timestamp_minute != ts2->timestamp_minute) {
		return (ts1->timestamp_minute > ts2->timestamp_minute) 
			? 1
			: -1;
	}
	if (ts1->timestamp_second != ts2->timestamp_second) {
		return (ts1->timestamp_second > ts2->timestamp_second)
			? 1
			: -1;
	}
	return 0;
}

int
cmyth_timestamp_diff(cmyth_timestamp_t ts1, cmyth_timestamp_t ts2)
{
	struct tm tm_datetime;
	time_t start, end;

	memset(&tm_datetime, 0, sizeof(tm_datetime));
	tm_datetime.tm_year = ts1->timestamp_year - 1900;
	tm_datetime.tm_mon = ts1->timestamp_month - 1;
	tm_datetime.tm_mday = ts1->timestamp_day;
	tm_datetime.tm_hour = ts1->timestamp_hour;
	tm_datetime.tm_min = ts1->timestamp_minute;
	tm_datetime.tm_sec = ts1->timestamp_second;
	tm_datetime.tm_isdst = ts1->timestamp_isdst;
	start = mktime(&tm_datetime);

	memset(&tm_datetime, 0, sizeof(tm_datetime));
	tm_datetime.tm_year = ts2->timestamp_year - 1900;
	tm_datetime.tm_mon = ts2->timestamp_month - 1;
	tm_datetime.tm_mday = ts2->timestamp_day;
	tm_datetime.tm_hour = ts2->timestamp_hour;
	tm_datetime.tm_min = ts2->timestamp_minute;
	tm_datetime.tm_sec = ts2->timestamp_second;
	tm_datetime.tm_isdst = ts2->timestamp_isdst;
	end = mktime(&tm_datetime);

	return (int)(end - start);
}
