#include "goom_config.h"
#include "gfontrle.h"
#include "gfontlib.h"
#include <string.h>
#include <stdlib.h>

static Pixel  ***font_chars;
static int    *font_width;
static int    *font_height;
static Pixel  ***small_font_chars;
static int    *small_font_width;
static int    *small_font_height;

void gfont_load (void) {
	unsigned char *gfont;
	unsigned int i = 0, j = 0;
	unsigned int nba = 0;
	unsigned int current = 32;
        int    *font_pos;
	/* decompress le rle */

        
        
	gfont = malloc (the_font.width*the_font.height*the_font.bytes_per_pixel);
	while (i<the_font.rle_size) {
		unsigned char c = the_font.rle_pixel [i++];
		if (c == 0) {
			unsigned int nb = the_font.rle_pixel [i++];
			while (nb--)
				gfont[j++] = 0;
		}
		else
			gfont [j++] = c;
	}

	/* determiner les positions de chaque lettre. */

        font_height = calloc (256,sizeof(int));
        small_font_height = calloc (256,sizeof(int));
        font_width = calloc (256,sizeof(int));
        small_font_width = calloc (256,sizeof(int));
        font_chars = calloc (256,sizeof(int**));
        small_font_chars = calloc (256,sizeof(int**));
        font_pos = calloc (256,sizeof(int));

	for (i=0;i<the_font.width;i++) {
		unsigned char a = gfont [i*4 + 3];
		if (a)
			nba ++;
		else
			nba = 0;
		if (nba == 2) {
                    font_width [current] = i - font_pos [current];
                    small_font_width [current] = font_width [current]/2;
                        font_pos [++current] = i;
                        font_height [current] = the_font.height - 2;
                        small_font_height [current] = font_height [current]/2;
		}
	}
	font_pos [current] = 0;
        font_height [current] = 0;
        small_font_height [current] = 0;
	
	/* charger les lettres et convertir au format de la machine */
	
	for (i=33;i<current;i++) {
		int x; int y;
                font_chars [i] = malloc (font_height[i]*sizeof(int *));
                small_font_chars [i] = malloc (font_height[i]*sizeof(int *)/2);
                for (y = 0; y < font_height[i]; y++) {
                    font_chars [i][y] = malloc (font_width[i]*sizeof(int));
                    for (x = 0; x < font_width[i]; x++) {
                        unsigned int r,g,b,a;
                        r = gfont[(y+2)*(the_font.width*4)+(x*4+font_pos[i]*4)];
                        g = gfont[(y+2)*(the_font.width*4)+(x*4+font_pos[i]*4+1)];
                        b = gfont[(y+2)*(the_font.width*4)+(x*4+font_pos[i]*4+2)];
                        a = gfont[(y+2)*(the_font.width*4)+(x*4+font_pos[i]*4+3)];
                        font_chars [i][y][x].val =
                            (r<<(ROUGE*8))|(g<<(VERT*8))|(b<<(BLEU*8))|(a<<(ALPHA*8));
                    }
                }
                for (y = 0; y < font_height[i]/2; y++) {
                    small_font_chars [i][y] = malloc (font_width[i]*sizeof(int)/2);
                    for (x = 0; x < font_width[i]/2; x++) {
                        unsigned int r1,g1,b1,a1,r2,g2,b2,a2,r3,g3,b3,a3,r4,g4,b4,a4;
                        r1 = gfont[2*(y+1)*(the_font.width*4)+(x*8+font_pos[i]*4)];
                        g1 = gfont[2*(y+1)*(the_font.width*4)+(x*8+font_pos[i]*4+1)];
                        b1 = gfont[2*(y+1)*(the_font.width*4)+(x*8+font_pos[i]*4+2)];
                        a1 = gfont[2*(y+1)*(the_font.width*4)+(x*8+font_pos[i]*4+3)];
                        r2 = gfont[(2*y+3)*(the_font.width*4)+(x*8+font_pos[i]*4+4)];
                        g2 = gfont[(2*y+3)*(the_font.width*4)+(x*8+font_pos[i]*4+5)];
                        b2 = gfont[(2*y+3)*(the_font.width*4)+(x*8+font_pos[i]*4+6)];
                        a2 = gfont[(2*y+3)*(the_font.width*4)+(x*8+font_pos[i]*4+7)];
                        r3 = gfont[(2*y+3)*(the_font.width*4)+(x*8+font_pos[i]*4)];
                        g3 = gfont[(2*y+3)*(the_font.width*4)+(x*8+font_pos[i]*4+1)];
                        b3 = gfont[(2*y+3)*(the_font.width*4)+(x*8+font_pos[i]*4+2)];
                        a3 = gfont[(2*y+3)*(the_font.width*4)+(x*8+font_pos[i]*4+3)];
                        r4 = gfont[2*(y+1)*(the_font.width*4)+(x*8+font_pos[i]*4+4)];
                        g4 = gfont[2*(y+1)*(the_font.width*4)+(x*8+font_pos[i]*4+5)];
                        b4 = gfont[2*(y+1)*(the_font.width*4)+(x*8+font_pos[i]*4+6)];
                        a4 = gfont[2*(y+1)*(the_font.width*4)+(x*8+font_pos[i]*4+7)];
                        small_font_chars [i][y][x].val =
                            (((r1 + r2 + r3 + r4)>>2)<<(ROUGE*8))|
                            (((g1 + g2 + g3 + g4)>>2)<<(VERT*8))|
                            (((b1 + b2 + b3 + b4)>>2)<<(BLEU*8))|
                            (((a1 + a2 + a3 + a4)>>2)<<(ALPHA*8));
                    }
                }
        }

	/* definir les lettres restantes */ 
	
	for (i=0;i<256;i++) {
		if (font_chars[i]==0) {
                    font_chars[i]=font_chars[42];
                    small_font_chars[i]=small_font_chars[42];
                    font_width[i]=font_width[42];
                    font_pos[i]=font_pos[42];
                    font_height[i]=font_height[42];
                    small_font_width[i]=small_font_width[42];
                    small_font_height[i]=small_font_height[42];
                }
	}

        font_width [32] = (the_font.height / 2) - 1;
        small_font_width [32] = font_width [32]/2;
        font_chars [32] = 0;
        small_font_chars [32] = 0;
}

void    goom_draw_text (Pixel * buf,int resolx,int resoly,
												int x, int y,
												const char *str, float charspace, int center) {
	float   fx = (float) x;
	int     fin = 0;

        Pixel  ***cur_font_chars;
        int    *cur_font_width;
        int    *cur_font_height;

        if (resolx>320)
        {
            /* printf("use big\n"); */
            cur_font_chars = font_chars;
            cur_font_width = font_width;
            cur_font_height = font_height;
        }
        else
        {
            /* printf ("use small\n"); */
            cur_font_chars = small_font_chars;
            cur_font_width = small_font_width;
            cur_font_height = small_font_height;
        }

        if (cur_font_chars == NULL)
		return ;

	if (center) {
		unsigned char   *tmp = (unsigned char*)str;
		float   lg = -charspace;

		while (*tmp != '\0')
			lg += cur_font_width[*(tmp++)] + charspace;

		fx -= lg / 2;
	}

	while (!fin) {
		unsigned char    c = *str;

		x = (int) fx;

		if (c == '\0')
			fin = 1;
		else if (cur_font_chars[c]==0) {
			fx += cur_font_width[c] + charspace;
		}
		else {
			int     xx, yy;
			int     xmin = x;
			int     xmax = x + cur_font_width[c];
			int     ymin = y - cur_font_height[c];
			int     ymax = y;

			yy = ymin;

			if (xmin < 0)
				xmin = 0;

			if (xmin >= resolx - 1)
				return;

			if (xmax >= (int) resolx)
				xmax = resolx - 1;

			if (yy < 0)
				yy = 0;

			if (yy <= (int) resoly - 1) {
				if (ymax >= (int) resoly - 1)
					ymax = resoly - 1;

        for (; yy < ymax; yy++)
          for (xx = xmin; xx < xmax; xx++)
          {
              Pixel color = cur_font_chars[c][yy - ymin][xx - x];
              Pixel transparency;
              transparency.val = color.val & A_CHANNEL;
              if (transparency.val)
              {
                  if (transparency.val==A_CHANNEL) buf[yy * resolx + xx] = color;
                  else
                  {
                      Pixel back =  buf[yy * resolx + xx];
                      unsigned int a1 = color.channels.a;
                      unsigned int a2 = 255 - a1;
                      buf[yy * resolx + xx].channels.r = (unsigned char)((((unsigned int)color.channels.r * a1) + ((unsigned int)back.channels.r * a2)) >> 8);
                      buf[yy * resolx + xx].channels.g = (unsigned char)((((unsigned int)color.channels.g * a1) + ((unsigned int)back.channels.g * a2)) >> 8);
                      buf[yy * resolx + xx].channels.b = (unsigned char)((((unsigned int)color.channels.b * a1) + ((unsigned int)back.channels.b * a2)) >> 8);
                  }
              }
          }
			}
			fx += cur_font_width[c] + charspace;
		}
		str++;
	}
}
