/* 
   Unix SMB/CIFS implementation.
   NBT netbios library routines
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Luke Kenneth Casson Leighton 1994-1998
   Copyright (C) Jeremy Allison 1994-1998
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   
*/

#include "includes.h"

int num_response_packets = 0;

/***************************************************************************
  Add an expected response record into the list
  **************************************************************************/

static void add_response_record(struct subnet_record *subrec,
				struct response_record *rrec)
{
	struct response_record *rrec2;

	num_response_packets++; /* count of total number of packets still around */

	DEBUG(4,("add_response_record: adding response record id:%hu to subnet %s. num_records:%d\n",
		rrec->response_id, subrec->subnet_name, num_response_packets));

	if (!subrec->responselist) {
		subrec->responselist = rrec;
		rrec->prev = NULL;
		rrec->next = NULL;
		return;
	}
  
	for (rrec2 = subrec->responselist; rrec2->next; rrec2 = rrec2->next) 
		;
  
	rrec2->next = rrec;
	rrec->next = NULL;
	rrec->prev = rrec2;
}

/***************************************************************************
  Remove an expected response record from the list
  **************************************************************************/

void remove_response_record(struct subnet_record *subrec,
				struct response_record *rrec)
{
	if (rrec->prev)
		rrec->prev->next = rrec->next;
	if (rrec->next)
		rrec->next->prev = rrec->prev;

	if (subrec->responselist == rrec) 
		subrec->responselist = rrec->next; 

	if(rrec->userdata) {
		if(rrec->userdata->free_fn) {
			(*rrec->userdata->free_fn)(rrec->userdata);
		} else {
			ZERO_STRUCTP(rrec->userdata);
			SAFE_FREE(rrec->userdata);
		}
	}

	/* Ensure we can delete. */
	rrec->packet->locked = False;
	free_packet(rrec->packet);

	ZERO_STRUCTP(rrec);
	SAFE_FREE(rrec);

	num_response_packets--; /* count of total number of packets still around */
}

/****************************************************************************
  Create a response record for an outgoing packet.
  **************************************************************************/

struct response_record *make_response_record( struct subnet_record *subrec,
					      struct packet_struct *p,
					      response_function resp_fn,
					      timeout_response_function timeout_fn,
					      success_function success_fn,
					      fail_function fail_fn,
					      struct userdata_struct *userdata)
{
	struct response_record *rrec;
	struct nmb_packet *nmb = &p->packet.nmb;

	if (!(rrec = SMB_MALLOC_P(struct response_record))) {
		DEBUG(0,("make_response_queue_record: malloc fail for response_record.\n"));
		return NULL;
	}

	memset((char *)rrec, '\0', sizeof(*rrec));

	rrec->response_id = nmb->header.name_trn_id;

	rrec->resp_fn = resp_fn;
	rrec->timeout_fn = timeout_fn;
	rrec->success_fn = success_fn;
	rrec->fail_fn = fail_fn;

	rrec->packet = p;

	if(userdata) {
		/* Intelligent userdata. */
		if(userdata->copy_fn) {
			if((rrec->userdata = (*userdata->copy_fn)(userdata)) == NULL) {
				DEBUG(0,("make_response_queue_record: copy fail for userdata.\n"));
				ZERO_STRUCTP(rrec);
				SAFE_FREE(rrec);
				return NULL;
			}
		} else {
			/* Primitive userdata, do a memcpy. */
			if((rrec->userdata = (struct userdata_struct *)
					SMB_MALLOC(sizeof(struct userdata_struct)+userdata->userdata_len)) == NULL) {
				DEBUG(0,("make_response_queue_record: malloc fail for userdata.\n"));
				ZERO_STRUCTP(rrec);
				SAFE_FREE(rrec);
				return NULL;
			}
			rrec->userdata->copy_fn = userdata->copy_fn;
			rrec->userdata->free_fn = userdata->free_fn;
			rrec->userdata->userdata_len = userdata->userdata_len;
			memcpy(rrec->userdata->data, userdata->data, userdata->userdata_len);
		}
	} else {
		rrec->userdata = NULL;
	}

	rrec->num_msgs = 0;

	if(!nmb->header.nm_flags.bcast)
		rrec->repeat_interval = 5; /* 5 seconds for unicast packets. */
	else
		rrec->repeat_interval = 1; /* XXXX should be in ms */
	rrec->repeat_count = 3; /* 3 retries */
	rrec->repeat_time = time(NULL) + rrec->repeat_interval; /* initial retry time */

	/* This packet is not being processed. */
	rrec->in_expiration_processing = False;

	/* Lock the packet so we won't lose it while it's on the list. */
	p->locked = True;

	add_response_record(subrec, rrec);

	return rrec;
}

/****************************************************************************
  Find a response in a subnet's name query response list. 
  **************************************************************************/

static struct response_record *find_response_record_on_subnet(
                                struct subnet_record *subrec, uint16 id)
{  
	struct response_record *rrec = NULL;

	for (rrec = subrec->responselist; rrec; rrec = rrec->next) {
		if (rrec->response_id == id) {
			DEBUG(4, ("find_response_record: found response record id = %hu on subnet %s\n",
				id, subrec->subnet_name));
			break;
		}
	}
	return rrec;
}

/****************************************************************************
  Find a response in any subnet's name query response list. 
  **************************************************************************/

struct response_record *find_response_record(struct subnet_record **ppsubrec,
				uint16 id)
{  
	struct response_record *rrec = NULL;

	for ((*ppsubrec) = FIRST_SUBNET; (*ppsubrec); 
				(*ppsubrec) = NEXT_SUBNET_INCLUDING_UNICAST(*ppsubrec)) {
		if((rrec = find_response_record_on_subnet(*ppsubrec, id)) != NULL)
			return rrec;
	}

	/* There should never be response records on the remote_broadcast subnet.
			Sanity check to ensure this is so. */
	if(remote_broadcast_subnet->responselist != NULL) {
		DEBUG(0,("find_response_record: response record found on subnet %s. This should \
never happen !\n", remote_broadcast_subnet->subnet_name));
	}

	/* Now check the WINS server subnet if it exists. */
	if(wins_server_subnet != NULL) {
		*ppsubrec = wins_server_subnet;
		if((rrec = find_response_record_on_subnet(*ppsubrec, id))!= NULL)
			return rrec;
	}

	DEBUG(3,("find_response_record: response packet id %hu received with no \
matching record.\n", id));
 
	*ppsubrec = NULL;

	return NULL;
}

/****************************************************************************
  Check if a refresh is queued for a particular name on a particular subnet.
  **************************************************************************/
   
BOOL is_refresh_already_queued(struct subnet_record *subrec, struct name_record *namerec)
{  
	struct response_record *rrec = NULL;
   
	for (rrec = subrec->responselist; rrec; rrec = rrec->next) {
		struct packet_struct *p = rrec->packet;
		struct nmb_packet *nmb = &p->packet.nmb;

		if((nmb->header.opcode == NMB_NAME_REFRESH_OPCODE_8) ||
				(nmb->header.opcode == NMB_NAME_REFRESH_OPCODE_9)) {
			/* Yes it's a queued refresh - check if the name is correct. */
			if(nmb_name_equal(&nmb->question.question_name, &namerec->name))
			return True;
		}
	}

	return False;
}
