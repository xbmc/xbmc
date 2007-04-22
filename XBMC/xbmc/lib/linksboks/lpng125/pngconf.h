/* pngconf.h - machine configurable file for libpng
 *
 * libpng 1.2.5 - October 3, 2002
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2002 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 */

/* Any machine specific code is near the front of this file, so if you
 * are configuring libpng for a machine, you may want to read the section
 * starting here down to where it starts to typedef png_color, png_text,
 * and png_info.
 */

#ifndef PNGCONF_H
#define PNGCONF_H

#define PNG_NO_GLOBAL_ARRAYS

/* ysbox: There is already a version of libpng in d3dx8d.lib and the
 * Microsoft guys wisely kept the same names so we do that trick to
 * avoid duplicate symbols and/or bad linking
 */
#define png_access_version_number				linksboks_png_access_version_number
#define png_build_gamma_table				linksboks_png_build_gamma_table
#define png_build_grayscale_palette				linksboks_png_build_grayscale_palette
#define png_calculate_crc				linksboks_png_calculate_crc
#define png_check_chunk_name				linksboks_png_check_chunk_name
#define png_check_keyword				linksboks_png_check_keyword
#define png_check_sig				linksboks_png_check_sig
#define png_chunk_error				linksboks_png_chunk_error
#define png_chunk_warning				linksboks_png_chunk_warning
#define png_combine_row				linksboks_png_combine_row
#define png_convert_from_struct_tm				linksboks_png_convert_from_struct_tm
#define png_convert_from_time_t				linksboks_png_convert_from_time_t
#define png_convert_to_rfc1123				linksboks_png_convert_to_rfc1123
#define png_crc_error				linksboks_png_crc_error
#define png_crc_finish				linksboks_png_crc_finish
#define png_crc_read				linksboks_png_crc_read
#define png_create_info_struct				linksboks_png_create_info_struct
#define png_create_read_struct				linksboks_png_create_read_struct
#define png_create_read_struct_2				linksboks_png_create_read_struct_2
#define png_create_struct				linksboks_png_create_struct
#define png_create_struct_2				linksboks_png_create_struct_2
#define png_create_write_struct				linksboks_png_create_write_struct
#define png_create_write_struct_2				linksboks_png_create_write_struct_2
#define png_data_freer				linksboks_png_data_freer
#define png_decompress_chunk				linksboks_png_decompress_chunk
#define png_default_flush				linksboks_png_default_flush
#define png_default_read_data				linksboks_png_default_read_data
#define png_default_write_data				linksboks_png_default_write_data
#define png_destroy_info_struct				linksboks_png_destroy_info_struct
#define png_destroy_read_struct				linksboks_png_destroy_read_struct
#define png_destroy_struct				linksboks_png_destroy_struct
#define png_destroy_struct_2				linksboks_png_destroy_struct_2
#define png_destroy_write_struct				linksboks_png_destroy_write_struct
#define png_do_background				linksboks_png_do_background
#define png_do_bgr				linksboks_png_do_bgr
#define png_do_chop				linksboks_png_do_chop
#define png_do_dither				linksboks_png_do_dither
#define png_do_expand				linksboks_png_do_expand
#define png_do_expand_palette				linksboks_png_do_expand_palette
#define png_do_gamma				linksboks_png_do_gamma
#define png_do_gray_to_rgb				linksboks_png_do_gray_to_rgb
#define png_do_invert				linksboks_png_do_invert
#define png_do_pack				linksboks_png_do_pack
#define png_do_packswap				linksboks_png_do_packswap
#define png_do_read_filler				linksboks_png_do_read_filler
#define png_do_read_interlace				linksboks_png_do_read_interlace
#define png_do_read_intrapixel				linksboks_png_do_read_intrapixel
#define png_do_read_invert_alpha				linksboks_png_do_read_invert_alpha
#define png_do_read_swap_alpha				linksboks_png_do_read_swap_alpha
#define png_do_read_transformations				linksboks_png_do_read_transformations
#define png_do_rgb_to_gray				linksboks_png_do_rgb_to_gray
#define png_do_shift				linksboks_png_do_shift
#define png_do_strip_filler				linksboks_png_do_strip_filler
#define png_do_swap				linksboks_png_do_swap
#define png_do_unpack				linksboks_png_do_unpack
#define png_do_unshift				linksboks_png_do_unshift
#define png_do_write_interlace				linksboks_png_do_write_interlace
#define png_do_write_intrapixel				linksboks_png_do_write_intrapixel
#define png_do_write_invert_alpha				linksboks_png_do_write_invert_alpha
#define png_do_write_swap_alpha				linksboks_png_do_write_swap_alpha
#define png_do_write_transformations				linksboks_png_do_write_transformations
#define png_error				linksboks_png_error
#define png_flush				linksboks_png_flush
#define png_free				linksboks_png_free
#define png_free_data				linksboks_png_free_data
#define png_free_default				linksboks_png_free_default
#define png_get_asm_flagmask				linksboks_png_get_asm_flagmask
#define png_get_asm_flags				linksboks_png_get_asm_flags
#define png_get_bit_depth				linksboks_png_get_bit_depth
#define png_get_bKGD				linksboks_png_get_bKGD
#define png_get_channels				linksboks_png_get_channels
#define png_get_cHRM				linksboks_png_get_cHRM
#define png_get_cHRM_fixed				linksboks_png_get_cHRM_fixed
#define png_get_color_type				linksboks_png_get_color_type
#define png_get_compression_buffer_size				linksboks_png_get_compression_buffer_size
#define png_get_compression_type				linksboks_png_get_compression_type
#define png_get_copyright				linksboks_png_get_copyright
#define png_get_error_ptr				linksboks_png_get_error_ptr
#define png_get_filter_type				linksboks_png_get_filter_type
#define png_get_gAMA				linksboks_png_get_gAMA
#define png_get_gAMA_fixed				linksboks_png_get_gAMA_fixed
#define png_get_header_ver				linksboks_png_get_header_ver
#define png_get_header_version				linksboks_png_get_header_version
#define png_get_hIST				linksboks_png_get_hIST
#define png_get_iCCP				linksboks_png_get_iCCP
#define png_get_IHDR				linksboks_png_get_IHDR
#define png_get_image_height				linksboks_png_get_image_height
#define png_get_image_width				linksboks_png_get_image_width
#define png_get_int_32				linksboks_png_get_int_32
#define png_get_interlace_type				linksboks_png_get_interlace_type
#define png_get_io_ptr				linksboks_png_get_io_ptr
#define png_ver				linksboks_png_ver
#define png_get_mem_ptr				linksboks_png_get_mem_ptr
#define png_get_mmx_bitdepth_threshold				linksboks_png_get_mmx_bitdepth_threshold
#define png_get_mmx_flagmask				linksboks_png_get_mmx_flagmask
#define png_get_mmx_rowbytes_threshold				linksboks_png_get_mmx_rowbytes_threshold
#define png_get_oFFs				linksboks_png_get_oFFs
#define png_get_pCAL				linksboks_png_get_pCAL
#define png_get_pHYs				linksboks_png_get_pHYs
#define png_get_pixel_aspect_ratio				linksboks_png_get_pixel_aspect_ratio
#define png_get_pixels_per_meter				linksboks_png_get_pixels_per_meter
#define png_get_PLTE				linksboks_png_get_PLTE
#define png_get_progressive_ptr				linksboks_png_get_progressive_ptr
#define png_get_rgb_to_gray_status				linksboks_png_get_rgb_to_gray_status
#define png_get_rowbytes				linksboks_png_get_rowbytes
#define png_get_rows				linksboks_png_get_rows
#define png_get_sBIT				linksboks_png_get_sBIT
#define png_get_sCAL				linksboks_png_get_sCAL
#define png_get_signature				linksboks_png_get_signature
#define png_get_sPLT				linksboks_png_get_sPLT
#define png_get_sRGB				linksboks_png_get_sRGB
#define png_get_text				linksboks_png_get_text
#define png_get_tIME				linksboks_png_get_tIME
#define png_get_tRNS				linksboks_png_get_tRNS
#define png_get_uint_16				linksboks_png_get_uint_16
#define png_get_uint_32				linksboks_png_get_uint_32
#define png_get_unknown_chunks				linksboks_png_get_unknown_chunks
#define png_get_user_chunk_ptr				linksboks_png_get_user_chunk_ptr
#define png_get_user_transform_ptr				linksboks_png_get_user_transform_ptr
#define png_get_valid				linksboks_png_get_valid
#define png_get_x_offset_microns				linksboks_png_get_x_offset_microns
#define png_get_x_offset_pixels				linksboks_png_get_x_offset_pixels
#define png_get_x_pixels_per_meter				linksboks_png_get_x_pixels_per_meter
#define png_get_y_offset_microns				linksboks_png_get_y_offset_microns
#define png_get_y_offset_pixels				linksboks_png_get_y_offset_pixels
#define png_get_y_pixels_per_meter				linksboks_png_get_y_pixels_per_meter
#define png_handle_as_unknown				linksboks_png_handle_as_unknown
#define png_handle_bKGD				linksboks_png_handle_bKGD
#define png_handle_cHRM				linksboks_png_handle_cHRM
#define png_handle_gAMA				linksboks_png_handle_gAMA
#define png_handle_hIST				linksboks_png_handle_hIST
#define png_handle_iCCP				linksboks_png_handle_iCCP
#define png_handle_IEND				linksboks_png_handle_IEND
#define png_handle_IHDR				linksboks_png_handle_IHDR
#define png_handle_oFFs				linksboks_png_handle_oFFs
#define png_handle_pCAL				linksboks_png_handle_pCAL
#define png_handle_pHYs				linksboks_png_handle_pHYs
#define png_handle_PLTE				linksboks_png_handle_PLTE
#define png_handle_sBIT				linksboks_png_handle_sBIT
#define png_handle_sCAL				linksboks_png_handle_sCAL
#define png_handle_sPLT				linksboks_png_handle_sPLT
#define png_handle_sRGB				linksboks_png_handle_sRGB
#define png_handle_tEXt				linksboks_png_handle_tEXt
#define png_handle_tIME				linksboks_png_handle_tIME
#define png_handle_tRNS				linksboks_png_handle_tRNS
#define png_handle_unknown				linksboks_png_handle_unknown
#define png_handle_zTXt				linksboks_png_handle_zTXt
#define png_info_destroy				linksboks_png_info_destroy
//#define png_info_init				linksboks_png_info_init
#define png_info_init_3				linksboks_png_info_init_3
#define png_init_io				linksboks_png_init_io
#define png_init_mmx_flags				linksboks_png_init_mmx_flags
#define png_init_read_transformations				linksboks_png_init_read_transformations
#define png_malloc				linksboks_png_malloc
#define png_malloc_default				linksboks_png_malloc_default
#define png_malloc_warn				linksboks_png_malloc_warn
#define png_memcpy_check				linksboks_png_memcpy_check
#define png_memset_check				linksboks_png_memset_check
#define png_mmx_support				linksboks_png_mmx_support
#define png_permit_empty_plte				linksboks_png_permit_empty_plte
#define png_permit_mng_features				linksboks_png_permit_mng_features
#define png_process_data				linksboks_png_process_data
#define png_process_IDAT_data				linksboks_png_process_IDAT_data
#define png_process_some_data				linksboks_png_process_some_data
#define png_progressive_combine_row				linksboks_png_progressive_combine_row
#define png_push_crc_finish				linksboks_png_push_crc_finish
#define png_push_crc_skip				linksboks_png_push_crc_skip
#define png_push_fill_buffer				linksboks_png_push_fill_buffer
#define png_push_handle_tEXt				linksboks_png_push_handle_tEXt
#define png_push_handle_unknown				linksboks_png_push_handle_unknown
#define png_push_handle_zTXt				linksboks_png_push_handle_zTXt
#define png_push_have_end				linksboks_png_push_have_end
#define png_push_have_info				linksboks_png_push_have_info
#define png_push_have_row				linksboks_png_push_have_row
#define png_push_process_row				linksboks_png_push_process_row
#define png_push_read_chunk				linksboks_png_push_read_chunk
#define png_push_read_IDAT				linksboks_png_push_read_IDAT
#define png_push_read_sig				linksboks_png_push_read_sig
#define png_push_read_tEXt				linksboks_png_push_read_tEXt
#define png_push_read_zTXt				linksboks_png_push_read_zTXt
#define png_push_restore_buffer				linksboks_png_push_restore_buffer
#define png_push_save_buffer				linksboks_png_push_save_buffer
#define png_read_data				linksboks_png_read_data
#define png_read_destroy				linksboks_png_read_destroy
#define png_read_end				linksboks_png_read_end
#define png_read_filter_row				linksboks_png_read_filter_row
#define png_read_finish_row				linksboks_png_read_finish_row
#define png_read_image				linksboks_png_read_image
#define png_read_info				linksboks_png_read_info
//#define png_read_init				linksboks_png_read_init
#define png_read_init_2				linksboks_png_read_init_2
#define png_read_init_3				linksboks_png_read_init_3
#define png_read_push_finish_row				linksboks_png_read_push_finish_row
#define png_read_row				linksboks_png_read_row
#define png_read_rows				linksboks_png_read_rows
#define png_read_start_row				linksboks_png_read_start_row
#define png_read_transform_info				linksboks_png_read_transform_info
#define png_read_update_info				linksboks_png_read_update_info
#define png_reset_crc				linksboks_png_reset_crc
#define png_reset_zstream				linksboks_png_reset_zstream
#define png_save_int_32				linksboks_png_save_int_32
#define png_save_uint_16				linksboks_png_save_uint_16
#define png_save_uint_32				linksboks_png_save_uint_32
#define png_set_asm_flags				linksboks_png_set_asm_flags
#define png_set_background				linksboks_png_set_background
#define png_set_bgr				linksboks_png_set_bgr
#define png_set_bKGD				linksboks_png_set_bKGD
#define png_set_cHRM				linksboks_png_set_cHRM
#define png_set_cHRM_fixed				linksboks_png_set_cHRM_fixed
#define png_set_compression_buffer_size				linksboks_png_set_compression_buffer_size
#define png_set_compression_level				linksboks_png_set_compression_level
#define png_set_compression_mem_level				linksboks_png_set_compression_mem_level
#define png_set_compression_method				linksboks_png_set_compression_method
#define png_set_compression_strategy				linksboks_png_set_compression_strategy
#define png_set_compression_window_bits				linksboks_png_set_compression_window_bits
#define png_set_crc_action				linksboks_png_set_crc_action
#define png_set_dither				linksboks_png_set_dither
#define png_set_error_fn				linksboks_png_set_error_fn
#define png_set_expand				linksboks_png_set_expand
#define png_set_filler				linksboks_png_set_filler
#define png_set_filter				linksboks_png_set_filter
#define png_set_filter_heuristics				linksboks_png_set_filter_heuristics
#define png_set_flush				linksboks_png_set_flush
#define png_set_gAMA				linksboks_png_set_gAMA
#define png_set_gAMA_fixed				linksboks_png_set_gAMA_fixed
#define png_set_gamma				linksboks_png_set_gamma
#define png_set_gray_1_2_4_to_8				linksboks_png_set_gray_1_2_4_to_8
#define png_set_gray_to_rgb				linksboks_png_set_gray_to_rgb
#define png_set_hIST				linksboks_png_set_hIST
#define png_set_iCCP				linksboks_png_set_iCCP
#define png_set_IHDR				linksboks_png_set_IHDR
#define png_set_interlace_handling				linksboks_png_set_interlace_handling
#define png_set_invalid				linksboks_png_set_invalid
#define png_set_invert_alpha				linksboks_png_set_invert_alpha
#define png_set_invert_mono				linksboks_png_set_invert_mono
#define png_set_keep_unknown_chunks				linksboks_png_set_keep_unknown_chunks
#define png_set_mem_fn				linksboks_png_set_mem_fn
#define png_set_mmx_thresholds				linksboks_png_set_mmx_thresholds
#define png_set_oFFs				linksboks_png_set_oFFs
#define png_set_packing				linksboks_png_set_packing
#define png_set_packswap				linksboks_png_set_packswap
#define png_set_palette_to_rgb				linksboks_png_set_palette_to_rgb
#define png_set_pCAL				linksboks_png_set_pCAL
#define png_set_pHYs				linksboks_png_set_pHYs
#define png_set_PLTE				linksboks_png_set_PLTE
#define png_set_progressive_read_fn				linksboks_png_set_progressive_read_fn
#define png_set_read_fn				linksboks_png_set_read_fn
#define png_set_read_status_fn				linksboks_png_set_read_status_fn
#define png_set_read_user_chunk_fn				linksboks_png_set_read_user_chunk_fn
#define png_set_read_user_transform_fn				linksboks_png_set_read_user_transform_fn
#define png_set_rgb_to_gray				linksboks_png_set_rgb_to_gray
#define png_set_rgb_to_gray_fixed				linksboks_png_set_rgb_to_gray_fixed
#define png_set_rows				linksboks_png_set_rows
#define png_set_sBIT				linksboks_png_set_sBIT
#define png_set_sCAL				linksboks_png_set_sCAL
#define png_set_shift				linksboks_png_set_shift
#define png_set_sig_bytes				linksboks_png_set_sig_bytes
#define png_set_sPLT				linksboks_png_set_sPLT
#define png_set_sRGB				linksboks_png_set_sRGB
#define png_set_sRGB_gAMA_and_cHRM				linksboks_png_set_sRGB_gAMA_and_cHRM
#define png_set_strip_16				linksboks_png_set_strip_16
#define png_set_strip_alpha				linksboks_png_set_strip_alpha
#define png_set_strip_error_numbers				linksboks_png_set_strip_error_numbers
#define png_set_swap				linksboks_png_set_swap
#define png_set_swap_alpha				linksboks_png_set_swap_alpha
#define png_set_text				linksboks_png_set_text
#define png_set_text_2				linksboks_png_set_text_2
#define png_set_tIME				linksboks_png_set_tIME
#define png_set_tRNS				linksboks_png_set_tRNS
#define png_set_tRNS_to_alpha				linksboks_png_set_tRNS_to_alpha
#define png_set_unknown_chunk_location				linksboks_png_set_unknown_chunk_location
#define png_set_unknown_chunks				linksboks_png_set_unknown_chunks
#define png_set_user_transform_info				linksboks_png_set_user_transform_info
#define png_set_write_fn				linksboks_png_set_write_fn
#define png_set_write_status_fn				linksboks_png_set_write_status_fn
#define png_set_write_user_transform_fn				linksboks_png_set_write_user_transform_fn
#define png_sig_cmp				linksboks_png_sig_cmp
#define png_start_read_image				linksboks_png_start_read_image
#define png_warning				linksboks_png_warning
#define png_write_bKGD				linksboks_png_write_bKGD
#define png_write_cHRM				linksboks_png_write_cHRM
#define png_write_cHRM_fixed				linksboks_png_write_cHRM_fixed
#define png_write_chunk				linksboks_png_write_chunk
#define png_write_chunk_data				linksboks_png_write_chunk_data
#define png_write_chunk_end				linksboks_png_write_chunk_end
#define png_write_chunk_start				linksboks_png_write_chunk_start
#define png_write_data				linksboks_png_write_data
#define png_write_destroy				linksboks_png_write_destroy
#define png_write_end				linksboks_png_write_end
#define png_write_filtered_row				linksboks_png_write_filtered_row
#define png_write_find_filter				linksboks_png_write_find_filter
#define png_write_finish_row				linksboks_png_write_finish_row
#define png_write_flush				linksboks_png_write_flush
#define png_write_gAMA				linksboks_png_write_gAMA
#define png_write_gAMA_fixed				linksboks_png_write_gAMA_fixed
#define png_write_hIST				linksboks_png_write_hIST
#define png_write_iCCP				linksboks_png_write_iCCP
#define png_write_IDAT				linksboks_png_write_IDAT
#define png_write_IEND				linksboks_png_write_IEND
#define png_write_IHDR				linksboks_png_write_IHDR
#define png_write_image				linksboks_png_write_image
#define png_write_info				linksboks_png_write_info
#define png_write_info_before_PLTE	linksboks_png_write_info_before_PLTE
//#define png_write_init				linksboks_png_write_init
#define png_write_init_2			linksboks_png_write_init_2
#define png_write_init_3			linksboks_png_write_init_3
#define png_write_oFFs				linksboks_png_write_oFFs
#define png_write_pCAL				linksboks_png_write_pCAL
#define png_write_pHYs				linksboks_png_write_pHYs
#define png_write_PLTE				linksboks_png_write_PLTE
#define png_write_row				linksboks_png_write_row
#define png_write_rows				linksboks_png_write_rows
#define png_write_sBIT				linksboks_png_write_sBIT
#define png_write_sCAL				linksboks_png_write_sCAL
#define png_write_sig				linksboks_png_write_sig
#define png_write_sPLT				linksboks_png_write_sPLT
#define png_write_sRGB				linksboks_png_write_sRGB
#define png_write_start_row			linksboks_png_write_start_row
#define png_write_tEXt				linksboks_png_write_tEXt
#define png_write_tIME				linksboks_png_write_tIME
#define png_write_tRNS				linksboks_png_write_tRNS
#define png_write_zTXt				linksboks_png_write_zTXt
#define png_zalloc				linksboks_png_zalloc
#define png_zfree				linksboks_png_zfree


/* Xbox/LinksBoks: Section everything */
#ifdef _XBOX
#pragma code_seg( "LNKSBOKS" )
#pragma data_seg( "LBKS_RW" )
#pragma bss_seg( "LBKS_RW" )
#endif


/* This is the size of the compression buffer, and thus the size of
 * an IDAT chunk.  Make this whatever size you feel is best for your
 * machine.  One of these will be allocated per png_struct.  When this
 * is full, it writes the data to the disk, and does some other
 * calculations.  Making this an extremely small size will slow
 * the library down, but you may want to experiment to determine
 * where it becomes significant, if you are concerned with memory
 * usage.  Note that zlib allocates at least 32Kb also.  For readers,
 * this describes the size of the buffer available to read the data in.
 * Unless this gets smaller than the size of a row (compressed),
 * it should not make much difference how big this is.
 */

#ifndef PNG_ZBUF_SIZE
#  define PNG_ZBUF_SIZE 8192
#endif

/* Enable if you want a write-only libpng */

#ifndef PNG_NO_READ_SUPPORTED
#  define PNG_READ_SUPPORTED
#endif

/* Enable if you want a read-only libpng */

#ifndef PNG_NO_WRITE_SUPPORTED
#  define PNG_WRITE_SUPPORTED
#endif

/* Enabled by default in 1.2.0.  You can disable this if you don't need to
   support PNGs that are embedded in MNG datastreams */
#if !defined(PNG_1_0_X) && !defined(PNG_NO_MNG_FEATURES)
#  ifndef PNG_MNG_FEATURES_SUPPORTED
#    define PNG_MNG_FEATURES_SUPPORTED
#  endif
#endif

#ifndef PNG_NO_FLOATING_POINT_SUPPORTED
#  ifndef PNG_FLOATING_POINT_SUPPORTED
#    define PNG_FLOATING_POINT_SUPPORTED
#  endif
#endif

/* If you are running on a machine where you cannot allocate more
 * than 64K of memory at once, uncomment this.  While libpng will not
 * normally need that much memory in a chunk (unless you load up a very
 * large file), zlib needs to know how big of a chunk it can use, and
 * libpng thus makes sure to check any memory allocation to verify it
 * will fit into memory.
#define PNG_MAX_MALLOC_64K
 */
#if defined(MAXSEG_64K) && !defined(PNG_MAX_MALLOC_64K)
#  define PNG_MAX_MALLOC_64K
#endif

/* Special munging to support doing things the 'cygwin' way:
 * 'Normal' png-on-win32 defines/defaults:
 *   PNG_BUILD_DLL -- building dll
 *   PNG_USE_DLL   -- building an application, linking to dll
 *   (no define)   -- building static library, or building an
 *                    application and linking to the static lib
 * 'Cygwin' defines/defaults:
 *   PNG_BUILD_DLL -- (ignored) building the dll
 *   (no define)   -- (ignored) building an application, linking to the dll
 *   PNG_STATIC    -- (ignored) building the static lib, or building an 
 *                    application that links to the static lib.
 *   ALL_STATIC    -- (ignored) building various static libs, or building an 
 *                    application that links to the static libs.
 * Thus,
 * a cygwin user should define either PNG_BUILD_DLL or PNG_STATIC, and
 * this bit of #ifdefs will define the 'correct' config variables based on
 * that. If a cygwin user *wants* to define 'PNG_USE_DLL' that's okay, but
 * unnecessary.
 *
 * Also, the precedence order is:
 *   ALL_STATIC (since we can't #undef something outside our namespace)
 *   PNG_BUILD_DLL
 *   PNG_STATIC
 *   (nothing) == PNG_USE_DLL
 * 
 * CYGWIN (2002-01-20): The preceding is now obsolete. With the advent
 *   of auto-import in binutils, we no longer need to worry about 
 *   __declspec(dllexport) / __declspec(dllimport) and friends.  Therefore,
 *   we don't need to worry about PNG_STATIC or ALL_STATIC when it comes
 *   to __declspec() stuff.  However, we DO need to worry about 
 *   PNG_BUILD_DLL and PNG_STATIC because those change some defaults
 *   such as CONSOLE_IO and whether GLOBAL_ARRAYS are allowed.
 */
#if defined(__CYGWIN__)
#  if defined(ALL_STATIC)
#    if defined(PNG_BUILD_DLL)
#      undef PNG_BUILD_DLL
#    endif
#    if defined(PNG_USE_DLL)
#      undef PNG_USE_DLL
#    endif
#    if defined(PNG_DLL)
#      undef PNG_DLL
#    endif
#    if !defined(PNG_STATIC)
#      define PNG_STATIC
#    endif
#  else
#    if defined (PNG_BUILD_DLL)
#      if defined(PNG_STATIC)
#        undef PNG_STATIC
#      endif
#      if defined(PNG_USE_DLL)
#        undef PNG_USE_DLL
#      endif
#      if !defined(PNG_DLL)
#        define PNG_DLL
#      endif
#    else
#      if defined(PNG_STATIC)
#        if defined(PNG_USE_DLL)
#          undef PNG_USE_DLL
#        endif
#        if defined(PNG_DLL)
#          undef PNG_DLL
#        endif
#      else
#        if !defined(PNG_USE_DLL)
#          define PNG_USE_DLL
#        endif
#        if !defined(PNG_DLL)
#          define PNG_DLL
#        endif
#      endif  
#    endif  
#  endif
#endif

/* This protects us against compilers that run on a windowing system
 * and thus don't have or would rather us not use the stdio types:
 * stdin, stdout, and stderr.  The only one currently used is stderr
 * in png_error() and png_warning().  #defining PNG_NO_CONSOLE_IO will
 * prevent these from being compiled and used. #defining PNG_NO_STDIO
 * will also prevent these, plus will prevent the entire set of stdio
 * macros and functions (FILE *, printf, etc.) from being compiled and used,
 * unless (PNG_DEBUG > 0) has been #defined.
 *
 * #define PNG_NO_CONSOLE_IO
 * #define PNG_NO_STDIO
 */

#if defined(_WIN32_WCE)
#  include <windows.h>
   /* Console I/O functions are not supported on WindowsCE */
#  define PNG_NO_CONSOLE_IO
#  ifdef PNG_DEBUG
#    undef PNG_DEBUG
#  endif
#endif

#ifdef PNG_BUILD_DLL
#  ifndef PNG_CONSOLE_IO_SUPPORTED
#    ifndef PNG_NO_CONSOLE_IO
#      define PNG_NO_CONSOLE_IO
#    endif
#  endif
#endif

#  ifdef PNG_NO_STDIO
#    ifndef PNG_NO_CONSOLE_IO
#      define PNG_NO_CONSOLE_IO
#    endif
#    ifdef PNG_DEBUG
#      if (PNG_DEBUG > 0)
#        include <stdio.h>
#      endif
#    endif
#  else
#    if !defined(_WIN32_WCE)
/* "stdio.h" functions are not supported on WindowsCE */
#      include <stdio.h>
#    endif
#  endif

/* This macro protects us against machines that don't have function
 * prototypes (ie K&R style headers).  If your compiler does not handle
 * function prototypes, define this macro and use the included ansi2knr.
 * I've always been able to use _NO_PROTO as the indicator, but you may
 * need to drag the empty declaration out in front of here, or change the
 * ifdef to suit your own needs.
 */
#ifndef PNGARG

#ifdef OF /* zlib prototype munger */
#  define PNGARG(arglist) OF(arglist)
#else

#ifdef _NO_PROTO
#  define PNGARG(arglist) ()
#  ifndef PNG_TYPECAST_NULL
#     define PNG_TYPECAST_NULL
#  endif
#else
#  define PNGARG(arglist) arglist
#endif /* _NO_PROTO */

#endif /* OF */

#endif /* PNGARG */

/* Try to determine if we are compiling on a Mac.  Note that testing for
 * just __MWERKS__ is not good enough, because the Codewarrior is now used
 * on non-Mac platforms.
 */
#ifndef MACOS
#  if (defined(__MWERKS__) && defined(macintosh)) || defined(applec) || \
      defined(THINK_C) || defined(__SC__) || defined(TARGET_OS_MAC)
#    define MACOS
#  endif
#endif

/* enough people need this for various reasons to include it here */
#if !defined(MACOS) && !defined(RISCOS) && !defined(_WIN32_WCE)
#  include <sys/types.h>
#endif

#if !defined(PNG_SETJMP_NOT_SUPPORTED) && !defined(PNG_NO_SETJMP_SUPPORTED)
#  define PNG_SETJMP_SUPPORTED
#endif

#ifdef PNG_SETJMP_SUPPORTED
/* This is an attempt to force a single setjmp behaviour on Linux.  If
 * the X config stuff didn't define _BSD_SOURCE we wouldn't need this.
 */

#  ifdef __linux__
#    ifdef _BSD_SOURCE
#      define PNG_SAVE_BSD_SOURCE
#      undef _BSD_SOURCE
#    endif
#    ifdef _SETJMP_H
      __png.h__ already includes setjmp.h;
      __dont__ include it again.;
#    endif
#  endif /* __linux__ */

   /* include setjmp.h for error handling */
#  include <setjmp.h>

#  ifdef __linux__
#    ifdef PNG_SAVE_BSD_SOURCE
#      define _BSD_SOURCE
#      undef PNG_SAVE_BSD_SOURCE
#    endif
#  endif /* __linux__ */
#endif /* PNG_SETJMP_SUPPORTED */

#ifdef BSD
#  include <strings.h>
#else
#  include <string.h>
#endif

/* Other defines for things like memory and the like can go here.  */
#ifdef PNG_INTERNAL

#include <stdlib.h>

/* The functions exported by PNG_EXTERN are PNG_INTERNAL functions, which
 * aren't usually used outside the library (as far as I know), so it is
 * debatable if they should be exported at all.  In the future, when it is
 * possible to have run-time registry of chunk-handling functions, some of
 * these will be made available again.
 */
#define PNG_EXTERN extern

//#define PNG_EXTERN

/* Other defines specific to compilers can go here.  Try to keep
 * them inside an appropriate ifdef/endif pair for portability.
 */

#if defined(PNG_FLOATING_POINT_SUPPORTED)
#  if defined(MACOS)
     /* We need to check that <math.h> hasn't already been included earlier
      * as it seems it doesn't agree with <fp.h>, yet we should really use
      * <fp.h> if possible.
      */
#    if !defined(__MATH_H__) && !defined(__MATH_H) && !defined(__cmath__)
#      include <fp.h>
#    endif
#  else
#    include <math.h>
#  endif
#  if defined(_AMIGA) && defined(__SASC) && defined(_M68881)
     /* Amiga SAS/C: We must include builtin FPU functions when compiling using
      * MATH=68881
      */
#    include <m68881.h>
#  endif
#endif

/* Codewarrior on NT has linking problems without this. */
#if (defined(__MWERKS__) && defined(WIN32)) || defined(__STDC__)
#  define PNG_ALWAYS_EXTERN
#endif

/* For some reason, Borland C++ defines memcmp, etc. in mem.h, not
 * stdlib.h like it should (I think).  Or perhaps this is a C++
 * "feature"?
 */
#ifdef __TURBOC__
#  include <mem.h>
#  include "alloc.h"
#endif

#if defined(_MSC_VER) && (defined(WIN32) || defined(_Windows) || \
    defined(_WINDOWS) || defined(_WIN32) || defined(__WIN32__))
#  include <malloc.h>
#endif

/* This controls how fine the dithering gets.  As this allocates
 * a largish chunk of memory (32K), those who are not as concerned
 * with dithering quality can decrease some or all of these.
 */
#ifndef PNG_DITHER_RED_BITS
#  define PNG_DITHER_RED_BITS 5
#endif
#ifndef PNG_DITHER_GREEN_BITS
#  define PNG_DITHER_GREEN_BITS 5
#endif
#ifndef PNG_DITHER_BLUE_BITS
#  define PNG_DITHER_BLUE_BITS 5
#endif

/* This controls how fine the gamma correction becomes when you
 * are only interested in 8 bits anyway.  Increasing this value
 * results in more memory being used, and more pow() functions
 * being called to fill in the gamma tables.  Don't set this value
 * less then 8, and even that may not work (I haven't tested it).
 */

#ifndef PNG_MAX_GAMMA_8
#  define PNG_MAX_GAMMA_8 11
#endif

/* This controls how much a difference in gamma we can tolerate before
 * we actually start doing gamma conversion.
 */
#ifndef PNG_GAMMA_THRESHOLD
#  define PNG_GAMMA_THRESHOLD 0.05
#endif

#endif /* PNG_INTERNAL */

/* The following uses const char * instead of char * for error
 * and warning message functions, so some compilers won't complain.
 * If you do not want to use const, define PNG_NO_CONST here.
 */

#ifndef PNG_NO_CONST
#  define PNG_CONST const
#else
#  define PNG_CONST
#endif

/* The following defines give you the ability to remove code from the
 * library that you will not be using.  I wish I could figure out how to
 * automate this, but I can't do that without making it seriously hard
 * on the users.  So if you are not using an ability, change the #define
 * to and #undef, and that part of the library will not be compiled.  If
 * your linker can't find a function, you may want to make sure the
 * ability is defined here.  Some of these depend upon some others being
 * defined.  I haven't figured out all the interactions here, so you may
 * have to experiment awhile to get everything to compile.  If you are
 * creating or using a shared library, you probably shouldn't touch this,
 * as it will affect the size of the structures, and this will cause bad
 * things to happen if the library and/or application ever change.
 */

/* Any features you will not be using can be undef'ed here */

/* GR-P, 0.96a: Set "*TRANSFORMS_SUPPORTED as default but allow user
 * to turn it off with "*TRANSFORMS_NOT_SUPPORTED" or *PNG_NO_*_TRANSFORMS
 * on the compile line, then pick and choose which ones to define without
 * having to edit this file. It is safe to use the *TRANSFORMS_NOT_SUPPORTED
 * if you only want to have a png-compliant reader/writer but don't need
 * any of the extra transformations.  This saves about 80 kbytes in a
 * typical installation of the library. (PNG_NO_* form added in version
 * 1.0.1c, for consistency)
 */

/* The size of the png_text structure changed in libpng-1.0.6 when
 * iTXt is supported.  It is turned off by default, to support old apps
 * that malloc the png_text structure instead of calling png_set_text()
 * and letting libpng malloc it.  It will be turned on by default in
 * libpng-1.3.0.
 */

#ifndef PNG_iTXt_SUPPORTED
#  if !defined(PNG_READ_iTXt_SUPPORTED) && !defined(PNG_NO_READ_iTXt)
#    define PNG_NO_READ_iTXt
#  endif
#  if !defined(PNG_WRITE_iTXt_SUPPORTED) && !defined(PNG_NO_WRITE_iTXt)
#    define PNG_NO_WRITE_iTXt
#  endif
#endif

/* The following support, added after version 1.0.0, can be turned off here en
 * masse by defining PNG_LEGACY_SUPPORTED in case you need binary compatibility
 * with old applications that require the length of png_struct and png_info
 * to remain unchanged.
 */

#ifdef PNG_LEGACY_SUPPORTED
#  define PNG_NO_FREE_ME
#  define PNG_NO_READ_UNKNOWN_CHUNKS
#  define PNG_NO_WRITE_UNKNOWN_CHUNKS
#  define PNG_NO_READ_USER_CHUNKS
#  define PNG_NO_READ_iCCP
#  define PNG_NO_WRITE_iCCP
#  define PNG_NO_READ_iTXt
#  define PNG_NO_WRITE_iTXt
#  define PNG_NO_READ_sCAL
#  define PNG_NO_WRITE_sCAL
#  define PNG_NO_READ_sPLT
#  define PNG_NO_WRITE_sPLT
#  define PNG_NO_INFO_IMAGE
#  define PNG_NO_READ_RGB_TO_GRAY
#  define PNG_NO_READ_USER_TRANSFORM
#  define PNG_NO_WRITE_USER_TRANSFORM
#  define PNG_NO_USER_MEM
#  define PNG_NO_READ_EMPTY_PLTE
#  define PNG_NO_MNG_FEATURES
#  define PNG_NO_FIXED_POINT_SUPPORTED
#endif

/* Ignore attempt to turn off both floating and fixed point support */
#if !defined(PNG_FLOATING_POINT_SUPPORTED) || \
    !defined(PNG_NO_FIXED_POINT_SUPPORTED)
#  define PNG_FIXED_POINT_SUPPORTED
#endif

#ifndef PNG_NO_FREE_ME
#  define PNG_FREE_ME_SUPPORTED
#endif

#if defined(PNG_READ_SUPPORTED)

#if !defined(PNG_READ_TRANSFORMS_NOT_SUPPORTED) && \
      !defined(PNG_NO_READ_TRANSFORMS)
#  define PNG_READ_TRANSFORMS_SUPPORTED
#endif

#ifdef PNG_READ_TRANSFORMS_SUPPORTED
#  ifndef PNG_NO_READ_EXPAND
#    define PNG_READ_EXPAND_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SHIFT
#    define PNG_READ_SHIFT_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_PACK
#    define PNG_READ_PACK_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_BGR
#    define PNG_READ_BGR_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SWAP
#    define PNG_READ_SWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_PACKSWAP
#    define PNG_READ_PACKSWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_INVERT
#    define PNG_READ_INVERT_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_DITHER
#    define PNG_READ_DITHER_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_BACKGROUND
#    define PNG_READ_BACKGROUND_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_16_TO_8
#    define PNG_READ_16_TO_8_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_FILLER
#    define PNG_READ_FILLER_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_GAMMA
#    define PNG_READ_GAMMA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_GRAY_TO_RGB
#    define PNG_READ_GRAY_TO_RGB_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_SWAP_ALPHA
#    define PNG_READ_SWAP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_INVERT_ALPHA
#    define PNG_READ_INVERT_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_STRIP_ALPHA
#    define PNG_READ_STRIP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_USER_TRANSFORM
#    define PNG_READ_USER_TRANSFORM_SUPPORTED
#  endif
#  ifndef PNG_NO_READ_RGB_TO_GRAY
#    define PNG_READ_RGB_TO_GRAY_SUPPORTED
#  endif
#endif /* PNG_READ_TRANSFORMS_SUPPORTED */

#if !defined(PNG_NO_PROGRESSIVE_READ) && \
 !defined(PNG_PROGRESSIVE_READ_NOT_SUPPORTED)  /* if you don't do progressive */
#  define PNG_PROGRESSIVE_READ_SUPPORTED     /* reading.  This is not talking */
#endif                               /* about interlacing capability!  You'll */
              /* still have interlacing unless you change the following line: */

#define PNG_READ_INTERLACING_SUPPORTED /* required for PNG-compliant decoders */

#ifndef PNG_NO_READ_COMPOSITE_NODIV
#  ifndef PNG_NO_READ_COMPOSITED_NODIV  /* libpng-1.0.x misspelling */
#    define PNG_READ_COMPOSITE_NODIV_SUPPORTED   /* well tested on Intel, SGI */
#  endif
#endif

/* Deprecated, will be removed from version 2.0.0.
   Use PNG_MNG_FEATURES_SUPPORTED instead. */
#ifndef PNG_NO_READ_EMPTY_PLTE
#  define PNG_READ_EMPTY_PLTE_SUPPORTED
#endif

#endif /* PNG_READ_SUPPORTED */

#if defined(PNG_WRITE_SUPPORTED)

# if !defined(PNG_WRITE_TRANSFORMS_NOT_SUPPORTED) && \
    !defined(PNG_NO_WRITE_TRANSFORMS)
#  define PNG_WRITE_TRANSFORMS_SUPPORTED
#endif

#ifdef PNG_WRITE_TRANSFORMS_SUPPORTED
#  ifndef PNG_NO_WRITE_SHIFT
#    define PNG_WRITE_SHIFT_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_PACK
#    define PNG_WRITE_PACK_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_BGR
#    define PNG_WRITE_BGR_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_SWAP
#    define PNG_WRITE_SWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_PACKSWAP
#    define PNG_WRITE_PACKSWAP_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_INVERT
#    define PNG_WRITE_INVERT_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_FILLER
#    define PNG_WRITE_FILLER_SUPPORTED   /* same as WRITE_STRIP_ALPHA */
#  endif
#  ifndef PNG_NO_WRITE_SWAP_ALPHA
#    define PNG_WRITE_SWAP_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_INVERT_ALPHA
#    define PNG_WRITE_INVERT_ALPHA_SUPPORTED
#  endif
#  ifndef PNG_NO_WRITE_USER_TRANSFORM
#    define PNG_WRITE_USER_TRANSFORM_SUPPORTED
#  endif
#endif /* PNG_WRITE_TRANSFORMS_SUPPORTED */

#if defined(PNG_READ_USER_TRANSFORM_SUPPORTED) || \
    defined(PNG_WRITE_USER_TRANSFORM_SUPPORTED)
#  ifndef PNG_NO_USER_TRANSFORM_PTR
#    define PNG_USER_TRANSFORM_PTR_SUPPORTED
#  endif
#endif

#define PNG_WRITE_INTERLACING_SUPPORTED  /* not required for PNG-compliant
                                            encoders, but can cause trouble
                                            if left undefined */

#if !defined(PNG_NO_WRITE_WEIGHTED_FILTER) && \
     defined(PNG_FLOATING_POINT_SUPPORTED)
#  define PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
#endif

#ifndef PNG_1_0_X
#ifndef PNG_NO_ERROR_NUMBERS
#define PNG_ERROR_NUMBERS_SUPPORTED
#endif
#endif /* PNG_1_0_X */

#ifndef PNG_NO_WRITE_FLUSH
#  define PNG_WRITE_FLUSH_SUPPORTED
#endif

/* Deprecated, see PNG_MNG_FEATURES_SUPPORTED, above */
#ifndef PNG_NO_WRITE_EMPTY_PLTE
#  define PNG_WRITE_EMPTY_PLTE_SUPPORTED
#endif

#endif /* PNG_WRITE_SUPPORTED */

#ifndef PNG_NO_STDIO
#  define PNG_TIME_RFC1123_SUPPORTED
#endif

/* This adds extra functions in pngget.c for accessing data from the
 * info pointer (added in version 0.99)
 * png_get_image_width()
 * png_get_image_height()
 * png_get_bit_depth()
 * png_get_color_type()
 * png_get_compression_type()
 * png_get_filter_type()
 * png_get_interlace_type()
 * png_get_pixel_aspect_ratio()
 * png_get_pixels_per_meter()
 * png_get_x_offset_pixels()
 * png_get_y_offset_pixels()
 * png_get_x_offset_microns()
 * png_get_y_offset_microns()
 */
#if !defined(PNG_NO_EASY_ACCESS) && !defined(PNG_EASY_ACCESS_SUPPORTED)
#  define PNG_EASY_ACCESS_SUPPORTED
#endif

/* PNG_ASSEMBLER_CODE was enabled by default in version 1.2.0 
   even when PNG_USE_PNGVCRD or PNG_USE_PNGGCCRD is not defined */
#if defined(PNG_READ_SUPPORTED) && !defined(PNG_NO_ASSEMBLER_CODE)
#  ifndef PNG_ASSEMBLER_CODE_SUPPORTED
#    define PNG_ASSEMBLER_CODE_SUPPORTED
#  endif
#  if !defined(PNG_MMX_CODE_SUPPORTED) && !defined(PNG_NO_MMX_CODE)
#    define PNG_MMX_CODE_SUPPORTED
#  endif
#endif

/* If you are sure that you don't need thread safety and you are compiling
   with PNG_USE_PNGCCRD for an MMX application, you can define this for
   faster execution.  See pnggccrd.c.
#define PNG_THREAD_UNSAFE_OK
*/

#if !defined(PNG_1_0_X)
#if !defined(PNG_NO_USER_MEM) && !defined(PNG_USER_MEM_SUPPORTED)
#  define PNG_USER_MEM_SUPPORTED
#endif
#endif /* PNG_1_0_X */

/* These are currently experimental features, define them if you want */

/* very little testing */
/*
#ifdef PNG_READ_SUPPORTED
#  ifndef PNG_READ_16_TO_8_ACCURATE_SCALE_SUPPORTED
#    define PNG_READ_16_TO_8_ACCURATE_SCALE_SUPPORTED
#  endif
#endif
*/

/* This is only for PowerPC big-endian and 680x0 systems */
/* some testing */
/*
#ifdef PNG_READ_SUPPORTED
#  ifndef PNG_PNG_READ_BIG_ENDIAN_SUPPORTED
#    define PNG_READ_BIG_ENDIAN_SUPPORTED
#  endif
#endif
*/

/* Buggy compilers (e.g., gcc 2.7.2.2) need this */
/*
#define PNG_NO_POINTER_INDEXING
*/

/* These functions are turned off by default, as they will be phased out. */
/*
#define  PNG_USELESS_TESTS_SUPPORTED
#define  PNG_CORRECT_PALETTE_SUPPORTED
*/

/* Any chunks you are not interested in, you can undef here.  The
 * ones that allocate memory may be expecially important (hIST,
 * tEXt, zTXt, tRNS, pCAL).  Others will just save time and make png_info
 * a bit smaller.
 */

#if defined(PNG_READ_SUPPORTED) && \
    !defined(PNG_READ_ANCILLARY_CHUNKS_NOT_SUPPORTED) && \
    !defined(PNG_NO_READ_ANCILLARY_CHUNKS)
#  define PNG_READ_ANCILLARY_CHUNKS_SUPPORTED
#endif

#if defined(PNG_WRITE_SUPPORTED) && \
    !defined(PNG_WRITE_ANCILLARY_CHUNKS_NOT_SUPPORTED) && \
    !defined(PNG_NO_WRITE_ANCILLARY_CHUNKS)
#  define PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED
#endif

#ifdef PNG_READ_ANCILLARY_CHUNKS_SUPPORTED

#ifdef PNG_NO_READ_TEXT
#  define PNG_NO_READ_iTXt
#  define PNG_NO_READ_tEXt
#  define PNG_NO_READ_zTXt
#endif
#ifndef PNG_NO_READ_bKGD
#  define PNG_READ_bKGD_SUPPORTED
#  define PNG_bKGD_SUPPORTED
#endif
#ifndef PNG_NO_READ_cHRM
#  define PNG_READ_cHRM_SUPPORTED
#  define PNG_cHRM_SUPPORTED
#endif
#ifndef PNG_NO_READ_gAMA
#  define PNG_READ_gAMA_SUPPORTED
#  define PNG_gAMA_SUPPORTED
#endif
#ifndef PNG_NO_READ_hIST
#  define PNG_READ_hIST_SUPPORTED
#  define PNG_hIST_SUPPORTED
#endif
#ifndef PNG_NO_READ_iCCP
#  define PNG_READ_iCCP_SUPPORTED
#  define PNG_iCCP_SUPPORTED
#endif
#ifndef PNG_NO_READ_iTXt
#  ifndef PNG_READ_iTXt_SUPPORTED
#    define PNG_READ_iTXt_SUPPORTED
#  endif
#  ifndef PNG_iTXt_SUPPORTED
#    define PNG_iTXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_READ_oFFs
#  define PNG_READ_oFFs_SUPPORTED
#  define PNG_oFFs_SUPPORTED
#endif
#ifndef PNG_NO_READ_pCAL
#  define PNG_READ_pCAL_SUPPORTED
#  define PNG_pCAL_SUPPORTED
#endif
#ifndef PNG_NO_READ_sCAL
#  define PNG_READ_sCAL_SUPPORTED
#  define PNG_sCAL_SUPPORTED
#endif
#ifndef PNG_NO_READ_pHYs
#  define PNG_READ_pHYs_SUPPORTED
#  define PNG_pHYs_SUPPORTED
#endif
#ifndef PNG_NO_READ_sBIT
#  define PNG_READ_sBIT_SUPPORTED
#  define PNG_sBIT_SUPPORTED
#endif
#ifndef PNG_NO_READ_sPLT
#  define PNG_READ_sPLT_SUPPORTED
#  define PNG_sPLT_SUPPORTED
#endif
#ifndef PNG_NO_READ_sRGB
#  define PNG_READ_sRGB_SUPPORTED
#  define PNG_sRGB_SUPPORTED
#endif
#ifndef PNG_NO_READ_tEXt
#  define PNG_READ_tEXt_SUPPORTED
#  define PNG_tEXt_SUPPORTED
#endif
#ifndef PNG_NO_READ_tIME
#  define PNG_READ_tIME_SUPPORTED
#  define PNG_tIME_SUPPORTED
#endif
#ifndef PNG_NO_READ_tRNS
#  define PNG_READ_tRNS_SUPPORTED
#  define PNG_tRNS_SUPPORTED
#endif
#ifndef PNG_NO_READ_zTXt
#  define PNG_READ_zTXt_SUPPORTED
#  define PNG_zTXt_SUPPORTED
#endif
#ifndef PNG_NO_READ_UNKNOWN_CHUNKS
#  define PNG_READ_UNKNOWN_CHUNKS_SUPPORTED
#  ifndef PNG_UNKNOWN_CHUNKS_SUPPORTED
#    define PNG_UNKNOWN_CHUNKS_SUPPORTED
#  endif
#  ifndef PNG_NO_HANDLE_AS_UNKNOWN
#    define PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#  endif
#endif
#if !defined(PNG_NO_READ_USER_CHUNKS) && \
     defined(PNG_READ_UNKNOWN_CHUNKS_SUPPORTED)
#  define PNG_READ_USER_CHUNKS_SUPPORTED
#  define PNG_USER_CHUNKS_SUPPORTED
#  ifdef PNG_NO_READ_UNKNOWN_CHUNKS
#    undef PNG_NO_READ_UNKNOWN_CHUNKS
#  endif
#  ifdef PNG_NO_HANDLE_AS_UNKNOWN
#    undef PNG_NO_HANDLE_AS_UNKNOWN
#  endif
#endif
#ifndef PNG_NO_READ_OPT_PLTE
#  define PNG_READ_OPT_PLTE_SUPPORTED /* only affects support of the */
#endif                      /* optional PLTE chunk in RGB and RGBA images */
#if defined(PNG_READ_iTXt_SUPPORTED) || defined(PNG_READ_tEXt_SUPPORTED) || \
    defined(PNG_READ_zTXt_SUPPORTED)
#  define PNG_READ_TEXT_SUPPORTED
#  define PNG_TEXT_SUPPORTED
#endif

#endif /* PNG_READ_ANCILLARY_CHUNKS_SUPPORTED */

#ifdef PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED

#ifdef PNG_NO_WRITE_TEXT
#  define PNG_NO_WRITE_iTXt
#  define PNG_NO_WRITE_tEXt
#  define PNG_NO_WRITE_zTXt
#endif
#ifndef PNG_NO_WRITE_bKGD
#  define PNG_WRITE_bKGD_SUPPORTED
#  ifndef PNG_bKGD_SUPPORTED
#    define PNG_bKGD_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_cHRM
#  define PNG_WRITE_cHRM_SUPPORTED
#  ifndef PNG_cHRM_SUPPORTED
#    define PNG_cHRM_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_gAMA
#  define PNG_WRITE_gAMA_SUPPORTED
#  ifndef PNG_gAMA_SUPPORTED
#    define PNG_gAMA_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_hIST
#  define PNG_WRITE_hIST_SUPPORTED
#  ifndef PNG_hIST_SUPPORTED
#    define PNG_hIST_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_iCCP
#  define PNG_WRITE_iCCP_SUPPORTED
#  ifndef PNG_iCCP_SUPPORTED
#    define PNG_iCCP_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_iTXt
#  ifndef PNG_WRITE_iTXt_SUPPORTED
#    define PNG_WRITE_iTXt_SUPPORTED
#  endif
#  ifndef PNG_iTXt_SUPPORTED
#    define PNG_iTXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_oFFs
#  define PNG_WRITE_oFFs_SUPPORTED
#  ifndef PNG_oFFs_SUPPORTED
#    define PNG_oFFs_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_pCAL
#  define PNG_WRITE_pCAL_SUPPORTED
#  ifndef PNG_pCAL_SUPPORTED
#    define PNG_pCAL_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sCAL
#  define PNG_WRITE_sCAL_SUPPORTED
#  ifndef PNG_sCAL_SUPPORTED
#    define PNG_sCAL_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_pHYs
#  define PNG_WRITE_pHYs_SUPPORTED
#  ifndef PNG_pHYs_SUPPORTED
#    define PNG_pHYs_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sBIT
#  define PNG_WRITE_sBIT_SUPPORTED
#  ifndef PNG_sBIT_SUPPORTED
#    define PNG_sBIT_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sPLT
#  define PNG_WRITE_sPLT_SUPPORTED
#  ifndef PNG_sPLT_SUPPORTED
#    define PNG_sPLT_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_sRGB
#  define PNG_WRITE_sRGB_SUPPORTED
#  ifndef PNG_sRGB_SUPPORTED
#    define PNG_sRGB_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tEXt
#  define PNG_WRITE_tEXt_SUPPORTED
#  ifndef PNG_tEXt_SUPPORTED
#    define PNG_tEXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tIME
#  define PNG_WRITE_tIME_SUPPORTED
#  ifndef PNG_tIME_SUPPORTED
#    define PNG_tIME_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_tRNS
#  define PNG_WRITE_tRNS_SUPPORTED
#  ifndef PNG_tRNS_SUPPORTED
#    define PNG_tRNS_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_zTXt
#  define PNG_WRITE_zTXt_SUPPORTED
#  ifndef PNG_zTXt_SUPPORTED
#    define PNG_zTXt_SUPPORTED
#  endif
#endif
#ifndef PNG_NO_WRITE_UNKNOWN_CHUNKS
#  define PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
#  ifndef PNG_UNKNOWN_CHUNKS_SUPPORTED
#    define PNG_UNKNOWN_CHUNKS_SUPPORTED
#  endif
#  ifndef PNG_NO_HANDLE_AS_UNKNOWN
#     ifndef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#       define PNG_HANDLE_AS_UNKNOWN_SUPPORTED
#     endif
#  endif
#endif
#if defined(PNG_WRITE_iTXt_SUPPORTED) || defined(PNG_WRITE_tEXt_SUPPORTED) || \
    defined(PNG_WRITE_zTXt_SUPPORTED)
#  define PNG_WRITE_TEXT_SUPPORTED
#  ifndef PNG_TEXT_SUPPORTED
#    define PNG_TEXT_SUPPORTED
#  endif
#endif

#endif /* PNG_WRITE_ANCILLARY_CHUNKS_SUPPORTED */

/* Turn this off to disable png_read_png() and
 * png_write_png() and leave the row_pointers member
 * out of the info structure.
 */
#ifndef PNG_NO_INFO_IMAGE
#  define PNG_INFO_IMAGE_SUPPORTED
#endif

/* need the time information for reading tIME chunks */
#if defined(PNG_tIME_SUPPORTED)
#  if !defined(_WIN32_WCE)
     /* "time.h" functions are not supported on WindowsCE */
#    include <time.h>
#  endif
#endif

/* Some typedefs to get us started.  These should be safe on most of the
 * common platforms.  The typedefs should be at least as large as the
 * numbers suggest (a png_uint_32 must be at least 32 bits long), but they
 * don't have to be exactly that size.  Some compilers dislike passing
 * unsigned shorts as function parameters, so you may be better off using
 * unsigned int for png_uint_16.  Likewise, for 64-bit systems, you may
 * want to have unsigned int for png_uint_32 instead of unsigned long.
 */

typedef unsigned long png_uint_32;
typedef long png_int_32;
typedef unsigned short png_uint_16;
typedef short png_int_16;
typedef unsigned char png_byte;

/* This is usually size_t.  It is typedef'ed just in case you need it to
   change (I'm not sure if you will or not, so I thought I'd be safe) */
typedef size_t png_size_t;

/* The following is needed for medium model support.  It cannot be in the
 * PNG_INTERNAL section.  Needs modification for other compilers besides
 * MSC.  Model independent support declares all arrays and pointers to be
 * large using the far keyword.  The zlib version used must also support
 * model independent data.  As of version zlib 1.0.4, the necessary changes
 * have been made in zlib.  The USE_FAR_KEYWORD define triggers other
 * changes that are needed. (Tim Wegner)
 */

/* Separate compiler dependencies (problem here is that zlib.h always
   defines FAR. (SJT) */
#ifdef __BORLANDC__
#  if defined(__LARGE__) || defined(__HUGE__) || defined(__COMPACT__)
#    define LDATA 1
#  else
#    define LDATA 0
#  endif
   /* GRR:  why is Cygwin in here?  Cygwin is not Borland C... */
#  if !defined(__WIN32__) && !defined(__FLAT__) && !defined(__CYGWIN__)
#    define PNG_MAX_MALLOC_64K
#    if (LDATA != 1)
#      ifndef FAR
#        define FAR __far
#      endif
#      define USE_FAR_KEYWORD
#    endif   /* LDATA != 1 */
     /* Possibly useful for moving data out of default segment.
      * Uncomment it if you want. Could also define FARDATA as
      * const if your compiler supports it. (SJT)
#    define FARDATA FAR
      */
#  endif  /* __WIN32__, __FLAT__, __CYGWIN__ */
#endif   /* __BORLANDC__ */


/* Suggest testing for specific compiler first before testing for
 * FAR.  The Watcom compiler defines both __MEDIUM__ and M_I86MM,
 * making reliance oncertain keywords suspect. (SJT)
 */

/* MSC Medium model */
#if defined(FAR)
#  if defined(M_I86MM)
#    define USE_FAR_KEYWORD
#    define FARDATA FAR
#    include <dos.h>
#  endif
#endif

/* SJT: default case */
#ifndef FAR
#  define FAR
#endif

/* At this point FAR is always defined */
#ifndef FARDATA
#  define FARDATA
#endif

/* Typedef for floating-point numbers that are converted
   to fixed-point with a multiple of 100,000, e.g., int_gamma */
typedef png_int_32 png_fixed_point;

/* Add typedefs for pointers */
typedef void            FAR * png_voidp;
typedef png_byte        FAR * png_bytep;
typedef png_uint_32     FAR * png_uint_32p;
typedef png_int_32      FAR * png_int_32p;
typedef png_uint_16     FAR * png_uint_16p;
typedef png_int_16      FAR * png_int_16p;
typedef PNG_CONST char  FAR * png_const_charp;
typedef char            FAR * png_charp;
typedef png_fixed_point FAR * png_fixed_point_p;

#ifndef PNG_NO_STDIO
#if defined(_WIN32_WCE)
typedef HANDLE                png_FILE_p;
#else
typedef FILE                * png_FILE_p;
#endif
#endif

#ifdef PNG_FLOATING_POINT_SUPPORTED
typedef double          FAR * png_doublep;
#endif

/* Pointers to pointers; i.e. arrays */
typedef png_byte        FAR * FAR * png_bytepp;
typedef png_uint_32     FAR * FAR * png_uint_32pp;
typedef png_int_32      FAR * FAR * png_int_32pp;
typedef png_uint_16     FAR * FAR * png_uint_16pp;
typedef png_int_16      FAR * FAR * png_int_16pp;
typedef PNG_CONST char  FAR * FAR * png_const_charpp;
typedef char            FAR * FAR * png_charpp;
typedef png_fixed_point FAR * FAR * png_fixed_point_pp;
#ifdef PNG_FLOATING_POINT_SUPPORTED
typedef double          FAR * FAR * png_doublepp;
#endif

/* Pointers to pointers to pointers; i.e., pointer to array */
typedef char            FAR * FAR * FAR * png_charppp;

/* libpng typedefs for types in zlib. If zlib changes
 * or another compression library is used, then change these.
 * Eliminates need to change all the source files.
 */
typedef charf *         png_zcharp;
typedef charf * FAR *   png_zcharpp;
typedef z_stream FAR *  png_zstreamp;

/*
 * Define PNG_BUILD_DLL if the module being built is a Windows
 * LIBPNG DLL.
 *
 * Define PNG_USE_DLL if you want to *link* to the Windows LIBPNG DLL.
 * It is equivalent to Microsoft predefined macro _DLL that is
 * automatically defined when you compile using the share
 * version of the CRT (C Run-Time library)
 *
 * The cygwin mods make this behavior a little different:
 * Define PNG_BUILD_DLL if you are building a dll for use with cygwin
 * Define PNG_STATIC if you are building a static library for use with cygwin,
 *   -or- if you are building an application that you want to link to the
 *   static library.
 * PNG_USE_DLL is defined by default (no user action needed) unless one of
 *   the other flags is defined.
 */

#if !defined(PNG_DLL) && (defined(PNG_BUILD_DLL) || defined(PNG_USE_DLL))
#  define PNG_DLL
#endif
/* If CYGWIN, then disallow GLOBAL ARRAYS unless building a static lib.
 * When building a static lib, default to no GLOBAL ARRAYS, but allow
 * command-line override
 */
#if defined(__CYGWIN__)
#  if !defined(PNG_STATIC)
#    if defined(PNG_USE_GLOBAL_ARRAYS)
#      undef PNG_USE_GLOBAL_ARRAYS
#    endif
#    if !defined(PNG_USE_LOCAL_ARRAYS)
#      define PNG_USE_LOCAL_ARRAYS
#    endif
#  else
#    if defined(PNG_USE_LOCAL_ARRAYS) || defined(PNG_NO_GLOBAL_ARRAYS)
#      if defined(PNG_USE_GLOBAL_ARRAYS)
#        undef PNG_USE_GLOBAL_ARRAYS
#      endif
#    endif
#  endif
#  if !defined(PNG_USE_LOCAL_ARRAYS) && !defined(PNG_USE_GLOBAL_ARRAYS)
#    define PNG_USE_LOCAL_ARRAYS
#  endif
#endif

/* Do not use global arrays (helps with building DLL's)
 * They are no longer used in libpng itself, since version 1.0.5c,
 * but might be required for some pre-1.0.5c applications.
 */
#if !defined(PNG_USE_LOCAL_ARRAYS) && !defined(PNG_USE_GLOBAL_ARRAYS)
#  if defined(PNG_NO_GLOBAL_ARRAYS) || (defined(__GNUC__) && defined(PNG_DLL))
#    define PNG_USE_LOCAL_ARRAYS
#  else
#    define PNG_USE_GLOBAL_ARRAYS
#  endif
#endif

#if defined(__CYGWIN__)
#  undef PNGAPI
#  define PNGAPI __cdecl
#  undef PNG_IMPEXP
#  define PNG_IMPEXP
#endif  

/* If you define PNGAPI, e.g., with compiler option "-DPNGAPI=__stdcall",
 * you may get warnings regarding the linkage of png_zalloc and png_zfree.
 * Don't ignore those warnings; you must also reset the default calling
 * convention in your compiler to match your PNGAPI, and you must build
 * zlib and your applications the same way you build libpng.
 */

#ifndef PNGAPI

#if defined(__MINGW32__) && !defined(PNG_MODULEDEF)
#  ifndef PNG_NO_MODULEDEF
#    define PNG_NO_MODULEDEF
#  endif
#endif

#if !defined(PNG_IMPEXP) && defined(PNG_BUILD_DLL) && !defined(PNG_NO_MODULEDEF)
#  define PNG_IMPEXP
#endif

#if defined(PNG_DLL) || defined(_DLL) || defined(__DLL__ ) || \
    (( defined(_Windows) || defined(_WINDOWS) || \
       defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_XBOX)))

#  if defined(__GNUC__) || (defined (_MSC_VER) && (_MSC_VER >= 800))
#    define PNGAPI __cdecl
#  else
#    define PNGAPI _cdecl
#  endif

#  if !defined(PNG_IMPEXP) && (!defined(PNG_DLL) || \
       0 /* WINCOMPILER_WITH_NO_SUPPORT_FOR_DECLIMPEXP */)
#     define PNG_IMPEXP
#  endif

#  if !defined(PNG_IMPEXP)

#     define PNG_EXPORT_TYPE1(type,symbol)  PNG_IMPEXP type PNGAPI symbol
#     define PNG_EXPORT_TYPE2(type,symbol)  type PNG_IMPEXP PNGAPI symbol

      /* Borland/Microsoft */
#     if defined(_MSC_VER) || defined(__BORLANDC__)
#        if (_MSC_VER >= 800) || (__BORLANDC__ >= 0x500)
#           define PNG_EXPORT PNG_EXPORT_TYPE1
#        else
#           define PNG_EXPORT PNG_EXPORT_TYPE2
#           if defined(PNG_BUILD_DLL)
#              define PNG_IMPEXP __export
#           else
#              define PNG_IMPEXP /*__import */ /* doesn't exist AFAIK in
                                                 VC++ */
#           endif                             /* Exists in Borland C++ for
                                                 C++ classes (== huge) */
#        endif
#     endif

#     if !defined(PNG_IMPEXP)
#        if defined(PNG_BUILD_DLL)
#           define PNG_IMPEXP __declspec(dllexport)
#        else
#           define PNG_IMPEXP __declspec(dllimport)
#        endif
#     endif
#  endif  /* PNG_IMPEXP */
#else /* !(DLL || non-cygwin WINDOWS) */
#    if (defined(__IBMC__) || defined(IBMCPP__)) && defined(__OS2__)
#      define PNGAPI _System
#      define PNG_IMPEXP
#    else
#      if 0 /* ... other platforms, with other meanings */
#      else
#        define PNGAPI
#        define PNG_IMPEXP
#      endif
#    endif
#endif
#endif

#ifndef PNGAPI
#  define PNGAPI
#endif
#ifndef PNG_IMPEXP
#  define PNG_IMPEXP
#endif

#ifndef PNG_EXPORT
#  define PNG_EXPORT(type,symbol) PNG_IMPEXP type PNGAPI symbol
#endif

#ifdef PNG_USE_GLOBAL_ARRAYS
#  ifndef PNG_EXPORT_VAR
#    define PNG_EXPORT_VAR(type) extern PNG_IMPEXP type
#  endif
#endif

/* User may want to use these so they are not in PNG_INTERNAL. Any library
 * functions that are passed far data must be model independent.
 */

#ifndef PNG_ABORT
#  define PNG_ABORT() abort()
#endif

#ifdef PNG_SETJMP_SUPPORTED
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#else
#  define png_jmpbuf(png_ptr) \
   (LIBPNG_WAS_COMPILED_WITH__PNG_SETJMP_NOT_SUPPORTED)
#endif

#if defined(USE_FAR_KEYWORD)  /* memory model independent fns */
/* use this to make far-to-near assignments */
#  define CHECK   1
#  define NOCHECK 0
#  define CVT_PTR(ptr) (png_far_to_near(png_ptr,ptr,CHECK))
#  define CVT_PTR_NOCHECK(ptr) (png_far_to_near(png_ptr,ptr,NOCHECK))
#  define png_strcpy _fstrcpy
#  define png_strlen _fstrlen
#  define png_memcmp _fmemcmp      /* SJT: added */
#  define png_memcpy _fmemcpy
#  define png_memset _fmemset
#else /* use the usual functions */
#  define CVT_PTR(ptr)         (ptr)
#  define CVT_PTR_NOCHECK(ptr) (ptr)
#  define png_strcpy strcpy
#  define png_strlen strlen
#  define png_memcmp memcmp     /* SJT: added */
#  define png_memcpy memcpy
#  define png_memset memset
#endif
/* End of memory model independent support */

/* Just a little check that someone hasn't tried to define something
 * contradictory.
 */
#if (PNG_ZBUF_SIZE > 65536) && defined(PNG_MAX_MALLOC_64K)
#  undef PNG_ZBUF_SIZE
#  define PNG_ZBUF_SIZE 65536
#endif

#ifdef PNG_READ_SUPPORTED
/* Prior to libpng-1.0.9, this block was in pngasmrd.h */
#if defined(PNG_INTERNAL)

/* These are the default thresholds before the MMX code kicks in; if either
 * rowbytes or bitdepth is below the threshold, plain C code is used.  These
 * can be overridden at runtime via the png_set_mmx_thresholds() call in
 * libpng 1.2.0 and later.  The values below were chosen by Intel.
 */

#ifndef PNG_MMX_ROWBYTES_THRESHOLD_DEFAULT
#  define PNG_MMX_ROWBYTES_THRESHOLD_DEFAULT  128  /*  >=  */
#endif
#ifndef PNG_MMX_BITDEPTH_THRESHOLD_DEFAULT
#  define PNG_MMX_BITDEPTH_THRESHOLD_DEFAULT  9    /*  >=  */   
#endif

/* Set this in the makefile for VC++ on Pentium, not here. */
/* Platform must be Pentium.  Makefile must assemble and load pngvcrd.c .
 * MMX will be detected at run time and used if present.
 */
#ifdef PNG_USE_PNGVCRD
#  define PNG_HAVE_ASSEMBLER_COMBINE_ROW
#  define PNG_HAVE_ASSEMBLER_READ_INTERLACE
#  define PNG_HAVE_ASSEMBLER_READ_FILTER_ROW
#endif

/* Set this in the makefile for gcc/as on Pentium, not here. */
/* Platform must be Pentium.  Makefile must assemble and load pnggccrd.c .
 * MMX will be detected at run time and used if present.
 */
#ifdef PNG_USE_PNGGCCRD
#  define PNG_HAVE_ASSEMBLER_COMBINE_ROW
#  define PNG_HAVE_ASSEMBLER_READ_INTERLACE
#  define PNG_HAVE_ASSEMBLER_READ_FILTER_ROW
#endif
/* - see pnggccrd.c for info about what is currently enabled */

#endif /* PNG_INTERNAL */
#endif /* PNG_READ_SUPPORTED */

#endif /* PNGCONF_H */

