/* gif.c
 * GIF parser
   (c) 2002 Karel 'Clock' Kulhavy
   This file is a part of the Links program, released under GPL.
*/

/* TODO: remove superfluous deco->im_width and deco->im_height */

#include "cfg.h"
#include "links.h"

#ifdef G

#ifdef HAVE_MATH_H
#include <math.h>
#endif

/****************************** Functions *************************************/

/* Takes the argument from global_cimg. Does not free the gif_decoder
 * struct itself. */
void gif_destroy_decoder(struct cached_image *cimg)
{
	struct gif_decoder *deco=cimg->decoder;

	if ((cimg->state==12||cimg->state==14)&&cimg->strip_optimized) mem_free(deco->actual_line);
	if (deco->color_map) mem_free(deco->color_map);
}

/* colors: number of triplets (color entries) */
void
alloc_color_map(int colors)
{
 struct gif_decoder* deco=global_cimg->decoder;

 if (deco->color_map) mem_free(deco->color_map);
 deco->color_map=mem_alloc(colors*3*sizeof(*(deco->color_map)));
}

/* 
   Initialize code table: construct codes 0...(1<<code_size)-1 with values
   0...(1<<code_size)-1 Codes (1<<code_size) and (1<<code_size)+1 are
   left intact -- they are of no use.
   Initializes CC and EOI
   Zeroes out the last_code. In normal data stream the first code must be
   in the table. However, if corrupted stream is to be received, a cause could
   happen that the first code would be out of table and as last code would
   be used something uninitialized and something very strange could happen
   (drawing a pixel from previous image or an infinite loop in outputting
   string)
*/
void
init_table()
{
 int i;
 struct gif_decoder *deco;

 deco=global_cimg->decoder;
 deco->code_size=deco->initial_code_size;
 deco->first_code=1;
 for (i=0;i<1<<deco->code_size;i++)
 {
  deco->table[i].pointer=-1;
  deco->table[i].end_char=i;
 }
 /* Here i=1<<code_size. */
 deco->CC=i;
 deco->EOI=i+1;
 deco->table_pos=i+2;
 for (;i<4096;i++)
 {
  deco->table[i].pointer=-2; 
 }
 deco->code_size++;
 deco->last_code=0;
}

/* 
   Outputs a single pixel.
   if end_callback_hit gets set, do not send any more data in.
*/
static inline void 
output_pixel(int c)
{
	struct gif_decoder *deco;
	struct cached_image *cimg=global_cimg;

	deco=global_cimg->decoder;
	if (c>=1<<deco->im_bpp){
		end_callback_hit=1;
		return;
	}
	deco->actual_line[deco->xoff*3]=deco->color_map[c*3];
	deco->actual_line[deco->xoff*3+1]=deco->color_map[c*3+1];
	deco->actual_line[deco->xoff*3+2]=deco->color_map[c*3+2];
	deco->xoff++;
	if (deco->xoff>=deco->im_width)
	{
		deco->xoff=0;
		global_cimg->rows_added=1;
		if (deco->interl_dist==1)
		{ 
			if (global_cimg->strip_optimized){
				buffer_to_bitmap_incremental(cimg
					,deco->actual_line, 1, deco->yoff,
					cimg->dregs, 1);
	  		}else{
	  			deco->actual_line+=deco->im_width*3;
	  		}
			deco->yoff++;
  		}else{
			int skip=deco->interl_dist&15;
   			int n=(deco->interl_dist==24)
				?8:(deco->interl_dist>>1);
			unsigned char *ptr;
   			int y;

			ptr=deco->actual_line+deco->im_width*3;
			for (y=deco->yoff+1;y<deco->im_height
				&&y<deco->yoff+n;y++){
				memcpy(ptr,deco
					->actual_line,deco->im_width*3);
				ptr+=deco->im_width*3;
			}
			deco->actual_line+=deco->im_width*3*skip;
			deco->yoff+=skip;
		}
		while (deco->yoff>=deco->im_height)
		{
			/* The vertical range is complete. */
			if (deco->interl_dist<=2)
			{
				end_callback_hit=1;
				return;
			}else{
				deco->interl_dist=(deco->interl_dist==24)
					?8:(deco->interl_dist>>1);
	    			deco->yoff=deco->interl_dist>>1;
				deco->actual_line=global_cimg
					->buffer+deco->yoff*3*deco->im_width;
   			}
  		}
 	}
}

/* Finds the first char of output string for given codeword. */
static inline int
find_first(int c)
{
	struct gif_decoder *deco;
	int p;
	int i;

	deco=global_cimg->decoder;
	for (i=0;i<4096;i++){
		p=deco->table[c].pointer;
		if (p==-1) break;
		if (p<-1||p>=4096) return 0;
		c=p;
		}
	return deco->table[c].end_char;
}

/* GIF code
   Supply a code and it outputs the string for c. 
   if end_callback_hit is set then it should not be called anymore.
*/
static inline void
output_string(int c)
{
	int pos=0;
	struct gif_decoder *deco;
 
	deco=global_cimg->decoder;
	while(1){
		if (pos==4096){
	  		/* Cycle in string */
		  	end_callback_hit=1;
		  	return;
		}
		deco->table[pos].garbage=deco->table[c].end_char;
		pos++;
		c=deco->table[c].pointer;
		if (c<0) break; /* We are at the end */
	}
	for (pos--;pos>=0;pos--)
	{
		output_pixel(deco->table[pos].garbage);
		if (end_callback_hit) return;
	}
}

/* Adds to the code table
   return value: 0 ok
                 1 fatal error
		 2 stop sending data
*/

static inline void
add_table(struct gif_decoder *deco,int end_char,int pointer)
{
	if (deco->table_pos>=4096){
	 	end_callback_hit=1;
	 	return; /* Overflow */
 	}
	deco->table[deco->table_pos].end_char=end_char;
	deco->table[deco->table_pos].pointer=pointer;
	deco->table_pos++;
	if (deco->table_pos==(1<<deco->code_size)&&deco->code_size!=12)
	{
		/* Table pos is a power of 2 */
		deco->code_size++;
	}
}

/* Yout throw inside a codeword and it processes it farther
   If the code==256, it means that end-of-file came.
   This is part of GIF code.
   If sets up end_callback_hit, no more codes should be sent into accept_codee.
*/
static inline void
accept_code(struct gif_decoder *deco,int c)
{
 int k;
 
 //k=k; /* To suppress warning */
 if (c>4096||c<0) return; /* Erroneous code word will be ignored */
 if (c==deco->CC) {
  init_table();
  return;
 }
 
 if (c==deco->EOI)
 {
  end_callback_hit=1;
  return;
 }
 
 if (deco->first_code)
 {
  deco->first_code=0;
  /* First code after init_table */
  /* Action: output the string for code */
  output_string(c);
  if (end_callback_hit) return;
  deco->last_code=c;
  return;
 }
 
 if (c>=deco->table_pos)
 {
  /* The code is not in the table */
  k=find_first(deco->last_code);
 }
 else
 {
  /* The code is in code table */
  k=find_first(c);
 }
  
 add_table(deco,k,deco->last_code);
 if (end_callback_hit) return;
 
 /* Output the string for code */
 output_string(c);
 if (end_callback_hit) return;
 deco->last_code=c; /* Update last code. */
}


/* You throw inside a byte, it depacks the bits out and makes code and then
   passes to the decoder (no headers, palettes etc. go through this.)
   sets end_callback_hit to 1 when no more data should be put in.
*/
static inline void
accept_byte(unsigned char c)
{
 int original_code_size;
 struct gif_decoder *deco;
 
 deco=global_cimg->decoder;
 deco->read_code|=(unsigned long)c<<deco->bits_read;  
 deco->bits_read+=8;
 while (deco->bits_read>=deco->code_size)
 {
  /* We have collected up a whole code word. */
  original_code_size=deco->code_size;
  accept_code(deco,deco->read_code&((1<<deco->code_size)-1));
  if (end_callback_hit) return;
  deco->read_code>>=original_code_size;
  deco->bits_read-=original_code_size;
 }
}

/* if deco->transparent >=0, then fill it with transparent colour.
 * actual line must exist, must be set to the beginning of the image,
 * and the buffer must be formatted. */
void implant_transparent(struct gif_decoder *deco)
{
	if (deco->transparent>=0&&deco->transparent<(1<<deco->im_bpp)){
		if (global_cimg->strip_optimized){
			compute_background_8(deco->color_map+3*deco->transparent,
				global_cimg);	
		}else{
			memcpy(deco->color_map+3*deco->transparent
				,deco->actual_line,3);
	        }
	}
}

/* Dimensions are in deco->im_width and deco->im_height */
void gif_dimensions_known(void)
{
	struct gif_decoder *deco;

	deco=global_cimg->decoder;
	global_cimg->buffer_bytes_per_pixel=3;
	global_cimg->width=deco->im_width;
	global_cimg->height=deco->im_height;
	global_cimg->red_gamma=global_cimg->green_gamma
		=global_cimg->blue_gamma=sRGB_gamma;
#ifdef __XBOX__
	global_cimg->strip_optimized = 0;
#else
	global_cimg->strip_optimized=(deco->interl_dist==1
		&&deco->im_width*deco->im_height>=65536);
	/* Run strip_optimized only from 65536 pixels up */
#endif
	header_dimensions_known(global_cimg);
}
	
/* Accepts one byte from GIF codestream
   Caller is responsible for destorying the decoder when
   end_callback_hit is 1.
*/
void inline
gif_accept_byte(int c)
{
	struct gif_decoder *deco;

	deco=global_cimg->decoder;

	switch(deco->state)
	{
		case 0: /* Reading signature and screen descriptor -- 13 bytes */
		deco->tbuf[deco->tlen]=c;
		deco->tlen++;
		if (deco->tlen>=13){
			if (strncmp(deco->tbuf,"GIF87a",6)
				&&strncmp(deco->tbuf,"GIF89a",6)){
	   			end_callback_hit=1;
	   			return; /* Invalid GIF header */
			}
			deco->im_bpp=(deco->tbuf[10]&7)+1;
			deco->tlen=0;
			if (deco->tbuf[10]<128){
				/* No global color map follows */
				deco->state=2; /* Reading image data */
			}else{
				/* Read global color map */
				alloc_color_map(1<<deco->im_bpp);
				deco->state=1;
			}
		}
		break;

		case 1: /* Reading global color map -- 3*(1<<im_bpp) bytes in GIF*/
		deco->color_map[deco->tlen]=c;
		deco->tlen++;
		if (deco->tlen>=3*(1<<deco->im_bpp)){
			deco->state=2;
			deco->tlen=0;
		}
		break;
  
		case 2: /* Reading garbage before and one ',' or '!' in GIF */
		switch (c){
			case ',':
			if (deco->im_width){
				/* Double header encountered */
				end_callback_hit=1;
				return;
			}
			deco->state=3;
			deco->tlen=0;
			break;
   
			case '!':
			deco->state=7;
			break;
		}
		break;

		case 3: /* Reading image descriptor -- 9 bytes in GIF */
		deco->tbuf[deco->tlen]=c;
		deco->tlen++;
		if (deco->tlen>=9){
			deco->im_width=deco->tbuf[4]+(deco->tbuf[5]<<8);
			deco->im_height=deco->tbuf[6]+(deco->tbuf[7]<<8);
			if (deco->im_width<=0||deco->im_height<=0){
				end_callback_hit=1;
				return; /* Bad dimensions */
			}
			if (deco->tbuf[8]&64){
				/* Interlaced order */
				deco->interl_dist=24; /* Actually 8, the 16 indicates 
						       * it is the first pass. */
			}else
				deco->interl_dist=1;
			gif_dimensions_known();
			deco->actual_line=global_cimg->strip_optimized
				?mem_alloc(global_cimg->width*global_cimg
				->buffer_bytes_per_pixel)
				:global_cimg->buffer;
			if (deco->tbuf[8]&128){
				deco->im_bpp=1+(deco->tbuf[8]&7);
				deco->tlen=0;
				alloc_color_map(1<<deco->im_bpp);
				deco->state=4;
			}else{
				deco->state=5;
				deco->tlen=0;
				deco->xoff=0;
				deco->yoff=0;
			}
	 	}	 
		break;

		case 4: /* Reading local colormap in GIF */
		deco->color_map[deco->tlen]=c;
		deco->tlen++;
		if (deco->tlen>=3*(1<<deco->im_bpp)){
			deco->state=5;
			deco->xoff=0;
			deco->yoff=0;
		}
		break;

		case 5: /* Reading code size block in GIF */
		deco->initial_code_size=c;
  		if (deco->initial_code_size<=1||deco->initial_code_size>8){
			end_callback_hit=1;
			return; /* Invalid initial code size */
		}
		if (!deco->color_map){
	 		end_callback_hit=1;
	 		return;
  		}
  		deco->bits_read=0;
		deco->read_code=0;  /* Nothing read */
		init_table(); /* Decoding is about to begin sets up code_size. */
		deco->state=6;
		deco->tlen=0;
		deco->remains=0;
		implant_transparent(deco);
		break;

		case 6: /* Reading image data in GIF */
	  	if (!deco->remains){
			/* This byte is a count byte. Feed it into remains. */
			deco->remains=c;
			if (!c){
    /* 0 count = end of data. */
    end_callback_hit=1; /* Don't send any following data */
    return;
   }
  } 
  else
  {
   accept_byte(c);
   if (end_callback_hit) return; /* No more data wanted */
   deco->remains--;
  }
  break;
  
  case 7: /* Reading a byte after '!' in GIF */
  if (c==0xf9) deco->state=9;
  else deco->state=8;
  deco->remains=0;
  deco->tlen=0;
  break;

  case 8: /* Skipping ignored blocks in GIF */
  case 9: /* Graphics control block block size */
  if (!deco->remains)
  {
   /* Byte count awaited */
   if (!c)
   {
    /* End. :-) */
    deco->state=2; /* Go and wait for ',' */
    deco->tlen=0;
    break;
   }
   deco->remains=c;
  }
  else
  {
   if (deco->state==9&&deco->tlen==3) deco->transparent=deco->transparent?c:-1;
   if (deco->state==9&&deco->tlen==0) deco->transparent=c&1;
   deco->remains--;
   deco->tlen++;
  }
  break;
 }
}

void gif_start(struct cached_image *cimg)
{
	struct gif_decoder *deco;

	deco=mem_calloc(sizeof(*deco));
	deco->transparent=-1;
	cimg->decoder=deco;
}

void gif_restart_internal(unsigned char *data, int length)
{
	struct gif_decoder *deco;

	deco=global_cimg->decoder;
	while(length){
		gif_accept_byte(*data);
		if (end_callback_hit) return;
		data++;
		length--;
	}
}

void gif_restart(unsigned char *data, int length)
{
#ifdef DEBUG
	if (!global_cimg->decoder) internal("NULL decoder in gif_restart");
#endif /* #ifdef DEBUG */
	end_callback_hit=0;
	gif_restart_internal(data, length);
	if (end_callback_hit) img_end(global_cimg);
}


#endif /* #ifdef G */
