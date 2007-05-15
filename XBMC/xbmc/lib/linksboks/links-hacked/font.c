/* Generic font handling */

#include "links.h"

inline static struct font *find_font_with_char(int, struct font **, int *, int *);
inline static struct letter *find_char_in_font(struct font *, int);
static inline int compute_width (int ix, int iy, int required_height);
static void prune_font_cache(struct graphics_driver *gd);

int n_fonts;

struct list_head styles={&styles,&styles};

struct list fontlist={&fontlist, &fontlist, 0, -1, NULL};

int dither_letters;

struct lru font_cache; /* This is a cache for screen-ready colored bitmaps
                        * of lettrs and/or alpha channels for these (are the
			* same size, only one byte per pixel and are used
			* for letters with an image under them )
			*/


/* This shall be hopefully reasonably fast and portable
 * We assume ix is <65536. If not, the letters will be smaller in
 * horizontal dimension (because of overflow) but this will not cause
 * a segfault. 65536 pixels wide bitmaps are not normal and will anyway
 * exhaust the memory.
 */

/* These constants represent contrast-enhancement and sharpen filter (which is one filter
 * together) that is applied onto the letters to enhance their appearance at low height.
 * They were determined by experiment for several values, interpolated, checked and tuned.
 * If you change them, don't wonder if the letters start to look strange.
 * The numers in the comments denote which height the line applies for.
 */
float fancy_constants[64]={
	0,3,		/*  1 */
	.1,3,   	/*  2 */
	.2,3,   	/*  3 */
	.3,2.9,   	/*  4 */
	.4,2.7,   	/*  5 */
	.4,2.5,   	/*  6 */
	.4,2,   	/*  7 */
	.5,2, 		/*  8 */
	.4,2, 		/*  9 */
	.38,1.9, 	/* 10 */
	.36,1.8, 	/* 11 */
	.33,1.7, 	/* 12 */
	.30,1.6, 	/* 13 */
	.25,1.5, 	/* 14 */
	.20,1.5, 	/* 15 */
	.15,1.5, 	/* 16 */
	.14,1.5, 	/* 17 */
	.13,1.5, 	/* 18 */
	.12,1.5, 	/* 19 */
	.12,1.5, 	/* 20 */
	.12,1.5, 	/* 21 */
	.12,1.5, 	/* 22 */
	.11,1.5, 	/* 23 */
	.10,1.4, 	/* 24 */
	.09,1.3, 	/* 25 */
	.08,1.3, 	/* 26 */
	.04,1.2, 	/* 27 */
	.04,1.2, 	/* 28 */
	.02,1.1, 	/* 29 */
	.02,1.1, 	/* 30 */
	.01,1, 		/* 31 */
	.01,1  		/* 32 */
};

struct letter null_letter={0,100,100,NULL};

void load_scaled_char(int code,void **dest, int *x, int y, struct font *font, struct style *style)
{
	unsigned char *interm;
	unsigned char *interm2;
	unsigned char *i2ptr,*dptr;
	int ix, iy, y0, x0, c;
	float conv0, conv1,sharpness,contrast;

        font->get_char(font,code,&interm,&ix,&iy);

        if (style->mono_space>=0)
		*x=compute_width(style->mono_space, style->mono_height, y);
	else
		*x=compute_width(ix,iy,y);
        if (display_optimize) *x*=3;
	scale_gray(interm, ix,iy, (unsigned char **)dest, *x, y);
        if (y>32||y<=0) return ; /* No convolution */
	ix=*x+2; /* There is one-pixel border around */
	iy=y+2;
	interm2=mem_alloc(ix*iy);
	i2ptr=interm2+ix+1;
	dptr=*dest;
	memset(interm2,0,ix);
	memset(interm2+(iy-1)*ix,0,ix);
	for (y0=y;y0;y0--){
		i2ptr[-1]=0;
		memcpy(i2ptr,dptr,*x);
		i2ptr[ix-1]=0;
		i2ptr+=ix;
		dptr+=*x;
	}
	i2ptr=interm2+ix+1;
	dptr=*dest;

	/* Determine the sharpness and contrast */
	sharpness=fancy_constants[2*y-2];
	contrast=fancy_constants[2*y-1];

	/* Compute the matrix constants from contrast and sharpness */
	conv0=(1+sharpness)*contrast;
	conv1=-sharpness*0.25*contrast;
	
	for (y0=y;y0;y0--){
		for (x0=*x;x0;x0--){
			/* Convolution */
			c=((*i2ptr)*conv0)+i2ptr[-ix]*conv1+i2ptr[-1]*conv1+i2ptr[1]*conv1+i2ptr[ix]*conv1+0.5;
			if (((unsigned)c)>=256) c=c<0?0:255;
			*dptr=c;
			dptr++;
			i2ptr++;
		}
		i2ptr+=2;
	}
	mem_free(interm2);
}

/* Adds required entry into font_cache and returns pointer to the entry.
 */
inline static struct font_cache_entry *supply_color_cache_entry (struct graphics_driver *gd, struct font *font, struct style *style, struct letter *letter)
{
	struct font_cache_entry *found, *new;
	unsigned short *primary_data;
	int do_free=0;
	struct font_cache_entry template;
	unsigned short red, green, blue;
	unsigned bytes_consumed;

	template.bitmap.y=style->height;

        found=mem_alloc(sizeof(*found));
        found->bitmap.y=style->height;
        if(letter == &null_letter){
                /* Draw null letter - just a rectangle */
                found->bitmap.x=found->bitmap.y*0.5;
                if(style->mono_space>0)
                        found->bitmap.x=compute_width(style->mono_space,style->mono_height,style->height);

                if(display_optimize) found->bitmap.x*=3;
                found->bitmap.data=mem_calloc(found->bitmap.x*found->bitmap.y);
                {
                        int x,y;
                        unsigned char *data=found->bitmap.data;
                        int xw=found->bitmap.x;
                        int x0=0.2*found->bitmap.x;
                        int x1=0.8*found->bitmap.x;
                        int yw=found->bitmap.y;
                        int y0=0.2*found->bitmap.y;
                        int y1=0.8*found->bitmap.y;

                        for(x=x0;x<x1;x++){
                                data[x+xw*y0]=255;
                                data[x+(y1-1)*xw]=255;
                        }
                        for(y=y0;y<y1;y++){
                                data[y*xw+x0]=255;
                                data[y*xw+x1]=255;
                        }
                }
        } else
                load_scaled_char(letter->code, &(found->bitmap.data), &(found->bitmap.x), found->bitmap.y, font, style);

        new=mem_alloc(sizeof(*new));
	new->bitmap=found->bitmap;
	new->r0=style->r0;
	new->g0=style->g0;
	new->b0=style->b0;
	new->r1=style->r1;
	new->g1=style->g1;
	new->b1=style->b1;
	new->mono_space=style->mono_space;
	new->mono_height=style->mono_height;

	primary_data=mem_alloc(3
			*new->bitmap.x*new->bitmap.y*sizeof(*primary_data));

	/* We assume the gamma of HTML styles is in sRGB space */
	round_color_sRGB_to_48(&red, &green, &blue,
		((style->r0)<<16)|((style->g0)<<8)|(style->b0));
	mix_two_colors(primary_data, found->bitmap.data,
		found->bitmap.x*found->bitmap.y,
		red,green,blue,
		apply_gamma_single_8_to_16(style->r1,user_gamma/sRGB_gamma),
		apply_gamma_single_8_to_16(style->g1,user_gamma/sRGB_gamma),
		apply_gamma_single_8_to_16(style->b1,user_gamma/sRGB_gamma)
	);
	if (display_optimize){
		/* A correction for LCD */
		new->bitmap.x/=3;
		decimate_3(&primary_data,new->bitmap.x,new->bitmap.y);
	}

        /* We have a buffer with photons */
	gd->get_empty_bitmap(&(new->bitmap));
	if (dither_letters)
		dither(primary_data, &(new->bitmap));
	else
		(*round_fn)(primary_data,&(new->bitmap));
	mem_free(primary_data);
	gd->register_bitmap(&(new->bitmap));

        mem_free(found->bitmap.data);
        mem_free(found);

        bytes_consumed=new->bitmap.x*new->bitmap.y*(gd->depth&7);
	/* Number of bytes per pixel in passed bitmaps */
	bytes_consumed+=sizeof(*new);
	bytes_consumed+=sizeof(struct lru_entry);
	lru_insert(&font_cache, new, &(letter->list), bytes_consumed);
	return new;
}



/* Prints a letter to the specified position and
 * returns the width of the printed letter */
inline static int print_letter(struct graphics_driver *gd, struct graphics_device *device, int x, int y, struct style *style, int code)
{	
        struct font *font;
	struct font_cache_entry *found;
	struct font_cache_entry template;
        struct letter *letter;

        font=find_font_with_char(code, style->table,NULL,NULL);
        if(!font) {
                letter=&null_letter;
        } else
                letter=find_char_in_font(font,code);

	template.r0=style->r0;
	template.r1=style->r1;
	template.g0=style->g0;
	template.g1=style->g1;
	template.b0=style->b0;
	template.b1=style->b1;
	template.bitmap.y=style->height;
	template.mono_space=style->mono_space;
	template.mono_height=style->mono_height;

	found=lru_lookup(&font_cache, &template, letter->list);
	if (!found) found=supply_color_cache_entry(gd, font, style, letter);
	gd->draw_bitmap(device, &(found->bitmap), x, y);
	prune_font_cache(gd);
        return found->bitmap.x;
}

/* Must return values that are:
 * >=0
 * <=height
 * at least 1 apart
 * Otherwise g_print_text will print nonsense (but won't segfault)
 */
inline static void get_underline_pos(int height, int *top, int *bottom)
{
	int thickness, baseline;
	thickness=(height+15)/16;
	baseline=height/7;
	if (baseline<=0) baseline=1;
	if (thickness>baseline) thickness=baseline;
	*top=height-baseline;
	*bottom=*top+thickness;
}

/* *width will be advanced by the width of the text */
void g_print_text(struct graphics_driver *gd, struct graphics_device *device,
int x, int y, struct style *style, unsigned char *text, int *width)
{
	int original_flags, top_underline, bottom_underline, original_width,
		my_width;
	struct rect saved_clip;

        if (y+style->height<=device->clip.y1||y>=device->clip.y2) goto o;
	if (style -> flags){
		/* Underline */
		if (!width){
		       width=&my_width;
		       *width=0;
		}
		original_flags=style->flags;
		original_width=*width;
		style -> flags=0;
		get_underline_pos(style->height, &top_underline, &bottom_underline);
		restrict_clip_area(device, &saved_clip, 0, 0, device->size.x2, y+
			top_underline);
		g_print_text(gd, device, x, y, style, text, width);
		gd->set_clip_area(device, &saved_clip);
		if (bottom_underline-top_underline==1){
			/* Line */
			drv->draw_hline(device, x, y+top_underline
				, x+*width-original_width
				, style->underline_color);
		}else{
			/* Area */
			drv->fill_area(device, x, y+top_underline,
					x+*width-original_width
					,y+bottom_underline,
					style->underline_color);
		}
		if (bottom_underline<style->height){
			/* Do the bottom half only if the underline is above
			 * the bottom of the letters.
			 */
			*width=original_width;
			restrict_clip_area(device, &saved_clip, 0,
				y+bottom_underline, device->size.x2,
				device->size.y2);
			g_print_text(gd, device, x, y, style, text, width);
			gd->set_clip_area(device, &saved_clip);
		}
		style -> flags=original_flags;
		return;
	}
	while (*text){
		int p;
		int u;
		GET_UTF_8(text, u);
		/* 00-09, 0b-1f, 80, 81, 84, 86-9f ignorovat
		 * 0a = LF
		 * 82 = ' '
		 * 83 = nobrk
		 * 85 = radkovy zlom
		 * a0 = NBSP
		 * ad = soft hyphen
		 */
		#if 0
		if (	(u>=0x00&&u<=0x09)||
			(u>=0x0b&&u<=0x1f)||
			u==0x80||
			u==0x82||
			u==0x84||
			(u>=0x86&&u<=0x9f)
		)continue;
		if (u==0x82)u=' ';
		#endif
		/* stare Mikulasovo patchovani, musim to opravit    -- Brain */
		if (!u || u == 0xad) continue;
		if (u == 0x01 || u == 0xa0) u = ' ';
		p=print_letter(gd,device,x,y,style, u);
		x += p;
		if (width) {
			*width += p;
			continue;
		}
		if (x>=device->clip.x2) return;
	}
	return;
	o:
                if (width) *width += g_text_width(style, text);
}


static inline int compute_width (int ix, int iy, int required_height)
{
	int width;
	unsigned long reg;
	
	reg=(unsigned long)aspect*(unsigned long)required_height;
	
	if (reg>=0x1000000UL){
		/* It's big */
		reg=(reg+32768)>>16;
		width=(reg*ix+(iy>>1))/iy;
	}else{
		/* It's small */
		reg=(reg+128)>>8;
		iy<<=8;
		width=(reg*ix+(iy>>1))/iy;
	}
	if (width<1) width=1;
	return width;
}

static void recode_font_name(unsigned char **name)
{
	int dashes=0;
	unsigned char *p;

        if ((!strcmp(*name,"monospaced"))||
            (!strcmp(*name,"monospace"))){
                unsigned char *tmpname=init_str();
                int len=0;
                add_to_str(&tmpname,&len,options_get("default_font_family_mono"));
                add_to_str(&tmpname,&len,"-medium-roman-serif-mono");
                *name=tmpname;
        }
        else if (!strcmp(*name,"")){
                unsigned char *tmpname=init_str();
                int len=0;
                add_to_str(&tmpname,&len,options_get("default_font_family_vari"));
                add_to_str(&tmpname,&len,"-medium-roman-serif-vari");
                *name=tmpname;
        }
	p=*name;
	while(*p){
		if (*p=='-')dashes++;
		p++;
	}
        if (dashes!=4){
                unsigned char *tmpname=init_str();
                int len=0;
                add_to_str(&tmpname,&len,options_get("default_font_family_vari"));
                add_to_str(&tmpname,&len,"-medium-roman-serif-vari");
                *name=tmpname;
        }
}

/* Compares single=a multi=b-c-a as matching.
 * 0 matches
 * 1 doesn't match
 */
inline static int compare_family(unsigned char *single, unsigned char *multi)
{
	unsigned char *p,*r;
	int single_length=strlen(single);

	r=multi;
	while(1){
		p=r;
		while (*r&&*r!='-')r++;
                if ((r-p==single_length)&&
                    !strncmp(single,p,r-p))
                        return 0;

                if (!*r)
                        return 1;

		r++;
	}
	return 1;
}

int compute_n_fonts()
{
        struct font *font;
        int N=0;

        foreach(font,fontlist)
                N++;

        return N;
}

/* Input name must contain exactly 4 dashes, otherwise the
 * result is undefined (parsing into weight, slant, adstyl, spacing
 * will result deterministically random results).
 * Returns 1 if the font is monospaced or 0 if not.
 */

static int fill_style_table(struct font **table, unsigned char *name)
{
	unsigned char *p;
	unsigned char *family, *weight, *slant, *adstyl, *spacing;
	int pass,result,f;
	int masks[6]={0x1f,0x1f,0xf,0x7,0x3,0x1};
	int xors[6]={0,0x10,0x8,0x4,0x2,0x1};
	/* Top bit of the values belongs to family, bottom to spacing */
	int monospaced;
	
        /* Parse the name */
	recode_font_name(&name);
	family=stracpy(name);
	p=family;
	while(*p&&*p!='-') p++;
	*p=0;
	p++;
	weight=p;
	while(*p&&*p!='-') p++;
	*p=0;
	p++;
	slant=p;
	while(*p&&*p!='-') p++;
	*p=0;
	p++;
	adstyl=p;
	while(*p&&*p!='-') p++;
	*p=0;
	p++;
	spacing=p;
	monospaced=!strcmp(spacing,"mono");
	
	for (pass=0;pass<6;pass++){
                struct font *font;

                foreach(font,fontlist){
                        int r0=compare_family(family,font->family);
                        int r1=!!strcmp(weight,font->weight);
                        int r2=!!strcmp(slant,font->slant);
                        int r3=!!strcmp(adstyl,font->adstyl);
                        int r4=!!strcmp(spacing,font->spacing);

                        result=r0;
			result<<=1;
			result|=r1;
			result<<=1;
			result|=r2;
			result<<=1;
			result|=r3;
			result<<=1;
			result|=r4;
			result^=xors[pass];
			result&=masks[pass];
                        if (!result){ /* Font complies */
                                /* printf("%s %s %s %s -> %s %s %s %s\n",family,weight,slant,spacing,font->family,font->weight,font->slant,font->spacing); */
                                *table++=font;
                        }
		}
	}
        mem_free(family);
	return monospaced;
}

struct style *g_invert_style(struct style *old)
{
	int length;

	struct style *st;
	st = mem_alloc(sizeof(struct style));
	st->refcount=1;
	st->r0=old->r1;
	st->g0=old->g1;
	st->b0=old->b1;
	st->r1=old->r0;
	st->g1=old->g0;
	st->b1=old->b0;
	st->height=old->height;
	st->flags=old->flags;
	if (st->flags)
	{
		/* We have to get a foreground color for underlining */
		st->underline_color=dip_get_color_sRGB(
			(st->r1<<16)|(st->g1<<8)|(st->b1));
	}
	length=sizeof(*st->table)*(n_fonts);
	st->table=mem_alloc(length);
	memcpy(st->table,old->table,length);
        st->font=stracpy(old->font);
        st->mono_space=old->mono_space;
	st->mono_height=old->mono_height;
        add_to_list(styles,st);
        return st;
}

/* Never returns NULL. */
struct style *g_get_style(int fg, int bg, int size, unsigned char *font, int flags)
{
	struct style *st;

        st = mem_alloc(sizeof(struct style));
        st->font=stracpy(font);
        st->refcount = 1;
	st->r0 = bg >> 16;
	st->g0 = (bg >> 8) & 255;
	st->b0 = bg & 255;
	st->r1 = fg >> 16;
	st->g1 = (fg >> 8) & 255;
	st->b1 = fg & 255;
	if (size<=0) size=1;
	st->height = size;
	st->flags=flags&FF_UNDERLINE;
        /* Commented out as we need to underline active links */
        /*if (st->flags)*/
	{
		/* We have to get a foreground color for underlining */
		st->underline_color=dip_get_color_sRGB(fg);
	}
	st->table=mem_alloc(sizeof(*(st->table))*(n_fonts));
	if(fill_style_table(st->table, font))
		load_metric(&(st->mono_space), &(st->mono_height),' ',st->table);
	else
		st->mono_space=-1;
        add_to_list(styles,st);
        return st;
}

struct style *g_clone_style(struct style *st)
{
	st->refcount++;
	return st;
}

void g_free_style(struct style *st)
{
        if(--st->refcount) return;
	del_from_list(st);
        mem_free(st->table);
        mem_free(st->font);
        mem_free(st);
}


/* 0=equality 1=inequality */
/* what's the crazy notation??? */
static int compare_font_entries(void *entry, void *template)
{
	struct font_cache_entry*e1=entry;
	struct font_cache_entry*e2=template;

        return(
		 (e1->r0!=e2->r0)||
		 (e1->g0!=e2->g0)||
		 (e1->b0!=e2->b0)||
		 (e1->r1!=e2->r1)||
		 (e1->g1!=e2->g1)||
		 (e1->b1!=e2->b1)||
		 (e1->bitmap.y!=e2->bitmap.y)||
		 (e1->mono_space!=e2->mono_space)||
		 (e1->mono_space>=0&&e1->mono_height!=e2->mono_height));
}

/* If the cache already exists, it is destroyed and reallocated. If you call it with the same
 * size argument, only a cache flush will yield.
 */
void init_font_cache(int bytes)
{
	lru_init(&font_cache, &compare_font_entries, bytes);
}

/* Ensures there are no lru_entry objects allocated - destroys them.
 * Also destroys the bitmaps asociated with them. Does not destruct the
 font_cache per se.
 */
void destroy_font_cache()
{
	struct font_cache_entry *bottom;
	
	while((bottom=lru_get_bottom(&font_cache))){
                drv->unregister_bitmap(&(bottom->bitmap));
		mem_free(bottom);
		lru_destroy_bottom(&font_cache);
	}
}

/* Prunes the cache to comply with maximum size */
static void prune_font_cache(struct graphics_driver *gd)
{
	struct font_cache_entry *bottom;

	while(font_cache.bytes>font_cache.max_bytes){
		/* Prune bottom entry of font cache */
		bottom=lru_get_bottom(&font_cache);
		if (!bottom){
#ifdef DEBUG
			if (font_cache.bytes||font_cache.items){
				internal("font cache is empty and contains some items or bytes at the same time.\n");
			}
#endif
			break;
		}
                gd->unregister_bitmap(&(bottom->bitmap));
		mem_free(bottom);
		lru_destroy_bottom(&font_cache);
	}
}

inline static struct letter *find_char_in_font(struct font *font, int code)
{
        int first=0;
        int last=font->n_letters-1;

        while(first<=last){
                int half=(first+last)>>1;
                int diff=font->letter[half].code-code;
                if (diff>=0){
                        if (diff==0){
                                return &(font->letter[half]);
				}else{
                                /* Value in table is bigger */
                                last=half-1;
                                }
                }else{
                        /* Value in the table is smaller */
                        first=half+1;
                }
        }
        return NULL;
}

inline static struct font *find_font_with_char(int code, struct font **table, int *xw, int *yw)
{
        int i;

        if(!table || !*table) return NULL;

        for(i=0;i<n_fonts;i++){
                struct font *font=table[i];
                struct letter *letter=find_char_in_font(font, code);
                if(letter){
                        if(xw) *xw=letter->xsize;
                        if(yw) *yw=letter->ysize;
                        return font;
                }
        }
        /* None found */
        return NULL;
}

inline static void load_metric(int *x, int *y, int code, struct font **table)
{
        int i;
        struct font *font;

        font=find_font_with_char(code,table,x,y);
        if((!font) || (!*x) || (!*y)){
                *x=0;
                *y=0;
        }
}


/* Returns 0 in case the char is not found. */
static inline int g_get_width(struct style *style, int charcode)
{
	int x, y, width;

	if (style->mono_space>=0){
		x=style->mono_space;
		y=style->mono_height;
        } else
                load_metric(&x,&y,charcode,style->table);

        if (!(x&&y))
                width=0.5*style->height;
        else
                width=compute_width(x,y,style->height);

        return width;
}

int g_text_width(struct style *style, unsigned char *text)
{
	int w = 0;
	while (*text) {
		int u;
		GET_UTF_8(text, u);
		if (!u || u == 0xad) continue;
		if (u == 0x01 || u == 0xa0) u = ' ';
		w += g_get_width(style, u);
	}
	return w;
}

int g_char_width(struct style *style, int charcode)
{
	return g_get_width(style, charcode);
}

int g_wrap_text(struct wrap_struct *w)
{
	while (*w->text) {
		int u;
		int s;
		if (*w->text == ' ') w->last_wrap = w->text,
				     w->last_wrap_obj = w->obj;
		GET_UTF_8(w->text, u);
		if (!u) continue;
		if (u == 0x01 || u == 0xa0) u = ' ';
		s = g_get_width(w->style, u);
		if (u == 0xad) s = 0;
		if ((w->pos += s) <= w->width) {
			c:
			if (u != 0xad || *w->text == ' ') continue;
			s = g_char_width(w->style, '-');
			if (w->pos + s <= w->width || (!w->last_wrap && !w->last_wrap_obj)) {
				w->last_wrap = w->text;
				w->last_wrap_obj = w->obj;
				continue;
			}
		}
		if (!w->last_wrap && !w->last_wrap_obj) goto c;
		return 0;
	}
	return 1;
}

struct font *create_font(unsigned char *family, unsigned char *weight, unsigned char *slant, unsigned char *adstyl, unsigned char *spacing)
{
        struct font *font = mem_alloc(sizeof(struct font));

        font->type = 0;

        font->family = stracpy(family);
        font->weight = stracpy(weight);
        font->slant = stracpy(slant);
        font->adstyl = stracpy(adstyl);
        font->spacing = stracpy(spacing);
        font->n_letters = 0;
        font->letter = NULL;
        font->filename = NULL;

        return font;
}

void register_font(struct font *font)
{
        if(!font) return;
        add_to_list(fontlist,font);
        n_fonts=compute_n_fonts();
}

void init_fonts()
{
		init_font_cache(options_get_int("cache_fonts_size"));
        init_list(fontlist);
        init_builtin_fonts();
#ifdef HAVE_FREETYPE
        init_freetype();
#endif
#ifdef XBOX_USE_XFONT)
		init_xfont();
#endif
        load_fonts_table();
}

void finalize_fonts()
{
        struct font *font;

        destroy_font_cache();
        save_fonts_table();

        foreach(font,fontlist){
                if(font->free_font) font->free_font(font);
                if(font->family) mem_free(font->family);
                if(font->weight) mem_free(font->weight);
                if(font->slant) mem_free(font->slant);
                if(font->adstyl) mem_free(font->adstyl);
                if(font->spacing) mem_free(font->spacing);
                if(font->letter) mem_free(font->letter);
                if(font->filename) mem_free(font->filename);
        }
#ifdef HAVE_FREETYPE
        finalize_freetype();
#endif
#ifdef XBOX_USE_XFONT
		finalize_xfont();
#endif
        finalize_builtin_fonts();

        free_list(fontlist);
}

/*
 Font table file format:

 int font_type;  Currently FONT_TYPE_FREETYPE only
 int filename_length;
 unsigned char *filename;
 int n_letters;
 struct letter letters[];
*/

void save_fonts_table()
{
        struct font *font;
        unsigned char *filename;
        int file;

        if (list_empty(fontlist))
                return;

        filename = stracpy(links_home);
        if (!filename)
		return;

        add_to_strn(&filename, "fonttable");
        file = open(filename, O_CREAT|O_TRUNC|O_WRONLY,S_IREAD|S_IWRITE);
        mem_free(filename);
        if (file<0)
                return;

        /* Cycle through all non-builtin fonts */
        foreachback(font,fontlist){
                if(font->font_type!=FONT_TYPE_BUILTIN){
                        int len=strlen(font->filename);
                        write(file,&(font->font_type),sizeof(int));
                        write(file,&len,sizeof(int));
                        write(file,font->filename,len);
                        write(file,&(font->n_letters),sizeof(int));
                        write(file,font->letter,sizeof(struct letter)*font->n_letters);
                }
        }
        close(file);
}

void load_fonts_table()
{
        struct font *font;
        unsigned char *filename;
        int file;
        int font_type;

        /*if(list_empty(fontlist))  return;*/

        filename = stracpy(links_home);
	if (!filename)
		return;

        add_to_strn(&filename, "fonttable");
        file = open(filename, O_RDONLY);
        mem_free(filename);

        if (file<0)
                return;

        while(read(file,&font_type,sizeof(int))>0){
                int len,n_letters;
                unsigned char *font_filename;
                struct letter *letter;

                /* TODO: check for fonttable sanity */

                read(file,&len,sizeof(int));
                font_filename=mem_alloc(len+1);
                read(file,font_filename,len);
                font_filename[len]='\0';

                read(file,&n_letters,sizeof(int));
                letter=mem_alloc(sizeof(struct letter)*n_letters);
                read(file,letter,sizeof(struct letter)*n_letters);

                switch(font_type){
#ifdef HAVE_FREETYPE
                case FONT_TYPE_FREETYPE:{
                        struct font *font=create_ft_font(font_filename);
                        if(font){
                                font->n_letters=n_letters;
                                font->letter=letter;
                                register_font(font);
                        }
                        break;
                }
#endif
#ifdef XBOX_USE_XFONT
                case FONT_TYPE_XFONT:{
                        struct font *font=create_xfont_font(font_filename);
                        if(font){
                                font->n_letters=n_letters;
                                font->letter=letter;
                                register_font(font);
                        }
                        break;
                }
#endif
				default:
                        if(letter) mem_free(letter);
                }
                mem_free(font_filename);
        }
        close(file);
}

/* We need to recreate all style tables after deleting fonts */
void recreate_style_tables()
{
        struct style *style;

        if(list_empty(fontlist))
                return;

        n_fonts=compute_n_fonts();

        foreach(style,styles){
                if(style->table)
                        mem_free(style->table);
                style->table=mem_alloc(sizeof(*(style->table))*(n_fonts));
                if(fill_style_table(style->table, style->font))
                        load_metric(&(style->mono_space), &(style->mono_height),' ',style->table);
                else
                        style->mono_space=-1;
        }
}

