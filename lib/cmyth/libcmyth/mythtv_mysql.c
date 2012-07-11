/*
 *  Copyright (C) 2004-2012, Eric Lund
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mysql/mysql.h>
#include <cmyth_local.h>
#include <safe_string.h>

void
cmyth_database_close(cmyth_database_t db)
{
    if(db->mysql != NULL)
    {
	mysql_close(db->mysql);
	db->mysql = NULL;
    }
}

cmyth_database_t
cmyth_database_init(char *host, char *db_name, char *user, char *pass)
{
	cmyth_database_t rtrn = ref_alloc(sizeof(*rtrn));
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);

	if (rtrn != NULL) {
	    rtrn->db_host = ref_strdup(host);
	    rtrn->db_user = ref_strdup(user);
	    rtrn->db_pass = ref_strdup(pass);
	    rtrn->db_name = ref_strdup(db_name);
	}

	return rtrn;
}

int
cmyth_database_set_host(cmyth_database_t db, char *host)
{
	cmyth_database_close(db);
	ref_release(db->db_host);
	db->db_host = ref_strdup(host);
	if(! db->db_host)
	    return 0;
	else
	    return 1;
}

int
cmyth_database_set_user(cmyth_database_t db, char *user)
{
	cmyth_database_close(db);
	ref_release(db->db_user);
	db->db_user = ref_strdup(user);
	if(! db->db_user)
	    return 0;
	else
	    return 1;
}

int
cmyth_database_set_pass(cmyth_database_t db, char *pass)
{
	cmyth_database_close(db);
	ref_release(db->db_user);
	db->db_pass = ref_strdup(pass);
	if(! db->db_pass)
	    return 0;
	else
	    return 1;
}

int
cmyth_database_set_name(cmyth_database_t db, char *name)
{
	cmyth_database_close(db);
	ref_release(db->db_name);
	db->db_name = ref_strdup(name);
	if(! db->db_name)
	    return 0;
	else
	    return 1;
}


static int
cmyth_db_check_connection(cmyth_database_t db)
{
    if(db->mysql != NULL)
    {
	/* Fetch the mysql stats (uptime and stuff) to check the connection is
	 * still good
	 */
	if(mysql_stat(db->mysql) == NULL)
	{
	    cmyth_database_close(db);
	}
    }
    if(db->mysql == NULL)
    {
	db->mysql = mysql_init(NULL);
	if(db->mysql == NULL)
	{
	    fprintf(stderr,"%s: mysql_init() failed, insufficient memory?",
		    __FUNCTION__);
	    return -1;
	}
	if(NULL == mysql_real_connect(db->mysql,
		    db->db_host,db->db_user,db->db_pass,db->db_name,0,NULL,0))
	{
	    fprintf(stderr,"%s: mysql_connect() failed: %s", __FUNCTION__,
		    mysql_error(db->mysql));
	    cmyth_database_close(db);
	    return -1;
	}
    }
    return 0;
}

MYSQL *
cmyth_db_get_connection(cmyth_database_t db)
{
    if(cmyth_db_check_connection(db) != 0)
    {
       cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
       					__FUNCTION__);
       return NULL;
    }

    /*
     * mythbackend stores any multi-byte characters using utf8 encoding within latin1 database
     * columns. The MySQL connection needs to be told to use a utf8 character set when reading the
     * database columns or any multi-byte characters will be treated as 2 or 3 subsequent latin1
     * characters with nonsense values.
     *
     * http://www.mythtv.org/wiki/Fixing_Corrupt_Database_Encoding#Note_on_MythTV_0.21-fixes_and_below_character_encoding
     * http://dev.mysql.com/doc/refman/5.0/en/charset-connection.html
     */
    if(mysql_query(db->mysql,"SET NAMES utf8;")) {
      cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
      return NULL;
    }

    return db->mysql;
}

int 
cmyth_schedule_recording(cmyth_conn_t conn, char * msg)
{
	int err=0;
	int count;
	char buf[256];

	fprintf (stderr, "In function : %s\n",__FUNCTION__);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n", __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&mutex);

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
                        "%s: cmyth_send_message() failed (%d)\n",__FUNCTION__,err);
		return err;
	}

	count = cmyth_rcv_length(conn);
	cmyth_rcv_string(conn, &err, buf, sizeof(buf)-1,count);
	pthread_mutex_unlock(&mutex);
	return err;
}

char *
cmyth_mysql_escape_chars(cmyth_database_t db, char * string) 
{
	char *N_string;
	size_t len;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
                           __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return NULL;
	}

	len = strlen(string);
	N_string=ref_alloc(len*2+1);
	mysql_real_escape_string(db->mysql,N_string,string,len); 

	return (N_string);
}

int 
cmyth_get_offset_mysql(cmyth_database_t db, int type, char *recordid, int chanid, char *title, char *subtitle, char *description, char *seriesid, char *programid)
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	char query[1000];
	int count;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}
	if (type == 1) { // startoffset
		sprintf (query,"SELECT startoffset FROM record WHERE (recordid='%s' AND chanid=%d AND title='%s' AND subtitle='%s' AND description='%s' AND seriesid='%s' AND programid='%s')",recordid,chanid,title,subtitle,description,seriesid,programid);
	}
	else if (type == 0) { //endoffset
		sprintf (query,"SELECT endoffset FROM record WHERE (recordid='%s' AND chanid=%d AND title='%s' AND subtitle='%s' AND description='%s' AND seriesid='%s' AND programid='%s')",recordid,chanid,title,subtitle,description,seriesid,programid);
	}

	cmyth_dbg(CMYTH_DBG_ERROR, "%s : query=%s\n",__FUNCTION__, query);
	
        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
		return -1;
        }
        res = mysql_store_result(db->mysql);
	if ( (count = (int)mysql_num_rows(res)) >0) {
		row = mysql_fetch_row(res);
		fprintf(stderr, "row grabbed done count=%d\n",count);
        	mysql_free_result(res);
		return atoi(row[0]);
	}
	else {
        	mysql_free_result(res);
		return 0;
	}
}

char *
cmyth_get_recordid_mysql(cmyth_database_t db, int chanid, char *title, char *subtitle, char *description, char *seriesid, char *programid)
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	char query[1000];
	int count;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return NULL;
	}
	sprintf (query,"SELECT recordid FROM record WHERE (chanid=%d AND title='%s' AND subtitle='%s' AND description='%s' AND seriesid='%s' AND programid='%s')",chanid,title,subtitle,description,seriesid,programid);

	cmyth_dbg(CMYTH_DBG_ERROR, "%s : query=%s\n",__FUNCTION__, query);
	
        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", __FUNCTION__, mysql_error(db->mysql));
		return NULL;
        }
        res = mysql_store_result(db->mysql);
	if ( (count = (int)mysql_num_rows(res)) >0) {
		row = mysql_fetch_row(res);
		fprintf(stderr, "row grabbed done count=%d\n",count);
        	mysql_free_result(res);
		return row[0];
	}
	else {
        	mysql_free_result(res);
		return "NULL";
	}
}

int 
cmyth_mysql_delete_scheduled_recording(cmyth_database_t db, char * query)
{
	int rows=0;
	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
                           __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}
	cmyth_dbg(CMYTH_DBG_ERROR, "mysql query :%s\n",query);

        if(mysql_real_query(db->mysql,query,(unsigned int) strlen(query))) {
                cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                           __FUNCTION__, mysql_error(db->mysql));
		return -1;
	}
	rows=(int)mysql_affected_rows(db->mysql);

	if (rows <=0) {
        	cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                	__FUNCTION__, mysql_error(db->mysql));
	}

	return rows;
}

int
cmyth_mysql_insert_into_record(cmyth_database_t db, char * query, char * query1, char * query2, char *title, char * subtitle, char * description, char * callsign)
{
	int rows=0;
	char *N_title;
	char *N_subtitle;
	char *N_description;
	char *N_callsign;
	char N_query[2570];

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
                           __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}

	N_title = ref_alloc(strlen(title)*2+1);
	mysql_real_escape_string(db->mysql,N_title,title,strlen(title)); 
	N_subtitle = ref_alloc(strlen(subtitle)*2+1);
	mysql_real_escape_string(db->mysql,N_subtitle,subtitle,strlen(subtitle)); 
	N_description = ref_alloc(strlen(description)*2+1);
	mysql_real_escape_string(db->mysql,N_description,description,strlen(description)); 
	N_callsign = ref_alloc(strlen(callsign)*2+1);
	mysql_real_escape_string(db->mysql,N_callsign,callsign,strlen(callsign)); 

	snprintf(N_query,2500,"%s '%s','%s','%s' %s '%s' %s",query,N_title,N_subtitle,N_description,query1,N_callsign,query2); 
	ref_release(N_title);
	ref_release(N_subtitle);
	ref_release(N_callsign);
	cmyth_dbg(CMYTH_DBG_ERROR, "mysql query :%s\n",N_query);

        if(mysql_real_query(db->mysql,N_query,(unsigned int) strlen(N_query))) {
                cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                           __FUNCTION__, mysql_error(db->mysql));
		return -1;
	}
	rows=(int)mysql_insert_id(db->mysql);

	if (rows <=0) {
        	cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                	__FUNCTION__, mysql_error(db->mysql));
	}


	return rows;
}

int
cmyth_mysql_get_prev_recorded(cmyth_database_t db, cmyth_program_t **prog)
{
	MYSQL_RES *res= NULL;
	MYSQL_ROW row;
	int n=0;
	int rows=0;
        const char *query = "SELECT oldrecorded.chanid, UNIX_TIMESTAMP(starttime), UNIX_TIMESTAMP(endtime), title, subtitle, description, category, seriesid, programid, channel.channum, channel.callsign, channel.name, findid, rectype, recstatus, recordid, duplicate FROM oldrecorded LEFT JOIN channel ON oldrecorded.chanid = channel.chanid ORDER BY `starttime` ASC";
	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}
        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                           __FUNCTION__, mysql_error(db->mysql));
		return -1;
        }
        res = mysql_store_result(db->mysql);
	while((row = mysql_fetch_row(res))) {
        	if (rows >= n) {
                	n+=10;
                       	*prog=realloc(*prog,sizeof(**prog)*(n));
               	}
		(*prog)[rows].chanid = safe_atoi(row[0]);
               	(*prog)[rows].recording=0;
		(*prog)[rows].starttime=(time_t)safe_atol(row[1]);
		(*prog)[rows].endtime= (time_t)safe_atol(row[2]);
		sizeof_strncpy((*prog)[rows].title, row[3]);
		sizeof_strncpy((*prog)[rows].subtitle, row[4]);
		sizeof_strncpy((*prog)[rows].description, row[5]);
		sizeof_strncpy((*prog)[rows].category, row[6]);
		sizeof_strncpy((*prog)[rows].seriesid, row[7]);
		sizeof_strncpy((*prog)[rows].programid, row[8]);
		(*prog)[rows].channum = safe_atoi(row[9]);
		sizeof_strncpy((*prog)[rows].callsign, row[10]);
		sizeof_strncpy((*prog)[rows].name, row[11]);
		//sizeof_strncpy((*prog)[rows].rec_status, row[14]);
		(*prog)[rows].rec_status=safe_atoi(row[14]);
		//fprintf(stderr, "row=%s   val=%d\n",row[14],(*prog)[rows].rec_status);
          	rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_ERROR, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_guide(cmyth_database_t db, cmyth_program_t **prog, time_t starttime, time_t endtime) 
{
	MYSQL_RES *res= NULL;
	MYSQL_ROW row;
        const char *query_str = "SELECT program.chanid,UNIX_TIMESTAMP(program.starttime),UNIX_TIMESTAMP(program.endtime),program.title,program.description,program.subtitle,program.programid,program.seriesid,program.category,channel.channum,channel.callsign,channel.name,channel.sourceid FROM program INNER JOIN channel ON program.chanid=channel.chanid WHERE ( ( starttime>=? and starttime<? ) OR ( starttime <? and endtime > ?) ) ORDER BY (channel.channum + 0), program.starttime ASC ";
	int rows=0;
	int n=0;
	cmyth_mysql_query_t * query;
	query = cmyth_mysql_query_create(db,query_str);

	if(cmyth_mysql_query_param_unixtime(query,starttime) < 0
	    || cmyth_mysql_query_param_unixtime(query,endtime) < 0
	    || cmyth_mysql_query_param_unixtime(query,starttime) < 0
	    || cmyth_mysql_query_param_unixtime(query,starttime) < 0)
	{
	    cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
	    ref_release(query);
	    return -1;
 	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if(res == NULL)
	{
	    cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
	    return -1;
	}


	while((row = mysql_fetch_row(res))) {
        	if (rows >= n) {
                	n+=10;
                       	*prog=ref_realloc(*prog,sizeof(**prog)*(n));
               	}
		(*prog)[rows].chanid = safe_atoi(row[0]);
               	(*prog)[rows].recording=0;
		(*prog)[rows].starttime= (time_t)safe_atol(row[1]);
		(*prog)[rows].endtime= (time_t)safe_atol(row[2]);
		sizeof_strncpy((*prog)[rows].title, row[3]);
		sizeof_strncpy((*prog)[rows].description, row[4]);
		sizeof_strncpy((*prog)[rows].subtitle, row[5]);
		sizeof_strncpy((*prog)[rows].programid, row[6]);
		sizeof_strncpy((*prog)[rows].seriesid, row[7]);
		sizeof_strncpy((*prog)[rows].category, row[8]);
		(*prog)[rows].channum = safe_atoi(row[9]);
		sizeof_strncpy((*prog)[rows].callsign, row[10]);
		sizeof_strncpy((*prog)[rows].name, row[11]);
		(*prog)[rows].sourceid = safe_atoi(row[12]);
		(*prog)[rows].startoffset=0;
		(*prog)[rows].endoffset=0;
          	rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_ERROR, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int 
cmyth_mysql_get_recgroups(cmyth_database_t db, cmyth_recgroups_t **sqlrecgroups)
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
        const char *query="SELECT DISTINCT recgroup FROM record";
	int rows=0;
	int n=0;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
                           __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}

        cmyth_dbg(CMYTH_DBG_ERROR, "%s: query= %s\n", __FUNCTION__, query);
        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                           __FUNCTION__, mysql_error(db->mysql));
		return -1;
        }
        res = mysql_store_result(db->mysql);
        while((row = mysql_fetch_row(res))) {
        	if (rows == n ) {
                	n++;
                       	*sqlrecgroups=realloc(*sqlrecgroups,sizeof(**sqlrecgroups)*(n));
               	}
		sizeof_strncpy ( (*sqlrecgroups)[rows].recgroups, row[0]);
        	cmyth_dbg(CMYTH_DBG_ERROR, "(*sqlrecgroups)[%d].recgroups =  %s\n",rows, (*sqlrecgroups)[rows].recgroups);
		rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_ERROR, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}


int
cmyth_mysql_get_prog_finder_char_title(cmyth_database_t db, cmyth_program_t **prog, time_t starttime, char *program_name) 
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
        char query[350];
	int rows=0;
	int n = 50;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
                           __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}

        if (strncmp(program_name, "@", 1)==0)
                snprintf(query, 350, "SELECT DISTINCT title FROM program " \
                                "WHERE ( title NOT REGEXP '^[A-Z0-9]' AND " \
				"title NOT REGEXP '^The [A-Z0-9]' AND " \
				"title NOT REGEXP '^A [A-Z0-9]' AND " \
				"starttime >= FROM_UNIXTIME(%d)) ORDER BY title", \
			(int)starttime);
        else
	        snprintf(query, 350, "SELECT DISTINCT title FROM program " \
					"where starttime >= FROM_UNIXTIME(%d) and " \
					"title like '%s%%' ORDER BY `title` ASC",
			 (int)starttime, program_name);

	fprintf(stderr, "%s\n", query);
        cmyth_dbg(CMYTH_DBG_ERROR, "%s: query= %s\n", __FUNCTION__, query);
        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                           __FUNCTION__, mysql_error(db->mysql));
		return -1;
        }
        res = mysql_store_result(db->mysql);
        while((row = mysql_fetch_row(res))) {
        	if (rows == n) {
                	n++;
                       	*prog=realloc(*prog,sizeof(**prog)*(n));
               	}
		sizeof_strncpy ( (*prog)[rows].title, row[0]);
        	cmyth_dbg(CMYTH_DBG_ERROR, "prog[%d].title =  %s\n",rows, (*prog)[rows].title);
		rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_ERROR, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_mysql_get_prog_finder_time(cmyth_database_t db, cmyth_program_t **prog,  time_t starttime, char *program_name) 
{
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
        char query[630];
	char *N_title;
	int rows=0;
	int n = 50;
	int ch;


	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
                           __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}

	N_title = ref_alloc(strlen(program_name)*2+1);
	mysql_real_escape_string(db->mysql,N_title,program_name,strlen(program_name)); 

        //sprintf(query, "SELECT chanid,starttime,endtime,title,description,subtitle,programid,seriesid,category FROM program WHERE starttime >= '%s' and title ='%s' ORDER BY `starttime` ASC ", starttime, N_title);
        snprintf(query, 630, "SELECT program.chanid,UNIX_TIMESTAMP(program.starttime),UNIX_TIMESTAMP(program.endtime),program.title,program.description,program.subtitle,program.programid,program.seriesid,program.category, channel.channum, channel.callsign, channel.name, channel.sourceid FROM program LEFT JOIN channel on program.chanid=channel.chanid WHERE starttime >= FROM_UNIXTIME(%d) and title ='%s' ORDER BY `starttime` ASC ", (int)starttime, N_title);
	ref_release(N_title);
	fprintf(stderr, "%s\n", query);
        cmyth_dbg(CMYTH_DBG_ERROR, "%s: query= %s\n", __FUNCTION__, query);
        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                           __FUNCTION__, mysql_error(db->mysql));
		return -1;
       	}
	cmyth_dbg(CMYTH_DBG_ERROR, "n =  %d\n",n);
        res = mysql_store_result(db->mysql);
	cmyth_dbg(CMYTH_DBG_ERROR, "n =  %d\n",n);
	while((row = mysql_fetch_row(res))) {
			cmyth_dbg(CMYTH_DBG_ERROR, "n =  %d\n",n);
        	if (rows == n) {
                	n++;
			cmyth_dbg(CMYTH_DBG_ERROR, "realloc n =  %d\n",n);
                       	*prog=realloc(*prog,sizeof(**prog)*(n));
               	}
			cmyth_dbg(CMYTH_DBG_ERROR, "rows =  %d\nrow[0]=%d\n",rows, row[0]);
			cmyth_dbg(CMYTH_DBG_ERROR, "row[1]=%d\n",row[1]);
			ch = atoi(row[0]);
			(*prog)[rows].chanid=ch;
			cmyth_dbg(CMYTH_DBG_ERROR, "prog[%d].chanid =  %d\n",rows, (*prog)[rows].chanid);
			(*prog)[rows].recording=0;
			(*prog)[rows].starttime = atoi(row[1]);
			(*prog)[rows].endtime = atoi(row[2]);
			sizeof_strncpy ((*prog)[rows].title, row[3]);
			sizeof_strncpy ((*prog)[rows].description, row[4]);
			sizeof_strncpy ((*prog)[rows].subtitle, row[5]);
			sizeof_strncpy ((*prog)[rows].programid, row[6]);
			sizeof_strncpy ((*prog)[rows].seriesid, row[7]);
			sizeof_strncpy ((*prog)[rows].category, row[8]);
			(*prog)[rows].channum = atoi (row[9]);
			sizeof_strncpy ((*prog)[rows].callsign,row[10]);
			sizeof_strncpy ((*prog)[rows].name,row[11]);
			(*prog)[rows].sourceid = atoi (row[12]);
        		cmyth_dbg(CMYTH_DBG_ERROR, "prog[%d].chanid =  %d\n",rows, (*prog)[rows].chanid);
        		cmyth_dbg(CMYTH_DBG_ERROR, "prog[%d].title =  %s\n",rows, (*prog)[rows].title);
			rows++;
        }
        mysql_free_result(res);
        cmyth_dbg(CMYTH_DBG_ERROR, "%s: rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int 
fill_program_recording_status(cmyth_conn_t conn, char * msg)
{
	int err=0;
	fprintf (stderr, "In function : %s\n",__FUNCTION__);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n", __FUNCTION__);
		return -1;
	}
	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
                        "%s: cmyth_send_message() failed (%d)\n",__FUNCTION__,err);
		return err;
	}
	return err;
}
int
cmyth_update_bookmark_setting(cmyth_database_t db, cmyth_proginfo_t prog)
{
	MYSQL_RES *res = NULL;
	const char *query_str = "UPDATE recorded SET bookmark = 1 WHERE chanid = ? AND starttime = ?";
	cmyth_mysql_query_t * query;
	char starttime[CMYTH_TIMESTAMP_LEN + 1];

	cmyth_timestamp_to_string(starttime, prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_long(query, prog->proginfo_chanId) < 0
		|| cmyth_mysql_query_param_str(query, starttime) < 0 ) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	mysql_free_result(res);
	return (1);
}

long long 
cmyth_get_bookmark_mark(cmyth_database_t db, cmyth_proginfo_t prog, long long bk, int mode)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT mark, type FROM recordedseek WHERE chanid = ? AND offset < ? AND (type = 6 or type = 9 ) AND starttime = ? ORDER by MARK DESC LIMIT 0, 1;";
	int rows = 0;
	long long mark=0;
	int rectype = 0;
	char start_ts_dt[CMYTH_TIMESTAMP_LEN + 1];
	cmyth_mysql_query_t * query;
	cmyth_timestamp_to_string(start_ts_dt, prog->proginfo_rec_start_ts);
	query = cmyth_mysql_query_create(db,query_str);

	if (cmyth_mysql_query_param_long(query, prog->proginfo_chanId) < 0
		|| cmyth_mysql_query_param_long(query, bk) < 0
		|| cmyth_mysql_query_param_str(query, start_ts_dt) < 0
		) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	while ((row = mysql_fetch_row(res))) {
		mark = safe_atoi(row[0]);
		rectype = safe_atoi(row[1]);
		rows++;
	}
	mysql_free_result(res);

	if (rectype == 6) {
		if (mode == 0) {
			mark=(mark-1)*15;
		}
		else if (mode == 1) {
			mark=(mark-1)*12;
		}
	}

	return mark;
}

int 
cmyth_get_bookmark_offset(cmyth_database_t db, long chanid, long long mark, char *starttime, int mode) 
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int offset=0;
	int rows = 0;
	int rectype = 0;
	cmyth_mysql_query_t * query;
	
	const char *query_str = "SELECT * FROM recordedseek WHERE chanid = ? AND mark<= ? AND starttime = ? ORDER BY MARK DESC LIMIT 1;";

	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_long(query, chanid) < 0
		|| cmyth_mysql_query_param_long(query, mark) < 0
		|| cmyth_mysql_query_param_str(query, starttime) < 0
		) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	while ((row = mysql_fetch_row(res))) {
		offset = safe_atoi(row[3]);
		rectype = safe_atoi(row[4]);
		rows++;
	}
	if (rectype != 9) {
		if (mode == 0) {
			mark=(mark/15)+1;
		}
		else if (mode == 1) {
			mark=(mark/12)+1;
		}
		query = cmyth_mysql_query_create(db, query_str);
		if (cmyth_mysql_query_param_long(query, chanid) < 0
			|| cmyth_mysql_query_param_long(query, mark) < 0
			|| cmyth_mysql_query_param_str(query, starttime) < 0
			) {
			cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
			ref_release(query);
			return -1;
		}
		res = cmyth_mysql_query_result(query);
		ref_release(query);
		if (res == NULL) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
			return -1;
		}
		while ((row = mysql_fetch_row(res))) {
			offset = safe_atoi(row[3]);
			rows++;
		}
	}
	mysql_free_result(res);
	return offset;
}

int
cmyth_mysql_query_commbreak_count(cmyth_database_t db, int chanid, char * start_ts_dt) {
	MYSQL_RES *res = NULL;
	int count = 0;
	char * query_str;
  cmyth_mysql_query_t * query;
	query_str = "SELECT * FROM recordedmarkup WHERE chanid = ? AND starttime = ? AND TYPE IN ( 4 )";

	query = cmyth_mysql_query_create(db,query_str);
	if ((cmyth_mysql_query_param_int(query, chanid) < 0
		|| cmyth_mysql_query_param_str(query, start_ts_dt) < 0
		)) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}
	count = (int)mysql_num_rows(res);
	mysql_free_result(res);
	return (count);
} 

int
cmyth_mysql_get_commbreak_list(cmyth_database_t db, int chanid, char * start_ts_dt, cmyth_commbreaklist_t breaklist, int conn_version) 
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	int resolution = 30;
	char * query_str;
  int rows = 0;
	int i;
  cmyth_mysql_query_t * query;
	cmyth_commbreak_t commbreak = NULL;
  long long start_previous = 0;
	long long end_previous = 0;

	if (conn_version >= 43) {
		query_str = "SELECT m.type,m.mark,s.mark,s.offset  FROM recordedmarkup m INNER JOIN recordedseek AS s ON m.chanid = s.chanid AND m.starttime = s.starttime  WHERE m.chanid = ? AND m.starttime = ? AND m.type in (?,?) and FLOOR(m.mark/?)=FLOOR(s.mark/?) ORDER BY `m`.`mark` LIMIT 300 ";
	}
	else { 
		query_str = "SELECT m.type AS type, m.mark AS mark, s.offset AS offset FROM recordedmarkup m INNER JOIN recordedseek AS s ON (m.chanid = s.chanid AND m.starttime = s.starttime AND (FLOOR(m.mark / 15) + 1) = s.mark) WHERE m.chanid = ? AND m.starttime = ? AND m.type IN (?, ?) ORDER BY mark;";
	}

	cmyth_dbg(CMYTH_DBG_ERROR,"%s, query=%s\n", __FUNCTION__,query_str);

	query = cmyth_mysql_query_create(db,query_str);

	if ((conn_version >= 43) && ( 
		cmyth_mysql_query_param_int(query, chanid) < 0
		|| cmyth_mysql_query_param_str(query, start_ts_dt) < 0
		|| cmyth_mysql_query_param_int(query, CMYTH_COMMBREAK_START) < 0
		|| cmyth_mysql_query_param_int(query, CMYTH_COMMBREAK_END) < 0
		|| cmyth_mysql_query_param_int(query, resolution) < 0
		|| cmyth_mysql_query_param_int(query, resolution) < 0
		)) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}

	if ((conn_version < 43) && (
		cmyth_mysql_query_param_int(query, chanid) < 0
		|| cmyth_mysql_query_param_str(query, start_ts_dt) < 0
		|| cmyth_mysql_query_param_int(query, CMYTH_COMMBREAK_START) < 0
		|| cmyth_mysql_query_param_int(query, CMYTH_COMMBREAK_END) < 0
		)) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return -1;
	}

	if (conn_version >= 43) {
		breaklist->commbreak_count = cmyth_mysql_query_commbreak_count(db,chanid,start_ts_dt);
	}
	else {
		breaklist->commbreak_count = (long)mysql_num_rows(res) / 2;
	}
	breaklist->commbreak_list = malloc(breaklist->commbreak_count * sizeof(cmyth_commbreak_t));

	if (!breaklist->commbreak_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			__FUNCTION__);
		return -1;
	}
	memset(breaklist->commbreak_list, 0, breaklist->commbreak_count * sizeof(cmyth_commbreak_t));

	i = 0;
	if (conn_version >= 43) {
		while ((row = mysql_fetch_row(res))) {
			if (safe_atoi(row[0]) == CMYTH_COMMBREAK_START) {
				if (safe_atoll(row[1]) != start_previous) {
					commbreak = cmyth_commbreak_create();
					commbreak->start_mark = safe_atoll(row[1]);
					commbreak->start_offset = safe_atoll(row[3]);
					start_previous = commbreak->start_mark;
				}
				else if (safe_atoll(row[1]) == safe_atoll(row[2])) { 
					commbreak = cmyth_commbreak_create();
					commbreak->start_mark = safe_atoll(row[1]);
					commbreak->start_offset = safe_atoll(row[3]);
				}
			} else if (safe_atoi(row[0]) == CMYTH_COMMBREAK_END) {
				if (safe_atoll(row[1]) != end_previous) {
					commbreak->end_mark = safe_atoll(row[1]);
					commbreak->end_offset = safe_atoll(row[3]);
					breaklist->commbreak_list[rows] = commbreak;
					end_previous = commbreak->end_mark;
					rows++;
				}
				else if (safe_atoll(row[1]) == safe_atoll(row[2])) {
					commbreak->end_mark = safe_atoll(row[1]);
					commbreak->end_offset = safe_atoll(row[3]);
					breaklist->commbreak_list[rows] = commbreak;
					if (end_previous != safe_atoll(row[1])) {
						rows++;
					}
				}
			}
			else {
				cmyth_dbg(CMYTH_DBG_ERROR, "%s: Unknown COMMBREAK returned\n", 
					__FUNCTION__);
				return -1;
			}
			i++;
		}
	}

	// mythtv protolcol version < 43 
	else {
		while ((row = mysql_fetch_row(res))) {
			if ((i % 2) == 0) {
				if (safe_atoi(row[0]) != CMYTH_COMMBREAK_START) {
					return -1;
				}
				commbreak = cmyth_commbreak_create();
				commbreak->start_mark = safe_atoll(row[1]);
				commbreak->start_offset = safe_atoll(row[2]);
				i++;
			} else {
				if (safe_atoi(row[0]) != CMYTH_COMMBREAK_END) {
					return -1;
				}
				commbreak->end_mark = safe_atoll(row[1]);
				commbreak->end_offset = safe_atoll(row[2]);
				breaklist->commbreak_list[rows] = commbreak;
				i = 0;
				rows++;
			}
		}
	}
	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_ERROR, "%s: COMMBREAK rows= %d\n", __FUNCTION__, rows);
	return rows;
}

int
cmyth_tuner_type_check(cmyth_database_t db, cmyth_recorder_t rec, int check_tuner_type) {
	MYSQL_RES *res=NULL;
	MYSQL_ROW row;
	const char * query_str = "SELECT cardtype from capturecard WHERE cardid=?";
	cmyth_mysql_query_t * query;

	if ( check_tuner_type == 0 ) {
		cmyth_dbg(CMYTH_DBG_ERROR,"MythTV Tuner check not enabled in Mythtv Options\n");
		return (1);
	}
	

	query = cmyth_mysql_query_create(db,query_str);
	if (cmyth_mysql_query_param_uint(query,rec->rec_id) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query failed\n",__FUNCTION__);
		ref_release(query);
		return -1;
	}
	res = cmyth_mysql_query_result(query);

	if(res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution\n",__FUNCTION__);
		return -1;
	}
	row = mysql_fetch_row(res);
	ref_release(query);
	mysql_free_result(res);
	if (strcmp(row[0],"MPEG") == 0) {
		return (1); //return the first available MPEG tuner
	}
	else if (strcmp(row[0],"HDHOMERUN") == 0) {
		return (1); //return the first available MPEG2TS tuner
	}
	else if (strcmp(row[0],"DVB") == 0) {
		return (1); //return the first available DVB tuner
	}
	else {
		return (0);
	}
}

int
cmyth_mythtv_remove_previous_recorded(cmyth_database_t db,char *query)
{
	MYSQL_RES *res=NULL;
	char N_query[128];
	int rows;

	if(cmyth_db_check_connection(db) != 0)
	{
               cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n",
                           __FUNCTION__);
               fprintf(stderr,"%s: cmyth_db_check_connection failed\n", __FUNCTION__);
	       return -1;
	}

	mysql_real_escape_string(db->mysql,N_query,query,strlen(query)); 

        if(mysql_query(db->mysql,query)) {
                 cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
                           __FUNCTION__, mysql_error(db->mysql));
		return -1;
       	}
	res = mysql_store_result(db->mysql);
	rows=(int)mysql_insert_id(db->mysql);
	if (rows <=0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: mysql_query() Failed: %s\n", 
			__FUNCTION__, mysql_error(db->mysql));
	}
	mysql_free_result(res);

	return rows;
}

int
cmyth_mysql_testdb_connection(cmyth_database_t db,char **message) {
	char *buf=malloc(sizeof(char)*1001);
	int new_conn = 0;
	if (db->mysql != NULL) {
		if (mysql_stat(db->mysql) == NULL) {
			cmyth_database_close(db);
			return -1;
			}
	}
	if (db->mysql == NULL) {
		db->mysql = mysql_init(NULL);
		new_conn = 1;
		if(db->mysql == NULL) {
			fprintf(stderr,"%s: mysql_init() failed, insufficient memory?", __FUNCTION__);
			snprintf(buf, 1000, "mysql_init() failed, insufficient memory?");
			*message=buf;
			return -1;
		}
		if (NULL == mysql_real_connect(db->mysql, db->db_host,db->db_user,db->db_pass,db->db_name,0,NULL,0)) {
			fprintf(stderr,"%s: mysql_connect() failed: %s\n", __FUNCTION__,
			mysql_error(db->mysql));
			snprintf(buf, 1000, "%s",mysql_error(db->mysql));
			fprintf (stderr,"buf = %s\n",buf);
			*message=buf;
			cmyth_database_close(db);
			return -1;
		}
	}
	snprintf(buf, 1000, "All Test Successful\n");
	*message=buf;
	return 1;
}

static void
cmyth_chanlist_destroy(cmyth_chanlist_t pl)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!pl) {
		return;
	}
	for (i  = 0; i < pl->chanlist_count; ++i) {
		if (pl->chanlist_list[i]) {
			ref_release(pl->chanlist_list[i]);
		}
		pl->chanlist_list[i] = NULL;
	}
	if (pl->chanlist_list) {
		free(pl->chanlist_list);
	}
}

cmyth_chanlist_t
cmyth_chanlist_create(void)
{
	cmyth_chanlist_t ret;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	ret = ref_alloc(sizeof(*ret));
	if (!ret) {
		return(NULL);
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_chanlist_destroy);

	ret->chanlist_list = NULL;
	ret->chanlist_count = 0;
	return ret;
}

cmyth_channel_t
cmyth_chanlist_get_item(cmyth_chanlist_t pl, int index)
{
	if (!pl) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program list\n",
			  __FUNCTION__);
		return NULL;
	}
	if (!pl->chanlist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL list\n",
			  __FUNCTION__);
		return NULL;
	}
	if ((index < 0) || (index >= pl->chanlist_count)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: index %d out of range\n",
			  __FUNCTION__, index);
		return NULL;
	}
	ref_hold(pl->chanlist_list[index]);
	return pl->chanlist_list[index];
}

int
cmyth_chanlist_get_count(cmyth_chanlist_t pl)
{
	if (!pl) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program list\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	return pl->chanlist_count;
}

static void
cmyth_channel_destroy(cmyth_channel_t pl)
{

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!pl) {
		return;
	}

	if(pl->name)
		ref_release(pl->name);
	if(pl->callsign)
		ref_release(pl->callsign);
	if(pl->icon)
		ref_release(pl->callsign);
}

long
cmyth_channel_chanid(cmyth_channel_t channel)
{
	if (!channel) {
		return -EINVAL;
	}
	return channel->chanid;
}

long
cmyth_channel_channum(cmyth_channel_t channel)
{
	if (!channel) {
		return -EINVAL;
	}
	return channel->channum;
}

char *
cmyth_channel_name(cmyth_channel_t channel)
{
	if (!channel) {
		return NULL;
	}
	return ref_hold(channel->name);
}

char *
cmyth_channel_icon(cmyth_channel_t channel)
{
	if (!channel) {
		return NULL;
	}
	return ref_hold(channel->icon);
}

int
cmyth_channel_visible(cmyth_channel_t channel)
{
	if (!channel) {
		return -EINVAL;
	}
	return channel->visible;
}

cmyth_channel_t
cmyth_channel_create(void)
{
	cmyth_channel_t ret = ref_alloc(sizeof(*ret));
  memset(ret, 0, sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_channel_destroy);

	return ret;
}


cmyth_chanlist_t cmyth_mysql_get_chanlist(cmyth_database_t db)
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	const char *query_str = "SELECT chanid, channum, name, icon, visible FROM channel;";
	int rows = 0;
	int i;
	cmyth_mysql_query_t * query;
	cmyth_channel_t channel;
	cmyth_chanlist_t chanlist;
	query = cmyth_mysql_query_create(db,query_str);

	res = cmyth_mysql_query_result(query);
	ref_release(query);
	if (res == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s, finalisation/execution of query failed!\n", __FUNCTION__);
		return NULL;
	}

	chanlist = cmyth_chanlist_create();

	chanlist->chanlist_count = (int)mysql_num_rows(res);
	chanlist->chanlist_list = malloc(chanlist->chanlist_count * sizeof(cmyth_chanlist_t));
	if (!chanlist->chanlist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: malloc() failed for list\n",
			__FUNCTION__);
                ref_release(chanlist);
		return NULL;
	}
	memset(chanlist->chanlist_list, 0, chanlist->chanlist_count * sizeof(cmyth_chanlist_t));

	i = 0;
	while ((row = mysql_fetch_row(res))) {
		channel = cmyth_channel_create();
		channel->chanid = safe_atol(row[0]);
		channel->channum = safe_atoi(row[1]);
		channel->name = ref_strdup(row[2]);
		channel->icon = ref_strdup(row[3]);
		channel->visible = safe_atoi(row[4]);
		chanlist->chanlist_list[rows] = channel;
		i = 0;
		rows++;
	}

	mysql_free_result(res);
	cmyth_dbg(CMYTH_DBG_ERROR, "%s: rows= %d\n", __FUNCTION__, rows);
	return chanlist;
}


int cmyth_livetv_keep_recording(cmyth_recorder_t rec, cmyth_database_t db, int keep)
{
	cmyth_proginfo_t prog;
	int autoexpire;
	const char* recgroup;
	cmyth_mysql_query_t * query;
	char timestamp[CMYTH_TIMESTAMP_LEN+1];

	if(cmyth_db_check_connection(db) != 0)
	{
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_db_check_connection failed\n", __FUNCTION__);
		return -1;
	}

	prog = cmyth_recorder_get_cur_proginfo(rec);
	if(!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_recorder_get_cur_proginfo failed\n", __FUNCTION__);
		return -1;
	}

	if(keep) {
		char* str;
		str = cmyth_conn_get_setting(rec->rec_conn, prog->proginfo_hostname, "AutoExpireDefault");
		if(!str) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: failed to get AutoExpireDefault\n", __FUNCTION__);
			ref_release(prog);
			return -1;
		}
		autoexpire = atol(str);
		recgroup = "Default";
		ref_release(str);
	} else {
		autoexpire = 10000;
		recgroup = "LiveTV";
	}


	sprintf(timestamp,
		"%4.4ld-%2.2ld-%2.2ld %2.2ld:%2.2ld:%2.2ld",
		prog->proginfo_rec_start_ts->timestamp_year,
		prog->proginfo_rec_start_ts->timestamp_month,
		prog->proginfo_rec_start_ts->timestamp_day,
		prog->proginfo_rec_start_ts->timestamp_hour,
		prog->proginfo_rec_start_ts->timestamp_minute,
		prog->proginfo_rec_start_ts->timestamp_second);

	query = cmyth_mysql_query_create(db,"UPDATE recorded SET autoexpire = ?, recgroup = ? WHERE chanid = ? AND starttime = ?");

	if(cmyth_mysql_query_param_long(query,autoexpire) < 0
	 || cmyth_mysql_query_param_str(query,recgroup) < 0
	 || cmyth_mysql_query_param_long(query,prog->proginfo_chanId) < 0
	 || cmyth_mysql_query_param_str(query,timestamp) < 0) 
	{
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, binding of query parameters failed! Maybe we're out of memory?\n", __FUNCTION__);
		ref_release(query);
		ref_release(prog);
		return -1;
	}

	if(cmyth_mysql_query(query) < 0)
	{
		cmyth_dbg(CMYTH_DBG_ERROR,"%s, finalisation/execution of query failed!\n", __FUNCTION__);
		ref_release(query);
		ref_release(prog);
		return -1;
	}
	ref_release(query);

	if(rec->rec_conn->conn_version >= 26)
	{
		char msg[256];
		int err;
		snprintf(msg, sizeof(msg), "QUERY_RECORDER %d[]:[]SET_LIVE_RECORDING[]:[]%d",
		 	rec->rec_id, keep);

		if ((err=cmyth_send_message(rec->rec_conn, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
			    "%s: cmyth_send_message() failed (%d)\n",
			    __FUNCTION__, err);
			return -1;
		}

		if ((err=cmyth_rcv_okay(rec->rec_conn, "ok")) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
			    "%s: cmyth_rcv_okay() failed (%d)\n",
			    __FUNCTION__, err);
			return -1;
		}
	}
	return 1;
}

