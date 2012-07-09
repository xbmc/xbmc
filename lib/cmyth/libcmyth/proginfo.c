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
 * proginfo.c - functions to manage MythTV program info.  This is
 *              information kept by MythTV to describe recordings and
 *              also to describe programs in the program guide.  The
 *              functions here allocate and fill out program
 *              information and lists of program information.  They
 *              also retrieve and manipulate recordings and program
 *              material based on program information.
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <cmyth_local.h>

/*
 * cmyth_proginfo_destroy(cmyth_proginfo_t p)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Destroy the program info structure pointed to by 'p' and release
 * its storage.  This should only be called by
 * ref_release(). All others should use
 * ref_release() to release references to a program info
 * structure.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_proginfo_destroy(cmyth_proginfo_t p)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!p) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!a\n", __FUNCTION__);
		return;
	}
	if (p->proginfo_title) {
		ref_release(p->proginfo_title);
	}
	if (p->proginfo_subtitle) {
		ref_release(p->proginfo_subtitle);
	}
	if (p->proginfo_description) {
		ref_release(p->proginfo_description);
	}
	if (p->proginfo_category) {
		ref_release(p->proginfo_category);
	}
	if (p->proginfo_chanstr) {
		ref_release(p->proginfo_chanstr);
	}
	if (p->proginfo_chansign) {
		ref_release(p->proginfo_chansign);
	}
	if (p->proginfo_channame) {
		ref_release(p->proginfo_channame);
	}
	if (p->proginfo_chanicon) {
		ref_release(p->proginfo_chanicon);
	}
	if (p->proginfo_url) {
		ref_release(p->proginfo_url);
	}
	if (p->proginfo_unknown_0) {
		ref_release(p->proginfo_unknown_0);
	}
	if (p->proginfo_hostname) {
		ref_release(p->proginfo_hostname);
	}
	if (p->proginfo_rec_priority) {
		ref_release(p->proginfo_rec_priority);
	}
	if (p->proginfo_rec_profile) {
		ref_release(p->proginfo_rec_profile);
	}
	if (p->proginfo_recgroup) {
		ref_release(p->proginfo_recgroup);
	}
	if (p->proginfo_chancommfree) {
		ref_release(p->proginfo_chancommfree);
	}
	if (p->proginfo_chan_output_filters) {
		ref_release(p->proginfo_chan_output_filters);
	}
	if (p->proginfo_seriesid) {
		ref_release(p->proginfo_seriesid);
	}
	if (p->proginfo_programid) {
		ref_release(p->proginfo_programid);
	}
	if (p->proginfo_inetref) {
		ref_release(p->proginfo_inetref);
	}
	if (p->proginfo_stars) {
		ref_release(p->proginfo_stars);
	}
	if (p->proginfo_pathname) {
		ref_release(p->proginfo_pathname);
	}
	if (p->proginfo_host) {
		ref_release(p->proginfo_host);
	}
	if (p->proginfo_playgroup) {
		ref_release(p->proginfo_playgroup);
	}
	if (p->proginfo_lastmodified) {
		ref_release(p->proginfo_lastmodified);
	}
	if (p->proginfo_start_ts) {
		ref_release(p->proginfo_start_ts);
	}
	if (p->proginfo_end_ts) {
		ref_release(p->proginfo_end_ts);
	}
	if (p->proginfo_rec_start_ts) {
		ref_release(p->proginfo_rec_start_ts);
	}
	if (p->proginfo_rec_end_ts) {
		ref_release(p->proginfo_rec_end_ts);
	}
	if (p->proginfo_originalairdate) {
		ref_release(p->proginfo_originalairdate);
	}
	if (p->proginfo_storagegroup) {
		ref_release(p->proginfo_storagegroup);
	}
	if (p->proginfo_recpriority_2) {
		ref_release(p->proginfo_recpriority_2);
	}
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
}

/*
 * cmyth_proginfo_create(void)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a programinfo structure to be used to hold program
 * information and return a pointer to the structure.  The structure
 * is initialized to default values.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_proginfo_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_proginfo_t
 */
cmyth_proginfo_t
cmyth_proginfo_create(void)
{
	cmyth_proginfo_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_proginfo_destroy);

	ret->proginfo_start_ts = cmyth_timestamp_create();
	if (!ret->proginfo_start_ts) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!!\n", __FUNCTION__);
		goto err;
	}
	ret->proginfo_end_ts = cmyth_timestamp_create();
	if (!ret->proginfo_end_ts) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!!!\n", __FUNCTION__);
		goto err;
	}
	ret->proginfo_rec_start_ts = cmyth_timestamp_create();
	if (!ret->proginfo_rec_start_ts) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!!!!\n", __FUNCTION__);
		goto err;
	}
	ret->proginfo_rec_end_ts = cmyth_timestamp_create();
	if (!ret->proginfo_rec_end_ts) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s } !!!!!\n", __FUNCTION__);
		goto err;
	}
	ret->proginfo_lastmodified = cmyth_timestamp_create();
	if (!ret->proginfo_lastmodified) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s } !!!!!!\n", __FUNCTION__);
		goto err;
	}
	ret->proginfo_originalairdate = cmyth_timestamp_create();
	if (!ret->proginfo_originalairdate) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s } !!!!!!!\n", __FUNCTION__);
		goto err;
	}
	ret->proginfo_title = NULL;
	ret->proginfo_subtitle = NULL;
	ret->proginfo_description = NULL;
	ret->proginfo_season = 0;
	ret->proginfo_episode = 0;
	ret->proginfo_category = NULL;
	ret->proginfo_chanId = 0;
	ret->proginfo_chanstr = NULL;
	ret->proginfo_chansign = NULL;
	ret->proginfo_channame = NULL;
	ret->proginfo_chanicon = NULL;
	ret->proginfo_url = NULL;
	ret->proginfo_pathname = NULL;
	ret->proginfo_host = NULL;
	ret->proginfo_port = -1;
	ret->proginfo_Length = 0;
	ret->proginfo_conflicting = 0;
	ret->proginfo_unknown_0 = NULL;
	ret->proginfo_recording = 0;
	ret->proginfo_override = 0;
	ret->proginfo_hostname = NULL;
	ret->proginfo_source_id = 0;
	ret->proginfo_card_id = 0;
	ret->proginfo_input_id = 0;
	ret->proginfo_rec_priority = 0;
	ret->proginfo_rec_status = 0;
	ret->proginfo_record_id = 0;
	ret->proginfo_rec_type = 0;
	ret->proginfo_rec_dups = 0;
	ret->proginfo_unknown_1 = 0;
	ret->proginfo_repeat = 0;
	ret->proginfo_program_flags = 0;
	ret->proginfo_rec_profile = NULL;
	ret->proginfo_recgroup = NULL;
	ret->proginfo_chancommfree = NULL;
	ret->proginfo_chan_output_filters = NULL;
	ret->proginfo_seriesid = NULL;
	ret->proginfo_programid = NULL;
	ret->proginfo_inetref = NULL;
	ret->proginfo_stars = NULL;
	ret->proginfo_version = 12;
	ret->proginfo_hasairdate = 0;
	ret->proginfo_playgroup = NULL;
	ret->proginfo_storagegroup = NULL;
	ret->proginfo_recpriority_2 = NULL;
	ret->proginfo_parentid = 0;
	ret->proginfo_audioproperties = 0;
	ret->proginfo_videoproperties = 0;
	ret->proginfo_subtitletype = 0;
	ret->proginfo_year = 0;
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;

    err:
	ref_release(ret);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s } !++\n", __FUNCTION__);
       	return NULL;
}

/*
 * cmyth_proginfo_create(void)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Duplicate a program information structure into a new one.  The sub-fields
 * get held, not actually copied.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_proginfo_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_proginfo_t
 */
static cmyth_proginfo_t
cmyth_proginfo_dup(cmyth_proginfo_t p)
{
	cmyth_proginfo_t ret = cmyth_proginfo_create();

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_proginfo_destroy);

	ret->proginfo_start_ts = ref_hold(p->proginfo_start_ts);
	ret->proginfo_end_ts = ref_hold(p->proginfo_end_ts);
	ret->proginfo_rec_start_ts = ref_hold(p->proginfo_rec_start_ts);
	ret->proginfo_rec_end_ts = ref_hold(p->proginfo_rec_end_ts);
	ret->proginfo_lastmodified = ref_hold(p->proginfo_lastmodified);
	ret->proginfo_originalairdate = ref_hold(p->proginfo_originalairdate);
	ret->proginfo_title = ref_hold(p->proginfo_title);
	ret->proginfo_subtitle = ref_hold(p->proginfo_subtitle);
	ret->proginfo_description = ref_hold(p->proginfo_description);
	ret->proginfo_season = p->proginfo_season;
	ret->proginfo_episode = p->proginfo_episode;
	ret->proginfo_category = ref_hold(p->proginfo_category);
	ret->proginfo_chanId = p->proginfo_chanId;
	ret->proginfo_chanstr = ref_hold(p->proginfo_chanstr);
	ret->proginfo_chansign = ref_hold(p->proginfo_chansign);
	ret->proginfo_channame = ref_hold(p->proginfo_channame);
	ret->proginfo_chanicon = ref_hold(p->proginfo_chanicon);
	ret->proginfo_url = ref_hold(p->proginfo_url);
	ret->proginfo_pathname = ref_hold(p->proginfo_pathname);
	ret->proginfo_host = ref_hold(p->proginfo_host);
	ret->proginfo_port = p->proginfo_port;
	ret->proginfo_Length = p->proginfo_Length;
	ret->proginfo_conflicting = p->proginfo_conflicting;
	ret->proginfo_unknown_0 = ref_hold(p->proginfo_unknown_0);
	ret->proginfo_recording = p->proginfo_recording;
	ret->proginfo_override = p->proginfo_override;
	ret->proginfo_hostname = ref_hold(p->proginfo_hostname);
	ret->proginfo_source_id = p->proginfo_source_id;
	ret->proginfo_card_id = p->proginfo_card_id;
	ret->proginfo_input_id = p->proginfo_input_id;
	ret->proginfo_rec_priority = ref_hold(p->proginfo_rec_priority);
	ret->proginfo_rec_status = p->proginfo_rec_status;
	ret->proginfo_record_id = p->proginfo_record_id;
	ret->proginfo_rec_type = p->proginfo_rec_type;
	ret->proginfo_rec_dups = p->proginfo_rec_dups;
	ret->proginfo_unknown_1 = p->proginfo_unknown_1;
	ret->proginfo_repeat = p->proginfo_repeat;
	ret->proginfo_program_flags = p->proginfo_program_flags;
	ret->proginfo_rec_profile = ref_hold(p->proginfo_rec_profile);
	ret->proginfo_recgroup = ref_hold(p->proginfo_recgroup);
	ret->proginfo_chancommfree = ref_hold(p->proginfo_chancommfree);
	ret->proginfo_chan_output_filters = ref_hold(p->proginfo_chan_output_filters);
	ret->proginfo_seriesid = ref_hold(p->proginfo_seriesid);
	ret->proginfo_programid = ref_hold(p->proginfo_programid);
	ret->proginfo_inetref = ref_hold(p->proginfo_inetref);
	ret->proginfo_stars = ref_hold(p->proginfo_stars);
	ret->proginfo_version = p->proginfo_version;
	ret->proginfo_hasairdate = p->proginfo_hasairdate;
	ret->proginfo_playgroup = ref_hold(p->proginfo_playgroup);
	ret->proginfo_storagegroup = ref_hold(p->proginfo_storagegroup);
	ret->proginfo_recpriority_2 = ref_hold(p->proginfo_recpriority_2);
	ret->proginfo_parentid = p->proginfo_parentid;
	ret->proginfo_audioproperties = p->proginfo_audioproperties;
	ret->proginfo_videoproperties = p->proginfo_videoproperties;
	ret->proginfo_subtitletype = p->proginfo_subtitletype;
	ret->proginfo_year = p->proginfo_year;
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

/*
 * cmyth_proginfo_check_recording(cmyth_conn_t control,
 *                                cmyth_proginfo_t prog)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Make a request on the control connection 'control' to check the
 * existence of the program 'prog' on the MythTV back end.
 *
 * Return Value:
 *
 * Success: 1 - if the recording exists, 0 - if it does not
 *
 * Failure: -(ERRNO)
 */
int
cmyth_proginfo_check_recording(cmyth_conn_t control, cmyth_proginfo_t prog)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	return -ENOSYS;
}

static int
delete_command(cmyth_conn_t control, cmyth_proginfo_t prog, char *cmd)
{
	long c = 0;
	char *buf;
	unsigned int len = ((2 * CMYTH_LONGLONG_LEN) + 
			    (6 * CMYTH_TIMESTAMP_LEN) +
			    (16 * CMYTH_LONG_LEN));
	char start_ts[CMYTH_TIMESTAMP_LEN + 1];
	char end_ts[CMYTH_TIMESTAMP_LEN + 1];
	char rec_start_ts[CMYTH_TIMESTAMP_LEN + 1];
	char rec_end_ts[CMYTH_TIMESTAMP_LEN + 1];
	char originalairdate[CMYTH_TIMESTAMP_LEN + 1];
	char lastmodified[CMYTH_TIMESTAMP_LEN + 1];
	int err = 0;
	int count = 0;
	long r = 0;
	int ret = 0;

	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no program info\n",
			  __FUNCTION__);
		return -EINVAL;
	}
#define S(a) ((a) == NULL ? "" : (a))

	len += strlen(S(prog->proginfo_title));
	len += strlen(S(prog->proginfo_subtitle));
	len += strlen(S(prog->proginfo_description));
	len += strlen(S(prog->proginfo_category));
	len += strlen(S(prog->proginfo_chanstr));
	len += strlen(S(prog->proginfo_chansign));
	len += strlen(S(prog->proginfo_channame));
	len += strlen(S(prog->proginfo_url));
	len += strlen(S(prog->proginfo_hostname));
	len += strlen(S(prog->proginfo_playgroup));
	len += strlen(S(prog->proginfo_seriesid));
	len += strlen(S(prog->proginfo_programid));
	len += strlen(S(prog->proginfo_inetref));
	len += strlen(S(prog->proginfo_recpriority_2));
	len += strlen(S(prog->proginfo_storagegroup));

	buf = alloca(len + 1+2048);
	if (!buf) {
		return -ENOMEM;
	}

	if(control->conn_version < 14)
	{
	    cmyth_timestamp_to_string(start_ts, prog->proginfo_start_ts);
	    cmyth_timestamp_to_string(end_ts, prog->proginfo_end_ts);
	    cmyth_timestamp_to_string(rec_start_ts,
	    			      prog->proginfo_rec_start_ts);
	    cmyth_timestamp_to_string(rec_end_ts, prog->proginfo_rec_end_ts);
	    cmyth_timestamp_to_string(originalairdate,
				      prog->proginfo_originalairdate);
	    cmyth_timestamp_to_string(lastmodified,
				      prog->proginfo_lastmodified);
	}
	else
	{
	    cmyth_datetime_to_string(start_ts, prog->proginfo_start_ts);
	    cmyth_datetime_to_string(end_ts, prog->proginfo_end_ts);
	    cmyth_datetime_to_string(rec_start_ts, prog->proginfo_rec_start_ts);
	    cmyth_datetime_to_string(rec_end_ts, prog->proginfo_rec_end_ts);
	    cmyth_datetime_to_string(originalairdate,
				     prog->proginfo_originalairdate);
	    cmyth_datetime_to_string(lastmodified, prog->proginfo_lastmodified);
	}

	if(control->conn_version > 32) {
	    cmyth_timestamp_to_isostring(originalairdate,
				 prog->proginfo_originalairdate);
	}

	if(control->conn_version < 12)
	{
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: delete not supported with protocol ver %d\n",
			  __FUNCTION__, control->conn_version);
		return -EINVAL;
	}
	sprintf(buf, "%s 0[]:[]", cmd);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_title));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_subtitle));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_description));
	if (control->conn_version >= 67) {
		sprintf(buf + strlen(buf), "%u[]:[]", prog->proginfo_season);
		sprintf(buf + strlen(buf), "%u[]:[]", prog->proginfo_episode);
	}
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_category));
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_chanId);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_chanstr));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_chansign));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_channame));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_url));
	if (control->conn_version >= 57) {
		sprintf(buf + strlen(buf), "%"PRId64"[]:[]", prog->proginfo_Length);
	} else {
		sprintf(buf + strlen(buf), "%d[]:[]", (int32_t)(prog->proginfo_Length >> 32));
		sprintf(buf + strlen(buf), "%d[]:[]", (int32_t)(prog->proginfo_Length & 0xffffffff));
	}
	sprintf(buf + strlen(buf), "%s[]:[]",  start_ts);
	sprintf(buf + strlen(buf), "%s[]:[]",  end_ts);
	if (control->conn_version < 57) {
		sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_unknown_0)); // "duplicate"
		sprintf(buf + strlen(buf), "%ld[]:[]", 0L); // "shareable"
	}
	sprintf(buf + strlen(buf), "%ld[]:[]", 0L); // "findid"
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_hostname));
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_source_id);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_card_id);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_input_id);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_rec_priority));
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_rec_status);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_record_id);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_rec_type);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_rec_dups);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_unknown_1); // "dupmethod"
	sprintf(buf + strlen(buf), "%s[]:[]",  rec_start_ts);
	sprintf(buf + strlen(buf), "%s[]:[]",  rec_end_ts);
	if (control->conn_version < 57) {
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_repeat);
	}
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_program_flags);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_recgroup));
	if (control->conn_version < 57) {
		sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_chancommfree));
	}
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_chan_output_filters));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_seriesid));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_programid));
	if (control->conn_version >= 67) {
		sprintf(buf + strlen(buf), "%s[]:[]", S(prog->proginfo_inetref));
	}
	sprintf(buf + strlen(buf), "%s[]:[]",  lastmodified);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_stars));
	sprintf(buf + strlen(buf), "%s[]:[]",  originalairdate);
	if (control->conn_version >= 15 && control->conn_version < 57) {
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_hasairdate);
	}
	if (control->conn_version >= 18) {
		sprintf(buf + strlen(buf), "%s[]:[]", S(prog->proginfo_playgroup));
	}
	if (control->conn_version >= 25) {
		sprintf(buf + strlen(buf), "%s[]:[]", S(prog->proginfo_recpriority_2));
	}
	if (control->conn_version >= 31) {
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_parentid);
	}
	if (control->conn_version >= 32) {
		sprintf(buf + strlen(buf), "%s[]:[]", S(prog->proginfo_storagegroup));
	}
	if (control->conn_version >= 35) {
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_audioproperties);
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_videoproperties);
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_subtitletype);
	}
	if (control->conn_version >= 43) {
		sprintf(buf + strlen(buf), "%d[]:[]", prog->proginfo_year);
	}
#undef S

	pthread_mutex_lock(&mutex);

	if ((err = cmyth_send_message(control, buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

	count = cmyth_rcv_length(control);
	if ((r=cmyth_rcv_long(control, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto out;
	}

    out:
	pthread_mutex_unlock(&mutex);

	return ret;
}


/*
 * cmyth_proginfo_delete_recording(cmyth_conn_t control,
 *                                 cmyth_proginfo_t prog)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Make a request on the control connection 'control' to delete the
 * program 'prog' from the MythTV back end.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_proginfo_delete_recording(cmyth_conn_t control, cmyth_proginfo_t prog)
{
	return delete_command(control, prog, "DELETE_RECORDING");
}

/*
 * cmyth_proginfo_forget_recording(cmyth_conn_t control,
 *                                 cmyth_proginfo_t prog)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Make a request on the control connection 'control' to tell the
 * MythTV back end to forget the program 'prog'.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_proginfo_forget_recording(cmyth_conn_t control, cmyth_proginfo_t prog)
{
	return delete_command(control, prog, "FORGET_RECORDING");
}

/*
 * cmyth_proginfo_stop_recording(cmyth_conn_t control,
 *                               cmyth_proginfo_t prog)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Make a request on the control connection 'control' to ask the
 * MythTV back end to stop recording the program described in 'prog'.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_proginfo_stop_recording(cmyth_conn_t control, cmyth_proginfo_t prog)
{
	return delete_command(control, prog, "STOP_RECORDING");
}

/*
 * cmyth_proginfo_get_recorder_num(cmyth_conn_t control,
 *                                 cmyth_rec_num_t rnum,
 *                                 cmyth_proginfo_t prog)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Make a request on the control connection 'control' to obtain the
 * recorder number for the program 'prog' and fill out 'rnum' with the
 * information.
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: -(ERRNO)
 */
int
cmyth_proginfo_get_recorder_num(cmyth_conn_t control,
				cmyth_rec_num_t rnum,
				cmyth_proginfo_t prog)
{
	return -ENOSYS;
}

/*
 * cmyth_proginfo_chan_id(cmyth_proginfo_t prog)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the channel identifier from a program information structure.
 *
 * Return Value:
 *
 * Success: A positive integer channel identifier (these are formed from
 *          the recorder number and the channel number).
 *
 * Failure: -(ERRNO)
 */
long
cmyth_proginfo_chan_id(cmyth_proginfo_t prog)
{
	if (!prog) {
		return -EINVAL;
	}
	return prog->proginfo_chanId;
}

/*
 * cmyth_proginfo_title(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_title' field of a program info structure.
 *
 * The returned string is a pointer to the string within the program
 * info structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_proginfo_title(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_title);
}

/*
 * cmyth_proginfo_subtitle(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_subtitle' field of a program info structure.
 *
 * The returned string is a pointer to the string within the program
 * info structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_proginfo_subtitle(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_subtitle);
}

/*
 * cmyth_proginfo_description(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_description' field of a program info structure.
 *
 * The returned string is a pointer to the string within the program
 * info structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_proginfo_description(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_description);
}

unsigned short
cmyth_proginfo_season(cmyth_proginfo_t prog)
{
	if (!prog) {
		return 0;
	}
	return prog->proginfo_season;
}

unsigned short
cmyth_proginfo_episode(cmyth_proginfo_t prog)
{
	if (!prog) {
		return 0;
	}
	return prog->proginfo_episode;
}

/*
 * cmyth_proginfo_category(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_category' field of a program info structure.
 *
 * The returned string is a pointer to the string within the program
 * info structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_proginfo_category(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_category);
}

char *
cmyth_proginfo_seriesid(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL series ID\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_seriesid);
}

char *
cmyth_proginfo_programid(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program ID\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_programid);
}

char *
cmyth_proginfo_inetref(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL inetref\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_inetref);
}

char *
cmyth_proginfo_stars(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL stars\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_stars);
}

char *
cmyth_proginfo_playgroup(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL playgroup\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_playgroup);
}

cmyth_timestamp_t
cmyth_proginfo_originalairdate(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL original air date\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_originalairdate);
}

/*
 * cmyth_proginfo_chanstr(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_chanstr' field of a program info structure.
 *
 * The returned string is a pointer to the string within the program
 * info structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_proginfo_chanstr(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_chanstr);
}

/*
 * cmyth_proginfo_chansign(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_chansign' field of a program info
 * structure.
 *
 * The returned string is a pointer to the string within the program
 * info structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_proginfo_chansign(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_chansign);
}

/*
 * cmyth_proginfo_channame(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_channame' field of a program info
 * structure.
 *
 * The returned string is a pointer to the string within the program
 * info structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_proginfo_channame(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_channame);
}

/*
 * cmyth_proginfo_channame(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_pathname' field of a program info
 * structure.
 *
 * The returned string is a pointer to the string within the program
 * info structure, so it should not be modified by the caller.  The
 * return value is a 'char *' for this reason.
 *
 * Return Value:
 *
 * Success: A pointer to a 'char *' pointing to the field.
 *
 * Failure: NULL
 */
char *
cmyth_proginfo_pathname(cmyth_proginfo_t prog)
{
	if (!prog) {
		return NULL;
	}
	return ref_hold(prog->proginfo_pathname);
}

/*
 * cmyth_proginfo_length(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_Length' field of a program info
 * structure.
 *
 * Return Value:
 *
 * Success: long long file length
 *
 * Failure: NULL
 */
long long
cmyth_proginfo_length(cmyth_proginfo_t prog)
{
	if (!prog) {
		return -1;
	}
	return prog->proginfo_Length;
}

/*
 * cmyth_proginfo_length_sec(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the program length in seconds.
 *
 * Return Value:
 *
 * Success: int file length
 *
 * Failure: NULL
 */
int
cmyth_proginfo_length_sec(cmyth_proginfo_t prog)
{
	int seconds;

	if (!prog) {
		return -1;
	}

	seconds = cmyth_timestamp_diff(prog->proginfo_start_ts,
				       prog->proginfo_end_ts);

	return seconds;
}
/*
 * cmyth_proginfo_start(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'start' timestamp from a program info structure.
 * This indicates a programmes start time.
 *
 * The returned timestamp is returned held.  It should be released
 * when no longer needed using ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_timestamp_t
 *
 * Failure: NULL
 */
cmyth_timestamp_t
cmyth_proginfo_start(cmyth_proginfo_t prog)
{
	if (!prog) {
		return NULL;
	}
	return ref_hold(prog->proginfo_start_ts);
}


/*
 * cmyth_proginfo_rec_end(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'end' timestamp from a program info structure.
 * This tells when a recording started.
 *
 * The returned timestamp is returned held.  It should be released
 * when no longer needed using ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_timestamp_t
 *
 * Failure: NULL
 */
cmyth_timestamp_t
cmyth_proginfo_end(cmyth_proginfo_t prog)
{
	if (!prog) {
		return NULL;
	}
	return ref_hold(prog->proginfo_end_ts);
}

/*
 * cmyth_proginfo_rec_start(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'rec_start' timestamp from a program info structure.
 * This tells when a recording started.
 *
 * The returned timestamp is returned held.  It should be released
 * when no longer needed using ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_timestamp_t
 *
 * Failure: NULL
 */
cmyth_timestamp_t
cmyth_proginfo_rec_start(cmyth_proginfo_t prog)
{
	if (!prog) {
		return NULL;
	}
	return ref_hold(prog->proginfo_rec_start_ts);
}


/*
 * cmyth_proginfo_rec_end(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'rec_end' timestamp from a program info structure.
 * This tells when a recording started.
 *
 * The returned timestamp is returned held.  It should be released
 * when no longer needed using ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_timestamp_t
 *
 * Failure: NULL
 */
cmyth_timestamp_t
cmyth_proginfo_rec_end(cmyth_proginfo_t prog)
{
	if (!prog) {
		return NULL;
	}
	return ref_hold(prog->proginfo_rec_end_ts);
}

/*
 * cmyth_proginfo_rec_status(cmyth_proginfo_t prog)
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the recording status from a program info structure.
 * Recording status has the following possible values:
 *
 *	RS_DELETED
 *	RS_STOPPED
 *	RS_RECORDED
 *	RS_RECORDING
 *	RS_WILL_RECORD
 *	RS_DONT_RECORD
 *	RS_PREVIOUS_RECORDING
 *	RS_CURRENT_RECORDING
 *	RS_EARLIER_RECORDING
 *	RS_TOO_MANY_RECORDINGS
 *	RS_CANCELLED
 *	RS_CONFLICT
 *	RS_LATER_SHOWING
 *	RS_REPEAT
 *	RS_LOW_DISKSPACE
 *	RS_TUNER_BUSY
 *
 * Return Value:
 *
 * Success: A recording status 
 *
 * Failure: 0 (an invalid status)
 */
cmyth_proginfo_rec_status_t
cmyth_proginfo_rec_status(cmyth_proginfo_t prog)
{
	if (!prog) {
		return 0;
	}
	return prog->proginfo_rec_status;
}

/*
 * cmyth_proginfo_flags(cmyth_proginfo_t prog)
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the flags mask from a program info structure.
 *
 * Return Value:
 *
 * Success: The program flag mask.
 *
 * Failure: 0 (an invalid status)
 */
unsigned long
cmyth_proginfo_flags(cmyth_proginfo_t prog)
{
  if (!prog) {
    return 0;
  }
  return prog->proginfo_program_flags;
}

/*
 * cmyth_proginfo_year(cmyth_proginfo_t prog)
 *
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Retrieves the 'proginfo_year' field of a program info
 * structure.
 *
 * Return Value:
 *
 * Success: the production year for the program
 *
 * Failure: 0
 */
unsigned short
cmyth_proginfo_year(cmyth_proginfo_t prog)
{
	if (!prog) {
		return 0;
	}
	return prog->proginfo_year;
}

static int
fill_command(cmyth_conn_t control, cmyth_proginfo_t prog, char *cmd)
{
	char *buf;
	unsigned int len = ((2 * CMYTH_LONGLONG_LEN) + 
			    (6 * CMYTH_TIMESTAMP_LEN) +
			    (16 * CMYTH_LONG_LEN));
	char start_ts[CMYTH_TIMESTAMP_LEN + 1];
	char end_ts[CMYTH_TIMESTAMP_LEN + 1];
	char rec_start_ts[CMYTH_TIMESTAMP_LEN + 1];
	char rec_end_ts[CMYTH_TIMESTAMP_LEN + 1];
	char originalairdate[CMYTH_TIMESTAMP_LEN + 1];
	char lastmodified[CMYTH_TIMESTAMP_LEN + 1];
	int err = 0;
	int ret = 0;
	char *host = "libcmyth";

	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no program info\n",
			  __FUNCTION__);
		return -EINVAL;
	}
#define S(a) ((a) == NULL ? "" : (a))

	len += strlen(S(prog->proginfo_title));
	len += strlen(S(prog->proginfo_subtitle));
	len += strlen(S(prog->proginfo_description));
	len += strlen(S(prog->proginfo_category));
	len += strlen(S(prog->proginfo_chanstr));
	len += strlen(S(prog->proginfo_chansign));
	len += strlen(S(prog->proginfo_channame));
	len += strlen(S(prog->proginfo_url));
	len += strlen(S(prog->proginfo_hostname));
	len += strlen(S(prog->proginfo_playgroup));
	len += strlen(S(prog->proginfo_seriesid));
	len += strlen(S(prog->proginfo_programid));
	len += strlen(S(prog->proginfo_inetref));
	len += strlen(S(prog->proginfo_recpriority_2));
	len += strlen(S(prog->proginfo_storagegroup));

	buf = alloca(len + 1+2048);
	if (!buf) {
		return -ENOMEM;
	}

	if(control->conn_version < 14)
	{
	    cmyth_timestamp_to_string(start_ts, prog->proginfo_start_ts);
	    cmyth_timestamp_to_string(end_ts, prog->proginfo_end_ts);
	    cmyth_timestamp_to_string(rec_start_ts,
	    				prog->proginfo_rec_start_ts);
	    cmyth_timestamp_to_string(rec_end_ts, prog->proginfo_rec_end_ts);
	    cmyth_timestamp_to_string(originalairdate,
				  prog->proginfo_originalairdate);
	    cmyth_timestamp_to_string(lastmodified,
	    				prog->proginfo_lastmodified);
	}
	else
	{
	    cmyth_datetime_to_string(start_ts, prog->proginfo_start_ts);
	    cmyth_datetime_to_string(end_ts, prog->proginfo_end_ts);
	    cmyth_datetime_to_string(rec_start_ts, prog->proginfo_rec_start_ts);
	    cmyth_datetime_to_string(rec_end_ts, prog->proginfo_rec_end_ts);
	    cmyth_datetime_to_string(originalairdate,
				     prog->proginfo_originalairdate);
	    cmyth_datetime_to_string(lastmodified, prog->proginfo_lastmodified);
	}

	if(control->conn_version > 32) {
	    cmyth_timestamp_to_isostring(originalairdate,
	                         prog->proginfo_originalairdate);
        }				 

	if (control->conn_version < 12)
	{
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: fill not supported with protocol ver %d\n",
			  __FUNCTION__, control->conn_version);
		return -EINVAL;
	}
	sprintf(buf, "%s %s[]:[]0[]:[]", cmd, host);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_title));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_subtitle));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_description));
	if (control->conn_version >= 67) {
		sprintf(buf + strlen(buf), "%u[]:[]", prog->proginfo_season);
		sprintf(buf + strlen(buf), "%u[]:[]", prog->proginfo_episode);
	}
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_category));
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_chanId);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_chanstr));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_chansign));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_channame));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_url));
	if (control->conn_version >= 57) {
		sprintf(buf + strlen(buf), "%"PRId64"[]:[]", prog->proginfo_Length);
	} else {
		sprintf(buf + strlen(buf), "%d[]:[]", (int32_t)(prog->proginfo_Length >> 32));
		sprintf(buf + strlen(buf), "%d[]:[]", (int32_t)(prog->proginfo_Length & 0xffffffff));
	}
	sprintf(buf + strlen(buf), "%s[]:[]",  start_ts);
	sprintf(buf + strlen(buf), "%s[]:[]",  end_ts);
	if (control->conn_version < 57) {
		sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_unknown_0)); // "duplicate"
		sprintf(buf + strlen(buf), "%ld[]:[]", 0L); // "shareable"
	}
	sprintf(buf + strlen(buf), "%ld[]:[]", 0L); // "findid"
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_hostname));
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_source_id);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_card_id);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_input_id);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_rec_priority));
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_rec_status);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_record_id);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_rec_type);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_rec_dups);
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_unknown_1); // "dupmethod"
	sprintf(buf + strlen(buf), "%s[]:[]",  rec_start_ts);
	sprintf(buf + strlen(buf), "%s[]:[]",  rec_end_ts);
	if (control->conn_version < 57) {
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_repeat);
	}
	sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_program_flags);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_recgroup));
	if (control->conn_version < 57) {
		sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_chancommfree));
	}
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_chan_output_filters));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_seriesid));
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_programid));
	if (control->conn_version >= 67) {
		sprintf(buf + strlen(buf), "%s[]:[]", S(prog->proginfo_inetref));
	}
	sprintf(buf + strlen(buf), "%s[]:[]",  lastmodified);
	sprintf(buf + strlen(buf), "%s[]:[]",  S(prog->proginfo_stars));
	sprintf(buf + strlen(buf), "%s[]:[]",  originalairdate);
	if (control->conn_version >= 15 && control->conn_version < 57) {
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_hasairdate);
	}
	if (control->conn_version >= 18) {
		sprintf(buf + strlen(buf), "%s[]:[]", S(prog->proginfo_playgroup));
	}
	if (control->conn_version >= 25) {
		sprintf(buf + strlen(buf), "%s[]:[]", S(prog->proginfo_recpriority_2));
	}
	if (control->conn_version >= 31) {
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_parentid);
	}
	if (control->conn_version >= 32) {
		sprintf(buf + strlen(buf), "%s[]:[]", S(prog->proginfo_storagegroup));
	}
	if (control->conn_version >= 35) {
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_audioproperties);
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_videoproperties);
		sprintf(buf + strlen(buf), "%ld[]:[]", prog->proginfo_subtitletype);
	}
	if (control->conn_version >= 43) {
		sprintf(buf + strlen(buf), "%d[]:[]", prog->proginfo_year);
	}
#undef S

	if ((err = cmyth_send_message(control, buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

    out:
	return ret;
}

/*
 * cmyth_proginfo_fill(cmyth_conn_t control, cmyth_proginfo_t prog)
 *
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Fill out a (possibly incomplete) program info.  Incomplete program
 * info comes from program listings.  Since this modifies the contents of
 * the supplied program info, it must never be called with a program info
 * that has more than one reference).
 *
 * Return Value:
 *
 * Success: 0
 *
 * Failure: a negative error code.
 */
static int
cmyth_proginfo_fill(cmyth_conn_t control, cmyth_proginfo_t prog)
{
	int err = 0;
	int count;
	int ret;
	long long length = 0;

	if (!control) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no program info\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&mutex);

	length = prog->proginfo_Length;
	if ((ret=fill_command(control, prog, "FILL_PROGRAM_INFO") != 0))
		goto out;

	count = cmyth_rcv_length(control);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto out;
	}
	if (cmyth_rcv_proginfo(control, &err, prog, count) != count) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_proginfo() < count\n", __FUNCTION__);
		ret = err;
		goto out;
	}

	/*
	 * Myth seems to cache the program length, rather than call stat()
	 * every time it needs to know.  Using FILL_PROGRAM_INFO has worked
	 * to force mythbackend to call stat() and return the correct length.
	 *
	 * However, some users are reporting that FILL_PROGRAM_INFO is
	 * returning 0 for the program length.  In that case, the original
	 * number is still probably wrong, but it's better than 0.
	 */
	if (prog->proginfo_Length == 0) {
		prog->proginfo_Length = length;
		ret = -1;
		goto out;
	}

	ret = 0;

    out:
	pthread_mutex_unlock(&mutex);

	return ret;
}

/*
 * cmyth_proginfo_get_detail(cmyth_conn_t control, cmyth_proginfo_t prog)
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Completely fill out a program info based on a supplied (possibly
 * incomplete) program info.  The supplied program info will be duplicated and
 * a pointer to the duplicate will be returned.
 *
 * NOTE: The original program info is released before the return.  If the
 *       caller wishes to retain access to the original it must already be
 *       held before the call.  This permits the called to replace a
 *       program info directly with the return from this function.
 *
 * Return Value:
 *
 * Success: A held, Non-NULL program_info
 *
 * Failure: NULL
 */
cmyth_proginfo_t
cmyth_proginfo_get_detail(cmyth_conn_t control, cmyth_proginfo_t p)
{
	cmyth_proginfo_t ret = cmyth_proginfo_dup(p);

	if (ret == NULL) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_proginfo_dup() failed\n",
			  __FUNCTION__);
		return NULL;
	}
	if (cmyth_proginfo_fill(control, ret) < 0) {
		ref_release(ret);
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_proginfo_fill() failed\n",
			  __FUNCTION__);
		return NULL;
	}
	return ret;
}

/*
 * cmyth_proginfo_compare(cmyth_proginfo_t a, cmyth_proginfo_t b)
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Compare two program info's and indicate whether they are the same program.
 *
 * Return Value:
 *
 * Same: 0
 *
 * Different: -1
 */
int
cmyth_proginfo_compare(cmyth_proginfo_t a, cmyth_proginfo_t b)
{
	if (a == b)
		return 0;

	if ((a == NULL) || (b == NULL))
		return -1;

#define STRCMP(a, b) ( (a && b && (strcmp(a,b) == 0)) ? 0 : \
		       ((a == NULL) && (b == NULL) ? 0 : -1) )

	if (STRCMP(a->proginfo_title, b->proginfo_title) != 0)
		return -1;
	if (STRCMP(a->proginfo_subtitle, b->proginfo_subtitle) != 0)
		return -1;
	if (STRCMP(a->proginfo_description, b->proginfo_description) != 0)
		return -1;
	if (STRCMP(a->proginfo_chanstr, b->proginfo_chanstr) != 0)
		return -1;

	if (a->proginfo_url && b->proginfo_url) {
          char* aa = strrchr(a->proginfo_url, '/');
          char* bb = strrchr(b->proginfo_url, '/');
          if (strcmp(aa ? aa+1 : a->proginfo_url, bb ? bb+1 : b->proginfo_url) != 0)
		return -1;
	} else if(!a->proginfo_url != !b->proginfo_url)
		return -1;

	if (cmyth_timestamp_compare(a->proginfo_start_ts,
				    b->proginfo_start_ts) != 0)
		return -1;
	if (cmyth_timestamp_compare(a->proginfo_end_ts,
				    b->proginfo_end_ts) != 0)
		return -1;

	return 0;
}

/*
 * cmyth_proginfo_host(cmyth_proginfo_t prog)
 *
 * Scope: PUBLIC
 *
 * Description
 *
 * Get the host name of the recorder serving the program 'prog'.
 *
 * Return Value:
 *
 * Success: A held, non-NULL string pointer
 *
 * Failure: NULL
 */
char*
cmyth_proginfo_host(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: no program info\n", __FUNCTION__);
		return NULL;
	}

	return ref_hold(prog->proginfo_host);
}

int
cmyth_proginfo_port(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: no program info\n", __FUNCTION__);
		return -1;
	}

	return prog->proginfo_port;
}

long
cmyth_proginfo_card_id(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: no program info\n", __FUNCTION__);
		return -1;
	}

	return prog->proginfo_card_id;
}

char *
cmyth_proginfo_recgroup(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_recgroup);
}

char *
cmyth_proginfo_chanicon(cmyth_proginfo_t prog)
{
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL program information\n",
			  __FUNCTION__);
		return NULL;
	}
	return ref_hold(prog->proginfo_chanicon);
}

int 
cmyth_get_delete_list(cmyth_conn_t conn, char * msg, cmyth_proglist_t prog)
{
        int err=0;
        int count;
        int prog_count=0;

        if (!conn) {
                cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n", __FUNCTION__);
                return -1;
        }
        pthread_mutex_lock(&mutex);
        if ((err = cmyth_send_message(conn, msg)) < 0) {
                fprintf (stderr, "ERROR %d \n",err);
                cmyth_dbg(CMYTH_DBG_ERROR,
                        "%s: cmyth_send_message() failed (%d)\n",__FUNCTION__,err);
                return err;
        }
        count = cmyth_rcv_length(conn);
        cmyth_rcv_proglist(conn, &err, prog, count);
        prog_count=cmyth_proglist_get_count(prog);
        pthread_mutex_unlock(&mutex);
        return prog_count;
}

cmyth_proginfo_t
cmyth_proginfo_get_from_basename(cmyth_conn_t control, const char* basename)
{
	int err = 0;
	int count, i;
	char msg[4096];
	char *base;
	cmyth_proginfo_t prog = NULL;
	cmyth_proglist_t list = NULL;

	if (!control) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	/*
	 * mythbackend doesn't support spaces in basenames
	 * when doing QUERY_RECORDING.  If there are spaces, fallback
	 * to enumerating all recordings
	 */
	if(control->conn_version >= 32 && strchr(basename, ' ') == NULL) {
		pthread_mutex_lock(&mutex);

		snprintf(msg, sizeof(msg), "QUERY_RECORDING BASENAME %s",
			 basename);

		if ((err=cmyth_send_message(control, msg)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_send_message() failed (%d)\n",
			  	__FUNCTION__, err);
			goto out;
		}

		count = cmyth_rcv_length(control);
		if (count < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_length() failed (%d)\n",
				  __FUNCTION__, count);
			goto out;
		}

		i = cmyth_rcv_string(control, &err, msg, sizeof(msg), count);
		if (err) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_string() failed\n",
				  __FUNCTION__);
			goto out;
		}
		count -= i;

		if (strcmp(msg, "OK") != 0) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: didn't recieve OK as response\n",
				  __FUNCTION__);
			goto out;
		}

		prog = cmyth_proginfo_create();
		if (cmyth_rcv_proginfo(control, &err, prog, count) != count) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_proginfo() < count\n", __FUNCTION__);
			goto out;
		}

		pthread_mutex_unlock(&mutex);
		return prog;
out:
		pthread_mutex_unlock(&mutex);
		if(prog)
			ref_release(prog);
		return NULL;

	} else {

		list = cmyth_proglist_get_all_recorded(control);
		if (!list) {
			cmyth_dbg(CMYTH_DBG_ERROR, "%s: no program list\n",
				  __FUNCTION__);
		}

		count = cmyth_proglist_get_count(list);
		for (i = 0;i < count; i++) {
			prog = cmyth_proglist_get_item(list, i);
			if (!prog) {
				cmyth_dbg(CMYTH_DBG_DEBUG, "%s: no program info\n",
					  __FUNCTION__);
				continue;
			}
			base = strrchr(prog->proginfo_pathname, '/');
			if (!base || strcmp(base+1, basename) !=0) {
				ref_release(prog);
				prog = NULL;
				continue;
			}
			break;
		}
		ref_release(list);
		return prog;
	}

}
