/**
	@mainpage SOIL

	Jonathan Dummer
	2007-07-26-10.36

	Simple OpenGL Image Library

	A tiny c library for uploading images as
	textures into OpenGL.  Also saving and
	loading of images is supported.

	I'm using Sean's Tool Box image loader as a base:
	http://www.nothings.org/

	I'm upgrading it to load TGA and DDS files, and a direct
	path for loading DDS files straight into OpenGL textures,
	when applicable.

	Image Formats:
	- BMP		load & save
	- TGA		load & save
	- DDS		load & save
	- PNG		load
	- JPG		load

	OpenGL Texture Features:
	- resample to power-of-two sizes
	- MIPmap generation
	- compressed texture S3TC formats (if supported)
	- can pre-multiply alpha for you, for better compositing
	- can flip image about the y-axis (except pre-compressed DDS files)

	Thanks to:
	* Sean Barret - for the awesome stb_image
	* Dan Venkitachalam - for finding some non-compliant DDS files, and patching some explicit casts
	* everybody at gamedev.net
**/

#ifndef HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY
#define HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY

#ifdef __cplusplus
extern "C" {
#endif

/**
	The format of images that may be loaded (force_channels).
	SOIL_LOAD_AUTO leaves the image in whatever format it was found.
	SOIL_LOAD_L forces the image to load as Luminous (greyscale)
	SOIL_LOAD_LA forces the image to load as Luminous with Alpha
	SOIL_LOAD_RGB forces the image to load as Red Green Blue
	SOIL_LOAD_RGBA forces the image to load as Red Green Blue Alpha
**/
enum
{
	SOIL_LOAD_AUTO = 0,
	SOIL_LOAD_L = 1,
	SOIL_LOAD_LA = 2,
	SOIL_LOAD_RGB = 3,
	SOIL_LOAD_RGBA = 4
};

/**
	Passed in as reuse_texture_ID, will cause SOIL to
	register a new texture ID using glGenTextures().
	If the value passed into reuse_texture_ID > 0 then
	SOIL will just re-use that texture ID (great for
	reloading image assets in-game!)
**/
enum
{
	SOIL_CREATE_NEW_ID = 0
};

/**
	flags you can pass into SOIL_load_OGL_texture()
	and SOIL_create_OGL_texture().
	(note that if SOIL_FLAG_DDS_LOAD_DIRECT is used
	the rest of the flags with the exception of
	SOIL_FLAG_TEXTURE_REPEATS will be ignored while
	loading already-compressed DDS files.)

	SOIL_FLAG_POWER_OF_TWO: force the image to be POT
	SOIL_FLAG_MIPMAPS: generate mipmaps for the texture
	SOIL_FLAG_TEXTURE_REPEATS: otherwise will clamp
	SOIL_FLAG_MULTIPLY_ALPHA: for using (GL_ONE,GL_ONE_MINUS_SRC_ALPHA) blending
	SOIL_FLAG_INVERT_Y: flip the image vertically
	SOIL_FLAG_COMPRESS_TO_DXT: if the card can display them, will convert RGB to DXT1, RGBA to DXT5
	SOIL_FLAG_DDS_LOAD_DIRECT: will load DDS files directly without _ANY_ additional processing
	SOIL_FLAG_NTSC_SAFE_RGB: clamps RGB components to the range [16,235]
	SOIL_FLAG_CoCg_Y: Google YCoCg; RGB=>CoYCg, RGBA=>CoCgAY
**/
enum
{
	SOIL_FLAG_POWER_OF_TWO = 1,
	SOIL_FLAG_MIPMAPS = 2,
	SOIL_FLAG_TEXTURE_REPEATS = 4,
	SOIL_FLAG_MULTIPLY_ALPHA = 8,
	SOIL_FLAG_INVERT_Y = 16,
	SOIL_FLAG_COMPRESS_TO_DXT = 32,
	SOIL_FLAG_DDS_LOAD_DIRECT = 64,
	SOIL_FLAG_NTSC_SAFE_RGB = 128,
	SOIL_FLAG_CoCg_Y = 256
};

/**
	The types of images that may be saved.
	(TGA supports uncompressed RGB / RGBA)
	(BMP supports uncompressed RGB)
	(DDS supports DXT1 and DXT5)
**/
enum
{
	SOIL_SAVE_TYPE_TGA = 0,
	SOIL_SAVE_TYPE_BMP = 1,
	SOIL_SAVE_TYPE_DDS = 2
};

/**
	Defines the order of faces in a DDS cubemap.
	I recommend that you use the same order in single
	image cubemap files, so they will be interchangeable
	with DDS cubemaps when using SOIL.
**/
#define SOIL_DDS_CUBEMAP_FACE_ORDER "EWUDNS"

/**
	Loads an image from disk into an OpenGL texture.
	\param filename the name of the file to upload as a texture
	\param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
	\param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the texture ID (overwriting the old texture)
	\param flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_DDS_LOAD_DIRECT
	\return 0-failed, otherwise returns the OpenGL texture handle
**/
unsigned int
	SOIL_load_OGL_texture
	(
		const char *filename,
		int force_channels,
		unsigned int reuse_texture_ID,
		unsigned int flags
	);

/**
	Loads 6 images from disk into an OpenGL cubemap texture.
	\param x_pos_file the name of the file to upload as the +x cube face
	\param x_neg_file the name of the file to upload as the -x cube face
	\param y_pos_file the name of the file to upload as the +y cube face
	\param y_neg_file the name of the file to upload as the -y cube face
	\param z_pos_file the name of the file to upload as the +z cube face
	\param z_neg_file the name of the file to upload as the -z cube face
	\param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
	\param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the texture ID (overwriting the old texture)
	\param flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_DDS_LOAD_DIRECT
	\return 0-failed, otherwise returns the OpenGL texture handle
**/
unsigned int
	SOIL_load_OGL_cubemap
	(
		const char *x_pos_file,
		const char *x_neg_file,
		const char *y_pos_file,
		const char *y_neg_file,
		const char *z_pos_file,
		const char *z_neg_file,
		int force_channels,
		unsigned int reuse_texture_ID,
		unsigned int flags
	);

/**
	Loads 1 image from disk and splits it into an OpenGL cubemap texture.
	\param filename the name of the file to upload as a texture
	\param face_order the order of the faces in the file, any combination of NSWEUD, for North, South, Up, etc.
	\param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
	\param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the texture ID (overwriting the old texture)
	\param flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_DDS_LOAD_DIRECT
	\return 0-failed, otherwise returns the OpenGL texture handle
**/
unsigned int
	SOIL_load_OGL_single_cubemap
	(
		const char *filename,
		const char face_order[6],
		int force_channels,
		unsigned int reuse_texture_ID,
		unsigned int flags
	);

/**
	Loads an image from RAM into an OpenGL texture.
	\param buffer the image data in RAM just as if it were still in a file
	\param buffer_length the size of the buffer in bytes
	\param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
	\param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the texture ID (overwriting the old texture)
	\param flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_DDS_LOAD_DIRECT
	\return 0-failed, otherwise returns the OpenGL texture handle
**/
unsigned int
	SOIL_load_OGL_texture_from_memory
	(
		const unsigned char *const buffer,
		int buffer_length,
		int force_channels,
		unsigned int reuse_texture_ID,
		unsigned int flags
	);

/**
	Loads 6 images from memory into an OpenGL cubemap texture.
	\param x_pos_buffer the image data in RAM to upload as the +x cube face
	\param x_pos_buffer_length the size of the above buffer
	\param x_neg_buffer the image data in RAM to upload as the +x cube face
	\param x_neg_buffer_length the size of the above buffer
	\param y_pos_buffer the image data in RAM to upload as the +x cube face
	\param y_pos_buffer_length the size of the above buffer
	\param y_neg_buffer the image data in RAM to upload as the +x cube face
	\param y_neg_buffer_length the size of the above buffer
	\param z_pos_buffer the image data in RAM to upload as the +x cube face
	\param z_pos_buffer_length the size of the above buffer
	\param z_neg_buffer the image data in RAM to upload as the +x cube face
	\param z_neg_buffer_length the size of the above buffer
	\param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
	\param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the texture ID (overwriting the old texture)
	\param flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_DDS_LOAD_DIRECT
	\return 0-failed, otherwise returns the OpenGL texture handle
**/
unsigned int
	SOIL_load_OGL_cubemap_from_memory
	(
		const unsigned char *const x_pos_buffer,
		int x_pos_buffer_length,
		const unsigned char *const x_neg_buffer,
		int x_neg_buffer_length,
		const unsigned char *const y_pos_buffer,
		int y_pos_buffer_length,
		const unsigned char *const y_neg_buffer,
		int y_neg_buffer_length,
		const unsigned char *const z_pos_buffer,
		int z_pos_buffer_length,
		const unsigned char *const z_neg_buffer,
		int z_neg_buffer_length,
		int force_channels,
		unsigned int reuse_texture_ID,
		unsigned int flags
	);

/**
	Loads 1 image from RAM and splits it into an OpenGL cubemap texture.
	\param buffer the image data in RAM just as if it were still in a file
	\param buffer_length the size of the buffer in bytes
	\param face_order the order of the faces in the file, any combination of NSWEUD, for North, South, Up, etc.
	\param force_channels 0-image format, 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
	\param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the texture ID (overwriting the old texture)
	\param flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_DDS_LOAD_DIRECT
	\return 0-failed, otherwise returns the OpenGL texture handle
**/
unsigned int
	SOIL_load_OGL_single_cubemap_from_memory
	(
		const unsigned char *const buffer,
		int buffer_length,
		const char face_order[6],
		int force_channels,
		unsigned int reuse_texture_ID,
		unsigned int flags
	);

/**
	Creates a 2D OpenGL texture from raw image data.  Note that the raw data is
	_NOT_ freed after the upload (so the user can load various versions).
	\param data the raw data to be uploaded as an OpenGL texture
	\param width the width of the image in pixels
	\param height the height of the image in pixels
	\param channels the number of channels: 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
	\param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the texture ID (overwriting the old texture)
	\param flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT
	\return 0-failed, otherwise returns the OpenGL texture handle
**/
unsigned int
	SOIL_create_OGL_texture
	(
		const unsigned char *const data,
		int width, int height, int channels,
		unsigned int reuse_texture_ID,
		unsigned int flags
	);

/**
	Creates an OpenGL cubemap texture by splitting up 1 image into 6 parts.
	\param data the raw data to be uploaded as an OpenGL texture
	\param width the width of the image in pixels
	\param height the height of the image in pixels
	\param channels the number of channels: 1-luminous, 2-luminous/alpha, 3-RGB, 4-RGBA
	\param face_order the order of the faces in the file, and combination of NSWEUD, for North, South, Up, etc.
	\param reuse_texture_ID 0-generate a new texture ID, otherwise reuse the texture ID (overwriting the old texture)
	\param flags can be any of SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_COMPRESS_TO_DXT | SOIL_FLAG_DDS_LOAD_DIRECT
	\return 0-failed, otherwise returns the OpenGL texture handle
**/
unsigned int
	SOIL_create_OGL_single_cubemap
	(
		const unsigned char *const data,
		int width, int height, int channels,
		const char face_order[6],
		unsigned int reuse_texture_ID,
		unsigned int flags
	);

/**
	Captures the OpenGL window (RGB) and saves it to disk
	\return 0 if it failed, otherwise returns 1
**/
int
	SOIL_save_screenshot
	(
		const char *filename,
		int image_type,
		int x, int y,
		int width, int height
	);

/**
	Loads an image from disk into an array of unsigned chars.
	\return 0 if failed, otherwise returns 1
**/
unsigned char*
	SOIL_load_image
	(
		const char *filename,
		int *width, int *height, int *channels,
		int force_channels
	);

/**
	Loads an image from memory into an array of unsigned chars.
	\return 0 if failed, otherwise returns 1
**/
unsigned char*
	SOIL_load_image_from_memory
	(
		const unsigned char *const buffer,
		int buffer_length,
		int *width, int *height, int *channels,
		int force_channels
	);

/**
	Saves an image from an array of unsigned chars (RGBA) to disk
	\return 0 if failed, otherwise returns 1
**/
int
	SOIL_save_image
	(
		const char *filename,
		int image_type,
		int width, int height, int channels,
		const unsigned char *const data
	);

/**
	Frees the image data (note, this is just C's "free()"...this function is
	present mostly so C++ programmers don't forget to use "free()" and call
	"delete []" instead [8^)
**/
void
	SOIL_free_image_data
	(
		unsigned char *img_data
	);

/**
	This function resturn a pointer to a string describing the last thing
	that happened inside SOIL.  It can be used to determine why an image
	failed to load.
**/
const char*
	SOIL_last_result
	(
		void
	);


#ifdef __cplusplus
}
#endif

#endif /* HEADER_SIMPLE_OPENGL_IMAGE_LIBRARY	*/
