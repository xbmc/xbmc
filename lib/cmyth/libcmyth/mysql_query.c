/*
 *  Copyright (C) 2006-2009, Simon Hyde
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

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <cmyth_local.h>

#define CMYTH_ULONG_STRLEN ((sizeof(long)*3)+1)
#define CMYTH_LONG_STRLEN (CMYTH_ULONG_STRLEN+1)

/**
 * Hold in-progress query
 */
struct cmyth_mysql_query_s
{
    char * buf;
    const char * source;
    const char * source_pos;
    int buf_size, buf_used, source_len;
    cmyth_database_t db;
};


/**
 * Internal only! Registered as callback to de-allocate actual buffer if
 * the query is de-allocated
 * \param p pointer to the query data structure
 */
static void
query_destroy(void *p)
{
    cmyth_mysql_query_t * query = (cmyth_mysql_query_t *)p;
    if(query->buf != NULL)
    {
	ref_release(query->buf);
	query->buf = NULL;
	query->buf_size = 0;
    }
    if(query->db != NULL)
    {
	ref_release(query->db);
	query->db = NULL;
    }
}

/**
 * Allocate a dynamic query string to have parameters added to it
 * \param db database connection object
 * \param query_string Query string with ? placemarks for all dynamic
 * 			parameters, this is NOT copied and must therefore
 * 			remain valid for the life of the query.
 */
cmyth_mysql_query_t * 
cmyth_mysql_query_create(cmyth_database_t db, const char * query_string)
{
    cmyth_mysql_query_t * out;
    out = ref_alloc(sizeof(*out));
    if(out != NULL)
    {
	ref_set_destroy(out,query_destroy);
	out->source = out->source_pos = query_string;
	out->source_len = strlen(out->source);
	out->buf_size = out->source_len *2;
	out->buf_used = 0;
	out->db = ref_hold(db);
	out->buf = ref_alloc(out->buf_size);
	if(out->buf == NULL)
	{
	    ref_release(out);
	    out = NULL;
	}
	else
	{
	    out->buf[0] = '\0';
	}

    }
    return out;
}

void
cmyth_mysql_query_reset(cmyth_mysql_query_t *query)
{
    query->buf_used = 0;
    query->source_pos = query->source;
}

static int
query_buffer_check_len(cmyth_mysql_query_t *query, int len)
{
    if(len + query->buf_used >= query->buf_size)
    {
	/* Increase buffer size by len or out->source_len, whichever
	 * is bigger
	 */
	if(query->source_len > len) 
	    query->buf_size += query->source_len;
	else
	    query->buf_size += len;
	query->buf = ref_realloc(query->buf,query->buf_size);
	if(query->buf == NULL)
	{
	    cmyth_mysql_query_reset(query);
	    return -1;
	}
    }
    return 0;
}

static int
query_buffer_add(cmyth_mysql_query_t *query, const char *buf,int len)
{
    int ret = query_buffer_check_len(query,len);
    if(ret < 0)
	return ret;
    memcpy(query->buf + query->buf_used,buf,len);
    query->buf_used +=len;
    query->buf[query->buf_used] = '\0';
    return len;
}

static inline int
query_buffer_add_str(cmyth_mysql_query_t *query, const char *str)
{
    return query_buffer_add(query,str,strlen(str));
}

static int
query_buffer_add_escape_str(cmyth_mysql_query_t *query, const char *str)
{
    int ret;
    int srclen = strlen(str);
    MYSQL * mysql;
    unsigned long destlen;
    /*According to the mysql C API refrence, there must be sourcelen*2 +1
     * characters of space in the destination buffer
     */
    ret = query_buffer_check_len(query,srclen*2 +1);
    if(ret < 0)
	return ret;
    mysql = cmyth_db_get_connection(query->db);
    if(mysql == NULL)
	return -1;
    destlen = mysql_real_escape_string(mysql, query->buf + query->buf_used,
					str, srclen);
    query->buf_used += destlen;
    /* MySQL claims it null terminates, but do so anyway just in case we've
     * done something stupid
     */
    query->buf[query->buf_used] = '\0';
    return destlen;
}

static int
query_begin_next_param(cmyth_mysql_query_t *query)
{
    int len,ret;
    const char * endpos = strchr(query->source_pos,(int)'?');
    /*No more parameter insertion points left!*/
    if(endpos == NULL)
	return -1;
    len = endpos - query->source_pos;
    ret = query_buffer_add(query,query->source_pos,len);
    query->source_pos = endpos + 1;
    return ret;
}

static inline int
query_buffer_add_long(cmyth_mysql_query_t * query, long param)
{
    char buf[CMYTH_LONG_STRLEN];
    sprintf(buf,"%ld",param);
    return query_buffer_add_str(query,buf);
}

static inline int
query_buffer_add_ulong(cmyth_mysql_query_t * query, long param)
{
    char buf[CMYTH_ULONG_STRLEN];
    sprintf(buf,"%lu",param);
    return query_buffer_add_str(query,buf);
}

/**
 * Add a long integer parameter
 * \param query the query object
 * \param param the integer to add
 */
int
cmyth_mysql_query_param_long(cmyth_mysql_query_t * query,long param)
{
    int ret;
    ret = query_begin_next_param(query);
    if(ret < 0)
	return ret;
    return query_buffer_add_long(query,param);
}

/**
 * Add an unsigned long integer parameter
 * \param query the query object
 * \param param the integer to add
 */
int
cmyth_mysql_query_param_ulong(cmyth_mysql_query_t * query,unsigned long param)
{
    int ret;
    ret = query_begin_next_param(query);
    if(ret < 0)
	return ret;
    return query_buffer_add_ulong(query,param);
}

/**
 * Add an integer parameter
 * \param query the query object
 * \param param the integer to add
 */
int
cmyth_mysql_query_param_int(cmyth_mysql_query_t * query,int param)
{
    return cmyth_mysql_query_param_long(query,(long)param);
}

/**
 * Add an unsigned integer parameter
 * \param query the query object
 * \param param the integer to add
 */
int
cmyth_mysql_query_param_uint(cmyth_mysql_query_t * query,int param)
{
    return cmyth_mysql_query_param_ulong(query,(unsigned long)param);
}

/**
 * Add, and convert a unixtime to mysql date/timestamp
 * \param query the query object
 * \param param the time to add
 */
int
cmyth_mysql_query_param_unixtime(cmyth_mysql_query_t * query, time_t param)
{
    int ret;
    ret = query_begin_next_param(query);
    if(ret < 0)
	return ret;
    ret = query_buffer_add_str(query,"FROM_UNIXTIME(");
    if(ret < 0)
	return ret;
    ret = query_buffer_add_long(query,(long)param);
    if(ret < 0)
	return ret;
    return query_buffer_add_str(query,")");
}


/**
 * Add (including adding quotes), and escape a string parameter.
 * \param query the query object
 * \param param the string to add
 */
int
cmyth_mysql_query_param_str(cmyth_mysql_query_t * query, const char *param)
{
    int ret;
    ret = query_begin_next_param(query);
    if(ret < 0)
	return ret;
    if(param == NULL)
	return query_buffer_add_str(query,"NULL");
    ret = query_buffer_add_str(query,"'");
    if(ret < 0)
	return ret;
    ret = query_buffer_add_escape_str(query,param);
    if(ret < 0)
	return ret;
    return query_buffer_add_str(query,"'");
}

/**
 * Get the completed query string
 * \return If all fields haven't been filled, or there is some other failure
 * 	this will return NULL, otherwise a string is returned. The returned
 * 	string must be released by the caller using ref_release().
 */
char *
cmyth_mysql_query_string(cmyth_mysql_query_t * query)
{
    if(strchr(query->source_pos, (int)'?') != NULL)
    {
	return NULL;/*Still more parameters to be added*/
    }
    if(query_buffer_add_str(query,query->source_pos) < 0)
	return NULL;
    /*Point source_pos to the '\0' at the end of the string so this can
     * be called multiple times
     */
    query->source_pos = query->source + query->source_len;
    return ref_hold(query->buf);
}


MYSQL_RES *
cmyth_mysql_query_result(cmyth_mysql_query_t * query)
{
    MYSQL_RES * retval = NULL;
    int ret;
    char * query_str;
    MYSQL *mysql = cmyth_db_get_connection(query->db);
    if(mysql == NULL)
	return NULL;
    query_str = cmyth_mysql_query_string(query);
    if(query_str == NULL)
	return NULL;
    ret = mysql_query(mysql,query_str);
    ref_release(query_str);
    if(ret != 0)
    {
	 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query(%s) Failed: %s\n",
				__FUNCTION__, query_str, mysql_error(mysql));
	 return NULL;
    }
    retval = mysql_store_result(mysql);
    if(retval == NULL)
    {
	 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_use_result Failed: %s\n",
				__FUNCTION__, query_str, mysql_error(mysql));
    }
    return retval;
}

int
cmyth_mysql_query(cmyth_mysql_query_t * query)
{
	int ret;
	char * query_str;
	MYSQL *mysql = cmyth_db_get_connection(query->db);
	if(mysql == NULL)
		return -1;
	query_str = cmyth_mysql_query_string(query);
	if(query_str == NULL)
		return -1;
	ret = mysql_query(mysql,query_str);
	ref_release(query_str);
	if(ret != 0)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query(%s) Failed: %s\n",
				__FUNCTION__, query_str, mysql_error(mysql));
		return -1;
	}
	return 0;
}
