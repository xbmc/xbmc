/*
	Jonathan Dummer
	2007-07-31-10.32

	simple DXT compression / decompression code

	public domain
*/

#include "image_DXT.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*	set this =1 if you want to use the covarince matrix method...
	which is better than my method of using standard deviations
	overall, except on the infintesimal chance that the power
	method fails for finding the largest eigenvector	*/
#define USE_COV_MAT	1

/********* Function Prototypes *********/
/*
	Takes a 4x4 block of pixels and compresses it into 8 bytes
	in DXT1 format (color only, no alpha).  Speed is valued
	over prettyness, at least for now.
*/
void compress_DDS_color_block(
				int channels,
				const unsigned char *const uncompressed,
				unsigned char compressed[8] );
/*
	Takes a 4x4 block of pixels and compresses the alpha
	component it into 8 bytes for use in DXT5 DDS files.
	Speed is valued over prettyness, at least for now.
*/
void compress_DDS_alpha_block(
				const unsigned char *const uncompressed,
				unsigned char compressed[8] );

/********* Actual Exposed Functions *********/
int
	save_image_as_DDS
	(
		const char *filename,
		int width, int height, int channels,
		const unsigned char *const data
	)
{
	/*	variables	*/
	FILE *fout;
	unsigned char *DDS_data;
	DDS_header header;
	int DDS_size;
	/*	error check	*/
	if( (NULL == filename) ||
		(width < 1) || (height < 1) ||
		(channels < 1) || (channels > 4) ||
		(data == NULL ) )
	{
		return 0;
	}
	/*	Convert the image	*/
	if( (channels & 1) == 1 )
	{
		/*	no alpha, just use DXT1	*/
		DDS_data = convert_image_to_DXT1( data, width, height, channels, &DDS_size );
	} else
	{
		/*	has alpha, so use DXT5	*/
		DDS_data = convert_image_to_DXT5( data, width, height, channels, &DDS_size );
	}
	/*	save it	*/
	memset( &header, 0, sizeof( DDS_header ) );
	header.dwMagic = ('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24);
	header.dwSize = 124;
	header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE;
	header.dwWidth = width;
	header.dwHeight = height;
	header.dwPitchOrLinearSize = DDS_size;
	header.sPixelFormat.dwSize = 32;
	header.sPixelFormat.dwFlags = DDPF_FOURCC;
	if( (channels & 1) == 1 )
	{
		header.sPixelFormat.dwFourCC = ('D' << 0) | ('X' << 8) | ('T' << 16) | ('1' << 24);
	} else
	{
		header.sPixelFormat.dwFourCC = ('D' << 0) | ('X' << 8) | ('T' << 16) | ('5' << 24);
	}
	header.sCaps.dwCaps1 = DDSCAPS_TEXTURE;
	/*	write it out	*/
	fout = fopen( filename, "wb");
	fwrite( &header, sizeof( DDS_header ), 1, fout );
	fwrite( DDS_data, 1, DDS_size, fout );
	fclose( fout );
	/*	done	*/
	free( DDS_data );
	return 1;
}

unsigned char* convert_image_to_DXT1(
		const unsigned char *const uncompressed,
		int width, int height, int channels,
		int *out_size )
{
	unsigned char *compressed;
	int i, j, x, y;
	unsigned char ublock[16*3];
	unsigned char cblock[8];
	int index = 0, chan_step = 1;
	int block_count = 0;
	/*	error check	*/
	*out_size = 0;
	if( (width < 1) || (height < 1) ||
		(NULL == uncompressed) ||
		(channels < 1) || (channels > 4) )
	{
		return NULL;
	}
	/*	for channels == 1 or 2, I do not step forward for R,G,B values	*/
	if( channels < 3 )
	{
		chan_step = 0;
	}
	/*	get the RAM for the compressed image
		(8 bytes per 4x4 pixel block)	*/
	*out_size = ((width+3) >> 2) * ((height+3) >> 2) * 8;
	compressed = (unsigned char*)malloc( *out_size );
	/*	go through each block	*/
	for( j = 0; j < height; j += 4 )
	{
		for( i = 0; i < width; i += 4 )
		{
			/*	copy this block into a new one	*/
			int idx = 0;
			int mx = 4, my = 4;
			if( j+4 >= height )
			{
				my = height - j;
			}
			if( i+4 >= width )
			{
				mx = width - i;
			}
			for( y = 0; y < my; ++y )
			{
				for( x = 0; x < mx; ++x )
				{
					ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels];
					ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels+chan_step];
					ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels+chan_step+chan_step];
				}
				for( x = mx; x < 4; ++x )
				{
					ublock[idx++] = ublock[0];
					ublock[idx++] = ublock[1];
					ublock[idx++] = ublock[2];
				}
			}
			for( y = my; y < 4; ++y )
			{
				for( x = 0; x < 4; ++x )
				{
					ublock[idx++] = ublock[0];
					ublock[idx++] = ublock[1];
					ublock[idx++] = ublock[2];
				}
			}
			/*	compress the block	*/
			++block_count;
			compress_DDS_color_block( 3, ublock, cblock );
			/*	copy the data from the block into the main block	*/
			for( x = 0; x < 8; ++x )
			{
				compressed[index++] = cblock[x];
			}
		}
	}
	return compressed;
}

unsigned char* convert_image_to_DXT5(
		const unsigned char *const uncompressed,
		int width, int height, int channels,
		int *out_size )
{
	unsigned char *compressed;
	int i, j, x, y;
	unsigned char ublock[16*4];
	unsigned char cblock[8];
	int index = 0, chan_step = 1;
	int block_count = 0, has_alpha;
	/*	error check	*/
	*out_size = 0;
	if( (width < 1) || (height < 1) ||
		(NULL == uncompressed) ||
		(channels < 1) || ( channels > 4) )
	{
		return NULL;
	}
	/*	for channels == 1 or 2, I do not step forward for R,G,B vales	*/
	if( channels < 3 )
	{
		chan_step = 0;
	}
	/*	# channels = 1 or 3 have no alpha, 2 & 4 do have alpha	*/
	has_alpha = 1 - (channels & 1);
	/*	get the RAM for the compressed image
		(16 bytes per 4x4 pixel block)	*/
	*out_size = ((width+3) >> 2) * ((height+3) >> 2) * 16;
	compressed = (unsigned char*)malloc( *out_size );
	/*	go through each block	*/
	for( j = 0; j < height; j += 4 )
	{
		for( i = 0; i < width; i += 4 )
		{
			/*	local variables, and my block counter	*/
			int idx = 0;
			int mx = 4, my = 4;
			if( j+4 >= height )
			{
				my = height - j;
			}
			if( i+4 >= width )
			{
				mx = width - i;
			}
			for( y = 0; y < my; ++y )
			{
				for( x = 0; x < mx; ++x )
				{
					ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels];
					ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels+chan_step];
					ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels+chan_step+chan_step];
					ublock[idx++] =
						has_alpha * uncompressed[(j+y)*width*channels+(i+x)*channels+channels-1]
						+ (1-has_alpha)*255;
				}
				for( x = mx; x < 4; ++x )
				{
					ublock[idx++] = ublock[0];
					ublock[idx++] = ublock[1];
					ublock[idx++] = ublock[2];
					ublock[idx++] = ublock[3];
				}
			}
			for( y = my; y < 4; ++y )
			{
				for( x = 0; x < 4; ++x )
				{
					ublock[idx++] = ublock[0];
					ublock[idx++] = ublock[1];
					ublock[idx++] = ublock[2];
					ublock[idx++] = ublock[3];
				}
			}
			/*	now compress the alpha block	*/
			compress_DDS_alpha_block( ublock, cblock );
			/*	copy the data from the compressed alpha block into the main buffer	*/
			for( x = 0; x < 8; ++x )
			{
				compressed[index++] = cblock[x];
			}
			/*	then compress the color block	*/
			++block_count;
			compress_DDS_color_block( 4, ublock, cblock );
			/*	copy the data from the compressed color block into the main buffer	*/
			for( x = 0; x < 8; ++x )
			{
				compressed[index++] = cblock[x];
			}
		}
	}
	return compressed;
}

/********* Helper Functions *********/
int convert_bit_range( int c, int from_bits, int to_bits )
{
	int b = (1 << (from_bits - 1)) + c * ((1 << to_bits) - 1);
	return (b + (b >> from_bits)) >> from_bits;
}

int rgb_to_565( int r, int g, int b )
{
	return
		(convert_bit_range( r, 8, 5 ) << 11) |
		(convert_bit_range( g, 8, 6 ) << 05) |
		(convert_bit_range( b, 8, 5 ) << 00);
}

void rgb_888_from_565( unsigned int c, int *r, int *g, int *b )
{
	*r = convert_bit_range( (c >> 11) & 31, 5, 8 );
	*g = convert_bit_range( (c >> 05) & 63, 6, 8 );
	*b = convert_bit_range( (c >> 00) & 31, 5, 8 );
}

void compute_color_line_STDEV(
		const unsigned char *const uncompressed,
		int channels,
		float point[3], float direction[3] )
{
	const float inv_16 = 1.0f / 16.0f;
	int i;
	float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
	float sum_rr = 0.0f, sum_gg = 0.0f, sum_bb = 0.0f;
	float sum_rg = 0.0f, sum_rb = 0.0f, sum_gb = 0.0f;
	/*	calculate all data needed for the covariance matrix
		( to compare with _rygdxt code)	*/
	for( i = 0; i < 16*channels; i += channels )
	{
		sum_r += uncompressed[i+0];
		sum_rr += uncompressed[i+0] * uncompressed[i+0];
		sum_g += uncompressed[i+1];
		sum_gg += uncompressed[i+1] * uncompressed[i+1];
		sum_b += uncompressed[i+2];
		sum_bb += uncompressed[i+2] * uncompressed[i+2];
		sum_rg += uncompressed[i+0] * uncompressed[i+1];
		sum_rb += uncompressed[i+0] * uncompressed[i+2];
		sum_gb += uncompressed[i+1] * uncompressed[i+2];
	}
	/*	convert the sums to averages	*/
	sum_r *= inv_16;
	sum_g *= inv_16;
	sum_b *= inv_16;
	/*	and convert the squares to the squares of the value - avg_value	*/
	sum_rr -= 16.0f * sum_r * sum_r;
	sum_gg -= 16.0f * sum_g * sum_g;
	sum_bb -= 16.0f * sum_b * sum_b;
	sum_rg -= 16.0f * sum_r * sum_g;
	sum_rb -= 16.0f * sum_r * sum_b;
	sum_gb -= 16.0f * sum_g * sum_b;
	/*	the point on the color line is the average	*/
	point[0] = sum_r;
	point[1] = sum_g;
	point[2] = sum_b;
	#if USE_COV_MAT
	/*
		The following idea was from ryg.
		(https://mollyrocket.com/forums/viewtopic.php?t=392)
		The method worked great (less RMSE than mine) most of
		the time, but had some issues handling some simple
		boundary cases, like full green next to full red,
		which would generate a covariance matrix like this:

		| 1  -1  0 |
		| -1  1  0 |
		| 0   0  0 |

		For a given starting vector, the power method can
		generate all zeros!  So no starting with {1,1,1}
		as I was doing!  This kind of error is still a
		slight posibillity, but will be very rare.
	*/
	/*	use the covariance matrix directly
		(1st iteration, don't use all 1.0 values!)	*/
	sum_r = 1.0f;
	sum_g = 2.718281828f;
	sum_b = 3.141592654f;
	direction[0] = sum_r*sum_rr + sum_g*sum_rg + sum_b*sum_rb;
	direction[1] = sum_r*sum_rg + sum_g*sum_gg + sum_b*sum_gb;
	direction[2] = sum_r*sum_rb + sum_g*sum_gb + sum_b*sum_bb;
	/*	2nd iteration, use results from the 1st guy	*/
	sum_r = direction[0];
	sum_g = direction[1];
	sum_b = direction[2];
	direction[0] = sum_r*sum_rr + sum_g*sum_rg + sum_b*sum_rb;
	direction[1] = sum_r*sum_rg + sum_g*sum_gg + sum_b*sum_gb;
	direction[2] = sum_r*sum_rb + sum_g*sum_gb + sum_b*sum_bb;
	/*	3rd iteration, use results from the 2nd guy	*/
	sum_r = direction[0];
	sum_g = direction[1];
	sum_b = direction[2];
	direction[0] = sum_r*sum_rr + sum_g*sum_rg + sum_b*sum_rb;
	direction[1] = sum_r*sum_rg + sum_g*sum_gg + sum_b*sum_gb;
	direction[2] = sum_r*sum_rb + sum_g*sum_gb + sum_b*sum_bb;
	#else
	/*	use my standard deviation method
		(very robust, a tiny bit slower and less accurate)	*/
	direction[0] = sqrt( sum_rr );
	direction[1] = sqrt( sum_gg );
	direction[2] = sqrt( sum_bb );
	/*	which has a greater component	*/
	if( sum_gg > sum_rr )
	{
		/*	green has greater component, so base the other signs off of green	*/
		if( sum_rg < 0.0f )
		{
			direction[0] = -direction[0];
		}
		if( sum_gb < 0.0f )
		{
			direction[2] = -direction[2];
		}
	} else
	{
		/*	red has a greater component	*/
		if( sum_rg < 0.0f )
		{
			direction[1] = -direction[1];
		}
		if( sum_rb < 0.0f )
		{
			direction[2] = -direction[2];
		}
	}
	#endif
}

void LSE_master_colors_max_min(
		int *cmax, int *cmin,
		int channels,
		const unsigned char *const uncompressed )
{
	int i, j;
	/*	the master colors	*/
	int c0[3], c1[3];
	/*	used for fitting the line	*/
	float sum_x[] = { 0.0f, 0.0f, 0.0f };
	float sum_x2[] = { 0.0f, 0.0f, 0.0f };
	float dot_max = 1.0f, dot_min = -1.0f;
	float vec_len2 = 0.0f;
	float dot;
	/*	error check	*/
	if( (channels < 3) || (channels > 4) )
	{
		return;
	}
	compute_color_line_STDEV( uncompressed, channels, sum_x, sum_x2 );
	vec_len2 = 1.0f / ( 0.00001f +
			sum_x2[0]*sum_x2[0] + sum_x2[1]*sum_x2[1] + sum_x2[2]*sum_x2[2] );
	/*	finding the max and min vector values	*/
	dot_max =
			(
				sum_x2[0] * uncompressed[0] +
				sum_x2[1] * uncompressed[1] +
				sum_x2[2] * uncompressed[2]
			);
	dot_min = dot_max;
	for( i = 1; i < 16; ++i )
	{
		dot =
			(
				sum_x2[0] * uncompressed[i*channels+0] +
				sum_x2[1] * uncompressed[i*channels+1] +
				sum_x2[2] * uncompressed[i*channels+2]
			);
		if( dot < dot_min )
		{
			dot_min = dot;
		} else if( dot > dot_max )
		{
			dot_max = dot;
		}
	}
	/*	and the offset (from the average location)	*/
	dot = sum_x2[0]*sum_x[0] + sum_x2[1]*sum_x[1] + sum_x2[2]*sum_x[2];
	dot_min -= dot;
	dot_max -= dot;
	/*	post multiply by the scaling factor	*/
	dot_min *= vec_len2;
	dot_max *= vec_len2;
	/*	OK, build the master colors	*/
	for( i = 0; i < 3; ++i )
	{
		/*	color 0	*/
		c0[i] = (int)(0.5f + sum_x[i] + dot_max * sum_x2[i]);
		if( c0[i] < 0 )
		{
			c0[i] = 0;
		} else if( c0[i] > 255 )
		{
			c0[i] = 255;
		}
		/*	color 1	*/
		c1[i] = (int)(0.5f + sum_x[i] + dot_min * sum_x2[i]);
		if( c1[i] < 0 )
		{
			c1[i] = 0;
		} else if( c1[i] > 255 )
		{
			c1[i] = 255;
		}
	}
	/*	down_sample (with rounding?)	*/
	i = rgb_to_565( c0[0], c0[1], c0[2] );
	j = rgb_to_565( c1[0], c1[1], c1[2] );
	if( i > j )
	{
		*cmax = i;
		*cmin = j;
	} else
	{
		*cmax = j;
		*cmin = i;
	}
}

void
	compress_DDS_color_block
	(
		int channels,
		const unsigned char *const uncompressed,
		unsigned char compressed[8]
	)
{
	/*	variables	*/
	int i;
	int next_bit;
	int enc_c0, enc_c1;
	int c0[4], c1[4];
	float color_line[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float vec_len2 = 0.0f, dot_offset = 0.0f;
	/*	stupid order	*/
	int swizzle4[] = { 0, 2, 3, 1 };
	/*	get the master colors	*/
	LSE_master_colors_max_min( &enc_c0, &enc_c1, channels, uncompressed );
	/*	store the 565 color 0 and color 1	*/
	compressed[0] = (enc_c0 >> 0) & 255;
	compressed[1] = (enc_c0 >> 8) & 255;
	compressed[2] = (enc_c1 >> 0) & 255;
	compressed[3] = (enc_c1 >> 8) & 255;
	/*	zero out the compressed data	*/
	compressed[4] = 0;
	compressed[5] = 0;
	compressed[6] = 0;
	compressed[7] = 0;
	/*	reconstitute the master color vectors	*/
	rgb_888_from_565( enc_c0, &c0[0], &c0[1], &c0[2] );
	rgb_888_from_565( enc_c1, &c1[0], &c1[1], &c1[2] );
	/*	the new vector	*/
	vec_len2 = 0.0f;
	for( i = 0; i < 3; ++i )
	{
		color_line[i] = (float)(c1[i] - c0[i]);
		vec_len2 += color_line[i] * color_line[i];
	}
	if( vec_len2 > 0.0f )
	{
		vec_len2 = 1.0f / vec_len2;
	}
	/*	pre-proform the scaling	*/
	color_line[0] *= vec_len2;
	color_line[1] *= vec_len2;
	color_line[2] *= vec_len2;
	/*	compute the offset (constant) portion of the dot product	*/
	dot_offset = color_line[0]*c0[0] + color_line[1]*c0[1] + color_line[2]*c0[2];
	/*	store the rest of the bits	*/
	next_bit = 8*4;
	for( i = 0; i < 16; ++i )
	{
		/*	find the dot product of this color, to place it on the line
			(should be [-1,1])	*/
		int next_value = 0;
		float dot_product =
			color_line[0] * uncompressed[i*channels+0] +
			color_line[1] * uncompressed[i*channels+1] +
			color_line[2] * uncompressed[i*channels+2] -
			dot_offset;
		/*	map to [0,3]	*/
		next_value = (int)( dot_product * 3.0f + 0.5f );
		if( next_value > 3 )
		{
			next_value = 3;
		} else if( next_value < 0 )
		{
			next_value = 0;
		}
		/*	OK, store this value	*/
		compressed[next_bit >> 3] |= swizzle4[ next_value ] << (next_bit & 7);
		next_bit += 2;
	}
	/*	done compressing to DXT1	*/
}

void
	compress_DDS_alpha_block
	(
		const unsigned char *const uncompressed,
		unsigned char compressed[8]
	)
{
	/*	variables	*/
	int i;
	int next_bit;
	int a0, a1;
	float scale_me;
	/*	stupid order	*/
	int swizzle8[] = { 1, 7, 6, 5, 4, 3, 2, 0 };
	/*	get the alpha limits (a0 > a1)	*/
	a0 = a1 = uncompressed[3];
	for( i = 4+3; i < 16*4; i += 4 )
	{
		if( uncompressed[i] > a0 )
		{
			a0 = uncompressed[i];
		} else if( uncompressed[i] < a1 )
		{
			a1 = uncompressed[i];
		}
	}
	/*	store those limits, and zero the rest of the compressed dataset	*/
	compressed[0] = a0;
	compressed[1] = a1;
	/*	zero out the compressed data	*/
	compressed[2] = 0;
	compressed[3] = 0;
	compressed[4] = 0;
	compressed[5] = 0;
	compressed[6] = 0;
	compressed[7] = 0;
	/*	store the all of the alpha values	*/
	next_bit = 8*2;
	scale_me = 7.9999f / (a0 - a1);
	for( i = 3; i < 16*4; i += 4 )
	{
		/*	convert this alpha value to a 3 bit number	*/
		int svalue;
		int value = (int)((uncompressed[i] - a1) * scale_me);
		svalue = swizzle8[ value&7 ];
		/*	OK, store this value, start with the 1st byte	*/
		compressed[next_bit >> 3] |= svalue << (next_bit & 7);
		if( (next_bit & 7) > 5 )
		{
			/*	spans 2 bytes, fill in the start of the 2nd byte	*/
			compressed[1 + (next_bit >> 3)] |= svalue >> (8 - (next_bit & 7) );
		}
		next_bit += 3;
	}
	/*	done compressing to DXT1	*/
}
