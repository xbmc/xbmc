#include <png.h>

int
loadpng (char *file_name, int *w, int *h, unsigned int ***buf)
{
	FILE   *fp;
	png_uint_32 width, height;
	int     bit_depth,

		color_type, interlace_type, compression_type, filter_type;
	int     rowbytes;

	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;

	int     x, y;
	unsigned int **row_pointers;

	/* OUVERTURE DU FICHIER */

	fp = fopen (file_name, "rb");

	if (!fp) {
		// fprintf (stderr, "Couldn't open file\n");
		return 1;
	}

	/* CREATION DES STRUCTURES */
	png_ptr = png_create_read_struct
		(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, NULL, NULL);
	if (!png_ptr) {
		fprintf (stderr, "Memory error\n");
		return 1;
	}

	info_ptr = png_create_info_struct (png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct (&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		fprintf (stderr, "Read error 1\n");
		return 1;
	}

	end_info = png_create_info_struct (png_ptr);
	if (!end_info) {
		png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);
		fprintf (stderr, "Read error 2\n");
		return 1;
	}

	/* CHARGEMENT DE L'IMAGE */
	if (setjmp (png_ptr->jmpbuf)) {
		png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
		fclose (fp);
		fprintf (stderr, "Erreur de chargement\n");
		return 1;
	}

	png_init_io (png_ptr, fp);
	png_set_read_status_fn (png_ptr, NULL);

	png_read_info (png_ptr, info_ptr);

	png_get_IHDR (png_ptr, info_ptr, &width, &height,
								&bit_depth, &color_type, &interlace_type,
								&compression_type, &filter_type);
/*
	printf ("taille : %dx%d\n",width,height);
	printf ("depth  : %d\n",bit_depth);
	printf ("color type : ");
	switch (color_type) {
		case PNG_COLOR_TYPE_GRAY:
			printf ("PNG_COLOR_TYPE_GRAY (bit depths 1, 2, 4, 8, 16)\n");
			break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			printf ("PNG_COLOR_TYPE_GRAY_ALPHA (bit depths 8, 16)\n");
			break;
		case PNG_COLOR_TYPE_PALETTE:
			printf ("PNG_COLOR_TYPE_PALETTE (bit depths 1, 2, 4, 8)\n");
			break;
		case PNG_COLOR_TYPE_RGB:
			printf ("PNG_COLOR_TYPE_RGB (bit_depths 8, 16)\n");
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			printf ("PNG_COLOR_TYPE_RGB_ALPHA (bit_depths 8, 16)\n");
			break;
	}
  */
	// printf ("PNG_COLOR_MASK_ALPHA   : %x\n", PNG_COLOR_MASK_ALPHA);
	// printf ("PNG_COLOR_MASK_COLOR   : %x\n", PNG_COLOR_MASK_COLOR);
	// printf ("PNG_COLOR_MASK_PALETTE : %x\n", PNG_COLOR_MASK_PALETTE);

	if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
		png_set_palette_to_rgb (png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8 (png_ptr);
	else if (color_type == PNG_COLOR_TYPE_GRAY ||
					 color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb (png_ptr);

	if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha (png_ptr);

	png_read_update_info (png_ptr, info_ptr);

//      printf ("channels : %d\n", png_get_channels (png_ptr, info_ptr));
	rowbytes = png_get_rowbytes (png_ptr, info_ptr);
//      printf ("rowbytes : %d\n", rowbytes);

	row_pointers = (unsigned int **) malloc (height * sizeof (unsigned int *));

	for (y = 0; y < height; y++)
		row_pointers[y] = (unsigned int *) malloc (4 * width);
	png_read_image (png_ptr, (png_bytepp) row_pointers);

	// for (y=0;y<height;y++) {
//              for (x=0;x<width;x++) {
//                      if (row_pointers[y][x] & 0xf000)
	// printf ("%x ",(((unsigned int**)row_pointers)[y][x])&0xf);
	// else
//                              printf (" ");
	// }
	// printf ("\n");
	// }

	png_read_end (png_ptr, end_info);
	png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);

	(*buf) = (unsigned int **) malloc (height * sizeof (void *));

	for (y = 0; y < height; y++) {
		(*buf)[y] = (unsigned int *) malloc (width * 4);
		for (x = 0; x < width; x++)
			(*buf)[y][x] = row_pointers[y][x];
	}
	*w = width;
	*h = height;

	return 0;
}
