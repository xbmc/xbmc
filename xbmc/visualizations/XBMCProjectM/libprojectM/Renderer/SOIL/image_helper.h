/*
    Jonathan Dummer

    Image helper functions

    MIT license
*/

#ifndef HEADER_IMAGE_HELPER
#define HEADER_IMAGE_HELPER

#ifdef __cplusplus
extern "C" {
#endif

/**
	This function upscales an image.
	Not to be used to create MIPmaps,
	but to make it square,
	or to make it a power-of-two sized.
**/
int
	up_scale_image
	(
		const unsigned char* const orig,
		int width, int height, int channels,
		unsigned char* resampled,
		int resampled_width, int resampled_height
	);

/**
	This function downscales an image.
	Used for creating MIPmaps,
	the incoming image should be a
	power-of-two sized.
**/
int
	mipmap_image
	(
		const unsigned char* const orig,
		int width, int height, int channels,
		unsigned char* resampled,
		int block_size_x, int block_size_y
	);

/**
	This function takes the RGB components of the image
	and scales each channel from [0,255] to [16,235].
	This makes the colors "Safe" for display on NTSC
	displays.  Note that this is _NOT_ a good idea for
	loading images like normal- or height-maps!
**/
int
	scale_image_RGB_to_NTSC_safe
	(
		unsigned char* orig,
		int width, int height, int channels
	);

/**
	This function takes the RGB components of the image
	and converts them into YCoCg.  3 components will be
	re-ordered to CoYCg (for optimum DXT1 compression),
	while 4 components will be ordered CoCgAY (for DXT5
	compression).
**/
int
	convert_RGB_to_YCoCg
	(
		unsigned char* orig,
		int width, int height, int channels
	);

/**
	This function takes the YCoCg components of the image
	and converts them into RGB.  See above.
**/
int
	convert_YCoCg_to_RGB
	(
		unsigned char* orig,
		int width, int height, int channels
	);

/**
	Converts an HDR image from an array
	of unsigned chars (RGBE) to RGBdivA
	\return 0 if failed, otherwise returns 1
**/
int
	RGBE_to_RGBdivA
	(
		unsigned char *image,
		int width, int height,
		int rescale_to_max
	);

/**
	Converts an HDR image from an array
	of unsigned chars (RGBE) to RGBdivA2
	\return 0 if failed, otherwise returns 1
**/
int
	RGBE_to_RGBdivA2
	(
		unsigned char *image,
		int width, int height,
		int rescale_to_max
	);

#ifdef __cplusplus
}
#endif

#endif /* HEADER_IMAGE_HELPER	*/
