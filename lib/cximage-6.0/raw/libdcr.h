/*
  libdcr version 0.1.8.89 11/Jan/2009

  libdcr : copyright (C) 2007-2009, Davide Pizzolato

  based on dcraw.c -- Dave Coffin's raw photo decoder
  Copyright 1997-2009 by Dave Coffin, dcoffin a cybercom o net

  Covered code is provided under this license on an "as is" basis, without warranty
  of any kind, either expressed or implied, including, without limitation, warranties
  that the covered code is free of defects, merchantable, fit for a particular purpose
  or non-infringing. The entire risk as to the quality and performance of the covered
  code is with you. Should any covered code prove defective in any respect, you (not
  the initial developer or any other contributor) assume the cost of any necessary
  servicing, repair or correction. This disclaimer of warranty constitutes an essential
  part of this license. No use of any covered code is authorized hereunder except under
  this disclaimer.

  No license is required to download and use libdcr.  However,
  to lawfully redistribute libdcr, you must either (a) offer, at
  no extra charge, full source code for all executable files
  containing RESTRICTED functions, (b) distribute this code under
  the GPL Version 2 or later, (c) remove all RESTRICTED functions,
  re-implement them, or copy them from an earlier, unrestricted
  revision of dcraw.c, or (d) purchase a license from the author
  of dcraw.c.

  --------------------------------------------------------------------------------

  dcraw.c home page: http://cybercom.net/~dcoffin/dcraw/
  libdcr  home page: http://www.xdp.it/libdcr/

 */

#ifndef __LIBDCR
#define __LIBDCR

#ifdef WIN32
#include <io.h>
#include <direct.h>
#include <setjmp.h>
#include <sys/types.h>
#endif
#if defined(_LINUX) || defined(__APPLE__)
#include <setjmp.h>
#include <sys/types.h>
#define _swab   swab
#define _getcwd getcwd
#endif
#include <time.h>

#define DCR_VERSION "8.91"

// read dcraw.c and libdcr.c license before enabling RESTRICTED code
#define RESTRICTED 0

#define NO_JPEG
#define NO_LCMS
//#define DJGPP

//#define COLORCHECK

#ifndef LONG_BIT
  #define LONG_BIT (8 * sizeof (long))
#endif

#ifndef uchar
  typedef unsigned char uchar;
#endif

#ifndef ushort
  typedef unsigned short ushort;
#endif

#ifdef DJGPP
  #define fseeko fseek
  #define ftello ftell
#else
//#define fgetc getc_unlocked
#endif

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif

// file object.
typedef void dcr_stream_obj;

// file operations.
typedef struct {
	int   (*read_)(dcr_stream_obj *obj, void *buf, int size, int cnt);
	int   (*write_)(dcr_stream_obj *obj, void *buf, int size, int cnt);
	long  (*seek_)(dcr_stream_obj *obj, long offset, int origin);
	int   (*close_)(dcr_stream_obj *obj);
	char* (*gets_)(dcr_stream_obj *obj, char *string, int n);
	int   (*eof_)(dcr_stream_obj *obj);
	long  (*tell_)(dcr_stream_obj *obj);
	int   (*getc_)(dcr_stream_obj *obj);
	int   (*scanf_)(dcr_stream_obj *obj,const char *format, void* output);
} dcr_stream_ops;

#define dcr_fread  (*p->ops_->read_)
#define dcr_fwrite (*p->ops_->write_)
#define dcr_fseek  (*p->ops_->seek_)
#define dcr_fclose (*p->ops_->close_)
#define dcr_fgets  (*p->ops_->gets_)
#define dcr_feof   (*p->ops_->eof_)
#define dcr_ftell  (*p->ops_->tell_)
#define dcr_fgetc  (*p->ops_->getc_)
#define dcr_fscanf (*p->ops_->scanf_)

typedef struct {
	char *dark_frame, *bpfile;
	int user_flip, user_black, user_qual, user_sat;
	int timestamp_only, thumbnail_only, identify_only;
	int use_fuji_rotate, write_to_stdout;
	float threshold, bright, user_mul[4];
	double aber[4];
	int med_passes, highlight;
	unsigned shot_select, multi_out;
	int output_color, output_bps, output_tiff;
	int half_size, four_color_rgb, verbose, document_mode;
	int no_auto_bright;
	unsigned greybox[4];
	int use_auto_wb, use_camera_wb, use_camera_matrix;

#ifndef NO_LCMS
	char *cam_profile, *out_profile;
#endif

} dcr_options;


struct dcr_decode {
	struct dcr_decode *branch[2];
	int leaf;
};

struct dcr_tiff {
	int width, height, bps, comp, phint, offset, flip, samples, bytes;
};

struct dcr_ph {
	int format, key_off, black, black_off, split_col, tag_21a;
	float tag_210;
};

struct dcr_tiff_tag {
	ushort tag, type;
	int count;
	union { char c[4]; short s[2]; int i; } val;
};

struct dcr_tiff_hdr {
	ushort order, magic;
	int ifd;
	ushort pad, ntag;
	struct dcr_tiff_tag tag[23];
	int nextifd;
	ushort pad2, nexif;
	struct dcr_tiff_tag exif[4];
	ushort pad3, ngps;
	struct dcr_tiff_tag gpst[10];
	short bps[4];
	int rat[10];
	unsigned gps[26];
	char desc[512], make[64], model[64], soft[32], date[20], artist[64];
};

struct dcr_jhead {
	int bits, high, wide, clrs, sraw, psv, restart, vpred[6];
	struct dcr_decode *huff[6];
	ushort *row;
};

/*
   All global variables are defined here, and all functions that
   access them are prefixed with "DCR_CLASS".  Note that a thread-safe
   C++ class cannot have non-const static local variables.
 */
typedef struct dcr_DCRAW DCRAW;

struct dcr_DCRAW {
	dcr_stream_ops *ops_;
	dcr_stream_obj *obj_;
	dcr_options opt;

	struct dcr_decode first_decode[2048], *second_decode, *free_decode;

	struct dcr_tiff tiff_ifd[10];

	struct dcr_ph ph1;

	short order;
	char *ifname, *meta_data;
	char cdesc[5], desc[512], make[64], model[64], model2[64], artist[64];
	float flash_used, canon_ev, iso_speed, shutter, aperture, focal_len;
	time_t timestamp;
	unsigned shot_order, kodak_cbpp, filters, exif_cfa, unique_id;
	off_t    strip_offset, data_offset;
	off_t    thumb_offset, meta_offset, profile_offset;
	unsigned thumb_length, meta_length, profile_length;
	unsigned thumb_misc, *oprof, fuji_layout;
	unsigned tiff_nifds, tiff_samples, tiff_bps, tiff_compress;
	unsigned black, maximum, mix_green, raw_color, use_gamma, zero_is_bad;
	unsigned zero_after_ff, is_raw, dng_version, is_foveon, data_error;
	unsigned tile_width, tile_length, gpsdata[32], load_flags;
	ushort raw_height, raw_width, height, width, top_margin, left_margin;
	ushort shrink, iheight, iwidth, fuji_width, thumb_width, thumb_height;
	int flip, tiff_flip, colors, quality;
	double pixel_aspect;
	ushort (*image)[4], white[8][8], curve[0x4001], cr2_slice[3], sraw_mul[4];
	float cam_mul[4], pre_mul[4], cmatrix[3][4], rgb_cam[3][4];
	int histogram[4][0x2000];
	void (*write_thumb)(DCRAW *, FILE *);
	void (*write_fun)(DCRAW *, FILE *);
	void (*load_raw)(DCRAW *);
	void (*thumb_load_raw)(DCRAW *);
	jmp_buf failure;
	char *sz_error;
  /* local statics below here */
	unsigned getbits_bitbuf;
	int      getbits_vbits;
  int      getbits_reset;

  int make_decoder_leaf;

  unsigned long long ph1_bits_bitbuf;
  int    ph1_bits_vbits;

  uchar pana_bits_buf[0x4000];
  int   pana_bits_vbits;

	struct dcr_decode *radc_token_dstart[18], *radc_token_dindex;

	unsigned sony_decrypt_pad[128], sony_decrypt_p;

  unsigned foveon_decoder_huff[1024];
};


#define DCR_CLASS

#define FORC(cnt) for (c=0; c < cnt; c++)
#define FORC3     FORC(3)
#define FORC4     FORC(4)
#define FORCC(p)  FORC(p->colors)

#define SQR(x) ((x)*(x))
#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define ULIM(x,y,z) ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
#define CLIP(x) LIM(x,0,65535)
#define SWAP(a,b) { a ^= b; a ^= (b ^= a); }

int		DCR_CLASS dcr_fc (DCRAW* p, int row, int col);
void	DCR_CLASS dcr_merror (DCRAW* p, void *ptr, char *where);
void	DCR_CLASS dcr_derror(DCRAW* p);
ushort	DCR_CLASS dcr_sget2 (DCRAW* p,uchar *s);
ushort	DCR_CLASS dcr_get2(DCRAW* p);
unsigned DCR_CLASS dcr_sget4 (DCRAW* p,uchar *s);
unsigned DCR_CLASS dcr_get4(DCRAW* p);
unsigned DCR_CLASS dcr_getint (DCRAW* p,int type);
double	DCR_CLASS dcr_getreal (DCRAW* p,int type);
void	DCR_CLASS dcr_read_shorts (DCRAW* p,ushort *pixel, int count);
void	DCR_CLASS dcr_canon_black (DCRAW* p, double dark[2]);
void	DCR_CLASS dcr_canon_600_fixed_wb (DCRAW* p,int temp);
int		DCR_CLASS dcr_canon_600_color (DCRAW* p,int ratio[2], int mar);
void	DCR_CLASS dcr_canon_600_auto_wb(DCRAW* p);
void	DCR_CLASS dcr_canon_600_coeff(DCRAW* p);
void	DCR_CLASS dcr_canon_600_load_raw(DCRAW* p);
void	DCR_CLASS dcr_remove_zeroes(DCRAW* p);
int		DCR_CLASS dcr_canon_s2is(DCRAW* p);
void	DCR_CLASS dcr_canon_a5_load_raw(DCRAW* p);
unsigned DCR_CLASS dcr_getbits (DCRAW* p, int nbits);
uchar * DCR_CLASS dcr_make_decoder (DCRAW* p, const uchar *source, int level);
void	DCR_CLASS dcr_crw_init_tables (DCRAW* p, unsigned table);
int		DCR_CLASS dcr_canon_has_lowbits(DCRAW* p);
void	DCR_CLASS dcr_canon_compressed_load_raw(DCRAW* p);
int		DCR_CLASS dcr_ljpeg_start (DCRAW* p, struct dcr_jhead *jh, int info_only);
int		DCR_CLASS dcr_ljpeg_diff (DCRAW* p, struct dcr_decode *dindex);
ushort * DCR_CLASS dcr_ljpeg_row (DCRAW* p, int jrow, struct dcr_jhead *jh);
void	DCR_CLASS dcr_lossless_jpeg_load_raw(DCRAW* p);
void	DCR_CLASS dcr_canon_sraw_load_raw(DCRAW* p);
void	DCR_CLASS dcr_adobe_copy_pixel (DCRAW* p, int row, int col, ushort **rp);
void	DCR_CLASS dcr_adobe_dng_load_raw_lj(DCRAW* p);
void	DCR_CLASS dcr_adobe_dng_load_raw_nc(DCRAW* p);
void	DCR_CLASS dcr_pentax_k10_load_raw(DCRAW* p);
void	DCR_CLASS dcr_nikon_compressed_load_raw(DCRAW* p);
int		DCR_CLASS dcr_nikon_is_compressed(DCRAW* p);
int		DCR_CLASS dcr_nikon_e995(DCRAW* p);
int		DCR_CLASS dcr_nikon_e2100(DCRAW* p);
void	DCR_CLASS dcr_nikon_3700(DCRAW* p);
int		DCR_CLASS dcr_minolta_z2(DCRAW* p);
void	DCR_CLASS dcr_nikon_e900_load_raw(DCRAW* p);
void	DCR_CLASS dcr_fuji_load_raw(DCRAW* p);
void	DCR_CLASS dcr_jpeg_thumb (DCRAW* p, FILE *tfp);
void	DCR_CLASS dcr_ppm_thumb (DCRAW* p, FILE *tfp);
void	DCR_CLASS dcr_layer_thumb (DCRAW* p, FILE *tfp);
void	DCR_CLASS dcr_rollei_thumb (DCRAW* p, FILE *tfp);
void	DCR_CLASS dcr_rollei_load_raw(DCRAW* p);
int		DCR_CLASS dcr_bayer (DCRAW* p, unsigned row, unsigned col);
void	DCR_CLASS dcr_phase_one_flat_field (DCRAW* p, int is_float, int nc);
void	DCR_CLASS dcr_phase_one_correct(DCRAW* p);
void	DCR_CLASS dcr_phase_one_load_raw(DCRAW* p);
unsigned DCR_CLASS dcr_ph1_bits (DCRAW* p,int nbits);
void	DCR_CLASS dcr_phase_one_load_raw_c(DCRAW* p);
void	DCR_CLASS dcr_hasselblad_load_raw(DCRAW* p);
void	DCR_CLASS dcr_leaf_hdr_load_raw(DCRAW* p);
void	DCR_CLASS dcr_unpacked_load_raw(DCRAW* p);
void	DCR_CLASS nokia_load_raw(DCRAW* p);
unsigned DCR_CLASS dcr_pana_bits (DCRAW* p,int nbits);
void	DCR_CLASS dcr_panasonic_load_raw(DCRAW* p);
void	DCR_CLASS dcr_sinar_4shot_load_raw(DCRAW* p);
void	DCR_CLASS dcr_imacon_full_load_raw(DCRAW* p);
void	DCR_CLASS dcr_packed_12_load_raw(DCRAW* p);
void	DCR_CLASS dcr_olympus_e300_load_raw(DCRAW* p);
void	DCR_CLASS dcr_olympus_e410_load_raw(DCRAW* p);
void	DCR_CLASS dcr_minolta_rd175_load_raw(DCRAW* p);
void	DCR_CLASS dcr_casio_qv5700_load_raw(DCRAW* p);
void	DCR_CLASS dcr_quicktake_100_load_raw(DCRAW* p);
int		DCR_CLASS dcr_radc_token (DCRAW* p, int tree);
void	DCR_CLASS dcr_kodak_radc_load_raw(DCRAW* p);
void	DCR_CLASS dcr_kodak_jpeg_load_raw(DCRAW* p);
void	DCR_CLASS dcr_kodak_jpeg_load_raw(DCRAW* p);
void	DCR_CLASS dcr_kodak_dc120_load_raw(DCRAW* p);
void	DCR_CLASS dcr_eight_bit_load_raw(DCRAW* p);
void	DCR_CLASS dcr_kodak_yrgb_load_raw(DCRAW* p);
void	DCR_CLASS dcr_kodak_262_load_raw(DCRAW* p);
int		DCR_CLASS dcr_kodak_65000_decode (DCRAW* p, short *out, int bsize);
void	DCR_CLASS dcr_kodak_65000_load_raw(DCRAW* p);
void	DCR_CLASS dcr_kodak_ycbcr_load_raw(DCRAW* p);
void	DCR_CLASS dcr_kodak_rgb_load_raw(DCRAW* p);
void	DCR_CLASS dcr_kodak_thumb_load_raw(DCRAW* p);
void	DCR_CLASS dcr_sony_load_raw(DCRAW* p);
void	DCR_CLASS dcr_sony_arw_load_raw(DCRAW* p);
void	DCR_CLASS dcr_sony_arw2_load_raw(DCRAW* p);
void	DCR_CLASS dcr_smal_decode_segment (DCRAW* p, unsigned seg[2][2], int holes);
void	DCR_CLASS dcr_smal_v6_load_raw(DCRAW* p);
void	DCR_CLASS dcr_fill_holes (DCRAW* p, int holes);
void	DCR_CLASS dcr_smal_v9_load_raw(DCRAW* p);

#if RESTRICTED
void	DCR_CLASS dcr_foveon_decoder (DCRAW* p, unsigned size, unsigned code);
void	DCR_CLASS dcr_foveon_thumb (DCRAW* p, FILE *tfp);
void	DCR_CLASS dcr_foveon_load_camf(DCRAW* p);
void	DCR_CLASS dcr_foveon_load_raw(DCRAW* p);
const char * DCR_CLASS dcr_foveon_camf_param (DCRAW* p, const char *block, const char *param);
void *	DCR_CLASS dcr_foveon_camf_matrix (DCRAW* p, unsigned dim[3], const char *name);
int		DCR_CLASS dcr_foveon_fixed (DCRAW* p, void *ptr, int size, const char *name);
short *	DCR_CLASS dcr_foveon_make_curve (DCRAW* p, double max, double mul, double filt);
void	DCR_CLASS dcr_foveon_make_curves(DCRAW* p, short **curvep, float dq[3], float div[3], float filt);
void	DCR_CLASS dcr_foveon_interpolate(DCRAW* p);
#endif //RESTRICTED

void	DCR_CLASS dcr_bad_pixels(DCRAW* p, char *fname);
void	DCR_CLASS dcr_subtract (DCRAW* p,char *fname);
void	DCR_CLASS dcr_cam_xyz_coeff (DCRAW* p, double cam_xyz[4][3]);
void	DCR_CLASS dcr_colorcheck(DCRAW* p);
void	DCR_CLASS dcr_wavelet_denoise(DCRAW* p);
void	DCR_CLASS dcr_scale_colors(DCRAW* p);
void	DCR_CLASS dcr_pre_interpolate(DCRAW* p);
void	DCR_CLASS dcr_border_interpolate (DCRAW* p, int border);
void	DCR_CLASS dcr_lin_interpolate(DCRAW* p);
void	DCR_CLASS dcr_vng_interpolate(DCRAW* p);
void	DCR_CLASS dcr_ppg_interpolate(DCRAW* p);
void	DCR_CLASS dcr_ahd_interpolate(DCRAW* p);
void	DCR_CLASS dcr_median_filter (DCRAW* p);
void	DCR_CLASS dcr_blend_highlights(DCRAW* p);
void	DCR_CLASS dcr_recover_highlights(DCRAW* p);
void	DCR_CLASS dcr_tiff_get (DCRAW* p, unsigned base,unsigned *tag, unsigned *type, unsigned *len, unsigned *save);
void	DCR_CLASS dcr_parse_thumb_note (DCRAW* p, int base, unsigned toff, unsigned tlen);
int		DCR_CLASS dcr_parse_tiff_ifd (DCRAW* p, int base);
void	DCR_CLASS dcr_parse_makernote (DCRAW* p, int base, int uptag);
void	DCR_CLASS dcr_get_timestamp (DCRAW* p, int reversed);
void	DCR_CLASS dcr_parse_exif (DCRAW* p, int base);
void	DCR_CLASS dcr_parse_gps (DCRAW* p, int base);
void	DCR_CLASS dcr_romm_coeff (DCRAW* p, float romm_cam[3][3]);
void	DCR_CLASS dcr_parse_mos (DCRAW* p, int offset);
void	DCR_CLASS dcr_linear_table (DCRAW* p, unsigned len);
void	DCR_CLASS dcr_parse_kodak_ifd (DCRAW* p, int base);
void	DCR_CLASS dcr_parse_minolta (DCRAW* p, int base);
int		DCR_CLASS dcr_parse_tiff_ifd (DCRAW* p, int base);
void	DCR_CLASS dcr_parse_tiff (DCRAW* p, int base);
void	DCR_CLASS dcr_parse_minolta (DCRAW* p, int base);
void	DCR_CLASS dcr_parse_external_jpeg(DCRAW* p);
void	DCR_CLASS dcr_ciff_block_1030(DCRAW* p);
void	DCR_CLASS dcr_parse_ciff (DCRAW* p, int offset, int length);
void	DCR_CLASS dcr_parse_rollei(DCRAW* p);
void	DCR_CLASS dcr_parse_sinar_ia(DCRAW* p);
void	DCR_CLASS dcr_parse_phase_one (DCRAW* p, int base);
void	DCR_CLASS dcr_parse_fuji (DCRAW* p, int offset);
int		DCR_CLASS dcr_parse_jpeg (DCRAW* p, int offset);
void	DCR_CLASS dcr_parse_riff(DCRAW* p);
void	DCR_CLASS dcr_parse_smal (DCRAW* p, int offset, int fsize);
void	DCR_CLASS dcr_parse_cine(DCRAW* p);
char *	DCR_CLASS dcr_foveon_gets (DCRAW* p, int offset, char *str, int len);
void	DCR_CLASS dcr_parse_foveon(DCRAW* p);
void	DCR_CLASS dcr_adobe_coeff (DCRAW* p, char *make, char *model);
void	DCR_CLASS dcr_simple_coeff (DCRAW* p, int index);
short	DCR_CLASS dcr_guess_byte_order (DCRAW* p, int words);
void	DCR_CLASS dcr_identify(DCRAW* p);
void	DCR_CLASS dcr_apply_profile (DCRAW* p, char *input, char *output);
void	DCR_CLASS dcr_convert_to_rgb(DCRAW* p);
void	DCR_CLASS dcr_fuji_rotate(DCRAW* p);
void	DCR_CLASS dcr_stretch(DCRAW* p);
int		DCR_CLASS dcr_flip_index (DCRAW* p, int row, int col);
void	DCR_CLASS dcr_gamma_lut (DCRAW* p, uchar lut[0x10000]);
void	DCR_CLASS dcr_tiff_head (DCRAW* p, struct dcr_tiff_hdr *th, int full);
void	DCR_CLASS dcr_jpeg_thumb (DCRAW* p, FILE *tfp);
void	DCR_CLASS dcr_write_ppm_tiff (DCRAW* p, FILE *ofp);
void	DCR_CLASS dcr_init_dcraw(DCRAW* p);
void	DCR_CLASS dcr_cleanup_dcraw(DCRAW* p);
void	DCR_CLASS dcr_print_manual(int argc, char **argv);
int 	DCR_CLASS dcr_parse_command_line_options(DCRAW* p, int argc, char **argv, int *arg);

#endif
