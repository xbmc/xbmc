#include <stdlib.h>
#include <string.h>
#include "mp4ffint.h"

#ifdef USE_TAGGING

static uint32_t fix_byte_order_32(uint32_t src)
{
    uint32_t result;
    uint32_t a, b, c, d;
    int8_t data[4];
    
    memcpy(data,&src,sizeof(src));
    a = (uint8_t)data[0];
    b = (uint8_t)data[1];
    c = (uint8_t)data[2];
    d = (uint8_t)data[3];

    result = (a<<24) | (b<<16) | (c<<8) | d;
    return (uint32_t)result;
}

static uint16_t fix_byte_order_16(uint16_t src)
{
    uint16_t result;
    uint16_t a, b;
    int8_t data[2];
    
    memcpy(data,&src,sizeof(src));
    a = (uint8_t)data[0];
    b = (uint8_t)data[1];

    result = (a<<8) | b;
    return (uint16_t)result;
}


typedef struct
{
	void * data;
	unsigned written;
	unsigned allocated;
	unsigned error;
} membuffer;

unsigned membuffer_write(membuffer * buf,const void * ptr,unsigned bytes)
{
	unsigned dest_size = buf->written + bytes;

	if (buf->error) return 0;
	if (dest_size > buf->allocated)
	{
		do
		{
			buf->allocated <<= 1;
		} while(dest_size > buf->allocated);
		
		{
			void * newptr = realloc(buf->data,buf->allocated);
			if (newptr==0)
			{
				free(buf->data);
				buf->data = 0;
				buf->error = 1;
				return 0;
			}
			buf->data = newptr;
		}
	}

	if (ptr) memcpy((char*)buf->data + buf->written,ptr,bytes);
	buf->written += bytes;
	return bytes;
}

#define membuffer_write_data membuffer_write

unsigned membuffer_write_int32(membuffer * buf,uint32_t data)
{
	uint8_t temp[4] = {(uint8_t)(data>>24),(uint8_t)(data>>16),(uint8_t)(data>>8),(uint8_t)data};	
	return membuffer_write_data(buf,temp,4);
}

unsigned membuffer_write_int24(membuffer * buf,uint32_t data)
{
	uint8_t temp[3] = {(uint8_t)(data>>16),(uint8_t)(data>>8),(uint8_t)data};
	return membuffer_write_data(buf,temp,3);
}

unsigned membuffer_write_int16(membuffer * buf,uint16_t data)
{
	uint8_t temp[2] = {(uint8_t)(data>>8),(uint8_t)data};
	return membuffer_write_data(buf,temp,2);
}

unsigned membuffer_write_atom_name(membuffer * buf,const char * data)
{
	return membuffer_write_data(buf,data,4)==4 ? 1 : 0;
}

void membuffer_write_atom(membuffer * buf,const char * name,unsigned size,const void * data)
{
	membuffer_write_int32(buf,size + 8);
	membuffer_write_atom_name(buf,name);
	membuffer_write_data(buf,data,size);
}

unsigned membuffer_write_string(membuffer * buf,const char * data)
{
	return membuffer_write_data(buf,data,strlen(data));
}

unsigned membuffer_write_int8(membuffer * buf,uint8_t data)
{
	return membuffer_write_data(buf,&data,1);
}

void * membuffer_get_ptr(const membuffer * buf)
{
	return buf->data;
}

unsigned membuffer_get_size(const membuffer * buf)
{
	return buf->written;
}

unsigned membuffer_error(const membuffer * buf)
{
	return buf->error;
}

void membuffer_set_error(membuffer * buf) {buf->error = 1;}

unsigned membuffer_transfer_from_file(membuffer * buf,mp4ff_t * src,unsigned bytes)
{
	unsigned oldsize;
	void * bufptr;
	
	oldsize = membuffer_get_size(buf);
	if (membuffer_write_data(buf,0,bytes) != bytes) return 0;

	bufptr = membuffer_get_ptr(buf);
	if (bufptr==0) return 0;
	
	if ((unsigned)mp4ff_read_data(src,(char*)bufptr + oldsize,bytes)!=bytes)
	{
		membuffer_set_error(buf);
		return 0;
	}
	
	return bytes;
}


membuffer * membuffer_create()
{
	const unsigned initial_size = 256;

	membuffer * buf = (membuffer *) malloc(sizeof(membuffer));
	buf->data = malloc(initial_size);
	buf->written = 0;
	buf->allocated = initial_size;
	buf->error = buf->data == 0 ? 1 : 0;

	return buf;
}

void membuffer_free(membuffer * buf)
{
	if (buf->data) free(buf->data);
	free(buf);
}

void * membuffer_detach(membuffer * buf)
{
	void * ret;

	if (buf->error) return 0;

	ret = realloc(buf->data,buf->written);
	
	if (ret == 0) free(buf->data);

	buf->data = 0;
	buf->error = 1;
	
	return ret;
}

#if 0
/* metadata tag structure */
typedef struct
{
    char *item;
    char *value;
} mp4ff_tag_t;

/* metadata list structure */
typedef struct
{
    mp4ff_tag_t *tags;
    uint32_t count;
} mp4ff_metadata_t;
#endif

typedef struct
{
	const char * atom;
	const char * name;	
} stdmeta_entry;

static stdmeta_entry stdmetas[] = 
{
	{"©nam","title"},
	{"©ART","artist"},
	{"©wrt","writer"},
	{"©alb","album"},
	{"©day","date"},
	{"©too","tool"},
	{"©cmt","comment"},
//	{"©gen","genre"},
	{"cpil","compilation"},
//	{"trkn","track"},
//	{"disk","disc"},
//	{"gnre","genre"},
	{"covr","cover"},
};


static const char* find_standard_meta(const char * name) //returns atom name if found, 0 if not
{
	unsigned n;
	for(n=0;n<sizeof(stdmetas)/sizeof(stdmetas[0]);n++)
	{
		if (!stricmp(name,stdmetas[n].name)) return stdmetas[n].atom;
	}
    return 0;
}

static void membuffer_write_track_tag(membuffer * buf,const char * name,uint32_t index,uint32_t total)
{
	membuffer_write_int32(buf,8 /*atom header*/ + 8 /*data atom header*/ + 8 /*flags + reserved*/ + 8 /*actual data*/ );
	membuffer_write_atom_name(buf,name);
	membuffer_write_int32(buf,8 /*data atom header*/ + 8 /*flags + reserved*/ + 8 /*actual data*/ );
	membuffer_write_atom_name(buf,"data");
	membuffer_write_int32(buf,0);//flags
	membuffer_write_int32(buf,0);//reserved
	membuffer_write_int16(buf,0);
	membuffer_write_int16(buf,(uint16_t)index);//track number
	membuffer_write_int16(buf,(uint16_t)total);//total tracks
	membuffer_write_int16(buf,0);
}

static void membuffer_write_int16_tag(membuffer * buf,const char * name,uint16_t value)
{
	membuffer_write_int32(buf,8 /*atom header*/ + 8 /*data atom header*/ + 8 /*flags + reserved*/ + 2 /*actual data*/ );
	membuffer_write_atom_name(buf,name);
	membuffer_write_int32(buf,8 /*data atom header*/ + 8 /*flags + reserved*/ + 2 /*actual data*/ );
	membuffer_write_atom_name(buf,"data");
	membuffer_write_int32(buf,0);//flags
	membuffer_write_int32(buf,0);//reserved
	membuffer_write_int16(buf,value);//value
}

static void membuffer_write_std_tag(membuffer * buf,const char * name,const char * value)
{
	membuffer_write_int32(buf,8 /*atom header*/ + 8 /*data atom header*/ + 8 /*flags + reserved*/ + strlen(value) );
	membuffer_write_atom_name(buf,name);
	membuffer_write_int32(buf,8 /*data atom header*/ + 8 /*flags + reserved*/ + strlen(value));
	membuffer_write_atom_name(buf,"data");
	membuffer_write_int32(buf,1);//flags
	membuffer_write_int32(buf,0);//reserved
	membuffer_write_data(buf,value,strlen(value));
}

static void membuffer_write_custom_tag(membuffer * buf,const char * name,const char * value)
{
	membuffer_write_int32(buf,8 /*atom header*/ + 0x1C /*weirdo itunes atom*/ + 12 /*name atom header*/ + strlen(name) + 16 /*data atom header + flags*/ + strlen(value) );
	membuffer_write_atom_name(buf,"----");
	membuffer_write_int32(buf,0x1C);//weirdo itunes atom
	membuffer_write_atom_name(buf,"mean");
	membuffer_write_int32(buf,0);
	membuffer_write_data(buf,"com.apple.iTunes",16);
	membuffer_write_int32(buf,12 + strlen(name));
	membuffer_write_atom_name(buf,"name");
	membuffer_write_int32(buf,0);
	membuffer_write_data(buf,name,strlen(name));
	membuffer_write_int32(buf,8 /*data atom header*/ + 8 /*flags + reserved*/ + strlen(value));
	membuffer_write_atom_name(buf,"data");
	membuffer_write_int32(buf,1);//flags
	membuffer_write_int32(buf,0);//reserved
	membuffer_write_data(buf,value,strlen(value));

}

static uint32_t myatoi(const char * param)
{
	return param ? atoi(param) : 0;
}

static uint32_t create_ilst(const mp4ff_metadata_t * data,void ** out_buffer,uint32_t * out_size)
{
	membuffer * buf = membuffer_create();
	unsigned metaptr;
	char * mask = (char*)malloc(data->count);
	memset(mask,0,data->count);

	{
		const char * tracknumber_ptr = 0, * totaltracks_ptr = 0;
		const char * discnumber_ptr = 0, * totaldiscs_ptr = 0;
		const char * genre_ptr = 0, * tempo_ptr = 0;
		for(metaptr = 0; metaptr < data->count; metaptr++)
		{
			mp4ff_tag_t * tag = &data->tags[metaptr];
			if (!stricmp(tag->item,"tracknumber") || !stricmp(tag->item,"track"))
			{
				if (tracknumber_ptr==0) tracknumber_ptr = tag->value;
				mask[metaptr] = 1;
			}
			else if (!stricmp(tag->item,"totaltracks"))
			{
				if (totaltracks_ptr==0) totaltracks_ptr = tag->value;
				mask[metaptr] = 1;
			}
			else if (!stricmp(tag->item,"discnumber") || !stricmp(tag->item,"disc"))
			{
				if (discnumber_ptr==0) discnumber_ptr = tag->value;
				mask[metaptr] = 1;
			}
			else if (!stricmp(tag->item,"totaldiscs"))
			{
				if (totaldiscs_ptr==0) totaldiscs_ptr = tag->value;
				mask[metaptr] = 1;
			}
			else if (!stricmp(tag->item,"genre"))
			{
				if (genre_ptr==0) genre_ptr = tag->value;
				mask[metaptr] = 1;
			}
			else if (!stricmp(tag->item,"tempo"))
			{
				if (tempo_ptr==0) tempo_ptr = tag->value;
				mask[metaptr] = 1;
			}

		}

		if (tracknumber_ptr) membuffer_write_track_tag(buf,"trkn",myatoi(tracknumber_ptr),myatoi(totaltracks_ptr));
		if (discnumber_ptr) membuffer_write_track_tag(buf,"disk",myatoi(discnumber_ptr),myatoi(totaldiscs_ptr));
		if (tempo_ptr) membuffer_write_int16_tag(buf,"tmpo",(uint16_t)myatoi(tempo_ptr));

		if (genre_ptr)
		{
			uint32_t index = mp4ff_meta_genre_to_index(genre_ptr);
			if (index==0)
				membuffer_write_std_tag(buf,"©gen",genre_ptr);
			else
				membuffer_write_int16_tag(buf,"gnre",(uint16_t)index);
		}
	}
	
	for(metaptr = 0; metaptr < data->count; metaptr++)
	{
		if (!mask[metaptr])
		{
			mp4ff_tag_t * tag = &data->tags[metaptr];
			const char * std_meta_atom = find_standard_meta(tag->item);
			if (std_meta_atom)
			{
				membuffer_write_std_tag(buf,std_meta_atom,tag->value);
			}
			else
			{
				membuffer_write_custom_tag(buf,tag->item,tag->value);
			}
		}
	}

	free(mask);

	if (membuffer_error(buf))
	{
		membuffer_free(buf);
		return 0;
	}

	*out_size = membuffer_get_size(buf);
	*out_buffer = membuffer_detach(buf);
	membuffer_free(buf);

	return 1;
}

static uint32_t find_atom(mp4ff_t * f,uint64_t base,uint32_t size,const char * name)
{
	uint32_t remaining = size;
	uint64_t atom_offset = base;
	for(;;)
	{
		char atom_name[4];
		uint32_t atom_size;

		mp4ff_set_position(f,atom_offset);
		
		if (remaining < 8) break;
		atom_size = mp4ff_read_int32(f);
		if (atom_size > remaining || atom_size < 8) break;
		mp4ff_read_data(f,atom_name,4);
		
		if (!memcmp(atom_name,name,4))
		{
			mp4ff_set_position(f,atom_offset);
			return 1;
		}
		
		remaining -= atom_size;
		atom_offset += atom_size;
	}
	return 0;
}

static uint32_t find_atom_v2(mp4ff_t * f,uint64_t base,uint32_t size,const char * name,uint32_t extraheaders,const char * name_inside)
{
	uint64_t first_base = (uint64_t)(-1);
	while(find_atom(f,base,size,name))//try to find atom <name> with atom <name_inside> in it
	{
		uint64_t mybase = mp4ff_position(f);
		uint32_t mysize = mp4ff_read_int32(f);

		if (first_base == (uint64_t)(-1)) first_base = mybase;

		if (mysize < 8 + extraheaders) break;

		if (find_atom(f,mybase+(8+extraheaders),mysize-(8+extraheaders),name_inside))
		{
			mp4ff_set_position(f,mybase);
			return 2;
		}
		base += mysize;
		if (size<=mysize) {size=0;break;}
		size -= mysize;
	}

	if (first_base != (uint64_t)(-1))//wanted atom inside not found
	{
		mp4ff_set_position(f,first_base);
		return 1;
	}
	else return 0;	
}

static uint32_t create_meta(const mp4ff_metadata_t * data,void ** out_buffer,uint32_t * out_size)
{
	membuffer * buf;
	uint32_t ilst_size;
	void * ilst_buffer;

	if (!create_ilst(data,&ilst_buffer,&ilst_size)) return 0;

	buf = membuffer_create();

	membuffer_write_int32(buf,0);
	membuffer_write_atom(buf,"ilst",ilst_size,ilst_buffer);
	free(ilst_buffer);

	*out_size = membuffer_get_size(buf);
	*out_buffer = membuffer_detach(buf);
	membuffer_free(buf);
	return 1;
}

static uint32_t create_udta(const mp4ff_metadata_t * data,void ** out_buffer,uint32_t * out_size)
{
	membuffer * buf;
	uint32_t meta_size;
	void * meta_buffer;

	if (!create_meta(data,&meta_buffer,&meta_size)) return 0;

	buf = membuffer_create();

	membuffer_write_atom(buf,"meta",meta_size,meta_buffer);

	free(meta_buffer);

	*out_size = membuffer_get_size(buf);
	*out_buffer = membuffer_detach(buf);
	membuffer_free(buf);
	return 1;
}

static uint32_t modify_moov(mp4ff_t * f,const mp4ff_metadata_t * data,void ** out_buffer,uint32_t * out_size)
{
	uint64_t total_base = f->moov_offset + 8;
	uint32_t total_size = (uint32_t)(f->moov_size - 8);

	uint64_t udta_offset,meta_offset,ilst_offset;
	uint32_t udta_size,  meta_size,  ilst_size;
	
	uint32_t new_ilst_size;
	void * new_ilst_buffer;
	
	uint8_t * p_out;
	int32_t size_delta;
	
	
	if (!find_atom_v2(f,total_base,total_size,"udta",0,"meta"))
	{
		membuffer * buf;
		void * new_udta_buffer;
		uint32_t new_udta_size;
		if (!create_udta(data,&new_udta_buffer,&new_udta_size)) return 0;
		
		buf = membuffer_create();
		mp4ff_set_position(f,total_base);
		membuffer_transfer_from_file(buf,f,total_size);
		
		membuffer_write_atom(buf,"udta",new_udta_size,new_udta_buffer);

		free(new_udta_buffer);
	
		*out_size = membuffer_get_size(buf);
		*out_buffer = membuffer_detach(buf);
		membuffer_free(buf);
		return 1;		
	}
	else
	{
		udta_offset = mp4ff_position(f);
		udta_size = mp4ff_read_int32(f);
		if (!find_atom_v2(f,udta_offset+8,udta_size-8,"meta",4,"ilst"))
		{
			membuffer * buf;
			void * new_meta_buffer;
			uint32_t new_meta_size;
			if (!create_meta(data,&new_meta_buffer,&new_meta_size)) return 0;
			
			buf = membuffer_create();
			mp4ff_set_position(f,total_base);
			membuffer_transfer_from_file(buf,f,(uint32_t)(udta_offset - total_base));
			
			membuffer_write_int32(buf,udta_size + 8 + new_meta_size);
			membuffer_write_atom_name(buf,"udta");
			membuffer_transfer_from_file(buf,f,udta_size);
						
			membuffer_write_atom(buf,"meta",new_meta_size,new_meta_buffer);
			free(new_meta_buffer);
		
			*out_size = membuffer_get_size(buf);
			*out_buffer = membuffer_detach(buf);
			membuffer_free(buf);
			return 1;		
		}
		meta_offset = mp4ff_position(f);
		meta_size = mp4ff_read_int32(f);
		if (!find_atom(f,meta_offset+12,meta_size-12,"ilst")) return 0;//shouldn't happen, find_atom_v2 above takes care of it
		ilst_offset = mp4ff_position(f);
		ilst_size = mp4ff_read_int32(f);

		if (!create_ilst(data,&new_ilst_buffer,&new_ilst_size)) return 0;
		
		size_delta = new_ilst_size - (ilst_size - 8);

		*out_size = total_size + size_delta;
		*out_buffer = malloc(*out_size);
		if (*out_buffer == 0)
		{
			free(new_ilst_buffer);
			return 0;
		}

		p_out = (uint8_t*)*out_buffer;
		
		mp4ff_set_position(f,total_base);
		mp4ff_read_data(f,p_out,(uint32_t)(udta_offset - total_base )); p_out += (uint32_t)(udta_offset - total_base );
		*(uint32_t*)p_out = fix_byte_order_32(mp4ff_read_int32(f) + size_delta); p_out += 4;
		mp4ff_read_data(f,p_out,4); p_out += 4;
		mp4ff_read_data(f,p_out,(uint32_t)(meta_offset - udta_offset - 8)); p_out += (uint32_t)(meta_offset - udta_offset - 8);
		*(uint32_t*)p_out = fix_byte_order_32(mp4ff_read_int32(f) + size_delta); p_out += 4;
		mp4ff_read_data(f,p_out,4); p_out += 4;
		mp4ff_read_data(f,p_out,(uint32_t)(ilst_offset - meta_offset - 8)); p_out += (uint32_t)(ilst_offset - meta_offset - 8);
		*(uint32_t*)p_out = fix_byte_order_32(mp4ff_read_int32(f) + size_delta); p_out += 4;
		mp4ff_read_data(f,p_out,4); p_out += 4;

		memcpy(p_out,new_ilst_buffer,new_ilst_size);
		p_out += new_ilst_size;

		mp4ff_set_position(f,ilst_offset + ilst_size);
		mp4ff_read_data(f,p_out,(uint32_t)(total_size - (ilst_offset - total_base) - ilst_size));

		free(new_ilst_buffer);
	}
	return 1;

}

int32_t mp4ff_meta_update(mp4ff_callback_t *f,const mp4ff_metadata_t * data)
{
	void * new_moov_data;
	uint32_t new_moov_size;

    mp4ff_t *ff = malloc(sizeof(mp4ff_t));

    memset(ff, 0, sizeof(mp4ff_t));
    ff->stream = f;
	mp4ff_set_position(ff,0);

    parse_atoms(ff,1);


	if (!modify_moov(ff,data,&new_moov_data,&new_moov_size))
	{
		mp4ff_close(ff);
		return 0;
	}

    /* copy moov atom to end of the file */
    if (ff->last_atom != ATOM_MOOV)
    {
        char *free_data = "free";

        /* rename old moov to free */
        mp4ff_set_position(ff, ff->moov_offset + 4);
        mp4ff_write_data(ff, free_data, 4);
	
        mp4ff_set_position(ff, ff->file_size);
		mp4ff_write_int32(ff,new_moov_size + 8);
		mp4ff_write_data(ff,"moov",4);
		mp4ff_write_data(ff, new_moov_data, new_moov_size);
    }
	else
	{
        mp4ff_set_position(ff, ff->moov_offset);
		mp4ff_write_int32(ff,new_moov_size + 8);
		mp4ff_write_data(ff,"moov",4);
		mp4ff_write_data(ff, new_moov_data, new_moov_size);
	}

	mp4ff_truncate(ff);

	mp4ff_close(ff);
    return 1;
}
#endif
