/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2007             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: stdio-based convenience library for opening/seeking/decoding
 last mod: $Id: vorbisfile.c 13294 2007-07-24 01:08:23Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "os.h"
#include "misc.h"

/* A 'chained bitstream' is a Vorbis bitstream that contains more than
   one logical bitstream arranged end to end (the only form of Ogg
   multiplexing allowed in a Vorbis bitstream; grouping [parallel
   multiplexing] is not allowed in Vorbis) */

/* A Vorbis file can be played beginning to end (streamed) without
   worrying ahead of time about chaining (see decoder_example.c).  If
   we have the whole file, however, and want random access
   (seeking/scrubbing) or desire to know the total length/time of a
   file, we need to account for the possibility of chaining. */

/* We can handle things a number of ways; we can determine the entire
   bitstream structure right off the bat, or find pieces on demand.
   This example determines and caches structure for the entire
   bitstream, but builds a virtual decoder on the fly when moving
   between links in the chain. */

/* There are also different ways to implement seeking.  Enough
   information exists in an Ogg bitstream to seek to
   sample-granularity positions in the output.  Or, one can seek by
   picking some portion of the stream roughly in the desired area if
   we only want coarse navigation through the stream. */

/*************************************************************************
 * Many, many internal helpers.  The intention is not to be confusing; 
 * rampant duplication and monolithic function implementation would be 
 * harder to understand anyway.  The high level functions are last.  Begin
 * grokking near the end of the file */

/* read a little more data from the file/pipe into the ogg_sync framer
*/
#define CHUNKSIZE 65536

static long _get_data(OggVorbis_File *vf){
  errno=0;
  if(!(vf->callbacks.read_func))return(-1);
  if(vf->datasource){
    char *buffer=ogg_sync_buffer(&vf->oy,CHUNKSIZE);
    long bytes=(vf->callbacks.read_func)(buffer,1,CHUNKSIZE,vf->datasource);
    if(bytes>0)ogg_sync_wrote(&vf->oy,bytes);
    if(bytes==0 && errno)return(-1);
    return(bytes);
  }else
    return(0);
}

/* save a tiny smidge of verbosity to make the code more readable */
static int _seek_helper(OggVorbis_File *vf,ogg_int64_t offset){
  if(vf->datasource){ 
    if(!(vf->callbacks.seek_func)||
       (vf->callbacks.seek_func)(vf->datasource, offset, SEEK_SET) == -1)
      return OV_EREAD;
    vf->offset=offset;
    ogg_sync_reset(&vf->oy);
  }else{
    /* shouldn't happen unless someone writes a broken callback */
    return OV_EFAULT;
  }
  return 0;
}

/* The read/seek functions track absolute position within the stream */

/* from the head of the stream, get the next page.  boundary specifies
   if the function is allowed to fetch more data from the stream (and
   how much) or only use internally buffered data.

   boundary: -1) unbounded search
              0) read no additional data; use cached only
	      n) search for a new page beginning for n bytes

   return:   <0) did not find a page (OV_FALSE, OV_EOF, OV_EREAD)
              n) found a page at absolute offset n */

static ogg_int64_t _get_next_page(OggVorbis_File *vf,ogg_page *og,
				  ogg_int64_t boundary){
  if(boundary>0)boundary+=vf->offset;
  while(1){
    long more;

    if(boundary>0 && vf->offset>=boundary)return(OV_FALSE);
    more=ogg_sync_pageseek(&vf->oy,og);
    
    if(more<0){
      /* skipped n bytes */
      vf->offset-=more;
    }else{
      if(more==0){
	/* send more paramedics */
	if(!boundary)return(OV_FALSE);
	{
	  long ret=_get_data(vf);
	  if(ret==0)return(OV_EOF);
	  if(ret<0)return(OV_EREAD);
	}
      }else{
	/* got a page.  Return the offset at the page beginning,
           advance the internal offset past the page end */
	ogg_int64_t ret=vf->offset;
	vf->offset+=more;
	return(ret);
	
      }
    }
  }
}

/* find the latest page beginning before the current stream cursor
   position. Much dirtier than the above as Ogg doesn't have any
   backward search linkage.  no 'readp' as it will certainly have to
   read. */
/* returns offset or OV_EREAD, OV_FAULT */
static ogg_int64_t _get_prev_page(OggVorbis_File *vf,ogg_page *og){
  ogg_int64_t begin=vf->offset;
  ogg_int64_t end=begin;
  ogg_int64_t ret;
  ogg_int64_t offset=-1;

  while(offset==-1){
    begin-=CHUNKSIZE;
    if(begin<0)
      begin=0;

    ret=_seek_helper(vf,begin);
    if(ret)return(ret);

    while(vf->offset<end){
      ret=_get_next_page(vf,og,end-vf->offset);
      if(ret==OV_EREAD)return(OV_EREAD);
      if(ret<0){
	break;
      }else{
	offset=ret;
      }
    }
  }

  /* we have the offset.  Actually snork and hold the page now */
  ret=_seek_helper(vf,offset);
  if(ret)return(ret);

  ret=_get_next_page(vf,og,CHUNKSIZE);
  if(ret<0)
    /* this shouldn't be possible */
    return(OV_EFAULT);

  return(offset);
}

static void _add_serialno(ogg_page *og,long **serialno_list, int *n){
  long s = ogg_page_serialno(og);
  (*n)++;

  if(serialno_list){
    *serialno_list = _ogg_realloc(*serialno_list, sizeof(*serialno_list)*(*n));
  }else{
    *serialno_list = _ogg_malloc(sizeof(**serialno_list));
  }
  
  (*serialno_list)[(*n)-1] = s;
}

/* returns nonzero if found */
static int _lookup_serialno(ogg_page *og, long *serialno_list, int n){
  long s = ogg_page_serialno(og);

  if(serialno_list){
    while(n--){
      if(*serialno_list == s) return 1;
      serialno_list++;
    }
  }
  return 0;
}

/* start parsing pages at current offset, remembering all serial
   numbers.  Stop logging at first non-bos page */
static int _get_serialnos(OggVorbis_File *vf, long **s, int *n){
  ogg_page og;

  *s=NULL;
  *n=0;

  while(1){
    ogg_int64_t llret=_get_next_page(vf,&og,CHUNKSIZE);
    if(llret==OV_EOF)return(0);
    if(llret<0)return(llret);
    if(!ogg_page_bos(&og)) return 0;

    /* look for duplicate serialnos; add this one if unique */
    if(_lookup_serialno(&og,*s,*n)){
      if(*s)_ogg_free(*s);
      *s=0;
      *n=0;
      return(OV_EBADHEADER);
    }

    _add_serialno(&og,s,n);
  }
}

/* finds each bitstream link one at a time using a bisection search
   (has to begin by knowing the offset of the lb's initial page).
   Recurses for each link so it can alloc the link storage after
   finding them all, then unroll and fill the cache at the same time */
static int _bisect_forward_serialno(OggVorbis_File *vf,
				    ogg_int64_t begin,
				    ogg_int64_t searched,
				    ogg_int64_t end,
				    long *currentno_list,
				    int  currentnos,
				    long m){
  ogg_int64_t endsearched=end;
  ogg_int64_t next=end;
  ogg_page og;
  ogg_int64_t ret;
  
  /* the below guards against garbage seperating the last and
     first pages of two links. */
  while(searched<endsearched){
    ogg_int64_t bisect;
    
    if(endsearched-searched<CHUNKSIZE){
      bisect=searched;
    }else{
      bisect=(searched+endsearched)/2;
    }
    
    ret=_seek_helper(vf,bisect);
    if(ret)return(ret);

    ret=_get_next_page(vf,&og,-1);
    if(ret==OV_EREAD)return(OV_EREAD);
    if(ret<0 || !_lookup_serialno(&og,currentno_list,currentnos)){
      endsearched=bisect;
      if(ret>=0)next=ret;
    }else{
      searched=ret+og.header_len+og.body_len;
    }
  }

  {
    long *next_serialno_list=NULL;
    int next_serialnos=0;

    ret=_seek_helper(vf,next);
    if(ret)return(ret);
    ret=_get_serialnos(vf,&next_serialno_list,&next_serialnos);
    if(ret)return(ret);
    
    if(searched>=end || next_serialnos==0){
      vf->links=m+1;
      vf->offsets=_ogg_malloc((vf->links+1)*sizeof(*vf->offsets));
      vf->offsets[m+1]=searched;
    }else{
      ret=_bisect_forward_serialno(vf,next,vf->offset,
				   end,next_serialno_list,next_serialnos,m+1);
      if(ret)return(ret);
    }
    
    if(next_serialno_list)_ogg_free(next_serialno_list);
  }
  vf->offsets[m]=begin;
  return(0);
}

/* uses the local ogg_stream storage in vf; this is important for
   non-streaming input sources */
static int _fetch_headers(OggVorbis_File *vf,vorbis_info *vi,vorbis_comment *vc,
			  long *serialno,ogg_page *og_ptr){
  ogg_page og;
  ogg_packet op;
  int i,ret;
  int allbos=0;

  if(!og_ptr){
    ogg_int64_t llret=_get_next_page(vf,&og,CHUNKSIZE);
    if(llret==OV_EREAD)return(OV_EREAD);
    if(llret<0)return(OV_ENOTVORBIS);
    og_ptr=&og;
  }

  vorbis_info_init(vi);
  vorbis_comment_init(vc);

  /* extract the first set of vorbis headers we see in the headerset */

  while(1){
  
    /* if we're past the ID headers, we won't be finding a Vorbis
       stream in this link */
    if(!ogg_page_bos(og_ptr)){
      ret = OV_ENOTVORBIS;
      goto bail_header;
    }

    /* prospective stream setup; we need a stream to get packets */
    ogg_stream_reset_serialno(&vf->os,ogg_page_serialno(og_ptr));
    ogg_stream_pagein(&vf->os,og_ptr);

    if(ogg_stream_packetout(&vf->os,&op) > 0 &&
       vorbis_synthesis_idheader(&op)){

      /* continue Vorbis header load; past this point, any error will
	 render this link useless (we won't continue looking for more
	 Vorbis streams */
      if(serialno)*serialno=vf->os.serialno;
      vf->ready_state=STREAMSET;
      if((ret=vorbis_synthesis_headerin(vi,vc,&op)))
	goto bail_header;

      i=0;
      while(i<2){ /* get a page loop */
	
	while(i<2){ /* get a packet loop */

	  int result=ogg_stream_packetout(&vf->os,&op);
	  if(result==0)break;
	  if(result==-1){
	    ret=OV_EBADHEADER;
	    goto bail_header;
	  }
	
	  if((ret=vorbis_synthesis_headerin(vi,vc,&op)))
	    goto bail_header;

	  i++;
	}

	while(i<2){
	  if(_get_next_page(vf,og_ptr,CHUNKSIZE)<0){
	    ret=OV_EBADHEADER;
	    goto bail_header;
	  }

	  /* if this page belongs to the correct stream, go parse it */
	  if(vf->os.serialno == ogg_page_serialno(og_ptr)){
	    ogg_stream_pagein(&vf->os,og_ptr);
	    break;
	  }

	  /* if we never see the final vorbis headers before the link
	     ends, abort */
	  if(ogg_page_bos(og_ptr)){
	    if(allbos){
	      ret = OV_EBADHEADER;
	      goto bail_header;
	    }else
	      allbos=1;
	  }

	  /* otherwise, keep looking */
	}
      }

      return 0; 
    }

    /* this wasn't vorbis, get next page, try again */
    {
      ogg_int64_t llret=_get_next_page(vf,og_ptr,CHUNKSIZE);
      if(llret==OV_EREAD)return(OV_EREAD);
      if(llret<0)return(OV_ENOTVORBIS);
    } 
  }

 bail_header:
  vorbis_info_clear(vi);
  vorbis_comment_clear(vc);
  vf->ready_state=OPENED;

  return ret;
}

/* last step of the OggVorbis_File initialization; get all the
   vorbis_info structs and PCM positions.  Only called by the seekable
   initialization (local stream storage is hacked slightly; pay
   attention to how that's done) */

/* this is void and does not propogate errors up because we want to be
   able to open and use damaged bitstreams as well as we can.  Just
   watch out for missing information for links in the OggVorbis_File
   struct */
static void _prefetch_all_headers(OggVorbis_File *vf, ogg_int64_t dataoffset){
  ogg_page og;
  int i;
  ogg_int64_t ret;

  vf->vi=_ogg_realloc(vf->vi,vf->links*sizeof(*vf->vi));
  vf->vc=_ogg_realloc(vf->vc,vf->links*sizeof(*vf->vc));
  vf->serialnos=_ogg_malloc(vf->links*sizeof(*vf->serialnos));
  vf->dataoffsets=_ogg_malloc(vf->links*sizeof(*vf->dataoffsets));
  vf->pcmlengths=_ogg_malloc(vf->links*2*sizeof(*vf->pcmlengths));
  
  for(i=0;i<vf->links;i++){
    if(i==0){
      /* we already grabbed the initial header earlier.  Just set the offset */
      vf->serialnos[i]=vf->current_serialno;
      vf->dataoffsets[i]=dataoffset;
      ret=_seek_helper(vf,dataoffset);
      if(ret)
	vf->dataoffsets[i]=-1;

    }else{

      /* seek to the location of the initial header */

      ret=_seek_helper(vf,vf->offsets[i]);
      if(ret){
	vf->dataoffsets[i]=-1;
      }else{
	if(_fetch_headers(vf,vf->vi+i,vf->vc+i,vf->serialnos+i,NULL)<0){
	  vf->dataoffsets[i]=-1;
	}else{
	  vf->dataoffsets[i]=vf->offset;
	}
      }
    }

    /* fetch beginning PCM offset */

    if(vf->dataoffsets[i]!=-1){
      ogg_int64_t accumulated=0;
      long        lastblock=-1;
      int         result;

      ogg_stream_reset_serialno(&vf->os,vf->serialnos[i]);

      while(1){
	ogg_packet op;

	ret=_get_next_page(vf,&og,-1);
	if(ret<0)
	  /* this should not be possible unless the file is
             truncated/mangled */
	  break;
       
	if(ogg_page_bos(&og)) break;

	if(ogg_page_serialno(&og)!=vf->serialnos[i])
	  continue;
	
	/* count blocksizes of all frames in the page */
	ogg_stream_pagein(&vf->os,&og);
	while((result=ogg_stream_packetout(&vf->os,&op))){
	  if(result>0){ /* ignore holes */
	    long thisblock=vorbis_packet_blocksize(vf->vi+i,&op);
	    if(lastblock!=-1)
	      accumulated+=(lastblock+thisblock)>>2;
	    lastblock=thisblock;
	  }
	}

	if(ogg_page_granulepos(&og)!=-1){
	  /* pcm offset of last packet on the first audio page */
	  accumulated= ogg_page_granulepos(&og)-accumulated;
	  break;
	}
      }

      /* less than zero?  This is a stream with samples trimmed off
         the beginning, a normal occurrence; set the offset to zero */
      if(accumulated<0)accumulated=0;

      vf->pcmlengths[i*2]=accumulated;
    }

    /* get the PCM length of this link. To do this,
       get the last page of the stream */
    {
      ogg_int64_t end=vf->offsets[i+1];
      ret=_seek_helper(vf,end);
      if(ret){
	/* this should not be possible */
	vorbis_info_clear(vf->vi+i);
	vorbis_comment_clear(vf->vc+i);
      }else{
	
	while(1){
	  ret=_get_prev_page(vf,&og);
	  if(ret<0){
	    /* this should not be possible */
	    vorbis_info_clear(vf->vi+i);
	    vorbis_comment_clear(vf->vc+i);
	    break;
	  }
	  if(ogg_page_serialno(&og)==vf->serialnos[i]){
	    if(ogg_page_granulepos(&og)!=-1){
	      vf->pcmlengths[i*2+1]=ogg_page_granulepos(&og)-vf->pcmlengths[i*2];
	      break;
	    }
	  }
	  vf->offset=ret;
	}
      }
    }
  }
}

static int _make_decode_ready(OggVorbis_File *vf){
  if(vf->ready_state>STREAMSET)return 0;
  if(vf->ready_state<STREAMSET)return OV_EFAULT;
  if(vf->seekable){
    if(vorbis_synthesis_init(&vf->vd,vf->vi+vf->current_link))
      return OV_EBADLINK;
  }else{
    if(vorbis_synthesis_init(&vf->vd,vf->vi))
      return OV_EBADLINK;
  }    
  vorbis_block_init(&vf->vd,&vf->vb);
  vf->ready_state=INITSET;
  vf->bittrack=0.f;
  vf->samptrack=0.f;
  return 0;
}

static int _open_seekable2(OggVorbis_File *vf){
  ogg_int64_t dataoffset=vf->offset,end;
  long *serialno_list=NULL;
  int serialnos=0;
  int ret;
  ogg_page og;

  /* we're partially open and have a first link header state in
     storage in vf */
  /* we can seek, so set out learning all about this file */
  if(vf->callbacks.seek_func && vf->callbacks.tell_func){
    (vf->callbacks.seek_func)(vf->datasource,0,SEEK_END);
    vf->offset=vf->end=(vf->callbacks.tell_func)(vf->datasource);
  }else{
    vf->offset=vf->end=-1;
  }

  /* If seek_func is implemented, tell_func must also be implemented */
  if(vf->end==-1) return(OV_EINVAL);

  /* We get the offset for the last page of the physical bitstream.
     Most OggVorbis files will contain a single logical bitstream */
  end=_get_prev_page(vf,&og);
  if(end<0)return(end);

  /* back to beginning, learn all serialnos of first link */
  ret=_seek_helper(vf,0);
  if(ret)return(ret);
  ret=_get_serialnos(vf,&serialno_list,&serialnos);
  if(ret)return(ret);

  /* now determine bitstream structure recursively */
  if(_bisect_forward_serialno(vf,0,0,end+1,serialno_list,serialnos,0)<0)return(OV_EREAD);  
  if(serialno_list)_ogg_free(serialno_list);

  /* the initial header memory is referenced by vf after; don't free it */
  _prefetch_all_headers(vf,dataoffset);
  return(ov_raw_seek(vf,0));
}

/* clear out the current logical bitstream decoder */ 
static void _decode_clear(OggVorbis_File *vf){
  vorbis_dsp_clear(&vf->vd);
  vorbis_block_clear(&vf->vb);
  vf->ready_state=OPENED;
}

/* fetch and process a packet.  Handles the case where we're at a
   bitstream boundary and dumps the decoding machine.  If the decoding
   machine is unloaded, it loads it.  It also keeps pcm_offset up to
   date (seek and read both use this.  seek uses a special hack with
   readp). 

   return: <0) error, OV_HOLE (lost packet) or OV_EOF
            0) need more data (only if readp==0)
	    1) got a packet 
*/

static int _fetch_and_process_packet(OggVorbis_File *vf,
				     ogg_packet *op_in,
				     int readp,
				     int spanp){
  ogg_page og;

  /* handle one packet.  Try to fetch it from current stream state */
  /* extract packets from page */
  while(1){
    
    /* process a packet if we can.  If the machine isn't loaded,
       neither is a page */
    if(vf->ready_state==INITSET){
      while(1) {
      	ogg_packet op;
      	ogg_packet *op_ptr=(op_in?op_in:&op);
	int result=ogg_stream_packetout(&vf->os,op_ptr);
	ogg_int64_t granulepos;

	op_in=NULL;
	if(result==-1)return(OV_HOLE); /* hole in the data. */
	if(result>0){
	  /* got a packet.  process it */
	  granulepos=op_ptr->granulepos;
	  if(!vorbis_synthesis(&vf->vb,op_ptr)){ /* lazy check for lazy
						    header handling.  The
						    header packets aren't
						    audio, so if/when we
						    submit them,
						    vorbis_synthesis will
						    reject them */

	    /* suck in the synthesis data and track bitrate */
	    {
	      int oldsamples=vorbis_synthesis_pcmout(&vf->vd,NULL);
	      /* for proper use of libvorbis within libvorbisfile,
                 oldsamples will always be zero. */
	      if(oldsamples)return(OV_EFAULT);
	      
	      vorbis_synthesis_blockin(&vf->vd,&vf->vb);
	      vf->samptrack+=vorbis_synthesis_pcmout(&vf->vd,NULL)-oldsamples;
	      vf->bittrack+=op_ptr->bytes*8;
	    }
	  
	    /* update the pcm offset. */
	    if(granulepos!=-1 && !op_ptr->e_o_s){
	      int link=(vf->seekable?vf->current_link:0);
	      int i,samples;
	    
	      /* this packet has a pcm_offset on it (the last packet
	         completed on a page carries the offset) After processing
	         (above), we know the pcm position of the *last* sample
	         ready to be returned. Find the offset of the *first*

	         As an aside, this trick is inaccurate if we begin
	         reading anew right at the last page; the end-of-stream
	         granulepos declares the last frame in the stream, and the
	         last packet of the last page may be a partial frame.
	         So, we need a previous granulepos from an in-sequence page
	         to have a reference point.  Thus the !op_ptr->e_o_s clause
	         above */

	      if(vf->seekable && link>0)
		granulepos-=vf->pcmlengths[link*2];
	      if(granulepos<0)granulepos=0; /* actually, this
					       shouldn't be possible
					       here unless the stream
					       is very broken */

	      samples=vorbis_synthesis_pcmout(&vf->vd,NULL);
	    
	      granulepos-=samples;
	      for(i=0;i<link;i++)
	        granulepos+=vf->pcmlengths[i*2+1];
	      vf->pcm_offset=granulepos;
	    }
	    return(1);
	  }
	}
	else 
	  break;
      }
    }

    if(vf->ready_state>=OPENED){
      ogg_int64_t ret;
      
      while(1){ 
	/* the loop is not strictly necessary, but there's no sense in
	   doing the extra checks of the larger loop for the common
	   case in a multiplexed bistream where the page is simply
	   part of a different logical bitstream; keep reading until
	   we get one with the correct serialno */
	
	if(!readp)return(0);
	if((ret=_get_next_page(vf,&og,-1))<0){
	  return(OV_EOF); /* eof. leave unitialized */
	}

	/* bitrate tracking; add the header's bytes here, the body bytes
	   are done by packet above */
	vf->bittrack+=og.header_len*8;
	
	if(vf->ready_state==INITSET){
	  if(vf->current_serialno!=ogg_page_serialno(&og)){
	    
	    /* two possibilities: 
	       1) our decoding just traversed a bitstream boundary
	       2) another stream is multiplexed into this logical section? */
	    
	    if(ogg_page_bos(&og)){
	      /* boundary case */
	      if(!spanp)
		return(OV_EOF);
	      
	      _decode_clear(vf);
	      
	      if(!vf->seekable){
		vorbis_info_clear(vf->vi);
		vorbis_comment_clear(vf->vc);
	      }
	      break;

	    }else
	      continue; /* possibility #2 */
	  }
	}

	break;
      }
    }

    /* Do we need to load a new machine before submitting the page? */
    /* This is different in the seekable and non-seekable cases.  

       In the seekable case, we already have all the header
       information loaded and cached; we just initialize the machine
       with it and continue on our merry way.

       In the non-seekable (streaming) case, we'll only be at a
       boundary if we just left the previous logical bitstream and
       we're now nominally at the header of the next bitstream
    */

    if(vf->ready_state!=INITSET){ 
      int link;

      if(vf->ready_state<STREAMSET){
	if(vf->seekable){
	  long serialno = ogg_page_serialno(&og);

	  /* match the serialno to bitstream section.  We use this rather than
	     offset positions to avoid problems near logical bitstream
	     boundaries */

	  for(link=0;link<vf->links;link++)
	    if(vf->serialnos[link]==serialno)break;

	  if(link==vf->links) continue; /* not the desired Vorbis
					   bitstream section; keep
					   trying */

	  vf->current_serialno=serialno;
	  vf->current_link=link;
	  
	  ogg_stream_reset_serialno(&vf->os,vf->current_serialno);
	  vf->ready_state=STREAMSET;
	  
	}else{
	  /* we're streaming */
	  /* fetch the three header packets, build the info struct */
	  
	  int ret=_fetch_headers(vf,vf->vi,vf->vc,&vf->current_serialno,&og);
	  if(ret)return(ret);
	  vf->current_link++;
	  link=0;
	}
      }
      
      {
	int ret=_make_decode_ready(vf);
	if(ret<0)return ret;
      }
    }

    /* the buffered page is the data we want, and we're ready for it;
       add it to the stream state */
    ogg_stream_pagein(&vf->os,&og);

  }
}

/* if, eg, 64 bit stdio is configured by default, this will build with
   fseek64 */
static int _fseek64_wrap(FILE *f,ogg_int64_t off,int whence){
  if(f==NULL)return(-1);
  return fseek(f,off,whence);
}

static int _ov_open1(void *f,OggVorbis_File *vf,char *initial,
		     long ibytes, ov_callbacks callbacks){
  int offsettest=((f && callbacks.seek_func)?callbacks.seek_func(f,0,SEEK_CUR):-1);
  int ret;
  
  memset(vf,0,sizeof(*vf));
  vf->datasource=f;
  vf->callbacks = callbacks;

  /* init the framing state */
  ogg_sync_init(&vf->oy);

  /* perhaps some data was previously read into a buffer for testing
     against other stream types.  Allow initialization from this
     previously read data (as we may be reading from a non-seekable
     stream) */
  if(initial){
    char *buffer=ogg_sync_buffer(&vf->oy,ibytes);
    memcpy(buffer,initial,ibytes);
    ogg_sync_wrote(&vf->oy,ibytes);
  }

  /* can we seek? Stevens suggests the seek test was portable */
  if(offsettest!=-1)vf->seekable=1;

  /* No seeking yet; Set up a 'single' (current) logical bitstream
     entry for partial open */
  vf->links=1;
  vf->vi=_ogg_calloc(vf->links,sizeof(*vf->vi));
  vf->vc=_ogg_calloc(vf->links,sizeof(*vf->vc));
  ogg_stream_init(&vf->os,-1); /* fill in the serialno later */

  /* Try to fetch the headers, maintaining all the storage */
  if((ret=_fetch_headers(vf,vf->vi,vf->vc,&vf->current_serialno,NULL))<0){
    vf->datasource=NULL;
    ov_clear(vf);
  }else 
    vf->ready_state=PARTOPEN;
  return(ret);
}

static int _ov_open2(OggVorbis_File *vf){
  if(vf->ready_state != PARTOPEN) return OV_EINVAL;
  vf->ready_state=OPENED;
  if(vf->seekable){
    int ret=_open_seekable2(vf);
    if(ret){
      vf->datasource=NULL;
      ov_clear(vf);
    }
    return(ret);
  }else
    vf->ready_state=STREAMSET;

  return 0;
}


/* clear out the OggVorbis_File struct */
int ov_clear(OggVorbis_File *vf){
  if(vf){
    vorbis_block_clear(&vf->vb);
    vorbis_dsp_clear(&vf->vd);
    ogg_stream_clear(&vf->os);
    
    if(vf->vi && vf->links){
      int i;
      for(i=0;i<vf->links;i++){
	vorbis_info_clear(vf->vi+i);
	vorbis_comment_clear(vf->vc+i);
      }
      _ogg_free(vf->vi);
      _ogg_free(vf->vc);
    }
    if(vf->dataoffsets)_ogg_free(vf->dataoffsets);
    if(vf->pcmlengths)_ogg_free(vf->pcmlengths);
    if(vf->serialnos)_ogg_free(vf->serialnos);
    if(vf->offsets)_ogg_free(vf->offsets);
    ogg_sync_clear(&vf->oy);
    if(vf->datasource && vf->callbacks.close_func)
      (vf->callbacks.close_func)(vf->datasource);
    memset(vf,0,sizeof(*vf));
  }
#ifdef DEBUG_LEAKS
  _VDBG_dump();
#endif
  return(0);
}

/* inspects the OggVorbis file and finds/documents all the logical
   bitstreams contained in it.  Tries to be tolerant of logical
   bitstream sections that are truncated/woogie. 

   return: -1) error
            0) OK
*/

int ov_open_callbacks(void *f,OggVorbis_File *vf,char *initial,long ibytes,
    ov_callbacks callbacks){
  int ret=_ov_open1(f,vf,initial,ibytes,callbacks);
  if(ret)return ret;
  return _ov_open2(vf);
}

int ov_open(FILE *f,OggVorbis_File *vf,char *initial,long ibytes){
  ov_callbacks callbacks = {
    (size_t (*)(void *, size_t, size_t, void *))  fread,
    (int (*)(void *, ogg_int64_t, int))              _fseek64_wrap,
    (int (*)(void *))                             fclose,
    (long (*)(void *))                            ftell
  };

  return ov_open_callbacks((void *)f, vf, initial, ibytes, callbacks);
}

int ov_fopen(char *path,OggVorbis_File *vf){
  int ret;
  FILE *f = fopen(path,"rb");
  if(!f) return -1;

  ret = ov_open(f,vf,NULL,0);
  if(ret) fclose(f);
  return ret;
}

 
/* cheap hack for game usage where downsampling is desirable; there's
   no need for SRC as we can just do it cheaply in libvorbis. */
 
int ov_halfrate(OggVorbis_File *vf,int flag){
  int i;
  if(vf->vi==NULL)return OV_EINVAL;
  if(!vf->seekable)return OV_EINVAL;
  if(vf->ready_state>=STREAMSET)
    _decode_clear(vf); /* clear out stream state; later on libvorbis
                          will be able to swap this on the fly, but
                          for now dumping the decode machine is needed
                          to reinit the MDCT lookups.  1.1 libvorbis
                          is planned to be able to switch on the fly */
  
  for(i=0;i<vf->links;i++){
    if(vorbis_synthesis_halfrate(vf->vi+i,flag)){
      ov_halfrate(vf,0);
      return OV_EINVAL;
    }
  }
  return 0;
}

int ov_halfrate_p(OggVorbis_File *vf){
  if(vf->vi==NULL)return OV_EINVAL;
  return vorbis_synthesis_halfrate_p(vf->vi);
}

/* Only partially open the vorbis file; test for Vorbisness, and load
   the headers for the first chain.  Do not seek (although test for
   seekability).  Use ov_test_open to finish opening the file, else
   ov_clear to close/free it. Same return codes as open. */

int ov_test_callbacks(void *f,OggVorbis_File *vf,char *initial,long ibytes,
    ov_callbacks callbacks)
{
  return _ov_open1(f,vf,initial,ibytes,callbacks);
}

int ov_test(FILE *f,OggVorbis_File *vf,char *initial,long ibytes){
  ov_callbacks callbacks = {
    (size_t (*)(void *, size_t, size_t, void *))  fread,
    (int (*)(void *, ogg_int64_t, int))              _fseek64_wrap,
    (int (*)(void *))                             fclose,
    (long (*)(void *))                            ftell
  };

  return ov_test_callbacks((void *)f, vf, initial, ibytes, callbacks);
}
  
int ov_test_open(OggVorbis_File *vf){
  if(vf->ready_state!=PARTOPEN)return(OV_EINVAL);
  return _ov_open2(vf);
}

/* How many logical bitstreams in this physical bitstream? */
long ov_streams(OggVorbis_File *vf){
  return vf->links;
}

/* Is the FILE * associated with vf seekable? */
long ov_seekable(OggVorbis_File *vf){
  return vf->seekable;
}

/* returns the bitrate for a given logical bitstream or the entire
   physical bitstream.  If the file is open for random access, it will
   find the *actual* average bitrate.  If the file is streaming, it
   returns the nominal bitrate (if set) else the average of the
   upper/lower bounds (if set) else -1 (unset).

   If you want the actual bitrate field settings, get them from the
   vorbis_info structs */

long ov_bitrate(OggVorbis_File *vf,int i){
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(i>=vf->links)return(OV_EINVAL);
  if(!vf->seekable && i!=0)return(ov_bitrate(vf,0));
  if(i<0){
    ogg_int64_t bits=0;
    int i;
    float br;
    for(i=0;i<vf->links;i++)
      bits+=(vf->offsets[i+1]-vf->dataoffsets[i])*8;
    /* This once read: return(rint(bits/ov_time_total(vf,-1)));
     * gcc 3.x on x86 miscompiled this at optimisation level 2 and above,
     * so this is slightly transformed to make it work.
     */
    br = bits/ov_time_total(vf,-1);
    return(rint(br));
  }else{
    if(vf->seekable){
      /* return the actual bitrate */
      return(rint((vf->offsets[i+1]-vf->dataoffsets[i])*8/ov_time_total(vf,i)));
    }else{
      /* return nominal if set */
      if(vf->vi[i].bitrate_nominal>0){
	return vf->vi[i].bitrate_nominal;
      }else{
	if(vf->vi[i].bitrate_upper>0){
	  if(vf->vi[i].bitrate_lower>0){
	    return (vf->vi[i].bitrate_upper+vf->vi[i].bitrate_lower)/2;
	  }else{
	    return vf->vi[i].bitrate_upper;
	  }
	}
	return(OV_FALSE);
      }
    }
  }
}

/* returns the actual bitrate since last call.  returns -1 if no
   additional data to offer since last call (or at beginning of stream),
   EINVAL if stream is only partially open 
*/
long ov_bitrate_instant(OggVorbis_File *vf){
  int link=(vf->seekable?vf->current_link:0);
  long ret;
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(vf->samptrack==0)return(OV_FALSE);
  ret=vf->bittrack/vf->samptrack*vf->vi[link].rate+.5;
  vf->bittrack=0.f;
  vf->samptrack=0.f;
  return(ret);
}

/* Guess */
long ov_serialnumber(OggVorbis_File *vf,int i){
  if(i>=vf->links)return(ov_serialnumber(vf,vf->links-1));
  if(!vf->seekable && i>=0)return(ov_serialnumber(vf,-1));
  if(i<0){
    return(vf->current_serialno);
  }else{
    return(vf->serialnos[i]);
  }
}

/* returns: total raw (compressed) length of content if i==-1
            raw (compressed) length of that logical bitstream for i==0 to n
	    OV_EINVAL if the stream is not seekable (we can't know the length)
	    or if stream is only partially open
*/
ogg_int64_t ov_raw_total(OggVorbis_File *vf,int i){
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(!vf->seekable || i>=vf->links)return(OV_EINVAL);
  if(i<0){
    ogg_int64_t acc=0;
    int i;
    for(i=0;i<vf->links;i++)
      acc+=ov_raw_total(vf,i);
    return(acc);
  }else{
    return(vf->offsets[i+1]-vf->offsets[i]);
  }
}

/* returns: total PCM length (samples) of content if i==-1 PCM length
	    (samples) of that logical bitstream for i==0 to n
	    OV_EINVAL if the stream is not seekable (we can't know the
	    length) or only partially open 
*/
ogg_int64_t ov_pcm_total(OggVorbis_File *vf,int i){
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(!vf->seekable || i>=vf->links)return(OV_EINVAL);
  if(i<0){
    ogg_int64_t acc=0;
    int i;
    for(i=0;i<vf->links;i++)
      acc+=ov_pcm_total(vf,i);
    return(acc);
  }else{
    return(vf->pcmlengths[i*2+1]);
  }
}

/* returns: total seconds of content if i==-1
            seconds in that logical bitstream for i==0 to n
	    OV_EINVAL if the stream is not seekable (we can't know the
	    length) or only partially open 
*/
double ov_time_total(OggVorbis_File *vf,int i){
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(!vf->seekable || i>=vf->links)return(OV_EINVAL);
  if(i<0){
    double acc=0;
    int i;
    for(i=0;i<vf->links;i++)
      acc+=ov_time_total(vf,i);
    return(acc);
  }else{
    return((double)(vf->pcmlengths[i*2+1])/vf->vi[i].rate);
  }
}

/* seek to an offset relative to the *compressed* data. This also
   scans packets to update the PCM cursor. It will cross a logical
   bitstream boundary, but only if it can't get any packets out of the
   tail of the bitstream we seek to (so no surprises).

   returns zero on success, nonzero on failure */

int ov_raw_seek(OggVorbis_File *vf,ogg_int64_t pos){
  ogg_stream_state work_os;
  int ret;

  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(!vf->seekable)
    return(OV_ENOSEEK); /* don't dump machine if we can't seek */

  if(pos<0 || pos>vf->end)return(OV_EINVAL);

  /* don't yet clear out decoding machine (if it's initialized), in
     the case we're in the same link.  Restart the decode lapping, and
     let _fetch_and_process_packet deal with a potential bitstream
     boundary */
  vf->pcm_offset=-1;
  ogg_stream_reset_serialno(&vf->os,
			    vf->current_serialno); /* must set serialno */
  vorbis_synthesis_restart(&vf->vd);
    
  ret=_seek_helper(vf,pos);
  if(ret)goto seek_error;

  /* we need to make sure the pcm_offset is set, but we don't want to
     advance the raw cursor past good packets just to get to the first
     with a granulepos.  That's not equivalent behavior to beginning
     decoding as immediately after the seek position as possible.

     So, a hack.  We use two stream states; a local scratch state and
     the shared vf->os stream state.  We use the local state to
     scan, and the shared state as a buffer for later decode. 

     Unfortuantely, on the last page we still advance to last packet
     because the granulepos on the last page is not necessarily on a
     packet boundary, and we need to make sure the granpos is
     correct. 
  */

  {
    ogg_page og;
    ogg_packet op;
    int lastblock=0;
    int accblock=0;
    int thisblock=0;
    int eosflag=0; 

    ogg_stream_init(&work_os,vf->current_serialno); /* get the memory ready */
    ogg_stream_reset(&work_os); /* eliminate the spurious OV_HOLE
                                   return from not necessarily
                                   starting from the beginning */

    while(1){
      if(vf->ready_state>=STREAMSET){
	/* snarf/scan a packet if we can */
	int result=ogg_stream_packetout(&work_os,&op);
      
	if(result>0){

	  if(vf->vi[vf->current_link].codec_setup){
	    thisblock=vorbis_packet_blocksize(vf->vi+vf->current_link,&op);
	    if(thisblock<0){
	      ogg_stream_packetout(&vf->os,NULL);
	      thisblock=0;
	    }else{
	      
	      if(eosflag)
		ogg_stream_packetout(&vf->os,NULL);
	      else
		if(lastblock)accblock+=(lastblock+thisblock)>>2;
	    }	    

	    if(op.granulepos!=-1){
	      int i,link=vf->current_link;
	      ogg_int64_t granulepos=op.granulepos-vf->pcmlengths[link*2];
	      if(granulepos<0)granulepos=0;
	      
	      for(i=0;i<link;i++)
		granulepos+=vf->pcmlengths[i*2+1];
	      vf->pcm_offset=granulepos-accblock;
	      break;
	    }
	    lastblock=thisblock;
	    continue;
	  }else
	    ogg_stream_packetout(&vf->os,NULL);
	}
      }
      
      if(!lastblock){
	if(_get_next_page(vf,&og,-1)<0){
	  vf->pcm_offset=ov_pcm_total(vf,-1);
	  break;
	}
      }else{
	/* huh?  Bogus stream with packets but no granulepos */
	vf->pcm_offset=-1;
	break;
      }
      
      /* has our decoding just traversed a bitstream boundary? */
      if(vf->ready_state>=STREAMSET){
	if(vf->current_serialno!=ogg_page_serialno(&og)){
	  
	  /* two possibilities: 
	     1) our decoding just traversed a bitstream boundary
	     2) another stream is multiplexed into this logical section? */
            
	  if(ogg_page_bos(&og)){
	    /* we traversed */
	    _decode_clear(vf); /* clear out stream state */
	    ogg_stream_clear(&work_os);
	  } /* else, do nothing; next loop will scoop another page */
	}
      }

      if(vf->ready_state<STREAMSET){
	int link;
	long serialno = ogg_page_serialno(&og);

	for(link=0;link<vf->links;link++)
	  if(vf->serialnos[link]==serialno)break;

	if(link==vf->links) continue; /* not the desired Vorbis
					 bitstream section; keep
					 trying */
	vf->current_link=link;
	vf->current_serialno=serialno;
	ogg_stream_reset_serialno(&vf->os,serialno);
	ogg_stream_reset_serialno(&work_os,serialno); 
	vf->ready_state=STREAMSET;
	
      }
    
      ogg_stream_pagein(&vf->os,&og);
      ogg_stream_pagein(&work_os,&og);
      eosflag=ogg_page_eos(&og);
    }
  }

  ogg_stream_clear(&work_os);
  vf->bittrack=0.f;
  vf->samptrack=0.f;
  return(0);

 seek_error:
  /* dump the machine so we're in a known state */
  vf->pcm_offset=-1;
  ogg_stream_clear(&work_os);
  _decode_clear(vf);
  return OV_EBADLINK;
}

/* Page granularity seek (faster than sample granularity because we
   don't do the last bit of decode to find a specific sample).

   Seek to the last [granule marked] page preceeding the specified pos
   location, such that decoding past the returned point will quickly
   arrive at the requested position. */
int ov_pcm_seek_page(OggVorbis_File *vf,ogg_int64_t pos){
  int link=-1;
  ogg_int64_t result=0;
  ogg_int64_t total=ov_pcm_total(vf,-1);
  
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(!vf->seekable)return(OV_ENOSEEK);

  if(pos<0 || pos>total)return(OV_EINVAL);
 
  /* which bitstream section does this pcm offset occur in? */
  for(link=vf->links-1;link>=0;link--){
    total-=vf->pcmlengths[link*2+1];
    if(pos>=total)break;
  }

  /* search within the logical bitstream for the page with the highest
     pcm_pos preceeding (or equal to) pos.  There is a danger here;
     missing pages or incorrect frame number information in the
     bitstream could make our task impossible.  Account for that (it
     would be an error condition) */

  /* new search algorithm by HB (Nicholas Vinen) */
  {
    ogg_int64_t end=vf->offsets[link+1];
    ogg_int64_t begin=vf->offsets[link];
    ogg_int64_t begintime = vf->pcmlengths[link*2];
    ogg_int64_t endtime = vf->pcmlengths[link*2+1]+begintime;
    ogg_int64_t target=pos-total+begintime;
    ogg_int64_t best=begin;
    
    ogg_page og;
    while(begin<end){
      ogg_int64_t bisect;
      
      if(end-begin<CHUNKSIZE){
	bisect=begin;
      }else{
	/* take a (pretty decent) guess. */
	bisect=begin + 
	  (target-begintime)*(end-begin)/(endtime-begintime) - CHUNKSIZE;
	if(bisect<=begin)
	  bisect=begin+1;
      }
      
      result=_seek_helper(vf,bisect);
      if(result) goto seek_error;
      
      while(begin<end){
	result=_get_next_page(vf,&og,end-vf->offset);
	if(result==OV_EREAD) goto seek_error;
	if(result<0){
	  if(bisect<=begin+1)
	    end=begin; /* found it */
	  else{
	    if(bisect==0) goto seek_error;
	    bisect-=CHUNKSIZE;
	    if(bisect<=begin)bisect=begin+1;
	    result=_seek_helper(vf,bisect);
	    if(result) goto seek_error;
	  }
	}else{
	  ogg_int64_t granulepos;

	  if(ogg_page_serialno(&og)!=vf->serialnos[link])
	    continue;

	  granulepos=ogg_page_granulepos(&og);
	  if(granulepos==-1)continue;
	  
	  if(granulepos<target){
	    best=result;  /* raw offset of packet with granulepos */ 
	    begin=vf->offset; /* raw offset of next page */
	    begintime=granulepos;
	    
	    if(target-begintime>44100)break;
	    bisect=begin; /* *not* begin + 1 */
	  }else{
	    if(bisect<=begin+1)
	      end=begin;  /* found it */
	    else{
	      if(end==vf->offset){ /* we're pretty close - we'd be stuck in */
		end=result;
		bisect-=CHUNKSIZE; /* an endless loop otherwise. */
		if(bisect<=begin)bisect=begin+1;
		result=_seek_helper(vf,bisect);
		if(result) goto seek_error;
	      }else{
		end=bisect;
		endtime=granulepos;
		break;
	      }
	    }
	  }
	}
      }
    }

    /* found our page. seek to it, update pcm offset. Easier case than
       raw_seek, don't keep packets preceeding granulepos. */
    {
      ogg_page og;
      ogg_packet op;
      
      /* seek */
      result=_seek_helper(vf,best);
      vf->pcm_offset=-1;
      if(result) goto seek_error;
      result=_get_next_page(vf,&og,-1);
      if(result<0) goto seek_error;
      
      if(link!=vf->current_link){
	/* Different link; dump entire decode machine */
	_decode_clear(vf);  
	
	vf->current_link=link;
	vf->current_serialno=vf->serialnos[link];
	vf->ready_state=STREAMSET;
	
      }else{
	vorbis_synthesis_restart(&vf->vd);
      }

      ogg_stream_reset_serialno(&vf->os,vf->current_serialno);
      ogg_stream_pagein(&vf->os,&og);

      /* pull out all but last packet; the one with granulepos */
      while(1){
	result=ogg_stream_packetpeek(&vf->os,&op);
	if(result==0){
	  /* !!! the packet finishing this page originated on a
             preceeding page. Keep fetching previous pages until we
             get one with a granulepos or without the 'continued' flag
             set.  Then just use raw_seek for simplicity. */
	  
	  result=_seek_helper(vf,best);
	  if(result<0) goto seek_error;
	  
	  while(1){
	    result=_get_prev_page(vf,&og);
	    if(result<0) goto seek_error;
	    if(ogg_page_serialno(&og)==vf->current_serialno &&
	       (ogg_page_granulepos(&og)>-1 ||
		!ogg_page_continued(&og))){
	      return ov_raw_seek(vf,result);
	    }
	    vf->offset=result;
	  }
	}
	if(result<0){
	  result = OV_EBADPACKET; 
	  goto seek_error;
	}
	if(op.granulepos!=-1){
	  vf->pcm_offset=op.granulepos-vf->pcmlengths[vf->current_link*2];
	  if(vf->pcm_offset<0)vf->pcm_offset=0;
	  vf->pcm_offset+=total;
	  break;
	}else
	  result=ogg_stream_packetout(&vf->os,NULL);
      }
    }
  }
  
  /* verify result */
  if(vf->pcm_offset>pos || pos>ov_pcm_total(vf,-1)){
    result=OV_EFAULT;
    goto seek_error;
  }
  vf->bittrack=0.f;
  vf->samptrack=0.f;
  return(0);
  
 seek_error:
  /* dump machine so we're in a known state */
  vf->pcm_offset=-1;
  _decode_clear(vf);
  return (int)result;
}

/* seek to a sample offset relative to the decompressed pcm stream 
   returns zero on success, nonzero on failure */

int ov_pcm_seek(OggVorbis_File *vf,ogg_int64_t pos){
  int thisblock,lastblock=0;
  int ret=ov_pcm_seek_page(vf,pos);
  if(ret<0)return(ret);
  if((ret=_make_decode_ready(vf)))return ret;

  /* discard leading packets we don't need for the lapping of the
     position we want; don't decode them */

  while(1){
    ogg_packet op;
    ogg_page og;

    int ret=ogg_stream_packetpeek(&vf->os,&op);
    if(ret>0){
      thisblock=vorbis_packet_blocksize(vf->vi+vf->current_link,&op);
      if(thisblock<0){
	ogg_stream_packetout(&vf->os,NULL);
	continue; /* non audio packet */
      }
      if(lastblock)vf->pcm_offset+=(lastblock+thisblock)>>2;
      
      if(vf->pcm_offset+((thisblock+
			  vorbis_info_blocksize(vf->vi,1))>>2)>=pos)break;
      
      /* remove the packet from packet queue and track its granulepos */
      ogg_stream_packetout(&vf->os,NULL);
      vorbis_synthesis_trackonly(&vf->vb,&op);  /* set up a vb with
                                                   only tracking, no
                                                   pcm_decode */
      vorbis_synthesis_blockin(&vf->vd,&vf->vb); 
      
      /* end of logical stream case is hard, especially with exact
	 length positioning. */
      
      if(op.granulepos>-1){
	int i;
	/* always believe the stream markers */
	vf->pcm_offset=op.granulepos-vf->pcmlengths[vf->current_link*2];
	if(vf->pcm_offset<0)vf->pcm_offset=0;
	for(i=0;i<vf->current_link;i++)
	  vf->pcm_offset+=vf->pcmlengths[i*2+1];
      }
	
      lastblock=thisblock;
      
    }else{
      if(ret<0 && ret!=OV_HOLE)break;
      
      /* suck in a new page */
      if(_get_next_page(vf,&og,-1)<0)break;
      if(ogg_page_bos(&og))_decode_clear(vf);
      
      if(vf->ready_state<STREAMSET){
	long serialno=ogg_page_serialno(&og);
	int link;
	
	for(link=0;link<vf->links;link++)
	  if(vf->serialnos[link]==serialno)break;
	if(link==vf->links) continue; 
	vf->current_link=link;
	
	vf->ready_state=STREAMSET;      
	vf->current_serialno=ogg_page_serialno(&og);
	ogg_stream_reset_serialno(&vf->os,serialno); 
	ret=_make_decode_ready(vf);
	if(ret)return ret;
	lastblock=0;
      }

      ogg_stream_pagein(&vf->os,&og);
    }
  }

  vf->bittrack=0.f;
  vf->samptrack=0.f;
  /* discard samples until we reach the desired position. Crossing a
     logical bitstream boundary with abandon is OK. */
  while(vf->pcm_offset<pos){
    ogg_int64_t target=pos-vf->pcm_offset;
    long samples=vorbis_synthesis_pcmout(&vf->vd,NULL);

    if(samples>target)samples=target;
    vorbis_synthesis_read(&vf->vd,samples);
    vf->pcm_offset+=samples;
    
    if(samples<target)
      if(_fetch_and_process_packet(vf,NULL,1,1)<=0)
	vf->pcm_offset=ov_pcm_total(vf,-1); /* eof */
  }
  return 0;
}

/* seek to a playback time relative to the decompressed pcm stream 
   returns zero on success, nonzero on failure */
int ov_time_seek(OggVorbis_File *vf,double seconds){
  /* translate time to PCM position and call ov_pcm_seek */

  int link=-1;
  ogg_int64_t pcm_total=0;
  double time_total=0.;

  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(!vf->seekable)return(OV_ENOSEEK);
  if(seconds<0)return(OV_EINVAL);
  
  /* which bitstream section does this time offset occur in? */
  for(link=0;link<vf->links;link++){
    double addsec = ov_time_total(vf,link);
    if(seconds<time_total+addsec)break;
    time_total+=addsec;
    pcm_total+=vf->pcmlengths[link*2+1];
  }

  if(link==vf->links)return(OV_EINVAL);

  /* enough information to convert time offset to pcm offset */
  {
    ogg_int64_t target=pcm_total+(seconds-time_total)*vf->vi[link].rate;
    return(ov_pcm_seek(vf,target));
  }
}

/* page-granularity version of ov_time_seek 
   returns zero on success, nonzero on failure */
int ov_time_seek_page(OggVorbis_File *vf,double seconds){
  /* translate time to PCM position and call ov_pcm_seek */

  int link=-1;
  ogg_int64_t pcm_total=0;
  double time_total=0.;

  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(!vf->seekable)return(OV_ENOSEEK);
  if(seconds<0)return(OV_EINVAL);
  
  /* which bitstream section does this time offset occur in? */
  for(link=0;link<vf->links;link++){
    double addsec = ov_time_total(vf,link);
    if(seconds<time_total+addsec)break;
    time_total+=addsec;
    pcm_total+=vf->pcmlengths[link*2+1];
  }

  if(link==vf->links)return(OV_EINVAL);

  /* enough information to convert time offset to pcm offset */
  {
    ogg_int64_t target=pcm_total+(seconds-time_total)*vf->vi[link].rate;
    return(ov_pcm_seek_page(vf,target));
  }
}

/* tell the current stream offset cursor.  Note that seek followed by
   tell will likely not give the set offset due to caching */
ogg_int64_t ov_raw_tell(OggVorbis_File *vf){
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  return(vf->offset);
}

/* return PCM offset (sample) of next PCM sample to be read */
ogg_int64_t ov_pcm_tell(OggVorbis_File *vf){
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  return(vf->pcm_offset);
}

/* return time offset (seconds) of next PCM sample to be read */
double ov_time_tell(OggVorbis_File *vf){
  int link=0;
  ogg_int64_t pcm_total=0;
  double time_total=0.f;
  
  if(vf->ready_state<OPENED)return(OV_EINVAL);
  if(vf->seekable){
    pcm_total=ov_pcm_total(vf,-1);
    time_total=ov_time_total(vf,-1);
  
    /* which bitstream section does this time offset occur in? */
    for(link=vf->links-1;link>=0;link--){
      pcm_total-=vf->pcmlengths[link*2+1];
      time_total-=ov_time_total(vf,link);
      if(vf->pcm_offset>=pcm_total)break;
    }
  }

  return((double)time_total+(double)(vf->pcm_offset-pcm_total)/vf->vi[link].rate);
}

/*  link:   -1) return the vorbis_info struct for the bitstream section
                currently being decoded
           0-n) to request information for a specific bitstream section
    
    In the case of a non-seekable bitstream, any call returns the
    current bitstream.  NULL in the case that the machine is not
    initialized */

vorbis_info *ov_info(OggVorbis_File *vf,int link){
  if(vf->seekable){
    if(link<0)
      if(vf->ready_state>=STREAMSET)
	return vf->vi+vf->current_link;
      else
      return vf->vi;
    else
      if(link>=vf->links)
	return NULL;
      else
	return vf->vi+link;
  }else{
    return vf->vi;
  }
}

/* grr, strong typing, grr, no templates/inheritence, grr */
vorbis_comment *ov_comment(OggVorbis_File *vf,int link){
  if(vf->seekable){
    if(link<0)
      if(vf->ready_state>=STREAMSET)
	return vf->vc+vf->current_link;
      else
	return vf->vc;
    else
      if(link>=vf->links)
	return NULL;
      else
	return vf->vc+link;
  }else{
    return vf->vc;
  }
}

static int host_is_big_endian() {
  ogg_int32_t pattern = 0xfeedface; /* deadbeef */
  unsigned char *bytewise = (unsigned char *)&pattern;
  if (bytewise[0] == 0xfe) return 1;
  return 0;
}

/* up to this point, everything could more or less hide the multiple
   logical bitstream nature of chaining from the toplevel application
   if the toplevel application didn't particularly care.  However, at
   the point that we actually read audio back, the multiple-section
   nature must surface: Multiple bitstream sections do not necessarily
   have to have the same number of channels or sampling rate.

   ov_read returns the sequential logical bitstream number currently
   being decoded along with the PCM data in order that the toplevel
   application can take action on channel/sample rate changes.  This
   number will be incremented even for streamed (non-seekable) streams
   (for seekable streams, it represents the actual logical bitstream
   index within the physical bitstream.  Note that the accessor
   functions above are aware of this dichotomy).

   input values: buffer) a buffer to hold packed PCM data for return
		 length) the byte length requested to be placed into buffer
		 bigendianp) should the data be packed LSB first (0) or
		             MSB first (1)
		 word) word size for output.  currently 1 (byte) or 
		       2 (16 bit short)

   return values: <0) error/hole in data (OV_HOLE), partial open (OV_EINVAL)
                   0) EOF
		   n) number of bytes of PCM actually returned.  The
		   below works on a packet-by-packet basis, so the
		   return length is not related to the 'length' passed
		   in, just guaranteed to fit.

	    *section) set to the logical bitstream number */

long ov_read(OggVorbis_File *vf,char *buffer,int length,
		    int bigendianp,int word,int sgned,int *bitstream){
  int i,j;
  int host_endian = host_is_big_endian();

  float **pcm;
  long samples;

  if(vf->ready_state<OPENED)return(OV_EINVAL);

  while(1){
    if(vf->ready_state==INITSET){
      samples=vorbis_synthesis_pcmout(&vf->vd,&pcm);
      if(samples)break;
    }

    /* suck in another packet */
    {
      int ret=_fetch_and_process_packet(vf,NULL,1,1);
      if(ret==OV_EOF)
	return(0);
      if(ret<=0)
	return(ret);
    }

  }

  if(samples>0){
  
    /* yay! proceed to pack data into the byte buffer */
    
    long channels=ov_info(vf,-1)->channels;
    long bytespersample=word * channels;
    vorbis_fpu_control fpu;
    if(samples>length/bytespersample)samples=length/bytespersample;

    if(samples <= 0)
      return OV_EINVAL;
    
    /* a tight loop to pack each size */
    {
      int val;
      if(word==1){
	int off=(sgned?0:128);
	vorbis_fpu_setround(&fpu);
	for(j=0;j<samples;j++)
	  for(i=0;i<channels;i++){
	    val=vorbis_ftoi(pcm[i][j]*128.f);
	    if(val>127)val=127;
	    else if(val<-128)val=-128;
	    *buffer++=val+off;
	  }
	vorbis_fpu_restore(fpu);
      }else{
	int off=(sgned?0:32768);
	
	if(host_endian==bigendianp){
	  if(sgned){
	    
	    vorbis_fpu_setround(&fpu);
	    for(i=0;i<channels;i++) { /* It's faster in this order */
	      float *src=pcm[i];
	      short *dest=((short *)buffer)+i;
	      for(j=0;j<samples;j++) {
		val=vorbis_ftoi(src[j]*32768.f);
		if(val>32767)val=32767;
		else if(val<-32768)val=-32768;
		*dest=val;
		dest+=channels;
	      }
	    }
	    vorbis_fpu_restore(fpu);
	    
	  }else{
	    
	    vorbis_fpu_setround(&fpu);
	    for(i=0;i<channels;i++) {
	      float *src=pcm[i];
	      short *dest=((short *)buffer)+i;
	      for(j=0;j<samples;j++) {
		val=vorbis_ftoi(src[j]*32768.f);
		if(val>32767)val=32767;
		else if(val<-32768)val=-32768;
		*dest=val+off;
		dest+=channels;
	      }
	    }
	    vorbis_fpu_restore(fpu);
	    
	  }
	}else if(bigendianp){
	  
	  vorbis_fpu_setround(&fpu);
	  for(j=0;j<samples;j++)
	    for(i=0;i<channels;i++){
	      val=vorbis_ftoi(pcm[i][j]*32768.f);
	      if(val>32767)val=32767;
	      else if(val<-32768)val=-32768;
	      val+=off;
	      *buffer++=(val>>8);
	      *buffer++=(val&0xff);
	    }
	  vorbis_fpu_restore(fpu);
	  
	}else{
	  int val;
	  vorbis_fpu_setround(&fpu);
	  for(j=0;j<samples;j++)
	    for(i=0;i<channels;i++){
	      val=vorbis_ftoi(pcm[i][j]*32768.f);
	      if(val>32767)val=32767;
	      else if(val<-32768)val=-32768;
	      val+=off;
	      *buffer++=(val&0xff);
	      *buffer++=(val>>8);
	  	}
	  vorbis_fpu_restore(fpu);  
	  
	}
      }
    }
    
    vorbis_synthesis_read(&vf->vd,samples);
    vf->pcm_offset+=samples;
    if(bitstream)*bitstream=vf->current_link;
    return(samples*bytespersample);
  }else{
    return(samples);
  }
}

/* input values: pcm_channels) a float vector per channel of output
		 length) the sample length being read by the app

   return values: <0) error/hole in data (OV_HOLE), partial open (OV_EINVAL)
                   0) EOF
		   n) number of samples of PCM actually returned.  The
		   below works on a packet-by-packet basis, so the
		   return length is not related to the 'length' passed
		   in, just guaranteed to fit.

	    *section) set to the logical bitstream number */



long ov_read_float(OggVorbis_File *vf,float ***pcm_channels,int length,
		   int *bitstream){

  if(vf->ready_state<OPENED)return(OV_EINVAL);

  while(1){
    if(vf->ready_state==INITSET){
      float **pcm;
      long samples=vorbis_synthesis_pcmout(&vf->vd,&pcm);
      if(samples){
	if(pcm_channels)*pcm_channels=pcm;
	if(samples>length)samples=length;
	vorbis_synthesis_read(&vf->vd,samples);
	vf->pcm_offset+=samples;
	if(bitstream)*bitstream=vf->current_link;
	return samples;

      }
    }

    /* suck in another packet */
    {
      int ret=_fetch_and_process_packet(vf,NULL,1,1);
      if(ret==OV_EOF)return(0);
      if(ret<=0)return(ret);
    }

  }
}

extern float *vorbis_window(vorbis_dsp_state *v,int W);
extern void _analysis_output_always(char *base,int i,float *v,int n,int bark,int dB,
			     ogg_int64_t off);

static void _ov_splice(float **pcm,float **lappcm,
		       int n1, int n2,
		       int ch1, int ch2,
		       float *w1, float *w2){
  int i,j;
  float *w=w1;
  int n=n1;

  if(n1>n2){
    n=n2;
    w=w2;
  }

  /* splice */
  for(j=0;j<ch1 && j<ch2;j++){
    float *s=lappcm[j];
    float *d=pcm[j];

    for(i=0;i<n;i++){
      float wd=w[i]*w[i];
      float ws=1.-wd;
      d[i]=d[i]*wd + s[i]*ws;
    }
  }
  /* window from zero */
  for(;j<ch2;j++){
    float *d=pcm[j];
    for(i=0;i<n;i++){
      float wd=w[i]*w[i];
      d[i]=d[i]*wd;
    }
  }

}
		
/* make sure vf is INITSET */
static int _ov_initset(OggVorbis_File *vf){
  while(1){
    if(vf->ready_state==INITSET)break;
    /* suck in another packet */
    {
      int ret=_fetch_and_process_packet(vf,NULL,1,0);
      if(ret<0 && ret!=OV_HOLE)return(ret);
    }
  }
  return 0;
}

/* make sure vf is INITSET and that we have a primed buffer; if
   we're crosslapping at a stream section boundary, this also makes
   sure we're sanity checking against the right stream information */
static int _ov_initprime(OggVorbis_File *vf){
  vorbis_dsp_state *vd=&vf->vd;
  while(1){
    if(vf->ready_state==INITSET)
      if(vorbis_synthesis_pcmout(vd,NULL))break;
    
    /* suck in another packet */
    {
      int ret=_fetch_and_process_packet(vf,NULL,1,0);
      if(ret<0 && ret!=OV_HOLE)return(ret);
    }
  }  
  return 0;
}

/* grab enough data for lapping from vf; this may be in the form of
   unreturned, already-decoded pcm, remaining PCM we will need to
   decode, or synthetic postextrapolation from last packets. */
static void _ov_getlap(OggVorbis_File *vf,vorbis_info *vi,vorbis_dsp_state *vd,
		       float **lappcm,int lapsize){
  int lapcount=0,i;
  float **pcm;

  /* try first to decode the lapping data */
  while(lapcount<lapsize){
    int samples=vorbis_synthesis_pcmout(vd,&pcm);
    if(samples){
      if(samples>lapsize-lapcount)samples=lapsize-lapcount;
      for(i=0;i<vi->channels;i++)
	memcpy(lappcm[i]+lapcount,pcm[i],sizeof(**pcm)*samples);
      lapcount+=samples;
      vorbis_synthesis_read(vd,samples);
    }else{
    /* suck in another packet */
      int ret=_fetch_and_process_packet(vf,NULL,1,0); /* do *not* span */
      if(ret==OV_EOF)break;
    }
  }
  if(lapcount<lapsize){
    /* failed to get lapping data from normal decode; pry it from the
       postextrapolation buffering, or the second half of the MDCT
       from the last packet */
    int samples=vorbis_synthesis_lapout(&vf->vd,&pcm);
    if(samples==0){
      for(i=0;i<vi->channels;i++)
	memset(lappcm[i]+lapcount,0,sizeof(**pcm)*lapsize-lapcount);
      lapcount=lapsize;
    }else{
      if(samples>lapsize-lapcount)samples=lapsize-lapcount;
      for(i=0;i<vi->channels;i++)
	memcpy(lappcm[i]+lapcount,pcm[i],sizeof(**pcm)*samples);
      lapcount+=samples;
    }
  }
}

/* this sets up crosslapping of a sample by using trailing data from
   sample 1 and lapping it into the windowing buffer of sample 2 */
int ov_crosslap(OggVorbis_File *vf1, OggVorbis_File *vf2){
  vorbis_info *vi1,*vi2;
  float **lappcm;
  float **pcm;
  float *w1,*w2;
  int n1,n2,i,ret,hs1,hs2;

  if(vf1==vf2)return(0); /* degenerate case */
  if(vf1->ready_state<OPENED)return(OV_EINVAL);
  if(vf2->ready_state<OPENED)return(OV_EINVAL);

  /* the relevant overlap buffers must be pre-checked and pre-primed
     before looking at settings in the event that priming would cross
     a bitstream boundary.  So, do it now */

  ret=_ov_initset(vf1);
  if(ret)return(ret);
  ret=_ov_initprime(vf2);
  if(ret)return(ret);

  vi1=ov_info(vf1,-1);
  vi2=ov_info(vf2,-1);
  hs1=ov_halfrate_p(vf1);
  hs2=ov_halfrate_p(vf2);

  lappcm=alloca(sizeof(*lappcm)*vi1->channels);
  n1=vorbis_info_blocksize(vi1,0)>>(1+hs1);
  n2=vorbis_info_blocksize(vi2,0)>>(1+hs2);
  w1=vorbis_window(&vf1->vd,0);
  w2=vorbis_window(&vf2->vd,0);

  for(i=0;i<vi1->channels;i++)
    lappcm[i]=alloca(sizeof(**lappcm)*n1);

  _ov_getlap(vf1,vi1,&vf1->vd,lappcm,n1);

  /* have a lapping buffer from vf1; now to splice it into the lapping
     buffer of vf2 */
  /* consolidate and expose the buffer. */
  vorbis_synthesis_lapout(&vf2->vd,&pcm);
  _analysis_output_always("pcmL",0,pcm[0],n1*2,0,0,0);
  _analysis_output_always("pcmR",0,pcm[1],n1*2,0,0,0);

  /* splice */
  _ov_splice(pcm,lappcm,n1,n2,vi1->channels,vi2->channels,w1,w2);
  
  /* done */
  return(0);
}

static int _ov_64_seek_lap(OggVorbis_File *vf,ogg_int64_t pos,
			   int (*localseek)(OggVorbis_File *,ogg_int64_t)){
  vorbis_info *vi;
  float **lappcm;
  float **pcm;
  float *w1,*w2;
  int n1,n2,ch1,ch2,hs;
  int i,ret;

  if(vf->ready_state<OPENED)return(OV_EINVAL);
  ret=_ov_initset(vf);
  if(ret)return(ret);
  vi=ov_info(vf,-1);
  hs=ov_halfrate_p(vf);
  
  ch1=vi->channels;
  n1=vorbis_info_blocksize(vi,0)>>(1+hs);
  w1=vorbis_window(&vf->vd,0);  /* window arrays from libvorbis are
				   persistent; even if the decode state
				   from this link gets dumped, this
				   window array continues to exist */

  lappcm=alloca(sizeof(*lappcm)*ch1);
  for(i=0;i<ch1;i++)
    lappcm[i]=alloca(sizeof(**lappcm)*n1);
  _ov_getlap(vf,vi,&vf->vd,lappcm,n1);

  /* have lapping data; seek and prime the buffer */
  ret=localseek(vf,pos);
  if(ret)return ret;
  ret=_ov_initprime(vf);
  if(ret)return(ret);

 /* Guard against cross-link changes; they're perfectly legal */
  vi=ov_info(vf,-1);
  ch2=vi->channels;
  n2=vorbis_info_blocksize(vi,0)>>(1+hs);
  w2=vorbis_window(&vf->vd,0);

  /* consolidate and expose the buffer. */
  vorbis_synthesis_lapout(&vf->vd,&pcm);

  /* splice */
  _ov_splice(pcm,lappcm,n1,n2,ch1,ch2,w1,w2);

  /* done */
  return(0);
}

int ov_raw_seek_lap(OggVorbis_File *vf,ogg_int64_t pos){
  return _ov_64_seek_lap(vf,pos,ov_raw_seek);
}

int ov_pcm_seek_lap(OggVorbis_File *vf,ogg_int64_t pos){
  return _ov_64_seek_lap(vf,pos,ov_pcm_seek);
}

int ov_pcm_seek_page_lap(OggVorbis_File *vf,ogg_int64_t pos){
  return _ov_64_seek_lap(vf,pos,ov_pcm_seek_page);
}

static int _ov_d_seek_lap(OggVorbis_File *vf,double pos,
			   int (*localseek)(OggVorbis_File *,double)){
  vorbis_info *vi;
  float **lappcm;
  float **pcm;
  float *w1,*w2;
  int n1,n2,ch1,ch2,hs;
  int i,ret;

  if(vf->ready_state<OPENED)return(OV_EINVAL);
  ret=_ov_initset(vf);
  if(ret)return(ret);
  vi=ov_info(vf,-1);
  hs=ov_halfrate_p(vf);

  ch1=vi->channels;
  n1=vorbis_info_blocksize(vi,0)>>(1+hs);
  w1=vorbis_window(&vf->vd,0);  /* window arrays from libvorbis are
				   persistent; even if the decode state
				   from this link gets dumped, this
				   window array continues to exist */

  lappcm=alloca(sizeof(*lappcm)*ch1);
  for(i=0;i<ch1;i++)
    lappcm[i]=alloca(sizeof(**lappcm)*n1);
  _ov_getlap(vf,vi,&vf->vd,lappcm,n1);

  /* have lapping data; seek and prime the buffer */
  ret=localseek(vf,pos);
  if(ret)return ret;
  ret=_ov_initprime(vf);
  if(ret)return(ret);

 /* Guard against cross-link changes; they're perfectly legal */
  vi=ov_info(vf,-1);
  ch2=vi->channels;
  n2=vorbis_info_blocksize(vi,0)>>(1+hs);
  w2=vorbis_window(&vf->vd,0);

  /* consolidate and expose the buffer. */
  vorbis_synthesis_lapout(&vf->vd,&pcm);

  /* splice */
  _ov_splice(pcm,lappcm,n1,n2,ch1,ch2,w1,w2);

  /* done */
  return(0);
}

int ov_time_seek_lap(OggVorbis_File *vf,double pos){
  return _ov_d_seek_lap(vf,pos,ov_time_seek);
}

int ov_time_seek_page_lap(OggVorbis_File *vf,double pos){
  return _ov_d_seek_lap(vf,pos,ov_time_seek_page);
}
