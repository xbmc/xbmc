/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 
   Copyright (C) Rafal Szczesniak    2002

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
 

#include "includes.h"
#include "net.h"

/**
 * @file net_cache.c
 * @brief This is part of the net tool which is basically command
 *        line wrapper for gencache.c functions (mainly for testing)
 *
 **/


/*
 * These routines are used via gencache_iterate() to display the cache's contents
 * (print_cache_entry) and to flush it (delete_cache_entry).
 * Both of them are defined by first arg of gencache_iterate() routine.
 */
static void print_cache_entry(const char* keystr, const char* datastr,
                              const time_t timeout, void* dptr)
{
	char *timeout_str;
	char *alloc_str = NULL;
	time_t now_t = time(NULL);
	struct tm timeout_tm, *now_tm;
	/* localtime returns statically allocated pointer, so timeout_tm
	   has to be copied somewhere else */

	now_tm = localtime(&timeout);
	if (!now_tm) {
		return;
	}
	memcpy(&timeout_tm, now_tm, sizeof(struct tm));
	now_tm = localtime(&now_t);
	if (!now_tm) {
		return;
	}

	/* form up timeout string depending whether it's today's date or not */
	if (timeout_tm.tm_year != now_tm->tm_year ||
			timeout_tm.tm_mon != now_tm->tm_mon ||
			timeout_tm.tm_mday != now_tm->tm_mday) {
	    
		timeout_str = asctime(&timeout_tm);
		if (!timeout_str) {
			return;
		}	
		timeout_str[strlen(timeout_str) - 1] = '\0';	/* remove tailing CR */
	} else {
		asprintf(&alloc_str, "%.2d:%.2d:%.2d", timeout_tm.tm_hour,
		         timeout_tm.tm_min, timeout_tm.tm_sec);
		if (!alloc_str) {
			return;
		}
		timeout_str = alloc_str;
	}
	
	d_printf("Key: %s\t Timeout: %s\t Value: %s  %s\n", keystr,
	         timeout_str, datastr, timeout > now_t ? "": "(expired)");

	SAFE_FREE(alloc_str);
}

static void delete_cache_entry(const char* keystr, const char* datastr,
                               const time_t timeout, void* dptr)
{
	if (!gencache_del(keystr))
		d_fprintf(stderr, "Couldn't delete entry! key = %s\n", keystr);
}


/**
 * Parse text representation of timeout value
 *
 * @param timeout_str string containing text representation of the timeout
 * @return numeric timeout of time_t type
 **/
static time_t parse_timeout(const char* timeout_str)
{
	char sign = '\0', *number = NULL, unit = '\0';
	int len, number_begin, number_end;
	time_t timeout;

	/* sign detection */
	if (timeout_str[0] == '!' || timeout_str[0] == '+') {
		sign = timeout_str[0];
		number_begin = 1;
	} else {
		number_begin = 0;
	}
	
	/* unit detection */
	len = strlen(timeout_str);
	switch (timeout_str[len - 1]) {
	case 's':
	case 'm':
	case 'h':
	case 'd':
	case 'w': unit = timeout_str[len - 1];
	}
	
	/* number detection */
	len = (sign) ? strlen(&timeout_str[number_begin]) : len;
	number_end = (unit) ? len - 1 : len;
	number = SMB_STRNDUP(&timeout_str[number_begin], number_end);
	
	/* calculate actual timeout value */
	timeout = (time_t)atoi(number);

	switch (unit) {
	case 'm': timeout *= 60; break;
	case 'h': timeout *= 60*60; break;
	case 'd': timeout *= 60*60*24; break;
	case 'w': timeout *= 60*60*24*7; break;  /* that's fair enough, I think :) */
	}
	
	switch (sign) {
	case '!': timeout = time(NULL) - timeout; break;
	case '+':
	default:  timeout += time(NULL); break;
	}
	
	if (number) SAFE_FREE(number);
	return timeout;	
}


/**
 * Add an entry to the cache. If it does exist, then set it.
 * 
 * @param argv key, value and timeout are passed in command line
 * @return 0 on success, otherwise failure
 **/
static int net_cache_add(int argc, const char **argv)
{
	const char *keystr, *datastr, *timeout_str;
	time_t timeout;
	
	if (argc < 3) {
		d_printf("\nUsage: net cache add <key string> <data string> <timeout>\n");
		return -1;
	}
	
	keystr = argv[0];
	datastr = argv[1];
	timeout_str = argv[2];
	
	/* parse timeout given in command line */
	timeout = parse_timeout(timeout_str);
	if (!timeout) {
		d_fprintf(stderr, "Invalid timeout argument.\n");
		return -1;
	}
	
	if (gencache_set(keystr, datastr, timeout)) {
		d_printf("New cache entry stored successfully.\n");
		gencache_shutdown();
		return 0;
	}
	
	d_fprintf(stderr, "Entry couldn't be added. Perhaps there's already such a key.\n");
	gencache_shutdown();
	return -1;
}


/**
 * Set new value of an existing entry in the cache. Fail If the entry doesn't
 * exist.
 * 
 * @param argv key being searched and new value and timeout to set in the entry
 * @return 0 on success, otherwise failure
 **/
static int net_cache_set(int argc, const char **argv)
{
	const char *keystr, *datastr, *timeout_str;
	time_t timeout;
	
	if (argc < 3) {
		d_printf("\nUsage: net cache set <key string> <data string> <timeout>\n");
		return -1;
	}
	
	keystr = argv[0];
	datastr = argv[1];
	timeout_str = argv[2];
	
	/* parse timeout given in command line */
	timeout = parse_timeout(timeout_str);
	if (!timeout) {
		d_fprintf(stderr, "Invalid timeout argument.\n");
		return -1;
	}
	
	if (gencache_set_only(keystr, datastr, timeout)) {
		d_printf("Cache entry set successfully.\n");
		gencache_shutdown();
		return 0;
	}

	d_fprintf(stderr, "Entry couldn't be set. Perhaps there's no such a key.\n");
	gencache_shutdown();
	return -1;
}


/**
 * Delete an entry in the cache
 * 
 * @param argv key to delete an entry of
 * @return 0 on success, otherwise failure
 **/
static int net_cache_del(int argc, const char **argv)
{
	const char *keystr = argv[0];
	
	if (argc < 1) {
		d_printf("\nUsage: net cache del <key string>\n");
		return -1;
	}
	
	if(gencache_del(keystr)) {
		d_printf("Entry deleted.\n");
		return 0;
	}

	d_fprintf(stderr, "Couldn't delete specified entry\n");
	return -1;
}


/**
 * Get and display an entry from the cache
 * 
 * @param argv key to search an entry of
 * @return 0 on success, otherwise failure
 **/
static int net_cache_get(int argc, const char **argv)
{
	const char* keystr = argv[0];
	char* valuestr;
	time_t timeout;

	if (argc < 1) {
		d_printf("\nUsage: net cache get <key>\n");
		return -1;
	}
	
	if (gencache_get(keystr, &valuestr, &timeout)) {
		print_cache_entry(keystr, valuestr, timeout, NULL);
		return 0;
	}

	d_fprintf(stderr, "Failed to find entry\n");
	return -1;
}


/**
 * Search an entry/entries in the cache
 * 
 * @param argv key pattern to match the entries to
 * @return 0 on success, otherwise failure
 **/
static int net_cache_search(int argc, const char **argv)
{
	const char* pattern;
	
	if (argc < 1) {
		d_printf("Usage: net cache search <pattern>\n");
		return -1;
	}
	
	pattern = argv[0];
	gencache_iterate(print_cache_entry, NULL, pattern);
	return 0;
}


/**
 * List the contents of the cache
 * 
 * @param argv ignored in this functionailty
 * @return always returns 0
 **/
static int net_cache_list(int argc, const char **argv)
{
	const char* pattern = "*";
	gencache_iterate(print_cache_entry, NULL, pattern);
	gencache_shutdown();
	return 0;
}


/**
 * Flush the whole cache
 * 
 * @param argv ignored in this functionality
 * @return always returns 0
 **/
static int net_cache_flush(int argc, const char **argv)
{
	const char* pattern = "*";
	gencache_iterate(delete_cache_entry, NULL, pattern);
	gencache_shutdown();
	return 0;
}


/**
 * Short help
 *
 * @param argv ignored in this functionality
 * @return always returns -1
 **/
static int net_cache_usage(int argc, const char **argv)
{
	d_printf("  net cache add \t add add new cache entry\n");
	d_printf("  net cache set \t set new value for existing cache entry\n");
	d_printf("  net cache del \t delete existing cache entry by key\n");
	d_printf("  net cache flush \t delete all entries existing in the cache\n");
	d_printf("  net cache get \t get cache entry by key\n");
	d_printf("  net cache search \t search for entries in the cache, by given pattern\n");
	d_printf("  net cache list \t list all cache entries (just like search for \"*\")\n");
	return -1;
}


/**
 * Entry point to 'net cache' subfunctionality
 *
 * @param argv arguments passed to further called functions
 * @return whatever further functions return
 **/
int net_cache(int argc, const char **argv)
{
	struct functable func[] = {
		{"add", net_cache_add},
		{"set", net_cache_set},
		{"del", net_cache_del},
		{"get", net_cache_get},
		{"search", net_cache_search},
		{"list", net_cache_list},
		{"flush", net_cache_flush},
		{NULL, NULL}
	};

	return net_run_function(argc, argv, func, net_cache_usage);
}
