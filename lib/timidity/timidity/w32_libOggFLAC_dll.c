
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef AU_OGGFLAC_DLL

#include <windows.h>
#define FLAC__EXPORT_H  /* don't include "OggFLAC/export.h" */
#define FLAC_API
#define OggFLAC__EXPORT_H  /* don't include "FLAC/export.h" */
#define OggFLAC_API
#include "FLAC/all.h"
#include "OggFLAC/all.h"
#include <windows.h>
#define FLAC__EXPORT_H  /* don't include "OggFLAC/export.h" */
#define FLAC_API
#define OggFLAC__EXPORT_H  /* don't include "FLAC/export.h" */
#define OggFLAC_API
#include "FLAC/all.h"
#include "OggFLAC/all.h"
/***************************************************************
  dynamic load library
  name : libOggFLAC
***************************************************************/


#include "w32_libOggFLAC_dll.h"


/***************************************************************
   for c source
 ***************************************************************/


#if defined(__cplusplus)
extern "C" {
#endif



libOggFLAC_dll_t *load_libOggFLAC_dll ( char *path )
{
  int err = 0;
  libOggFLAC_dll_t *dll = (libOggFLAC_dll_t *) malloc ( sizeof(libOggFLAC_dll_t) );
  if ( dll == NULL ) return NULL;
  dll->__h_dll = LoadLibrary ( path );
  if ( dll->__h_dll == NULL ) { free ( dll ); return NULL; };

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_new
  dll->OggFLAC__stream_encoder_new = (libOggFLAC_func_OggFLAC__stream_encoder_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_new" );
  if ( dll->OggFLAC__stream_encoder_new == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_delete
  dll->OggFLAC__stream_encoder_delete = (libOggFLAC_func_OggFLAC__stream_encoder_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_delete" );
  if ( dll->OggFLAC__stream_encoder_delete == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_serial_number
  dll->OggFLAC__stream_encoder_set_serial_number = (libOggFLAC_func_OggFLAC__stream_encoder_set_serial_number_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_serial_number" );
  if ( dll->OggFLAC__stream_encoder_set_serial_number == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_verify
  dll->OggFLAC__stream_encoder_set_verify = (libOggFLAC_func_OggFLAC__stream_encoder_set_verify_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_verify" );
  if ( dll->OggFLAC__stream_encoder_set_verify == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_streamable_subset
  dll->OggFLAC__stream_encoder_set_streamable_subset = (libOggFLAC_func_OggFLAC__stream_encoder_set_streamable_subset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_streamable_subset" );
  if ( dll->OggFLAC__stream_encoder_set_streamable_subset == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_do_mid_side_stereo
  dll->OggFLAC__stream_encoder_set_do_mid_side_stereo = (libOggFLAC_func_OggFLAC__stream_encoder_set_do_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_do_mid_side_stereo" );
  if ( dll->OggFLAC__stream_encoder_set_do_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_loose_mid_side_stereo
  dll->OggFLAC__stream_encoder_set_loose_mid_side_stereo = (libOggFLAC_func_OggFLAC__stream_encoder_set_loose_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_loose_mid_side_stereo" );
  if ( dll->OggFLAC__stream_encoder_set_loose_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_channels
  dll->OggFLAC__stream_encoder_set_channels = (libOggFLAC_func_OggFLAC__stream_encoder_set_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_channels" );
  if ( dll->OggFLAC__stream_encoder_set_channels == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_bits_per_sample
  dll->OggFLAC__stream_encoder_set_bits_per_sample = (libOggFLAC_func_OggFLAC__stream_encoder_set_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_bits_per_sample" );
  if ( dll->OggFLAC__stream_encoder_set_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_sample_rate
  dll->OggFLAC__stream_encoder_set_sample_rate = (libOggFLAC_func_OggFLAC__stream_encoder_set_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_sample_rate" );
  if ( dll->OggFLAC__stream_encoder_set_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_blocksize
  dll->OggFLAC__stream_encoder_set_blocksize = (libOggFLAC_func_OggFLAC__stream_encoder_set_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_blocksize" );
  if ( dll->OggFLAC__stream_encoder_set_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_max_lpc_order
  dll->OggFLAC__stream_encoder_set_max_lpc_order = (libOggFLAC_func_OggFLAC__stream_encoder_set_max_lpc_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_max_lpc_order" );
  if ( dll->OggFLAC__stream_encoder_set_max_lpc_order == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_qlp_coeff_precision
  dll->OggFLAC__stream_encoder_set_qlp_coeff_precision = (libOggFLAC_func_OggFLAC__stream_encoder_set_qlp_coeff_precision_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_qlp_coeff_precision" );
  if ( dll->OggFLAC__stream_encoder_set_qlp_coeff_precision == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search
  dll->OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search = (libOggFLAC_func_OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search" );
  if ( dll->OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_do_escape_coding
  dll->OggFLAC__stream_encoder_set_do_escape_coding = (libOggFLAC_func_OggFLAC__stream_encoder_set_do_escape_coding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_do_escape_coding" );
  if ( dll->OggFLAC__stream_encoder_set_do_escape_coding == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_do_exhaustive_model_search
  dll->OggFLAC__stream_encoder_set_do_exhaustive_model_search = (libOggFLAC_func_OggFLAC__stream_encoder_set_do_exhaustive_model_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_do_exhaustive_model_search" );
  if ( dll->OggFLAC__stream_encoder_set_do_exhaustive_model_search == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_min_residual_partition_order
  dll->OggFLAC__stream_encoder_set_min_residual_partition_order = (libOggFLAC_func_OggFLAC__stream_encoder_set_min_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_min_residual_partition_order" );
  if ( dll->OggFLAC__stream_encoder_set_min_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_max_residual_partition_order
  dll->OggFLAC__stream_encoder_set_max_residual_partition_order = (libOggFLAC_func_OggFLAC__stream_encoder_set_max_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_max_residual_partition_order" );
  if ( dll->OggFLAC__stream_encoder_set_max_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_rice_parameter_search_dist
  dll->OggFLAC__stream_encoder_set_rice_parameter_search_dist = (libOggFLAC_func_OggFLAC__stream_encoder_set_rice_parameter_search_dist_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_rice_parameter_search_dist" );
  if ( dll->OggFLAC__stream_encoder_set_rice_parameter_search_dist == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_total_samples_estimate
  dll->OggFLAC__stream_encoder_set_total_samples_estimate = (libOggFLAC_func_OggFLAC__stream_encoder_set_total_samples_estimate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_total_samples_estimate" );
  if ( dll->OggFLAC__stream_encoder_set_total_samples_estimate == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_metadata
  dll->OggFLAC__stream_encoder_set_metadata = (libOggFLAC_func_OggFLAC__stream_encoder_set_metadata_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_metadata" );
  if ( dll->OggFLAC__stream_encoder_set_metadata == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_write_callback
  dll->OggFLAC__stream_encoder_set_write_callback = (libOggFLAC_func_OggFLAC__stream_encoder_set_write_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_write_callback" );
  if ( dll->OggFLAC__stream_encoder_set_write_callback == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_client_data
  dll->OggFLAC__stream_encoder_set_client_data = (libOggFLAC_func_OggFLAC__stream_encoder_set_client_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_set_client_data" );
  if ( dll->OggFLAC__stream_encoder_set_client_data == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_state
  dll->OggFLAC__stream_encoder_get_state = (libOggFLAC_func_OggFLAC__stream_encoder_get_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_state" );
  if ( dll->OggFLAC__stream_encoder_get_state == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_FLAC_stream_encoder_state
  dll->OggFLAC__stream_encoder_get_FLAC_stream_encoder_state = (libOggFLAC_func_OggFLAC__stream_encoder_get_FLAC_stream_encoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_FLAC_stream_encoder_state" );
  if ( dll->OggFLAC__stream_encoder_get_FLAC_stream_encoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_verify_decoder_state
  dll->OggFLAC__stream_encoder_get_verify_decoder_state = (libOggFLAC_func_OggFLAC__stream_encoder_get_verify_decoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_verify_decoder_state" );
  if ( dll->OggFLAC__stream_encoder_get_verify_decoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_verify_decoder_error_stats
  dll->OggFLAC__stream_encoder_get_verify_decoder_error_stats = (libOggFLAC_func_OggFLAC__stream_encoder_get_verify_decoder_error_stats_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_verify_decoder_error_stats" );
  if ( dll->OggFLAC__stream_encoder_get_verify_decoder_error_stats == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_verify
  dll->OggFLAC__stream_encoder_get_verify = (libOggFLAC_func_OggFLAC__stream_encoder_get_verify_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_verify" );
  if ( dll->OggFLAC__stream_encoder_get_verify == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_streamable_subset
  dll->OggFLAC__stream_encoder_get_streamable_subset = (libOggFLAC_func_OggFLAC__stream_encoder_get_streamable_subset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_streamable_subset" );
  if ( dll->OggFLAC__stream_encoder_get_streamable_subset == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_do_mid_side_stereo
  dll->OggFLAC__stream_encoder_get_do_mid_side_stereo = (libOggFLAC_func_OggFLAC__stream_encoder_get_do_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_do_mid_side_stereo" );
  if ( dll->OggFLAC__stream_encoder_get_do_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_loose_mid_side_stereo
  dll->OggFLAC__stream_encoder_get_loose_mid_side_stereo = (libOggFLAC_func_OggFLAC__stream_encoder_get_loose_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_loose_mid_side_stereo" );
  if ( dll->OggFLAC__stream_encoder_get_loose_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_channels
  dll->OggFLAC__stream_encoder_get_channels = (libOggFLAC_func_OggFLAC__stream_encoder_get_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_channels" );
  if ( dll->OggFLAC__stream_encoder_get_channels == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_bits_per_sample
  dll->OggFLAC__stream_encoder_get_bits_per_sample = (libOggFLAC_func_OggFLAC__stream_encoder_get_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_bits_per_sample" );
  if ( dll->OggFLAC__stream_encoder_get_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_sample_rate
  dll->OggFLAC__stream_encoder_get_sample_rate = (libOggFLAC_func_OggFLAC__stream_encoder_get_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_sample_rate" );
  if ( dll->OggFLAC__stream_encoder_get_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_blocksize
  dll->OggFLAC__stream_encoder_get_blocksize = (libOggFLAC_func_OggFLAC__stream_encoder_get_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_blocksize" );
  if ( dll->OggFLAC__stream_encoder_get_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_max_lpc_order
  dll->OggFLAC__stream_encoder_get_max_lpc_order = (libOggFLAC_func_OggFLAC__stream_encoder_get_max_lpc_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_max_lpc_order" );
  if ( dll->OggFLAC__stream_encoder_get_max_lpc_order == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_qlp_coeff_precision
  dll->OggFLAC__stream_encoder_get_qlp_coeff_precision = (libOggFLAC_func_OggFLAC__stream_encoder_get_qlp_coeff_precision_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_qlp_coeff_precision" );
  if ( dll->OggFLAC__stream_encoder_get_qlp_coeff_precision == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search
  dll->OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search = (libOggFLAC_func_OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search" );
  if ( dll->OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_do_escape_coding
  dll->OggFLAC__stream_encoder_get_do_escape_coding = (libOggFLAC_func_OggFLAC__stream_encoder_get_do_escape_coding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_do_escape_coding" );
  if ( dll->OggFLAC__stream_encoder_get_do_escape_coding == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_do_exhaustive_model_search
  dll->OggFLAC__stream_encoder_get_do_exhaustive_model_search = (libOggFLAC_func_OggFLAC__stream_encoder_get_do_exhaustive_model_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_do_exhaustive_model_search" );
  if ( dll->OggFLAC__stream_encoder_get_do_exhaustive_model_search == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_min_residual_partition_order
  dll->OggFLAC__stream_encoder_get_min_residual_partition_order = (libOggFLAC_func_OggFLAC__stream_encoder_get_min_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_min_residual_partition_order" );
  if ( dll->OggFLAC__stream_encoder_get_min_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_max_residual_partition_order
  dll->OggFLAC__stream_encoder_get_max_residual_partition_order = (libOggFLAC_func_OggFLAC__stream_encoder_get_max_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_max_residual_partition_order" );
  if ( dll->OggFLAC__stream_encoder_get_max_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_rice_parameter_search_dist
  dll->OggFLAC__stream_encoder_get_rice_parameter_search_dist = (libOggFLAC_func_OggFLAC__stream_encoder_get_rice_parameter_search_dist_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_rice_parameter_search_dist" );
  if ( dll->OggFLAC__stream_encoder_get_rice_parameter_search_dist == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_total_samples_estimate
  dll->OggFLAC__stream_encoder_get_total_samples_estimate = (libOggFLAC_func_OggFLAC__stream_encoder_get_total_samples_estimate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_get_total_samples_estimate" );
  if ( dll->OggFLAC__stream_encoder_get_total_samples_estimate == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_init
  dll->OggFLAC__stream_encoder_init = (libOggFLAC_func_OggFLAC__stream_encoder_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_init" );
  if ( dll->OggFLAC__stream_encoder_init == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_finish
  dll->OggFLAC__stream_encoder_finish = (libOggFLAC_func_OggFLAC__stream_encoder_finish_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_finish" );
  if ( dll->OggFLAC__stream_encoder_finish == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_process
  dll->OggFLAC__stream_encoder_process = (libOggFLAC_func_OggFLAC__stream_encoder_process_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_process" );
  if ( dll->OggFLAC__stream_encoder_process == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_process_interleaved
  dll->OggFLAC__stream_encoder_process_interleaved = (libOggFLAC_func_OggFLAC__stream_encoder_process_interleaved_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_encoder_process_interleaved" );
  if ( dll->OggFLAC__stream_encoder_process_interleaved == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_new
  dll->OggFLAC__stream_decoder_new = (libOggFLAC_func_OggFLAC__stream_decoder_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_new" );
  if ( dll->OggFLAC__stream_decoder_new == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_delete
  dll->OggFLAC__stream_decoder_delete = (libOggFLAC_func_OggFLAC__stream_decoder_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_delete" );
  if ( dll->OggFLAC__stream_decoder_delete == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_read_callback
  dll->OggFLAC__stream_decoder_set_read_callback = (libOggFLAC_func_OggFLAC__stream_decoder_set_read_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_read_callback" );
  if ( dll->OggFLAC__stream_decoder_set_read_callback == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_write_callback
  dll->OggFLAC__stream_decoder_set_write_callback = (libOggFLAC_func_OggFLAC__stream_decoder_set_write_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_write_callback" );
  if ( dll->OggFLAC__stream_decoder_set_write_callback == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_callback
  dll->OggFLAC__stream_decoder_set_metadata_callback = (libOggFLAC_func_OggFLAC__stream_decoder_set_metadata_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_metadata_callback" );
  if ( dll->OggFLAC__stream_decoder_set_metadata_callback == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_error_callback
  dll->OggFLAC__stream_decoder_set_error_callback = (libOggFLAC_func_OggFLAC__stream_decoder_set_error_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_error_callback" );
  if ( dll->OggFLAC__stream_decoder_set_error_callback == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_client_data
  dll->OggFLAC__stream_decoder_set_client_data = (libOggFLAC_func_OggFLAC__stream_decoder_set_client_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_client_data" );
  if ( dll->OggFLAC__stream_decoder_set_client_data == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_serial_number
  dll->OggFLAC__stream_decoder_set_serial_number = (libOggFLAC_func_OggFLAC__stream_decoder_set_serial_number_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_serial_number" );
  if ( dll->OggFLAC__stream_decoder_set_serial_number == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_respond
  dll->OggFLAC__stream_decoder_set_metadata_respond = (libOggFLAC_func_OggFLAC__stream_decoder_set_metadata_respond_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_metadata_respond" );
  if ( dll->OggFLAC__stream_decoder_set_metadata_respond == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_respond_application
  dll->OggFLAC__stream_decoder_set_metadata_respond_application = (libOggFLAC_func_OggFLAC__stream_decoder_set_metadata_respond_application_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_metadata_respond_application" );
  if ( dll->OggFLAC__stream_decoder_set_metadata_respond_application == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_respond_all
  dll->OggFLAC__stream_decoder_set_metadata_respond_all = (libOggFLAC_func_OggFLAC__stream_decoder_set_metadata_respond_all_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_metadata_respond_all" );
  if ( dll->OggFLAC__stream_decoder_set_metadata_respond_all == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_ignore
  dll->OggFLAC__stream_decoder_set_metadata_ignore = (libOggFLAC_func_OggFLAC__stream_decoder_set_metadata_ignore_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_metadata_ignore" );
  if ( dll->OggFLAC__stream_decoder_set_metadata_ignore == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_ignore_application
  dll->OggFLAC__stream_decoder_set_metadata_ignore_application = (libOggFLAC_func_OggFLAC__stream_decoder_set_metadata_ignore_application_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_metadata_ignore_application" );
  if ( dll->OggFLAC__stream_decoder_set_metadata_ignore_application == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_ignore_all
  dll->OggFLAC__stream_decoder_set_metadata_ignore_all = (libOggFLAC_func_OggFLAC__stream_decoder_set_metadata_ignore_all_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_set_metadata_ignore_all" );
  if ( dll->OggFLAC__stream_decoder_set_metadata_ignore_all == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_state
  dll->OggFLAC__stream_decoder_get_state = (libOggFLAC_func_OggFLAC__stream_decoder_get_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_get_state" );
  if ( dll->OggFLAC__stream_decoder_get_state == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_FLAC_stream_decoder_state
  dll->OggFLAC__stream_decoder_get_FLAC_stream_decoder_state = (libOggFLAC_func_OggFLAC__stream_decoder_get_FLAC_stream_decoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_get_FLAC_stream_decoder_state" );
  if ( dll->OggFLAC__stream_decoder_get_FLAC_stream_decoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_channels
  dll->OggFLAC__stream_decoder_get_channels = (libOggFLAC_func_OggFLAC__stream_decoder_get_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_get_channels" );
  if ( dll->OggFLAC__stream_decoder_get_channels == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_channel_assignment
  dll->OggFLAC__stream_decoder_get_channel_assignment = (libOggFLAC_func_OggFLAC__stream_decoder_get_channel_assignment_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_get_channel_assignment" );
  if ( dll->OggFLAC__stream_decoder_get_channel_assignment == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_bits_per_sample
  dll->OggFLAC__stream_decoder_get_bits_per_sample = (libOggFLAC_func_OggFLAC__stream_decoder_get_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_get_bits_per_sample" );
  if ( dll->OggFLAC__stream_decoder_get_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_sample_rate
  dll->OggFLAC__stream_decoder_get_sample_rate = (libOggFLAC_func_OggFLAC__stream_decoder_get_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_get_sample_rate" );
  if ( dll->OggFLAC__stream_decoder_get_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_blocksize
  dll->OggFLAC__stream_decoder_get_blocksize = (libOggFLAC_func_OggFLAC__stream_decoder_get_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_get_blocksize" );
  if ( dll->OggFLAC__stream_decoder_get_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_init
  dll->OggFLAC__stream_decoder_init = (libOggFLAC_func_OggFLAC__stream_decoder_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_init" );
  if ( dll->OggFLAC__stream_decoder_init == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_finish
  dll->OggFLAC__stream_decoder_finish = (libOggFLAC_func_OggFLAC__stream_decoder_finish_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_finish" );
  if ( dll->OggFLAC__stream_decoder_finish == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_flush
  dll->OggFLAC__stream_decoder_flush = (libOggFLAC_func_OggFLAC__stream_decoder_flush_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_flush" );
  if ( dll->OggFLAC__stream_decoder_flush == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_reset
  dll->OggFLAC__stream_decoder_reset = (libOggFLAC_func_OggFLAC__stream_decoder_reset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_reset" );
  if ( dll->OggFLAC__stream_decoder_reset == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_process_single
  dll->OggFLAC__stream_decoder_process_single = (libOggFLAC_func_OggFLAC__stream_decoder_process_single_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_process_single" );
  if ( dll->OggFLAC__stream_decoder_process_single == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_process_until_end_of_metadata
  dll->OggFLAC__stream_decoder_process_until_end_of_metadata = (libOggFLAC_func_OggFLAC__stream_decoder_process_until_end_of_metadata_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_process_until_end_of_metadata" );
  if ( dll->OggFLAC__stream_decoder_process_until_end_of_metadata == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_process_until_end_of_stream
  dll->OggFLAC__stream_decoder_process_until_end_of_stream = (libOggFLAC_func_OggFLAC__stream_decoder_process_until_end_of_stream_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "OggFLAC__stream_decoder_process_until_end_of_stream" );
  if ( dll->OggFLAC__stream_decoder_process_until_end_of_stream == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__StreamEncoderStateString
  dll->OggFLAC__StreamEncoderStateString = (OggFLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "OggFLAC__StreamEncoderStateString" );
  if ( dll->OggFLAC__StreamEncoderStateString == NULL ) err++;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__StreamDecoderStateString
  dll->OggFLAC__StreamDecoderStateString = (OggFLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "OggFLAC__StreamDecoderStateString" );
  if ( dll->OggFLAC__StreamDecoderStateString == NULL ) err++;
#endif
  if ( err > 0 ) { free ( dll ); return NULL; }
  return dll;
}

void free_libOggFLAC_dll ( libOggFLAC_dll_t *dll )
{
  FreeLibrary ( (HMODULE) dll->__h_dll );
	free ( dll );
}


#ifndef IGNORE_libOggFLAC_OggFLAC__StreamEncoderStateString
OggFLAC_API const char * const* * g_libOggFLAC_OggFLAC__StreamEncoderStateString = NULL;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__StreamDecoderStateString
OggFLAC_API const char * const* * g_libOggFLAC_OggFLAC__StreamDecoderStateString = NULL;
#endif

static libOggFLAC_dll_t* volatile g_libOggFLAC_dll = NULL;
int g_load_libOggFLAC_dll ( char *path )
{
	if ( g_libOggFLAC_dll != NULL ) return 0;
	g_libOggFLAC_dll = load_libOggFLAC_dll ( path );
	if ( g_libOggFLAC_dll == NULL ) return -1;
#ifndef IGNORE_libOggFLAC_OggFLAC__StreamEncoderStateString
	g_libOggFLAC_OggFLAC__StreamEncoderStateString = g_libOggFLAC_dll->OggFLAC__StreamEncoderStateString;
#endif
#ifndef IGNORE_libOggFLAC_OggFLAC__StreamDecoderStateString
	g_libOggFLAC_OggFLAC__StreamDecoderStateString = g_libOggFLAC_dll->OggFLAC__StreamDecoderStateString;
#endif
	return 0;
}
void g_free_libOggFLAC_dll ( void )
{
	if ( g_libOggFLAC_dll != NULL ) {
		free_libOggFLAC_dll ( g_libOggFLAC_dll );
		g_libOggFLAC_dll = NULL;
	}
}

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_new
OggFLAC_API OggFLAC__StreamEncoder * OggFLAC__stream_encoder_new()
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_new))();
	}
	return (OggFLAC_API OggFLAC__StreamEncoder *)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_delete
OggFLAC_API void OggFLAC__stream_encoder_delete(OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		(*(g_libOggFLAC_dll->OggFLAC__stream_encoder_delete))(encoder);
	}
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_serial_number
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_serial_number(OggFLAC__StreamEncoder *encoder, long serial_number)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_serial_number))(encoder,serial_number);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_verify
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_verify(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_verify))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_streamable_subset
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_streamable_subset(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_streamable_subset))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_do_mid_side_stereo
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_do_mid_side_stereo))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_loose_mid_side_stereo
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_loose_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_loose_mid_side_stereo))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_channels
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_channels(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_channels))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_bits_per_sample
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_bits_per_sample(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_bits_per_sample))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_sample_rate
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_sample_rate(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_sample_rate))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_blocksize
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_blocksize(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_blocksize))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_max_lpc_order
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_max_lpc_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_max_lpc_order))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_qlp_coeff_precision
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_qlp_coeff_precision(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_qlp_coeff_precision))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_do_escape_coding
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_escape_coding(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_do_escape_coding))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_do_exhaustive_model_search
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_exhaustive_model_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_do_exhaustive_model_search))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_min_residual_partition_order
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_min_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_min_residual_partition_order))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_max_residual_partition_order
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_max_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_max_residual_partition_order))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_rice_parameter_search_dist
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_rice_parameter_search_dist(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_rice_parameter_search_dist))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_total_samples_estimate
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_total_samples_estimate(OggFLAC__StreamEncoder *encoder, FLAC__uint64 value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_total_samples_estimate))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_metadata
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_metadata(OggFLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_metadata))(encoder,metadata,num_blocks);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_write_callback
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_write_callback(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamEncoderWriteCallback value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_write_callback))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_set_client_data
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_client_data(OggFLAC__StreamEncoder *encoder, void *value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_set_client_data))(encoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_state
OggFLAC_API OggFLAC__StreamEncoderState OggFLAC__stream_encoder_get_state(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_state))(encoder);
	}
	return (OggFLAC_API OggFLAC__StreamEncoderState)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_FLAC_stream_encoder_state
OggFLAC_API FLAC__StreamEncoderState OggFLAC__stream_encoder_get_FLAC_stream_encoder_state(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_FLAC_stream_encoder_state))(encoder);
	}
	return (OggFLAC_API FLAC__StreamEncoderState)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_verify_decoder_state
OggFLAC_API FLAC__StreamDecoderState OggFLAC__stream_encoder_get_verify_decoder_state(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_verify_decoder_state))(encoder);
	}
	return (OggFLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_verify_decoder_error_stats
OggFLAC_API void OggFLAC__stream_encoder_get_verify_decoder_error_stats(const OggFLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	if ( g_libOggFLAC_dll != NULL ) {
		(*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_verify_decoder_error_stats))(encoder,absolute_sample,frame_number,channel,sample,expected,got);
	}
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_verify
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_verify(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_verify))(encoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_streamable_subset
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_streamable_subset(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_streamable_subset))(encoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_do_mid_side_stereo
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_mid_side_stereo(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_do_mid_side_stereo))(encoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_loose_mid_side_stereo
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_loose_mid_side_stereo(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_loose_mid_side_stereo))(encoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_channels
OggFLAC_API unsigned OggFLAC__stream_encoder_get_channels(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_channels))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_bits_per_sample
OggFLAC_API unsigned OggFLAC__stream_encoder_get_bits_per_sample(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_bits_per_sample))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_sample_rate
OggFLAC_API unsigned OggFLAC__stream_encoder_get_sample_rate(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_sample_rate))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_blocksize
OggFLAC_API unsigned OggFLAC__stream_encoder_get_blocksize(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_blocksize))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_max_lpc_order
OggFLAC_API unsigned OggFLAC__stream_encoder_get_max_lpc_order(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_max_lpc_order))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_qlp_coeff_precision
OggFLAC_API unsigned OggFLAC__stream_encoder_get_qlp_coeff_precision(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_qlp_coeff_precision))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search))(encoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_do_escape_coding
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_escape_coding(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_do_escape_coding))(encoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_do_exhaustive_model_search
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_exhaustive_model_search(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_do_exhaustive_model_search))(encoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_min_residual_partition_order
OggFLAC_API unsigned OggFLAC__stream_encoder_get_min_residual_partition_order(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_min_residual_partition_order))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_max_residual_partition_order
OggFLAC_API unsigned OggFLAC__stream_encoder_get_max_residual_partition_order(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_max_residual_partition_order))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_rice_parameter_search_dist
OggFLAC_API unsigned OggFLAC__stream_encoder_get_rice_parameter_search_dist(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_rice_parameter_search_dist))(encoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_get_total_samples_estimate
OggFLAC_API FLAC__uint64 OggFLAC__stream_encoder_get_total_samples_estimate(const OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_get_total_samples_estimate))(encoder);
	}
	return (OggFLAC_API FLAC__uint64)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_init
OggFLAC_API OggFLAC__StreamEncoderState OggFLAC__stream_encoder_init(OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_init))(encoder);
	}
	return (OggFLAC_API OggFLAC__StreamEncoderState)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_finish
OggFLAC_API void OggFLAC__stream_encoder_finish(OggFLAC__StreamEncoder *encoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		(*(g_libOggFLAC_dll->OggFLAC__stream_encoder_finish))(encoder);
	}
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_process
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_process(OggFLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_process))(encoder,buffer,samples);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_encoder_process_interleaved
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_process_interleaved(OggFLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_encoder_process_interleaved))(encoder,buffer,samples);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_new
OggFLAC_API OggFLAC__StreamDecoder * OggFLAC__stream_decoder_new()
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_new))();
	}
	return (OggFLAC_API OggFLAC__StreamDecoder *)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_delete
OggFLAC_API void OggFLAC__stream_decoder_delete(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		(*(g_libOggFLAC_dll->OggFLAC__stream_decoder_delete))(decoder);
	}
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_read_callback
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_read_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderReadCallback value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_read_callback))(decoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_write_callback
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_write_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderWriteCallback value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_write_callback))(decoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_callback
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderMetadataCallback value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_metadata_callback))(decoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_error_callback
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_error_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderErrorCallback value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_error_callback))(decoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_client_data
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_client_data(OggFLAC__StreamDecoder *decoder, void *value)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_client_data))(decoder,value);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_serial_number
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_serial_number(OggFLAC__StreamDecoder *decoder, long serial_number)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_serial_number))(decoder,serial_number);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_respond
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond(OggFLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_metadata_respond))(decoder,type);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_respond_application
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_metadata_respond_application))(decoder,id);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_respond_all
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_all(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_metadata_respond_all))(decoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_ignore
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore(OggFLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_metadata_ignore))(decoder,type);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_ignore_application
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_metadata_ignore_application))(decoder,id);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_set_metadata_ignore_all
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_all(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_set_metadata_ignore_all))(decoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_state
OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__stream_decoder_get_state(const OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_get_state))(decoder);
	}
	return (OggFLAC_API OggFLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_FLAC_stream_decoder_state
OggFLAC_API FLAC__StreamDecoderState OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(const OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_get_FLAC_stream_decoder_state))(decoder);
	}
	return (OggFLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_channels
OggFLAC_API unsigned OggFLAC__stream_decoder_get_channels(const OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_get_channels))(decoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_channel_assignment
OggFLAC_API FLAC__ChannelAssignment OggFLAC__stream_decoder_get_channel_assignment(const OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_get_channel_assignment))(decoder);
	}
	return (OggFLAC_API FLAC__ChannelAssignment)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_bits_per_sample
OggFLAC_API unsigned OggFLAC__stream_decoder_get_bits_per_sample(const OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_get_bits_per_sample))(decoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_sample_rate
OggFLAC_API unsigned OggFLAC__stream_decoder_get_sample_rate(const OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_get_sample_rate))(decoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_get_blocksize
OggFLAC_API unsigned OggFLAC__stream_decoder_get_blocksize(const OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_get_blocksize))(decoder);
	}
	return (OggFLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_init
OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__stream_decoder_init(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_init))(decoder);
	}
	return (OggFLAC_API OggFLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_finish
OggFLAC_API void OggFLAC__stream_decoder_finish(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		(*(g_libOggFLAC_dll->OggFLAC__stream_decoder_finish))(decoder);
	}
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_flush
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_flush(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_flush))(decoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_reset
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_reset(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_reset))(decoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_process_single
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_single(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_process_single))(decoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_process_until_end_of_metadata
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_metadata(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_process_until_end_of_metadata))(decoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libOggFLAC_OggFLAC__stream_decoder_process_until_end_of_stream
OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_stream(OggFLAC__StreamDecoder *decoder)
{
	if ( g_libOggFLAC_dll != NULL ) {
		return (*(g_libOggFLAC_dll->OggFLAC__stream_decoder_process_until_end_of_stream))(decoder);
	}
	return (OggFLAC_API FLAC__bool)0;
}
#endif

/*
  NOT IMPORT LIST(3)
    OggFLAC__stream_encoder_disable_constant_subframes
    OggFLAC__stream_encoder_disable_fixed_subframes
    OggFLAC__stream_encoder_disable_verbatim_subframes
*/

#if defined(__cplusplus)
}  /* extern "C" { */
#endif


#endif /* AU_FLAC_DLL */

