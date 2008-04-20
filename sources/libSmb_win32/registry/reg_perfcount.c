/* 
 *  Unix SMB/CIFS implementation.
 *  Virtual Windows Registry Layer
 *
 *  Copyright (C) Marcin Krzysztof Porwit    2005,
 *  Copyright (C) Gerald (Jerry) Carter      2005.
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_RPC_SRV

#define PERFCOUNT_MAX_LEN 256

#define PERFCOUNTDIR	"perfmon"
#define NAMES_DB	"names.tdb"
#define DATA_DB		"data.tdb"

/*********************************************************************
*********************************************************************/

static char* counters_directory( const char *dbname )
{
	static pstring fname;
	fstring path;
	
	if ( !dbname )
		return NULL;
	
	fstr_sprintf( path, "%s/%s", PERFCOUNTDIR, dbname );
	
	pstrcpy( fname, lock_path( path ) );
	
	return fname;
}

/*********************************************************************
*********************************************************************/

void perfcount_init_keys( void )
{
	char *p = lock_path(PERFCOUNTDIR);

	/* no registry keys; just create the perfmon directory */
	
	if ( !directory_exist( p, NULL ) )
		mkdir( p, 0755 );
	
	return;
}

/*********************************************************************
*********************************************************************/

uint32 reg_perfcount_get_base_index(void)
{
	const char *fname = counters_directory( NAMES_DB );
	TDB_CONTEXT *names;
	TDB_DATA kbuf, dbuf;
	char key[] = "1";
	uint32 retval = 0;
	char buf[PERFCOUNT_MAX_LEN];

	names = tdb_open_log(fname, 0, TDB_DEFAULT, O_RDONLY, 0444);

	if ( !names ) {
		DEBUG(1, ("reg_perfcount_get_base_index: unable to open [%s].\n", fname));
		return 0;
	}    
	/* needs to read the value of key "1" from the counter_names.tdb file, as that is
	   where the total number of counters is stored. We're assuming no holes in the
	   enumeration.
	   The format for the counter_names.tdb file is:
	   key        value
	   1          num_counters
	   2          perf_counter1
	   3          perf_counter1_help
	   4          perf_counter2
	   5          perf_counter2_help
	   even_num   perf_counter<even_num>
	   even_num+1 perf_counter<even_num>_help
	   and so on.
	   So last_counter becomes num_counters*2, and last_help will be last_counter+1 */
	kbuf.dptr = key;
	kbuf.dsize = strlen(key);
	dbuf = tdb_fetch(names, kbuf);
	if(dbuf.dptr == NULL)
	{
		DEBUG(1, ("reg_perfcount_get_base_index: failed to find key \'1\' in [%s].\n", fname));
		tdb_close(names);
		return 0;
	}
	else
	{
		tdb_close(names);
		memset(buf, 0, PERFCOUNT_MAX_LEN);
		memcpy(buf, dbuf.dptr, dbuf.dsize);
		retval = (uint32)atoi(buf);
		SAFE_FREE(dbuf.dptr);
		return retval;
	}
	return 0;
}

/*********************************************************************
*********************************************************************/

uint32 reg_perfcount_get_last_counter(uint32 base_index)
{
	uint32 retval;

	if(base_index == 0)
		retval = 0;
	else
		retval = base_index * 2;

	return retval;
}

/*********************************************************************
*********************************************************************/

uint32 reg_perfcount_get_last_help(uint32 last_counter)
{
	uint32 retval;

	if(last_counter == 0)
		retval = 0;
	else
		retval = last_counter + 1;

	return retval;
}


/*********************************************************************
*********************************************************************/

static uint32 _reg_perfcount_multi_sz_from_tdb(TDB_CONTEXT *tdb, 
					       int keyval,
					       char **retbuf,
					       uint32 buffer_size)
{
	TDB_DATA kbuf, dbuf;
	char temp[256];
	char *buf1 = *retbuf;
	uint32 working_size = 0;
	UNISTR2 name_index, name;

	memset(temp, 0, sizeof(temp));
	snprintf(temp, sizeof(temp), "%d", keyval);
	kbuf.dptr = temp;
	kbuf.dsize = strlen(temp);
	dbuf = tdb_fetch(tdb, kbuf);
	if(dbuf.dptr == NULL)
	{
		/* If a key isn't there, just bypass it -- this really shouldn't 
		   happen unless someone's mucking around with the tdb */
		DEBUG(3, ("_reg_perfcount_multi_sz_from_tdb: failed to find key [%s] in [%s].\n",
			  temp, tdb_name(tdb)));
		return buffer_size;
	}
	/* First encode the name_index */
	working_size = (kbuf.dsize + 1)*sizeof(uint16);
	buf1 = SMB_REALLOC(buf1, buffer_size + working_size);
	if(!buf1) {
		buffer_size = 0;
		return buffer_size;
	}
	init_unistr2(&name_index, kbuf.dptr, UNI_STR_TERMINATE);
	memcpy(buf1+buffer_size, (char *)name_index.buffer, working_size);
	buffer_size += working_size;
	/* Now encode the actual name */
	working_size = (dbuf.dsize + 1)*sizeof(uint16);
	buf1 = SMB_REALLOC(buf1, buffer_size + working_size);
	if(!buf1) {
		buffer_size = 0;
		return buffer_size;
	}
	memset(temp, 0, sizeof(temp));
	memcpy(temp, dbuf.dptr, dbuf.dsize);
	SAFE_FREE(dbuf.dptr);
	init_unistr2(&name, temp, UNI_STR_TERMINATE);
	memcpy(buf1+buffer_size, (char *)name.buffer, working_size);
	buffer_size += working_size;

	*retbuf = buf1;

	return buffer_size;
}

/*********************************************************************
*********************************************************************/

uint32 reg_perfcount_get_counter_help(uint32 base_index, char **retbuf)
{
	char *buf1 = NULL;
	uint32 buffer_size = 0;
	TDB_CONTEXT *names;
	const char *fname = counters_directory( NAMES_DB );
	int i;

	if(base_index == 0)
		return 0;

	names = tdb_open_log(fname, 0, TDB_DEFAULT, O_RDONLY, 0444);

	if(names == NULL)
	{
		DEBUG(1, ("reg_perfcount_get_counter_help: unable to open [%s].\n", fname));
		return 0;
	}    

	for(i = 1; i <= base_index; i++)
	{
		buffer_size = _reg_perfcount_multi_sz_from_tdb(names, (i*2)+1, retbuf, buffer_size);
	}
	tdb_close(names);

	/* Now terminate the MULTI_SZ with a double unicode NULL */
	buf1 = *retbuf;
	buf1 = SMB_REALLOC(buf1, buffer_size + 2);
	if(!buf1) {
		buffer_size = 0;
	} else {
		buf1[buffer_size++] = '\0';
		buf1[buffer_size++] = '\0';
	}

	*retbuf = buf1;

	return buffer_size;
}

/*********************************************************************
*********************************************************************/

uint32 reg_perfcount_get_counter_names(uint32 base_index, char **retbuf)
{
	char *buf1 = NULL;
	uint32 buffer_size = 0;
	TDB_CONTEXT *names;
	const char *fname = counters_directory( NAMES_DB );
	int i;

	if(base_index == 0)
		return 0;

	names = tdb_open_log(fname, 0, TDB_DEFAULT, O_RDONLY, 0444);

	if(names == NULL)
	{
		DEBUG(1, ("reg_perfcount_get_counter_names: unable to open [%s].\n", fname));
		return 0;
	}    

	buffer_size = _reg_perfcount_multi_sz_from_tdb(names, 1, retbuf, buffer_size);

	for(i = 1; i <= base_index; i++)
	{
		buffer_size = _reg_perfcount_multi_sz_from_tdb(names, i*2, retbuf, buffer_size);
	}
	tdb_close(names);

	/* Now terminate the MULTI_SZ with a double unicode NULL */
	buf1 = *retbuf;
	buf1 = SMB_REALLOC(buf1, buffer_size + 2);
	if(!buf1) {
		buffer_size = 0;
	} else {
		buf1[buffer_size++] = '\0';
		buf1[buffer_size++] = '\0';
	}

	*retbuf=buf1;

	return buffer_size;
}

/*********************************************************************
*********************************************************************/

static void _reg_perfcount_make_key(TDB_DATA *key,
				    char *buf,
				    int buflen,
				    int key_part1,
				    const char *key_part2)
{
	memset(buf, 0, buflen);
	if(key_part2 != NULL)
		snprintf(buf, buflen,"%d%s", key_part1, key_part2);
	else 
		snprintf(buf, buflen, "%d", key_part1);

	key->dptr = buf;
	key->dsize = strlen(buf);

	return;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_isparent(TDB_DATA data)
{
	if(data.dsize > 0)
	{
		if(data.dptr[0] == 'p')
			return True;
		else
			return False;
	}
	return False;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_ischild(TDB_DATA data)
{
	if(data.dsize > 0)
	{
		if(data.dptr[0] == 'c')
			return True;
		else
			return False;
	}
	return False;
}

/*********************************************************************
*********************************************************************/

static uint32 _reg_perfcount_get_numinst(int objInd, TDB_CONTEXT *names)
{
	TDB_DATA key, data;
	char buf[PERFCOUNT_MAX_LEN];

	_reg_perfcount_make_key(&key, buf, PERFCOUNT_MAX_LEN, objInd, "inst");
	data = tdb_fetch(names, key);

	if(data.dptr == NULL)
		return (uint32)PERF_NO_INSTANCES;
    
	memset(buf, 0, PERFCOUNT_MAX_LEN);
	memcpy(buf, data.dptr, data.dsize);
	return (uint32)atoi(buf);
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_add_object(PERF_DATA_BLOCK *block,
				      prs_struct *ps,
				      int num,
				      TDB_DATA data,
				      TDB_CONTEXT *names)
{
	int i;
	BOOL success = False;
	PERF_OBJECT_TYPE *obj;

	block->objects = (PERF_OBJECT_TYPE *)TALLOC_REALLOC_ARRAY(ps->mem_ctx,
								  block->objects,
								  PERF_OBJECT_TYPE,
								  block->NumObjectTypes+1);
	if(block->objects == NULL)
		return False;
	obj = &(block->objects[block->NumObjectTypes]);
	memset((void *)&(block->objects[block->NumObjectTypes]), 0, sizeof(PERF_OBJECT_TYPE));
	block->objects[block->NumObjectTypes].ObjectNameTitleIndex = num;
	block->objects[block->NumObjectTypes].ObjectNameTitlePointer = 0;
	block->objects[block->NumObjectTypes].ObjectHelpTitleIndex = num+1;
	block->objects[block->NumObjectTypes].ObjectHelpTitlePointer = 0;
	block->objects[block->NumObjectTypes].NumCounters = 0;
	block->objects[block->NumObjectTypes].DefaultCounter = 0;
	block->objects[block->NumObjectTypes].NumInstances = _reg_perfcount_get_numinst(num, names);
	block->objects[block->NumObjectTypes].counters = NULL;
	block->objects[block->NumObjectTypes].instances = NULL;
	block->objects[block->NumObjectTypes].counter_data.ByteLength = sizeof(uint32);
	block->objects[block->NumObjectTypes].counter_data.data = NULL;
	block->objects[block->NumObjectTypes].DetailLevel = PERF_DETAIL_NOVICE;
	block->NumObjectTypes+=1;

	for(i = 0; i < (int)obj->NumInstances; i++)
	{
		success = _reg_perfcount_add_instance(obj, ps, i, names);
	}

	return True;
}

/*********************************************************************
*********************************************************************/

BOOL _reg_perfcount_get_counter_data(TDB_DATA key, TDB_DATA *data)
{
	TDB_CONTEXT *counters;
	const char *fname = counters_directory( DATA_DB );
    
	counters = tdb_open_log(fname, 0, TDB_DEFAULT, O_RDONLY, 0444);

	if(counters == NULL)
	{
		DEBUG(1, ("reg_perfcount_get_counter_data: unable to open [%s].\n", fname));
		return False;
	}    

	*data = tdb_fetch(counters, key);
    
	tdb_close(counters);

	return True;
}

/*********************************************************************
*********************************************************************/

static uint32 _reg_perfcount_get_size_field(uint32 CounterType)
{
	uint32 retval;

	retval = CounterType;

	/* First mask out reserved lower 8 bits */
	retval = retval & 0xFFFFFF00;
	retval = retval << 22;
	retval = retval >> 22;

	return retval;
}

/*********************************************************************
*********************************************************************/

static uint32 _reg_perfcount_compute_scale(SMB_BIG_INT data)
{
	int scale = 0;
	if(data == 0)
		return scale;
	while(data > 100)
	{
		data /= 10;
		scale--;
	}
	while(data < 10)
	{
		data *= 10;
		scale++;
	}

	return (uint32)scale;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_get_counter_info(PERF_DATA_BLOCK *block,
					    prs_struct *ps,
					    int CounterIndex,
					    PERF_OBJECT_TYPE *obj,
					    TDB_CONTEXT *names)
{
	TDB_DATA key, data;
	char buf[PERFCOUNT_MAX_LEN];
	size_t dsize, padding;
	long int data32, dbuf[2];
	SMB_BIG_INT data64;
	uint32 counter_size;

	obj->counters[obj->NumCounters].DefaultScale = 0;
	dbuf[0] = dbuf[1] = 0;
	padding = 0;

	_reg_perfcount_make_key(&key, buf, PERFCOUNT_MAX_LEN, CounterIndex, "type");
	data = tdb_fetch(names, key);
	if(data.dptr == NULL)
	{
		DEBUG(3, ("_reg_perfcount_get_counter_info: No type data for counter [%d].\n", CounterIndex));
		return False;
	}
	memset(buf, 0, PERFCOUNT_MAX_LEN);
	memcpy(buf, data.dptr, data.dsize);
	obj->counters[obj->NumCounters].CounterType = atoi(buf);
	DEBUG(10, ("_reg_perfcount_get_counter_info: Got type [%d] for counter [%d].\n",
		   obj->counters[obj->NumCounters].CounterType, CounterIndex));
	SAFE_FREE(data.dptr);

	/* Fetch the actual data */
	_reg_perfcount_make_key(&key, buf, PERFCOUNT_MAX_LEN, CounterIndex, "");
	_reg_perfcount_get_counter_data(key, &data);
	if(data.dptr == NULL)
	{
		DEBUG(3, ("_reg_perfcount_get_counter_info: No counter data for counter [%d].\n", CounterIndex));
		return False;
	}
    
	counter_size = _reg_perfcount_get_size_field(obj->counters[obj->NumCounters].CounterType);

	if(counter_size == PERF_SIZE_DWORD)
	{
		dsize = sizeof(data32);
		memset(buf, 0, PERFCOUNT_MAX_LEN);
		memcpy(buf, data.dptr, data.dsize);
		data32 = strtol(buf, NULL, 0);
		if((obj->counters[obj->NumCounters].CounterType & 0x00000F00) == PERF_TYPE_NUMBER)
			obj->counters[obj->NumCounters].DefaultScale = _reg_perfcount_compute_scale((SMB_BIG_INT)data32);
		else
			obj->counters[obj->NumCounters].DefaultScale = 0;
		dbuf[0] = data32;
		padding = (dsize - (obj->counter_data.ByteLength%dsize)) % dsize;
	}
	else if(counter_size == PERF_SIZE_LARGE)
	{
		dsize = sizeof(data64);
		memset(buf, 0, PERFCOUNT_MAX_LEN);
		memcpy(buf, data.dptr, data.dsize);
		data64 = atof(buf);
		if((obj->counters[obj->NumCounters].CounterType & 0x00000F00) == PERF_TYPE_NUMBER)
			obj->counters[obj->NumCounters].DefaultScale = _reg_perfcount_compute_scale(data64);
		else
			obj->counters[obj->NumCounters].DefaultScale = 0;
		memcpy((void *)dbuf, (const void *)&data64, dsize);
		padding = (dsize - (obj->counter_data.ByteLength%dsize)) % dsize;
	}
	else /* PERF_SIZE_VARIABLE_LEN */
	{
		dsize = data.dsize;
		memset(buf, 0, PERFCOUNT_MAX_LEN);
		memcpy(buf, data.dptr, data.dsize);
	}
	SAFE_FREE(data.dptr);

	obj->counter_data.ByteLength += dsize + padding;
	obj->counter_data.data = TALLOC_REALLOC_ARRAY(ps->mem_ctx,
						      obj->counter_data.data,
						      uint8,
						      obj->counter_data.ByteLength - sizeof(uint32));
	if(obj->counter_data.data == NULL)
		return False;
	if(dbuf[0] != 0 || dbuf[1] != 0)
	{
		memcpy((void *)(obj->counter_data.data + 
				(obj->counter_data.ByteLength - (sizeof(uint32) + dsize))), 
		       (const void *)dbuf, dsize);
	}
	else
	{
		/* Handling PERF_SIZE_VARIABLE_LEN */
		memcpy((void *)(obj->counter_data.data +
				(obj->counter_data.ByteLength - (sizeof(uint32) + dsize))),
		       (const void *)buf, dsize);
	}
	obj->counters[obj->NumCounters].CounterOffset = obj->counter_data.ByteLength - dsize;
	if(obj->counters[obj->NumCounters].CounterOffset % dsize != 0)
	{
		DEBUG(3,("Improperly aligned counter [%d]\n", obj->NumCounters));
	}
	obj->counters[obj->NumCounters].CounterSize = dsize;

	return True;
}

/*********************************************************************
*********************************************************************/

PERF_OBJECT_TYPE *_reg_perfcount_find_obj(PERF_DATA_BLOCK *block, int objind)
{
	int i;

	PERF_OBJECT_TYPE *obj = NULL;

	for(i = 0; i < block->NumObjectTypes; i++)
	{
		if(block->objects[i].ObjectNameTitleIndex == objind)
		{
			obj = &(block->objects[i]);
		}
	}

	return obj;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_add_counter(PERF_DATA_BLOCK *block,
				       prs_struct *ps,
				       int num,
				       TDB_DATA data,
				       TDB_CONTEXT *names)
{
	char *begin, *end, *start, *stop;
	int parent;
	PERF_OBJECT_TYPE *obj;
	BOOL success = False;
	char buf[PERFCOUNT_MAX_LEN];
    
	obj = NULL;
	memset(buf, 0, PERFCOUNT_MAX_LEN);
	memcpy(buf, data.dptr, data.dsize);
	begin = index(buf, '[');
	end = index(buf, ']');
	if(begin == NULL || end == NULL)
		return False;
	start = begin+1;

	while(start < end)
	{
		stop = index(start, ',');
		if(stop == NULL)
			stop = end;
		*stop = '\0';
		parent = atoi(start);

		obj = _reg_perfcount_find_obj(block, parent);
		if(obj == NULL)
		{
			/* At this point we require that the parent object exist.
			   This can probably be handled better at some later time */
			DEBUG(3, ("_reg_perfcount_add_counter: Could not find parent object [%d] for counter [%d].\n",
				  parent, num));
			return False;
		}
		obj->counters = (PERF_COUNTER_DEFINITION *)TALLOC_REALLOC_ARRAY(ps->mem_ctx,
										obj->counters,
										PERF_COUNTER_DEFINITION,
										obj->NumCounters+1);
		if(obj->counters == NULL)
			return False;
		memset((void *)&(obj->counters[obj->NumCounters]), 0, sizeof(PERF_COUNTER_DEFINITION));
		obj->counters[obj->NumCounters].CounterNameTitleIndex=num;
		obj->counters[obj->NumCounters].CounterHelpTitleIndex=num+1;
		obj->counters[obj->NumCounters].DetailLevel = PERF_DETAIL_NOVICE;
		obj->counters[obj->NumCounters].ByteLength = sizeof(PERF_COUNTER_DEFINITION);
		success = _reg_perfcount_get_counter_info(block, ps, num, obj, names);
		obj->NumCounters += 1;
		start = stop + 1;
	}
    	
	/* Handle case of Objects/Counters without any counter data, which would suggest
	   that the required instances are not there yet, so change NumInstances from
	   PERF_NO_INSTANCES to 0 */

	return True;
}

/*********************************************************************
*********************************************************************/

BOOL _reg_perfcount_get_instance_info(PERF_INSTANCE_DEFINITION *inst,
				      prs_struct *ps,
				      int instId,
				      PERF_OBJECT_TYPE *obj,
				      TDB_CONTEXT *names)
{
	TDB_DATA key, data;
	char buf[PERFCOUNT_MAX_LEN], temp[PERFCOUNT_MAX_LEN];
	wpstring name;
	int pad;

	/* First grab the instance data from the data file */
	memset(temp, 0, PERFCOUNT_MAX_LEN);
	snprintf(temp, PERFCOUNT_MAX_LEN, "i%d", instId);
	_reg_perfcount_make_key(&key, buf, PERFCOUNT_MAX_LEN, obj->ObjectNameTitleIndex, temp);
	_reg_perfcount_get_counter_data(key, &data);
	if(data.dptr == NULL)
	{
		DEBUG(3, ("_reg_perfcount_get_instance_info: No instance data for instance [%s].\n",
			  buf));
		return False;
	}
	inst->counter_data.ByteLength = data.dsize + sizeof(inst->counter_data.ByteLength);
	inst->counter_data.data = TALLOC_REALLOC_ARRAY(ps->mem_ctx,
						       inst->counter_data.data,
						       uint8,
						       data.dsize);
	if(inst->counter_data.data == NULL)
		return False;
	memset(inst->counter_data.data, 0, data.dsize);
	memcpy(inst->counter_data.data, data.dptr, data.dsize);
	SAFE_FREE(data.dptr);

	/* Fetch instance name */
	memset(temp, 0, PERFCOUNT_MAX_LEN);
	snprintf(temp, PERFCOUNT_MAX_LEN, "i%dname", instId);
	_reg_perfcount_make_key(&key, buf, PERFCOUNT_MAX_LEN, obj->ObjectNameTitleIndex, temp);
	data = tdb_fetch(names, key);
	if(data.dptr == NULL)
	{
		/* Not actually an error, but possibly unintended? -- just logging FYI */
		DEBUG(3, ("_reg_perfcount_get_instance_info: No instance name for instance [%s].\n",
			  buf));
		inst->NameLength = 0;
	}
	else
	{
		memset(buf, 0, PERFCOUNT_MAX_LEN);
		memcpy(buf, data.dptr, data.dsize);
		rpcstr_push((void *)name, buf, sizeof(name), STR_TERMINATE);
		inst->NameLength = (strlen_w(name) * 2) + 2;
		inst->data = TALLOC_REALLOC_ARRAY(ps->mem_ctx,
						  inst->data,
						  uint8,
						  inst->NameLength);
		if (inst->data == NULL) {
			SAFE_FREE(data.dptr);
			return False;
		}
		memcpy(inst->data, name, inst->NameLength);
		SAFE_FREE(data.dptr);
	}

	inst->ParentObjectTitleIndex = 0;
	inst->ParentObjectTitlePointer = 0;
	inst->UniqueID = PERF_NO_UNIQUE_ID;
	inst->NameOffset = 6 * sizeof(uint32);
    
	inst->ByteLength = inst->NameOffset + inst->NameLength;
	/* Need to be aligned on a 64-bit boundary here for counter_data */
	if((pad = (inst->ByteLength % 8)))
	{
		pad = 8 - pad;
		inst->data = TALLOC_REALLOC_ARRAY(ps->mem_ctx,
						  inst->data,
						  uint8,
						  inst->NameLength + pad);
		memset(inst->data + inst->NameLength, 0, pad);
		inst->ByteLength += pad;
	}

	return True;
}

/*********************************************************************
*********************************************************************/

BOOL _reg_perfcount_add_instance(PERF_OBJECT_TYPE *obj,
				 prs_struct *ps,
				 int instInd,
				 TDB_CONTEXT *names)
{
	BOOL success;
	PERF_INSTANCE_DEFINITION *inst;

	success = False;

	if(obj->instances == NULL)
	{
		obj->instances = TALLOC_REALLOC_ARRAY(ps->mem_ctx, 
						      obj->instances,
						      PERF_INSTANCE_DEFINITION,
						      obj->NumInstances);
	}
	if(obj->instances == NULL)
		return False;
    
	memset(&(obj->instances[instInd]), 0, sizeof(PERF_INSTANCE_DEFINITION));
	inst = &(obj->instances[instInd]);
	success = _reg_perfcount_get_instance_info(inst, ps, instInd, obj, names);
    
	return True;
}

/*********************************************************************
*********************************************************************/

static int _reg_perfcount_assemble_global(PERF_DATA_BLOCK *block,
					  prs_struct *ps,
					  int base_index,
					  TDB_CONTEXT *names)
{
	BOOL success;
	int i, j, retval = 0;
	char keybuf[PERFCOUNT_MAX_LEN];
	TDB_DATA key, data;

	for(i = 1; i <= base_index; i++)
	{
		j = i*2;
		_reg_perfcount_make_key(&key, keybuf, PERFCOUNT_MAX_LEN, j, "rel");
		data = tdb_fetch(names, key);
		if(data.dptr != NULL)
		{
			if(_reg_perfcount_isparent(data))
				success = _reg_perfcount_add_object(block, ps, j, data, names);
			else if(_reg_perfcount_ischild(data))
				success = _reg_perfcount_add_counter(block, ps, j, data, names);
			else
			{
				DEBUG(3, ("Bogus relationship [%s] for counter [%d].\n", data.dptr, j));
				success = False;
			}
			if(success == False)
			{
				DEBUG(3, ("_reg_perfcount_assemble_global: Failed to add new relationship for counter [%d].\n", j));
				retval = -1;
			}
			SAFE_FREE(data.dptr);
		}
		else
			DEBUG(3, ("NULL relationship for counter [%d] using key [%s].\n", j, keybuf));
	}	
	return retval;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_get_64(SMB_BIG_UINT *retval,
				  TDB_CONTEXT *tdb,
				  int key_part1,
				  const char *key_part2)
{
	TDB_DATA key, data;
	char buf[PERFCOUNT_MAX_LEN];

	_reg_perfcount_make_key(&key, buf, PERFCOUNT_MAX_LEN, key_part1, key_part2);

	data = tdb_fetch(tdb, key);
	if(data.dptr == NULL)
	{
		DEBUG(3,("_reg_perfcount_get_64: No data found for key [%s].\n", key.dptr));
		return False;
	}

	memset(buf, 0, PERFCOUNT_MAX_LEN);
	memcpy(buf, data.dptr, data.dsize);
	SAFE_FREE(data.dptr);

	*retval = atof(buf);

	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_init_data_block_perf(PERF_DATA_BLOCK *block,
						TDB_CONTEXT *names)
{
	SMB_BIG_UINT PerfFreq, PerfTime, PerfTime100nSec;
	TDB_CONTEXT *counters;
	BOOL status = False;
	const char *fname = counters_directory( DATA_DB );
    
	counters = tdb_open_log(fname, 0, TDB_DEFAULT, O_RDONLY, 0444);
    
	if(counters == NULL)
	{
		DEBUG(1, ("reg_perfcount_init_data_block_perf: unable to open [%s].\n", fname));
		return False;
	}    
    
	status = _reg_perfcount_get_64(&PerfFreq, names, 0, "PerfFreq");
	if(status == False)
	{
		tdb_close(counters);
		return status;
	}
	memcpy((void *)&(block->PerfFreq), (const void *)&PerfFreq, sizeof(PerfFreq));

	status = _reg_perfcount_get_64(&PerfTime, counters, 0, "PerfTime");
	if(status == False)
	{
		tdb_close(counters);
		return status;
	}
	memcpy((void *)&(block->PerfTime), (const void *)&PerfTime, sizeof(PerfTime));

	status = _reg_perfcount_get_64(&PerfTime100nSec, counters, 0, "PerfTime100nSec");
	if(status == False)
	{
		tdb_close(counters);
		return status;
	}
	memcpy((void *)&(block->PerfTime100nSec), (const void *)&PerfTime100nSec, sizeof(PerfTime100nSec));

	tdb_close(counters);
	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_init_data_block(PERF_DATA_BLOCK *block,
					   prs_struct *ps, TDB_CONTEXT *names)
{
	wpstring temp;
	time_t tm;
 
	memset(temp, 0, sizeof(temp));
	rpcstr_push((void *)temp, "PERF", sizeof(temp), STR_TERMINATE);
	memcpy(block->Signature, temp, strlen_w(temp) *2);

	if(ps->bigendian_data == RPC_BIG_ENDIAN)
		block->LittleEndian = 0;
	else
		block->LittleEndian = 1;
	block->Version = 1;
	block->Revision = 1;
	block->TotalByteLength = 0;
	block->NumObjectTypes = 0;
	block->DefaultObject = -1;
	block->objects = NULL;
	tm = time(NULL);
	make_systemtime(&(block->SystemTime), gmtime(&tm));
	_reg_perfcount_init_data_block_perf(block, names);
	memset(temp, 0, sizeof(temp));
	rpcstr_push((void *)temp, global_myname(), sizeof(temp), STR_TERMINATE);
	block->SystemNameLength = (strlen_w(temp) * 2) + 2;
	block->data = TALLOC_ZERO_ARRAY(ps->mem_ctx, uint8, block->SystemNameLength + (8 - (block->SystemNameLength % 8)));
	if (block->data == NULL) {
		return False;
	}
	memcpy(block->data, temp, block->SystemNameLength);
	block->SystemNameOffset = sizeof(PERF_DATA_BLOCK) - sizeof(block->objects) - sizeof(block->data); 
	block->HeaderLength = block->SystemNameOffset + block->SystemNameLength;
	/* Make sure to adjust for 64-bit alignment for when we finish writing the system name,
	   so that the PERF_OBJECT_TYPE struct comes out 64-bit aligned */
	block->HeaderLength += 8 - (block->HeaderLength % 8);

	return True;
}

/*********************************************************************
*********************************************************************/

static uint32 _reg_perfcount_perf_data_block_fixup(PERF_DATA_BLOCK *block, prs_struct *ps)
{
	int obj, cnt, inst, pad, i;
	PERF_OBJECT_TYPE *object;
	PERF_INSTANCE_DEFINITION *instance;
	PERF_COUNTER_DEFINITION *counter;
	PERF_COUNTER_BLOCK *counter_data;
	char *temp = NULL, *src_addr, *dst_addr;

	block->TotalByteLength = 0;
	object = block->objects;
	for(obj = 0; obj < block->NumObjectTypes; obj++)
	{
		object[obj].TotalByteLength = 0;
		object[obj].DefinitionLength = 0;
		instance = object[obj].instances;
		counter = object[obj].counters;
		for(cnt = 0; cnt < object[obj].NumCounters; cnt++)
		{
			object[obj].TotalByteLength += counter[cnt].ByteLength;
			object[obj].DefinitionLength += counter[cnt].ByteLength;
		}
		if(object[obj].NumInstances != PERF_NO_INSTANCES)
		{
			for(inst = 0; inst < object[obj].NumInstances; inst++)
			{
				instance = &(object[obj].instances[inst]);
				object[obj].TotalByteLength += instance->ByteLength;
				counter_data = &(instance->counter_data);
				counter = &(object[obj].counters[object[obj].NumCounters - 1]);
				counter_data->ByteLength = counter->CounterOffset + counter->CounterSize + sizeof(counter_data->ByteLength);
				temp = TALLOC_REALLOC_ARRAY(ps->mem_ctx, 
							    temp, 
							    char, 
							    counter_data->ByteLength- sizeof(counter_data->ByteLength));
				if (temp == NULL) {
					return 0;
				}
				memset(temp, 0, counter_data->ByteLength - sizeof(counter_data->ByteLength));
				src_addr = (char *)counter_data->data;
				for(i = 0; i < object[obj].NumCounters; i++)
				{
					counter = &(object[obj].counters[i]);
					dst_addr = temp + counter->CounterOffset - sizeof(counter_data->ByteLength);
					memcpy(dst_addr, src_addr, counter->CounterSize);
				        src_addr += counter->CounterSize;
				}
				/* Make sure to be 64-bit aligned */
				if((pad = (counter_data->ByteLength % 8)))
				{
					pad = 8 - pad;
				}
				counter_data->data = TALLOC_REALLOC_ARRAY(ps->mem_ctx,
									 counter_data->data,
									 uint8,
									 counter_data->ByteLength - sizeof(counter_data->ByteLength) + pad);
				if (counter_data->data == NULL) {
					return 0;
				}
				memset(counter_data->data, 0, counter_data->ByteLength - sizeof(counter_data->ByteLength) + pad);
				memcpy(counter_data->data, temp, counter_data->ByteLength - sizeof(counter_data->ByteLength));
				counter_data->ByteLength += pad;
				object[obj].TotalByteLength += counter_data->ByteLength;
			}
		}
		else
		{
			/* Need to be 64-bit aligned at the end of the counter_data block, so pad counter_data to a 64-bit boundary,
			   so that the next PERF_OBJECT_TYPE can start on a 64-bit alignment */
			if((pad = (object[obj].counter_data.ByteLength % 8)))
			{
				pad = 8 - pad;
				object[obj].counter_data.data = TALLOC_REALLOC_ARRAY(ps->mem_ctx, 
										     object[obj].counter_data.data,
										     uint8, 
										     object[obj].counter_data.ByteLength + pad);
				memset((void *)(object[obj].counter_data.data + object[obj].counter_data.ByteLength), 0, pad);
				object[obj].counter_data.ByteLength += pad;
			}
			object[obj].TotalByteLength += object[obj].counter_data.ByteLength;
		}
		object[obj].HeaderLength = sizeof(*object) - (sizeof(counter) + sizeof(instance) + sizeof(PERF_COUNTER_BLOCK));
		object[obj].TotalByteLength += object[obj].HeaderLength;
		object[obj].DefinitionLength += object[obj].HeaderLength;
		
		block->TotalByteLength += object[obj].TotalByteLength;
	}

	return block->TotalByteLength;
}

/*********************************************************************
*********************************************************************/

uint32 reg_perfcount_get_perf_data_block(uint32 base_index, 
					 prs_struct *ps, 
					 PERF_DATA_BLOCK *block,
					 char *object_ids)
{
	uint32 buffer_size = 0;
	const char *fname = counters_directory( NAMES_DB );
	TDB_CONTEXT *names;
	int retval = 0;
	
	names = tdb_open_log(fname, 0, TDB_DEFAULT, O_RDONLY, 0444);

	if(names == NULL)
	{
		DEBUG(1, ("reg_perfcount_get_perf_data_block: unable to open [%s].\n", fname));
		return 0;
	}

	if (!_reg_perfcount_init_data_block(block, ps, names)) {
		DEBUG(0, ("_reg_perfcount_init_data_block failed\n"));
		tdb_close(names);
		return 0;
	}

	reg_perfcount_get_last_counter(base_index);
    
	if(object_ids == NULL)
	{
		/* we're getting a request for "Global" here */
		retval = _reg_perfcount_assemble_global(block, ps, base_index, names);
	}
	else
	{
		/* we're getting a request for a specific set of PERF_OBJECT_TYPES */
		retval = _reg_perfcount_assemble_global(block, ps, base_index, names);
	}
	buffer_size = _reg_perfcount_perf_data_block_fixup(block, ps);

	tdb_close(names);

	if (retval == -1) {
		return 0;
	}

	return buffer_size + block->HeaderLength;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_marshall_perf_data_block(prs_struct *ps, PERF_DATA_BLOCK block, int depth)
{
	int i;
	prs_debug(ps, depth, "", "_reg_perfcount_marshall_perf_data_block");
	depth++;

	if(!prs_align(ps))
		return False;
	for(i = 0; i < 4; i++)
	{
		if(!prs_uint16("Signature", ps, depth, &block.Signature[i]))
			return False;
	}
	if(!prs_uint32("Little Endian", ps, depth, &block.LittleEndian))
		return False;
	if(!prs_uint32("Version", ps, depth, &block.Version))
		return False;
	if(!prs_uint32("Revision", ps, depth, &block.Revision))
		return False;
	if(!prs_uint32("TotalByteLength", ps, depth, &block.TotalByteLength))
		return False;
	if(!prs_uint32("HeaderLength", ps, depth, &block.HeaderLength))
		return False;
	if(!prs_uint32("NumObjectTypes", ps, depth, &block.NumObjectTypes))
		return False;
	if(!prs_uint32("DefaultObject", ps, depth, &block.DefaultObject))
		return False;
	if(!spoolss_io_system_time("SystemTime", ps, depth, &block.SystemTime))
		return False;
	if(!prs_uint32("Padding", ps, depth, &block.Padding))
		return False;
	if(!prs_align_uint64(ps))
		return False;
	if(!prs_uint64("PerfTime", ps, depth, &block.PerfTime))
		return False;
	if(!prs_uint64("PerfFreq", ps, depth, &block.PerfFreq))
		return False;
	if(!prs_uint64("PerfTime100nSec", ps, depth, &block.PerfTime100nSec))
		return False;
	if(!prs_uint32("SystemNameLength", ps, depth, &block.SystemNameLength))
		return False;
	if(!prs_uint32("SystemNameOffset", ps, depth, &block.SystemNameOffset))
		return False;
	/* hack to make sure we're 64-bit aligned at the end of this whole mess */
	if(!prs_uint8s(False, "SystemName", ps, depth, block.data, 
		       block.HeaderLength - block.SystemNameOffset)) 
		return False;

	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_marshall_perf_counters(prs_struct *ps,
						  PERF_OBJECT_TYPE object,
						  int depth)
{
	int cnt;
	PERF_COUNTER_DEFINITION counter;

	prs_debug(ps, depth, "", "_reg_perfcount_marshall_perf_counters");
	depth++;
    
	for(cnt = 0; cnt < object.NumCounters; cnt++)
	{
		counter = object.counters[cnt];

		if(!prs_align(ps))
			return False;
		if(!prs_uint32("ByteLength", ps, depth, &counter.ByteLength))
			return False;
		if(!prs_uint32("CounterNameTitleIndex", ps, depth, &counter.CounterNameTitleIndex))
			return False;
		if(!prs_uint32("CounterNameTitlePointer", ps, depth, &counter.CounterNameTitlePointer))
			return False;
		if(!prs_uint32("CounterHelpTitleIndex", ps, depth, &counter.CounterHelpTitleIndex))
			return False;
		if(!prs_uint32("CounterHelpTitlePointer", ps, depth, &counter.CounterHelpTitlePointer))
			return False;
		if(!prs_uint32("DefaultScale", ps, depth, &counter.DefaultScale))
			return False;
		if(!prs_uint32("DetailLevel", ps, depth, &counter.DetailLevel))
			return False;
		if(!prs_uint32("CounterType", ps, depth, &counter.CounterType))
			return False;
		if(!prs_uint32("CounterSize", ps, depth, &counter.CounterSize))
			return False;
		if(!prs_uint32("CounterOffset", ps, depth, &counter.CounterOffset))
			return False;
	}

	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_marshall_perf_counter_data(prs_struct *ps, 
						      PERF_COUNTER_BLOCK counter_data, 
						      int depth)
{
	prs_debug(ps, depth, "", "_reg_perfcount_marshall_perf_counter_data");
	depth++;
    
	if(!prs_align_uint64(ps))
		return False;
    
	if(!prs_uint32("ByteLength", ps, depth, &counter_data.ByteLength))
		return False;
	if(!prs_uint8s(False, "CounterData", ps, depth, counter_data.data, counter_data.ByteLength - sizeof(uint32)))
		return False;
	if(!prs_align_uint64(ps))
		return False;

	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_marshall_perf_instances(prs_struct *ps,
						   PERF_OBJECT_TYPE object, 
						   int depth)
{
	PERF_INSTANCE_DEFINITION instance;
	int inst;

	prs_debug(ps, depth, "", "_reg_perfcount_marshall_perf_instances");
	depth++;

	for(inst = 0; inst < object.NumInstances; inst++)
	{
		instance = object.instances[inst];

		if(!prs_align(ps))
			return False;
		if(!prs_uint32("ByteLength", ps, depth, &instance.ByteLength))
			return False;
		if(!prs_uint32("ParentObjectTitleIndex", ps, depth, &instance.ParentObjectTitleIndex))
			return False;
		if(!prs_uint32("ParentObjectTitlePointer", ps, depth, &instance.ParentObjectTitlePointer))
			return False;
		if(!prs_uint32("UniqueID", ps, depth, &instance.UniqueID))
			return False;
		if(!prs_uint32("NameOffset", ps, depth, &instance.NameOffset))
			return False;
		if(!prs_uint32("NameLength", ps, depth, &instance.NameLength))
			return False;
		if(!prs_uint8s(False, "InstanceName", ps, depth, instance.data,
			       instance.ByteLength - instance.NameOffset))
			return False;
		if(_reg_perfcount_marshall_perf_counter_data(ps, instance.counter_data, depth) == False)
			return False;
	}
	
	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_marshall_perf_objects(prs_struct *ps, PERF_DATA_BLOCK block, int depth)
{
	int obj;

	PERF_OBJECT_TYPE object;
    
	prs_debug(ps, depth, "", "_reg_perfcount_marshall_perf_objects");
	depth++;

	for(obj = 0; obj < block.NumObjectTypes; obj++)
	{
		object = block.objects[obj];

		if(!prs_align(ps))
			return False;

		if(!prs_uint32("TotalByteLength", ps, depth, &object.TotalByteLength))
			return False;
		if(!prs_uint32("DefinitionLength", ps, depth, &object.DefinitionLength))
			return False;
		if(!prs_uint32("HeaderLength", ps, depth, &object.HeaderLength))
			return False;
		if(!prs_uint32("ObjectNameTitleIndex", ps, depth, &object.ObjectNameTitleIndex))
			return False;
		if(!prs_uint32("ObjectNameTitlePointer", ps, depth, &object.ObjectNameTitlePointer))
			return False;
		if(!prs_uint32("ObjectHelpTitleIndex", ps, depth, &object.ObjectHelpTitleIndex))
			return False;
		if(!prs_uint32("ObjectHelpTitlePointer", ps, depth, &object.ObjectHelpTitlePointer))
			return False;
		if(!prs_uint32("DetailLevel", ps, depth, &object.DetailLevel))
			return False;
		if(!prs_uint32("NumCounters", ps, depth, &object.NumCounters))
			return False;
		if(!prs_uint32("DefaultCounter", ps, depth, &object.DefaultCounter))
			return False;
		if(!prs_uint32("NumInstances", ps, depth, &object.NumInstances))
			return False;
		if(!prs_uint32("CodePage", ps, depth, &object.CodePage))
			return False;
		if(!prs_align_uint64(ps))
			return False;
		if(!prs_uint64("PerfTime", ps, depth, &object.PerfTime))
			return False;
		if(!prs_uint64("PerfFreq", ps, depth, &object.PerfFreq))
			return False;

		/* Now do the counters */
		/* If no instances, encode counter_data */
		/* If instances, encode instace plus counter data for each instance */
		if(_reg_perfcount_marshall_perf_counters(ps, object, depth) == False)
			return False;
		if(object.NumInstances == PERF_NO_INSTANCES)
		{
			if(_reg_perfcount_marshall_perf_counter_data(ps, object.counter_data, depth) == False)
				return False;
		}
		else
		{
			if(_reg_perfcount_marshall_perf_instances(ps, object, depth) == False)
				return False;
		}
	}

	return True;
}

/*********************************************************************
*********************************************************************/

static BOOL _reg_perfcount_marshall_hkpd(prs_struct *ps, PERF_DATA_BLOCK block)
{
	int depth = 0;
	if(_reg_perfcount_marshall_perf_data_block(ps, block, depth) == True)
	{
		if(_reg_perfcount_marshall_perf_objects(ps, block, depth) == True)
			return True;
	}
	return False;
}

/*********************************************************************
*********************************************************************/

WERROR reg_perfcount_get_hkpd(prs_struct *ps, uint32 max_buf_size, uint32 *outbuf_len, char *object_ids)
{
	/*
	 * For a detailed description of the layout of this structure,
	 * see http://msdn.microsoft.com/library/default.asp?url=/library/en-us/perfmon/base/performance_data_format.asp
	 */
	PERF_DATA_BLOCK block;
	uint32 buffer_size, base_index; 
    
	buffer_size = 0;
	base_index = reg_perfcount_get_base_index();
	ZERO_STRUCT(block);

	buffer_size = reg_perfcount_get_perf_data_block(base_index, ps, &block, object_ids);

	if(buffer_size < max_buf_size)
	{
		*outbuf_len = buffer_size;
		if(_reg_perfcount_marshall_hkpd(ps, block) == True)
			return WERR_OK;
		else
			return WERR_NOMEM;
	}
	else
	{
		*outbuf_len = max_buf_size;
		_reg_perfcount_marshall_perf_data_block(ps, block, 0);
		return WERR_INSUFFICIENT_BUFFER;
	}
}    
