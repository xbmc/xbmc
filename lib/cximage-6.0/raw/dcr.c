/*
  dcr.c is a command-line program to test libdcr.

  based on dcraw.c -- Dave Coffin's raw photo decoder
  Copyright 1997-2008 by Dave Coffin, dcoffin a cybercom o net

  This is a command-line ANSI C program to convert raw photos from
  any digital camera on any computer running any operating system.

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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "libdcr.h"


#ifdef LOCALEDIR
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) (String)
#endif

#ifdef __CYGWIN__
#include <io.h>
#endif
#ifdef WIN32
#include <sys/utime.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
typedef __int64 INT64;
typedef unsigned __int64 UINT64;
#else
#include <unistd.h>
#include <utime.h>
#include <netinet/in.h>
typedef long long INT64;
typedef unsigned long long UINT64;
#endif


int DCR_CLASS main (int argc, char **argv)
{
	DCRAW  dcr;

	int i, c, arg=0, status=0;

	char *ofname;
	FILE *ofp;
	const char *write_ext;

	struct utimbuf ut;

#ifndef LOCALTIME
	putenv ("TZ=UTC");
#endif
#ifdef LOCALEDIR
	setlocale (LC_CTYPE, "");
	setlocale (LC_MESSAGES, "");
	bindtextdomain ("dcraw", LOCALEDIR);
	textdomain ("dcraw");
#endif

	//!!! initialization
	dcr_init_dcraw(&dcr);

	// no arguments; show manual
	if (argc == 1) {
		dcr_print_manual(argc,argv);
		return 1;
	}

	//!!! setup library options, see dcr_print_manual for the available switches
	// call dcr_parse_command_line_options(&dcr,0,0,0) to set default options
	if (dcr_parse_command_line_options(&dcr,argc,argv,&arg))
		return 1;

	if (arg == argc) {
		fprintf (stderr,_("No files to process.\n"));
		return 1;
	}

	if (dcr.opt.write_to_stdout) {
		if (isatty(1)) {
			fprintf (stderr,_("Will not write an image to the terminal!\n"));
			return 1;
		}
#if defined(WIN32) || defined(DJGPP) || defined(__CYGWIN__)
		if (setmode(1,O_BINARY) < 0) {
			perror ("setmode()");
			return 1;
		}
#endif
	}

	for ( ; arg < argc; arg++) {

		status = 1;

		dcr.image = 0;
		dcr.oprof = 0;
		dcr.meta_data = 0;

		ofname = 0;
		ofp = stdout;

		//!!! set return point for error handling
		if (setjmp (dcr.failure)) {
#if !defined(__FreeBSD__)
			if (fileno((FILE*)dcr.obj_) > 2) (*dcr.ops_->close_)(dcr.obj_);
#endif
			if (fileno(ofp) > 2) fclose(ofp);
			status = 1;
			goto cleanup;
		}

		//!!! open input file
		dcr.ifname = argv[arg];
		if (!(dcr.obj_ = fopen (dcr.ifname, "rb"))) {
			perror (dcr.ifname);
			continue;
		}

		//!!! check file header
		dcr_identify(&dcr);
		status = !dcr.is_raw;


		if (dcr.opt.user_flip >= 0)
			dcr.flip = dcr.opt.user_flip;

		switch ((dcr.flip+3600) % 360) {
			case 270:  dcr.flip = 5;  break;
			case 180:  dcr.flip = 3;  break;
			case  90:  dcr.flip = 6;
		}

		if (dcr.opt.timestamp_only) {
			if ((status = !dcr.timestamp))
				fprintf (stderr,_("%s has no dcr.timestamp.\n"), dcr.ifname);
			else if (dcr.opt.identify_only)
				printf ("%10ld%10d %s\n", (long) dcr.timestamp, dcr.shot_order, dcr.ifname);
			else {
				if (dcr.opt.verbose)
					fprintf (stderr,_("%s time set to %d.\n"), dcr.ifname, (int) dcr.timestamp);
				ut.actime = ut.modtime = dcr.timestamp;
				utime (dcr.ifname, &ut);
			}
			goto next;
		}

		//!!! install function for output format conversion
		dcr.write_fun = &DCR_CLASS dcr_write_ppm_tiff;

		if (dcr.opt.thumbnail_only) {
			if ((status = !dcr.thumb_offset)) {
				fprintf (stderr,_("%s has no thumbnail.\n"), dcr.ifname);
				goto next;
			} else if (dcr.thumb_load_raw) {
				dcr.load_raw = dcr.thumb_load_raw;
				dcr.data_offset = dcr.thumb_offset;
				dcr.height = dcr.thumb_height;
				dcr.width  = dcr.thumb_width;
				dcr.filters = 0;
			} else {
				(*dcr.ops_->seek_)(dcr.obj_, dcr.thumb_offset, SEEK_SET);
				dcr.write_fun = dcr.write_thumb;
				goto thumbnail;
			}
		}

		//!!! verify special case
		if (dcr.load_raw == &DCR_CLASS dcr_kodak_ycbcr_load_raw) {
			dcr.height += dcr.height & 1;
			dcr.width  += dcr.width  & 1;
		}

		if (dcr.opt.identify_only && dcr.opt.verbose && dcr.make[0]) {
			printf (_("\nFilename: %s\n"), dcr.ifname);
			printf (_("Timestamp: %s"), ctime(&dcr.timestamp));
			printf (_("Camera: %s %s\n"), dcr.make, dcr.model);
			if (dcr.artist[0])
				printf (_("Owner: %s\n"), dcr.artist);
			if (dcr.dng_version) {
				printf (_("DNG Version: "));
				for (i=24; i >= 0; i -= 8)
					printf ("%d%c", dcr.dng_version >> i & 255, i ? '.':'\n');
			}
			printf (_("ISO speed: %d\n"), (int) dcr.iso_speed);
			printf (_("Shutter: "));
			if (dcr.shutter > 0 && dcr.shutter < 1)
				dcr.shutter = (printf ("1/"), 1 / dcr.shutter);
			printf (_("%0.1f sec\n"), dcr.shutter);
			printf (_("Aperture: f/%0.1f\n"), dcr.aperture);
			printf (_("Focal length: %0.1f mm\n"), dcr.focal_len);
			printf (_("Embedded ICC profile: %s\n"), dcr.profile_length ? _("yes"):_("no"));
			printf (_("Number of raw images: %d\n"), dcr.is_raw);
			if (dcr.pixel_aspect != 1)
				printf (_("Pixel Aspect Ratio: %0.6f\n"), dcr.pixel_aspect);
			if (dcr.thumb_offset)
				printf (_("Thumb size:  %4d x %d\n"), dcr.thumb_width, dcr.thumb_height);
			printf (_("Full size:   %4d x %d\n"), dcr.raw_width, dcr.raw_height);
		} else if (!dcr.is_raw)
			fprintf (stderr,_("Cannot decode file %s\n"), dcr.ifname);

		// last check before actual image decoding process
		if (!dcr.is_raw) goto next;

		if (!dcr.load_raw) goto next;

		//!!! shrinked decoding available and requested?
		dcr.shrink = dcr.filters && (dcr.opt.half_size || dcr.opt.threshold || dcr.opt.aber[0] != 1 || dcr.opt.aber[2] != 1);
		dcr.iheight = (dcr.height + dcr.shrink) >> dcr.shrink;
		dcr.iwidth  = (dcr.width  + dcr.shrink) >> dcr.shrink;

		if (dcr.opt.identify_only) {
			if (dcr.opt.verbose) {
				if (dcr.opt.use_fuji_rotate) {
					if (dcr.fuji_width) {
						dcr.fuji_width = (dcr.fuji_width - 1 + dcr.shrink) >> dcr.shrink;
						dcr.iwidth = (unsigned short)(dcr.fuji_width / sqrt(0.5));
						dcr.iheight = (unsigned short)((dcr.iheight - dcr.fuji_width) / sqrt(0.5));
					} else {
						if (dcr.pixel_aspect < 1) dcr.iheight = (unsigned short)(dcr.iheight / dcr.pixel_aspect + 0.5);
						if (dcr.pixel_aspect > 1) dcr.iwidth  = (unsigned short)(dcr.iwidth  * dcr.pixel_aspect + 0.5);
					}
				}
				if (dcr.flip & 4)
					SWAP(dcr.iheight,dcr.iwidth);
				printf (_("Image size:  %4d x %d\n"), dcr.width, dcr.height);
				printf (_("Output size: %4d x %d\n"), dcr.iwidth, dcr.iheight);
				printf (_("Raw dcr.colors: %d"), dcr.colors);
				if (dcr.filters) {
					printf (_("\nFilter pattern: "));
					if (!dcr.cdesc[3]) dcr.cdesc[3] = 'G';
					for (i=0; i < 16; i++)
						putchar (dcr.cdesc[dcr_fc(&dcr, i >> 1,i & 1)]);
				}
				printf (_("\nDaylight multipliers:"));
				FORCC((&dcr)) printf (" %f", dcr.pre_mul[c]);
				if (dcr.cam_mul[0] > 0) {
					printf (_("\nCamera multipliers:"));
					FORC4 printf (" %f", dcr.cam_mul[c]);
				}
				putchar ('\n');
			} else
				printf (_("%s is a %s %s image.\n"), dcr.ifname, dcr.make, dcr.model);
next:
			(*dcr.ops_->close_)(dcr.obj_);
			continue;
		}

		//!!! install custom camera matrix
		if (dcr.opt.use_camera_matrix && dcr.cmatrix[0][0] > 0.25) {
			memcpy (dcr.rgb_cam, dcr.cmatrix, sizeof dcr.cmatrix);
			dcr.raw_color = 0;
		}

		//!!! allocate memory for the image
		dcr.image = (ushort (*)[4]) calloc (dcr.iheight*dcr.iwidth, sizeof *dcr.image);
		dcr_merror (&dcr, dcr.image, "main()");

		if (dcr.meta_length) {
			dcr.meta_data = (char *) malloc (dcr.meta_length);
			dcr_merror (&dcr, dcr.meta_data, "main()");
		}

		if (dcr.opt.verbose)
			fprintf (stderr,_("Loading %s %s image from %s ...\n"),
			dcr.make, dcr.model, dcr.ifname);

		if (dcr.opt.shot_select >= dcr.is_raw)
			fprintf (stderr,_("%s: \"-s %d\" requests a nonexistent image!\n"),
			dcr.ifname, dcr.opt.shot_select);

		//!!! start image decoder
		(*dcr.ops_->seek_)(dcr.obj_, dcr.data_offset, SEEK_SET);
		(*dcr.load_raw)(&dcr);

		if (dcr.zero_is_bad) dcr_remove_zeroes(&dcr);

		dcr_bad_pixels(&dcr,dcr.opt.bpfile);

		if (dcr.opt.dark_frame) dcr_subtract (&dcr,dcr.opt.dark_frame);

		dcr.quality = 2 + !dcr.fuji_width;

		if (dcr.opt.user_qual >= 0) dcr.quality = dcr.opt.user_qual;

		if (dcr.opt.user_black >= 0) dcr.black = dcr.opt.user_black;

		if (dcr.opt.user_sat >= 0) dcr.maximum = dcr.opt.user_sat;

#ifdef COLORCHECK
		dcr_colorcheck(&dcr);
#endif

#if RESTRICTED
		if (dcr.is_foveon && !dcr.opt.document_mode) dcr_foveon_interpolate(&dcr);
#endif

		if (!dcr.is_foveon && dcr.opt.document_mode < 2) dcr_scale_colors(&dcr);

		dcr_pre_interpolate(&dcr);

		if (dcr.filters && !dcr.opt.document_mode) {
			if (dcr.quality == 0)
				dcr_lin_interpolate(&dcr);
			else if (dcr.quality == 1 || dcr.colors > 3)
				dcr_vng_interpolate(&dcr);
			else if (dcr.quality == 2)
				dcr_ppg_interpolate(&dcr);
			else dcr_ahd_interpolate(&dcr);
		}

		if (dcr.mix_green) {
			for (dcr.colors=3, i=0; i < dcr.height*dcr.width; i++) {
				dcr.image[i][1] = (dcr.image[i][1] + dcr.image[i][3]) >> 1;
			}
		}

		if (!dcr.is_foveon && dcr.colors == 3) dcr_median_filter(&dcr);

		if (!dcr.is_foveon && dcr.opt.highlight == 2) dcr_blend_highlights(&dcr);

		if (!dcr.is_foveon && dcr.opt.highlight > 2) dcr_recover_highlights(&dcr);

		if (dcr.opt.use_fuji_rotate) dcr_fuji_rotate(&dcr);

#ifndef NO_LCMS
		if (dcr.opt.cam_profile) dcr_apply_profile (&dcr, dcr.opt.cam_profile, dcr.opt.out_profile);
#endif

		dcr_convert_to_rgb(&dcr);

		if (dcr.opt.use_fuji_rotate) dcr_stretch(&dcr);

thumbnail:

		if (dcr.write_fun == &DCR_CLASS dcr_jpeg_thumb)
			write_ext = ".jpg";
		else if (dcr.opt.output_tiff && dcr.write_fun == &DCR_CLASS dcr_write_ppm_tiff)
			write_ext = ".tiff";
		else
			write_ext = ".pgm\0.ppm\0.ppm\0.pam" + dcr.colors*5-5;

		ofname = (char *) malloc (strlen(dcr.ifname) + 64);
		dcr_merror (&dcr, ofname, "main()");
		if (dcr.opt.write_to_stdout)
			strcpy (ofname,_("standard output"));
		else {
			char *cp;
			strcpy (ofname, dcr.ifname);
			if ((cp = strrchr (ofname, '.'))) *cp = 0;
			if (dcr.opt.multi_out)
				sprintf (ofname+strlen(ofname), "_%0*d",
				snprintf(0,0,"%d",dcr.is_raw-1), dcr.opt.shot_select);
			if (dcr.opt.thumbnail_only)
				strcat (ofname, ".thumb");
			strcat (ofname, write_ext);
			ofp = fopen (ofname, "wb");
			if (!ofp) {
				status = 1;
				perror (ofname);
				goto cleanup;
			}
		}

		if (dcr.opt.verbose)
			fprintf (stderr,_("Writing data to %s ...\n"), ofname);

		(*dcr.write_fun)(&dcr,ofp);

		(*dcr.ops_->close_)(dcr.obj_);

		if (ofp != stdout) fclose(ofp);

cleanup:

		dcr_cleanup_dcraw(&dcr);

		if (ofname) free (ofname);
		if (dcr.opt.multi_out) {
			if (++dcr.opt.shot_select < dcr.is_raw) arg--;
			else dcr.opt.shot_select = 0;
		}
  }
  return status;
}
