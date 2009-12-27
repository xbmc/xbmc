/*
    Jonathan Dummer

    image helper functions

    MIT license
*/

#include "image_helper.h"
#include <stdlib.h>
#include <math.h>

/*	Upscaling the image uses simple bilinear interpolation	*/
int
	up_scale_image
	(
		const unsigned char* const orig,
		int width, int height, int channels,
		unsigned char* resampled,
		int resampled_width, int resampled_height
	)
{
	float dx, dy;
	int x, y, c;

    /* error(s) check	*/
    if ( 	(width < 1) || (height < 1) ||
            (resampled_width < 2) || (resampled_height < 2) ||
            (channels < 1) ||
            (NULL == orig) || (NULL == resampled) )
    {
        /*	signify badness	*/
        return 0;
    }
    /*
		for each given pixel in the new map, find the exact location
		from the original map which would contribute to this guy
	*/
    dx = (width - 1.0f) / (resampled_width - 1.0f);
    dy = (height - 1.0f) / (resampled_height - 1.0f);
    for ( y = 0; y < resampled_height; ++y )
    {
    	/* find the base y index and fractional offset from that	*/
    	float sampley = y * dy;
    	int inty = (int)sampley;
    	/*	if( inty < 0 ) { inty = 0; } else	*/
		if( inty > height - 2 ) { inty = height - 2; }
		sampley -= inty;
        for ( x = 0; x < resampled_width; ++x )
        {
			float samplex = x * dx;
			int intx = (int)samplex;
			int base_index;
			/* find the base x index and fractional offset from that	*/
			/*	if( intx < 0 ) { intx = 0; } else	*/
			if( intx > width - 2 ) { intx = width - 2; }
			samplex -= intx;
			/*	base index into the original image	*/
			base_index = (inty * width + intx) * channels;
            for ( c = 0; c < channels; ++c )
            {
            	/*	do the sampling	*/
				float value = 0.5f;
				value += orig[base_index]
							*(1.0f-samplex)*(1.0f-sampley);
				value += orig[base_index+channels]
							*(samplex)*(1.0f-sampley);
				value += orig[base_index+width*channels]
							*(1.0f-samplex)*(sampley);
				value += orig[base_index+width*channels+channels]
							*(samplex)*(sampley);
				/*	move to the next channel	*/
				++base_index;
            	/*	save the new value	*/
            	resampled[y*resampled_width*channels+x*channels+c] =
						(unsigned char)(value);
            }
        }
    }
    /*	done	*/
    return 1;
}

int
	mipmap_image
	(
		const unsigned char* const orig,
		int width, int height, int channels,
		unsigned char* resampled,
		int block_size_x, int block_size_y
	)
{
	int mip_width, mip_height;
	int i, j, c;

	/*	error check	*/
	if( (width < 1) || (height < 1) ||
		(channels < 1) || (orig == NULL) ||
		(resampled == NULL) ||
		(block_size_x < 1) || (block_size_y < 1) )
	{
		/*	nothing to do	*/
		return 0;
	}
	mip_width = width / block_size_x;
	mip_height = height / block_size_y;
	if( mip_width < 1 )
	{
		mip_width = 1;
	}
	if( mip_height < 1 )
	{
		mip_height = 1;
	}
	for( j = 0; j < mip_height; ++j )
	{
		for( i = 0; i < mip_width; ++i )
		{
			for( c = 0; c < channels; ++c )
			{
				const int index = (j*block_size_y)*width*channels + (i*block_size_x)*channels + c;
				int sum_value;
				int u,v;
				int u_block = block_size_x;
				int v_block = block_size_y;
				int block_area;
				/*	do a bit of checking so we don't over-run the boundaries
					(necessary for non-square textures!)	*/
				if( block_size_x * (i+1) > width )
				{
					u_block = width - i*block_size_y;
				}
				if( block_size_y * (j+1) > height )
				{
					v_block = height - j*block_size_y;
				}
				block_area = u_block*v_block;
				/*	for this pixel, see what the average
					of all the values in the block are.
					note: start the sum at the rounding value, not at 0	*/
				sum_value = block_area >> 1;
				for( v = 0; v < v_block; ++v )
				for( u = 0; u < u_block; ++u )
				{
					sum_value += orig[index + v*width*channels + u*channels];
				}
				resampled[j*mip_width*channels + i*channels + c] = sum_value / block_area;
			}
		}
	}
	return 1;
}

int
	scale_image_RGB_to_NTSC_safe
	(
		unsigned char* orig,
		int width, int height, int channels
	)
{
	const float scale_lo = 16.0f - 0.499f;
	const float scale_hi = 235.0f + 0.499f;
	int i, j;
	int nc = channels;
	unsigned char scale_LUT[256];
	/*	error check	*/
	if( (width < 1) || (height < 1) ||
		(channels < 1) || (orig == NULL) )
	{
		/*	nothing to do	*/
		return 0;
	}
	/*	set up the scaling Look Up Table	*/
	for( i = 0; i < 256; ++i )
	{
		scale_LUT[i] = (unsigned char)((scale_hi - scale_lo) * i / 255.0f + scale_lo);
	}
	/*	for channels = 2 or 4, ignore the alpha component	*/
	nc -= 1 - (channels & 1);
	/*	OK, go through the image and scale any non-alpha components	*/
	for( i = 0; i < width*height*channels; i += channels )
	{
		for( j = 0; j < nc; ++j )
		{
			orig[i+j] = scale_LUT[orig[i+j]];
		}
	}
	return 1;
}

unsigned char clamp_byte( int x ) { return ( (x) < 0 ? (0) : ( (x) > 255 ? 255 : (x) ) ); }

/*
	This function takes the RGB components of the image
	and converts them into YCoCg.  3 components will be
	re-ordered to CoYCg (for optimum DXT1 compression),
	while 4 components will be ordered CoCgAY (for DXT5
	compression).
*/
int
	convert_RGB_to_YCoCg
	(
		unsigned char* orig,
		int width, int height, int channels
	)
{
	int i;
	/*	error check	*/
	if( (width < 1) || (height < 1) ||
		(channels < 3) || (channels > 4) ||
		(orig == NULL) )
	{
		/*	nothing to do	*/
		return -1;
	}
	/*	do the conversion	*/
	if( channels == 3 )
	{
		for( i = 0; i < width*height*3; i += 3 )
		{
			int r = orig[i+0];
			int g = (orig[i+1] + 1) >> 1;
			int b = orig[i+2];
			int tmp = (2 + r + b) >> 2;
			/*	Co	*/
			orig[i+0] = clamp_byte( 128 + ((r - b + 1) >> 1) );
			/*	Y	*/
			orig[i+1] = clamp_byte( g + tmp );
			/*	Cg	*/
			orig[i+2] = clamp_byte( 128 + g - tmp );
		}
	} else
	{
		for( i = 0; i < width*height*4; i += 4 )
		{
			int r = orig[i+0];
			int g = (orig[i+1] + 1) >> 1;
			int b = orig[i+2];
			unsigned char a = orig[i+3];
			int tmp = (2 + r + b) >> 2;
			/*	Co	*/
			orig[i+0] = clamp_byte( 128 + ((r - b + 1) >> 1) );
			/*	Cg	*/
			orig[i+1] = clamp_byte( 128 + g - tmp );
			/*	Alpha	*/
			orig[i+2] = a;
			/*	Y	*/
			orig[i+3] = clamp_byte( g + tmp );
		}
	}
	/*	done	*/
	return 0;
}

/*
	This function takes the YCoCg components of the image
	and converts them into RGB.  See above.
*/
int
	convert_YCoCg_to_RGB
	(
		unsigned char* orig,
		int width, int height, int channels
	)
{
	int i;
	/*	error check	*/
	if( (width < 1) || (height < 1) ||
		(channels < 3) || (channels > 4) ||
		(orig == NULL) )
	{
		/*	nothing to do	*/
		return -1;
	}
	/*	do the conversion	*/
	if( channels == 3 )
	{
		for( i = 0; i < width*height*3; i += 3 )
		{
			int co = orig[i+0] - 128;
			int y  = orig[i+1];
			int cg = orig[i+2] - 128;
			/*	R	*/
			orig[i+0] = clamp_byte( y + co - cg );
			/*	G	*/
			orig[i+1] = clamp_byte( y + cg );
			/*	B	*/
			orig[i+2] = clamp_byte( y - co - cg );
		}
	} else
	{
		for( i = 0; i < width*height*4; i += 4 )
		{
			int co = orig[i+0] - 128;
			int cg = orig[i+1] - 128;
			unsigned char a  = orig[i+2];
			int y  = orig[i+3];
			/*	R	*/
			orig[i+0] = clamp_byte( y + co - cg );
			/*	G	*/
			orig[i+1] = clamp_byte( y + cg );
			/*	B	*/
			orig[i+2] = clamp_byte( y - co - cg );
			/*	A	*/
			orig[i+3] = a;
		}
	}
	/*	done	*/
	return 0;
}

float
find_max_RGBE
(
	unsigned char *image,
    int width, int height
)
{
	float max_val = 0.0f;
	unsigned char *img = image;
	int i, j;
	for( i = width * height; i > 0; --i )
	{
		/* float scale = powf( 2.0f, img[3] - 128.0f ) / 255.0f; */
		float scale = ldexp( 1.0f / 255.0f, (int)(img[3]) - 128 );
		for( j = 0; j < 3; ++j )
		{
			if( img[j] * scale > max_val )
			{
				max_val = img[j] * scale;
			}
		}
		/* next pixel */
		img += 4;
	}
	return max_val;
}

int
RGBE_to_RGBdivA
(
    unsigned char *image,
    int width, int height,
    int rescale_to_max
)
{
	/* local variables */
	int i, iv;
	unsigned char *img = image;
	float scale = 1.0f;
	/* error check */
	if( (!image) || (width < 1) || (height < 1) )
	{
		return 0;
	}
	/* convert (note: no negative numbers, but 0.0 is possible) */
	if( rescale_to_max )
	{
		scale = 255.0f / find_max_RGBE( image, width, height );
	}
	for( i = width * height; i > 0; --i )
	{
		/* decode this pixel, and find the max */
		float r,g,b,e, m;
		/* e = scale * powf( 2.0f, img[3] - 128.0f ) / 255.0f; */
		e = scale * ldexp( 1.0f / 255.0f, (int)(img[3]) - 128 );
		r = e * img[0];
		g = e * img[1];
		b = e * img[2];
		m = (r > g) ? r : g;
		m = (b > m) ? b : m;
		/* and encode it into RGBdivA */
		iv = (m != 0.0f) ? (int)(255.0f / m) : 1.0f;
		iv = (iv < 1) ? 1 : iv;
		img[3] = (iv > 255) ? 255 : iv;
		iv = (int)(img[3] * r + 0.5f);
		img[0] = (iv > 255) ? 255 : iv;
		iv = (int)(img[3] * g + 0.5f);
		img[1] = (iv > 255) ? 255 : iv;
		iv = (int)(img[3] * b + 0.5f);
		img[2] = (iv > 255) ? 255 : iv;
		/* and on to the next pixel */
		img += 4;
	}
	return 1;
}

int
RGBE_to_RGBdivA2
(
    unsigned char *image,
    int width, int height,
    int rescale_to_max
)
{
	/* local variables */
	int i, iv;
	unsigned char *img = image;
	float scale = 1.0f;
	/* error check */
	if( (!image) || (width < 1) || (height < 1) )
	{
		return 0;
	}
	/* convert (note: no negative numbers, but 0.0 is possible) */
	if( rescale_to_max )
	{
		scale = 255.0f * 255.0f / find_max_RGBE( image, width, height );
	}
	for( i = width * height; i > 0; --i )
	{
		/* decode this pixel, and find the max */
		float r,g,b,e, m;
		/* e = scale * powf( 2.0f, img[3] - 128.0f ) / 255.0f; */
		e = scale * ldexp( 1.0f / 255.0f, (int)(img[3]) - 128 );
		r = e * img[0];
		g = e * img[1];
		b = e * img[2];
		m = (r > g) ? r : g;
		m = (b > m) ? b : m;
		/* and encode it into RGBdivA */
		iv = (m != 0.0f) ? (int)sqrtf( 255.0f * 255.0f / m ) : 1.0f;
		iv = (iv < 1) ? 1 : iv;
		img[3] = (iv > 255) ? 255 : iv;
		iv = (int)(img[3] * img[3] * r / 255.0f + 0.5f);
		img[0] = (iv > 255) ? 255 : iv;
		iv = (int)(img[3] * img[3] * g / 255.0f + 0.5f);
		img[1] = (iv > 255) ? 255 : iv;
		iv = (int)(img[3] * img[3] * b / 255.0f + 0.5f);
		img[2] = (iv > 255) ? 255 : iv;
		/* and on to the next pixel */
		img += 4;
	}
	return 1;
}
