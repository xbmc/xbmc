#include <stdio.h>
#include <string.h>

#define THE_FILE "gfont.c"
#define THE_FONT the_font
#define THE_FONTS "the_font"

#include THE_FILE

int main (int argc, char **argv) {
	int i = 1;
	int size = 0;
	unsigned char pc = the_font.pixel_data[0];
	int nbz = 0;
	unsigned char *rle;
	rle = malloc (the_font.width*the_font.height*the_font.bytes_per_pixel);

	while (i < the_font.width *the_font.height*the_font.bytes_per_pixel) {
		unsigned char c = the_font.pixel_data[i];
		if (pc==0) {
			nbz ++;
			if (c==0) {
				if (nbz == 0xff) {		
					rle [size++] = 0;
					rle [size++] = nbz;
					nbz = 0;
				}
			}
			else {
				rle [size++] = 0;
				rle [size++] = nbz;
				nbz = 0;
			}
		}
		else {
			rle [size++] = pc;
		}
		pc = c;
		i++;
	}
	
	printf ("/* RGBA C-Source image dump (with zRLE compression) */\n"
					"static const struct {\n"
					"  unsigned int width;\n"
					"  unsigned int height;\n"
					"  unsigned int bytes_per_pixel;\n"
					"  unsigned int rle_size;\n"
					"  unsigned char rle_pixel [%i];\n", size);
	printf ("} " THE_FONTS " = {\n"
					"%i, %i, %i, %i, {\n",
					the_font.width,the_font.height,the_font.bytes_per_pixel,size);

	printf ("%i",rle[0]);
	for (i=1;i<size;i++) {
		if (i%20)
			printf (",%i",rle[i]);
		else
			printf (",\n%i",rle[i]);
	}
	printf ("}};\n");
	printf (" /* Created by MiniCompress.. an iOS RLE compressor.\n"
					"  * Compress Rate : %2.2f %%\n"
					"  */\n", 100.0f * (float)size / (float)(the_font.width*the_font.height*the_font.bytes_per_pixel));
	return 0;
}
