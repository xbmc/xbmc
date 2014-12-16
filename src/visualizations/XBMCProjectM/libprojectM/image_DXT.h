/*
	Jonathan Dummer
	2007-07-31-10.32

	simple DXT compression / decompression code

	public domain
*/

#ifndef HEADER_IMAGE_DXT
#define HEADER_IMAGE_DXT

/**
	Converts an image from an array of unsigned chars (RGB or RGBA) to
	DXT1 or DXT5, then saves the converted image to disk.
	\return 0 if failed, otherwise returns 1
**/
int
save_image_as_DDS
(
    const char *filename,
    int width, int height, int channels,
    const unsigned char *const data
);

/**
	take an image and convert it to DXT1 (no alpha)
**/
unsigned char*
convert_image_to_DXT1
(
    const unsigned char *const uncompressed,
    int width, int height, int channels,
    int *out_size
);

/**
	take an image and convert it to DXT5 (with alpha)
**/
unsigned char*
convert_image_to_DXT5
(
    const unsigned char *const uncompressed,
    int width, int height, int channels,
    int *out_size
);

/**	A bunch of DirectDraw Surface structures and flags **/
typedef struct
{
    unsigned int    dwMagic;
    unsigned int    dwSize;
    unsigned int    dwFlags;
    unsigned int    dwHeight;
    unsigned int    dwWidth;
    unsigned int    dwPitchOrLinearSize;
    unsigned int    dwDepth;
    unsigned int    dwMipMapCount;
    unsigned int    dwReserved1[ 11 ];

    /*  DDPIXELFORMAT	*/
    struct
    {
        unsigned int    dwSize;
        unsigned int    dwFlags;
        unsigned int    dwFourCC;
        unsigned int    dwRGBBitCount;
        unsigned int    dwRBitMask;
        unsigned int    dwGBitMask;
        unsigned int    dwBBitMask;
        unsigned int    dwAlphaBitMask;
    }
    sPixelFormat;

    /*  DDCAPS2	*/
    struct
    {
        unsigned int    dwCaps1;
        unsigned int    dwCaps2;
        unsigned int    dwDDSX;
        unsigned int    dwReserved;
    }
    sCaps;
    unsigned int    dwReserved2;
}
DDS_header ;

/*	the following constants were copied directly off the MSDN website	*/

/*	The dwFlags member of the original DDSURFACEDESC2 structure
	can be set to one or more of the following values.	*/
#define DDSD_CAPS	0x00000001
#define DDSD_HEIGHT	0x00000002
#define DDSD_WIDTH	0x00000004
#define DDSD_PITCH	0x00000008
#define DDSD_PIXELFORMAT	0x00001000
#define DDSD_MIPMAPCOUNT	0x00020000
#define DDSD_LINEARSIZE	0x00080000
#define DDSD_DEPTH	0x00800000

/*	DirectDraw Pixel Format	*/
#define DDPF_ALPHAPIXELS	0x00000001
#define DDPF_FOURCC	0x00000004
#define DDPF_RGB	0x00000040

/*	The dwCaps1 member of the DDSCAPS2 structure can be
	set to one or more of the following values.	*/
#define DDSCAPS_COMPLEX	0x00000008
#define DDSCAPS_TEXTURE	0x00001000
#define DDSCAPS_MIPMAP	0x00400000

/*	The dwCaps2 member of the DDSCAPS2 structure can be
	set to one or more of the following values.		*/
#define DDSCAPS2_CUBEMAP	0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX	0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX	0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY	0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ	0x00008000
#define DDSCAPS2_VOLUME	0x00200000

#endif /* HEADER_IMAGE_DXT	*/
