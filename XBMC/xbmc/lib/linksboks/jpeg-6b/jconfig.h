/* jconfig.vc --- jconfig.h for Microsoft Visual C++ on Windows 95 or NT. */
/* see jconfig.doc for explanations */

/* ysbox: There is already a version of jpeglib in d3dx8d.lib and the
 * Microsoft guys wisely kept the same names so we do that trick to
 * avoid duplicate symbols and/or bad linking
 */
#define jcopy_block_row				linksboks_jcopy_block_row
#define jcopy_markers_execute				linksboks_jcopy_markers_execute
#define jcopy_markers_setup				linksboks_jcopy_markers_setup
#define jcopy_sample_rows				linksboks_jcopy_sample_rows
#define jdiv_round_up				linksboks_jdiv_round_up
#define jinit_1pass_quantizer				linksboks_jinit_1pass_quantizer
#define jinit_2pass_quantizer				linksboks_jinit_2pass_quantizer
#define jinit_c_coef_controller				linksboks_jinit_c_coef_controller
#define jinit_c_main_controller				linksboks_jinit_c_main_controller
#define jinit_c_master_control				linksboks_jinit_c_master_control
#define jinit_color_converter				linksboks_jinit_color_converter
#define jinit_color_deconverter				linksboks_jinit_color_deconverter
#define jinit_compress_master				linksboks_jinit_compress_master
#define jinit_c_prep_controller				linksboks_jinit_c_prep_controller
#define jinit_d_coef_controller				linksboks_jinit_d_coef_controller
#define jinit_d_main_controller				linksboks_jinit_d_main_controller
#define jinit_downsampler				linksboks_jinit_downsampler
#define jinit_d_post_controller				linksboks_jinit_d_post_controller
#define jinit_forward_dct				linksboks_jinit_forward_dct
#define jinit_huff_decoder				linksboks_jinit_huff_decoder
#define jinit_huff_encoder				linksboks_jinit_huff_encoder
#define jinit_input_controller				linksboks_jinit_input_controller
#define jinit_inverse_dct				linksboks_jinit_inverse_dct
#define jinit_marker_reader				linksboks_jinit_marker_reader
#define jinit_marker_writer				linksboks_jinit_marker_writer
#define jinit_master_decompress				linksboks_jinit_master_decompress
#define jinit_memory_mgr				linksboks_jinit_memory_mgr
#define jinit_merged_upsampler				linksboks_jinit_merged_upsampler
#define jinit_phuff_decoder				linksboks_jinit_phuff_decoder
#define jinit_phuff_encoder				linksboks_jinit_phuff_encoder
#define jinit_read_bmp				linksboks_jinit_read_bmp
#define jinit_read_gif				linksboks_jinit_read_gif
#define jinit_read_ppm				linksboks_jinit_read_ppm
#define jinit_read_targa				linksboks_jinit_read_targa
#define jinit_upsampler				linksboks_jinit_upsampler
#define jinit_write_bmp				linksboks_jinit_write_bmp
#define jinit_write_gif				linksboks_jinit_write_gif
#define jinit_write_ppm				linksboks_jinit_write_ppm
#define jinit_write_targa				linksboks_jinit_write_targa
#define jpeg_abort				linksboks_jpeg_abort
#define jpeg_abort_compress				linksboks_jpeg_abort_compress
#define jpeg_abort_decompress				linksboks_jpeg_abort_decompress
#define jpeg_add_quant_table				linksboks_jpeg_add_quant_table
#define jpeg_alloc_huff_table				linksboks_jpeg_alloc_huff_table
#define jpeg_alloc_quant_table				linksboks_jpeg_alloc_quant_table
#define jpeg_calc_output_dimensions				linksboks_jpeg_calc_output_dimensions
#define jpeg_consume_input				linksboks_jpeg_consume_input
#define jpeg_copy_critical_parameters				linksboks_jpeg_copy_critical_parameters
#define jpeg_CreateCompress				linksboks_jpeg_CreateCompress
#define jpeg_CreateDecompress				linksboks_jpeg_CreateDecompress
#define jpeg_default_colorspace				linksboks_jpeg_default_colorspace
#define jpeg_destroy				linksboks_jpeg_destroy
#define jpeg_destroy_compress				linksboks_jpeg_destroy_compress
#define jpeg_destroy_decompress				linksboks_jpeg_destroy_decompress
#define jpeg_fdct_float				linksboks_jpeg_fdct_float
#define jpeg_fdct_ifast				linksboks_jpeg_fdct_ifast
#define jpeg_fdct_islow				linksboks_jpeg_fdct_islow
#define jpeg_fill_bit_buffer				linksboks_jpeg_fill_bit_buffer
#define jpeg_finish_compress				linksboks_jpeg_finish_compress
#define jpeg_finish_decompress				linksboks_jpeg_finish_decompress
#define jpeg_finish_output				linksboks_jpeg_finish_output
#define jpeg_free_large				linksboks_jpeg_free_large
#define jpeg_free_small				linksboks_jpeg_free_small
#define jpeg_gen_optimal_table				linksboks_jpeg_gen_optimal_table
#define jpeg_get_large				linksboks_jpeg_get_large
#define jpeg_get_small				linksboks_jpeg_get_small
#define jpeg_has_multiple_scans				linksboks_jpeg_has_multiple_scans
#define jpeg_huff_decode				linksboks_jpeg_huff_decode
#define jpeg_idct_1x1				linksboks_jpeg_idct_1x1
#define jpeg_idct_2x2				linksboks_jpeg_idct_2x2
#define jpeg_idct_4x4				linksboks_jpeg_idct_4x4
#define jpeg_idct_float				linksboks_jpeg_idct_float
#define jpeg_idct_ifast				linksboks_jpeg_idct_ifast
#define jpeg_idct_islow				linksboks_jpeg_idct_islow
#define jpeg_input_complete				linksboks_jpeg_input_complete
#define jpeg_make_c_derived_tbl				linksboks_jpeg_make_c_derived_tbl
#define jpeg_make_d_derived_tbl				linksboks_jpeg_make_d_derived_tbl
#define jpeg_mem_available				linksboks_jpeg_mem_available
#define jpeg_mem_init				linksboks_jpeg_mem_init
#define jpeg_mem_term				linksboks_jpeg_mem_term
#define jpeg_natural_order				linksboks_jpeg_natural_order
#define jpeg_new_colormap				linksboks_jpeg_new_colormap
#define jpeg_open_backing_store				linksboks_jpeg_open_backing_store
#define jpeg_quality_scaling				linksboks_jpeg_quality_scaling
#define jpeg_read_coefficients				linksboks_jpeg_read_coefficients
#define jpeg_read_header				linksboks_jpeg_read_header
#define jpeg_read_raw_data				linksboks_jpeg_read_raw_data
#define jpeg_read_scanlines				linksboks_jpeg_read_scanlines
#define jpeg_resync_to_restart				linksboks_jpeg_resync_to_restart
#define jpeg_save_markers				linksboks_jpeg_save_markers
#define jpeg_set_colorspace				linksboks_jpeg_set_colorspace
#define jpeg_set_defaults				linksboks_jpeg_set_defaults
#define jpeg_set_linear_quality				linksboks_jpeg_set_linear_quality
#define jpeg_set_marker_processor				linksboks_jpeg_set_marker_processor
#define jpeg_set_quality				linksboks_jpeg_set_quality
#define jpeg_simple_progression				linksboks_jpeg_simple_progression
#define jpeg_start_compress				linksboks_jpeg_start_compress
#define jpeg_start_decompress				linksboks_jpeg_start_decompress
#define jpeg_start_output				linksboks_jpeg_start_output
#define jpeg_std_error				linksboks_jpeg_std_error
#define jpeg_stdio_dest				linksboks_jpeg_stdio_dest
#define jpeg_stdio_src				linksboks_jpeg_stdio_src
#define jpeg_std_message_table				linksboks_jpeg_std_message_table
#define jpeg_suppress_tables				linksboks_jpeg_suppress_tables
#define jpeg_write_coefficients				linksboks_jpeg_write_coefficients
#define jpeg_write_marker				linksboks_jpeg_write_marker
#define jpeg_write_m_byte				linksboks_jpeg_write_m_byte
#define jpeg_write_m_header				linksboks_jpeg_write_m_header
#define jpeg_write_raw_data				linksboks_jpeg_write_raw_data
#define jpeg_write_scanlines				linksboks_jpeg_write_scanlines
#define jpeg_write_tables				linksboks_jpeg_write_tables
#define jround_up				linksboks_jround_up
#define jtransform_adjust_parameters				linksboks_jtransform_adjust_parameters
#define jtransform_execute_transformation				linksboks_jtransform_execute_transformation
#define jtransform_request_workspace				linksboks_jtransform_request_workspace
#define jzero_far				linksboks_jzero_far
#define keymatch				linksboks_keymatch
#define read_color_map				linksboks_read_color_map
#define read_quant_tables				linksboks_read_quant_tables
#define read_scan_script				linksboks_read_scan_script
#define read_stdin				linksboks_read_stdin
#define set_quant_slots				linksboks_set_quant_slots
#define set_sample_factors				linksboks_set_sample_factors
#define ungetc				linksboks_ungetc
#define write_stdout				linksboks_write_stdout

/* Xbox/LinksBoks: Section everything */
#ifdef _XBOX
#pragma code_seg( "LNKSBOKS" )
#pragma data_seg( "LBKS_RW" )
#pragma bss_seg( "LBKS_RW" )
#endif


#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* #define void char */
/* #define const */
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS	/* we presume a 32-bit flat memory model */
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

#define NO_GETENV


/* Define "boolean" as unsigned char, not int, per Windows custom */
#ifndef __RPCNDR_H__		/* don't conflict if rpcndr.h already read */
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN		/* prevent jmorecfg.h from redefining it */


#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED		/* BMP image file format */
#define GIF_SUPPORTED		/* GIF image file format */
#define PPM_SUPPORTED		/* PBMPLUS PPM/PGM image file format */
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED		/* Targa image file format */

#define TWO_FILE_COMMANDLINE	/* optional */
#define USE_SETMODE		/* Microsoft has setmode() */
#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT		/* optional */

#endif /* JPEG_CJPEG_DJPEG */
