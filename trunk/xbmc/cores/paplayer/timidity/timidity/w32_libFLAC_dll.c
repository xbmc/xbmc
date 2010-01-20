

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef AU_FLAC_DLL


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
  name : libFLAC
***************************************************************/


#include "w32_libFLAC_dll.h"


/***************************************************************
   for c source
 ***************************************************************/


#if defined(__cplusplus)
extern "C" {
#endif



libFLAC_dll_t *load_libFLAC_dll ( char *path )
{
  int err = 0;
  libFLAC_dll_t *dll = (libFLAC_dll_t *) malloc ( sizeof(libFLAC_dll_t) );
  if ( dll == NULL ) return NULL;
  dll->__h_dll = LoadLibrary ( path );
  if ( dll->__h_dll == NULL ) { free ( dll ); return NULL; };

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_new
  dll->FLAC__stream_encoder_new = (libFLAC_func_FLAC__stream_encoder_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_new" );
  if ( dll->FLAC__stream_encoder_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_delete
  dll->FLAC__stream_encoder_delete = (libFLAC_func_FLAC__stream_encoder_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_delete" );
  if ( dll->FLAC__stream_encoder_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_verify
  dll->FLAC__stream_encoder_set_verify = (libFLAC_func_FLAC__stream_encoder_set_verify_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_verify" );
  if ( dll->FLAC__stream_encoder_set_verify == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_streamable_subset
  dll->FLAC__stream_encoder_set_streamable_subset = (libFLAC_func_FLAC__stream_encoder_set_streamable_subset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_streamable_subset" );
  if ( dll->FLAC__stream_encoder_set_streamable_subset == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_mid_side_stereo
  dll->FLAC__stream_encoder_set_do_mid_side_stereo = (libFLAC_func_FLAC__stream_encoder_set_do_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_do_mid_side_stereo" );
  if ( dll->FLAC__stream_encoder_set_do_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_loose_mid_side_stereo
  dll->FLAC__stream_encoder_set_loose_mid_side_stereo = (libFLAC_func_FLAC__stream_encoder_set_loose_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_loose_mid_side_stereo" );
  if ( dll->FLAC__stream_encoder_set_loose_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_channels
  dll->FLAC__stream_encoder_set_channels = (libFLAC_func_FLAC__stream_encoder_set_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_channels" );
  if ( dll->FLAC__stream_encoder_set_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_bits_per_sample
  dll->FLAC__stream_encoder_set_bits_per_sample = (libFLAC_func_FLAC__stream_encoder_set_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_bits_per_sample" );
  if ( dll->FLAC__stream_encoder_set_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_sample_rate
  dll->FLAC__stream_encoder_set_sample_rate = (libFLAC_func_FLAC__stream_encoder_set_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_sample_rate" );
  if ( dll->FLAC__stream_encoder_set_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_blocksize
  dll->FLAC__stream_encoder_set_blocksize = (libFLAC_func_FLAC__stream_encoder_set_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_blocksize" );
  if ( dll->FLAC__stream_encoder_set_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_max_lpc_order
  dll->FLAC__stream_encoder_set_max_lpc_order = (libFLAC_func_FLAC__stream_encoder_set_max_lpc_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_max_lpc_order" );
  if ( dll->FLAC__stream_encoder_set_max_lpc_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_qlp_coeff_precision
  dll->FLAC__stream_encoder_set_qlp_coeff_precision = (libFLAC_func_FLAC__stream_encoder_set_qlp_coeff_precision_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_qlp_coeff_precision" );
  if ( dll->FLAC__stream_encoder_set_qlp_coeff_precision == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_qlp_coeff_prec_search
  dll->FLAC__stream_encoder_set_do_qlp_coeff_prec_search = (libFLAC_func_FLAC__stream_encoder_set_do_qlp_coeff_prec_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_do_qlp_coeff_prec_search" );
  if ( dll->FLAC__stream_encoder_set_do_qlp_coeff_prec_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_escape_coding
  dll->FLAC__stream_encoder_set_do_escape_coding = (libFLAC_func_FLAC__stream_encoder_set_do_escape_coding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_do_escape_coding" );
  if ( dll->FLAC__stream_encoder_set_do_escape_coding == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_exhaustive_model_search
  dll->FLAC__stream_encoder_set_do_exhaustive_model_search = (libFLAC_func_FLAC__stream_encoder_set_do_exhaustive_model_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_do_exhaustive_model_search" );
  if ( dll->FLAC__stream_encoder_set_do_exhaustive_model_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_min_residual_partition_order
  dll->FLAC__stream_encoder_set_min_residual_partition_order = (libFLAC_func_FLAC__stream_encoder_set_min_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_min_residual_partition_order" );
  if ( dll->FLAC__stream_encoder_set_min_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_max_residual_partition_order
  dll->FLAC__stream_encoder_set_max_residual_partition_order = (libFLAC_func_FLAC__stream_encoder_set_max_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_max_residual_partition_order" );
  if ( dll->FLAC__stream_encoder_set_max_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_rice_parameter_search_dist
  dll->FLAC__stream_encoder_set_rice_parameter_search_dist = (libFLAC_func_FLAC__stream_encoder_set_rice_parameter_search_dist_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_rice_parameter_search_dist" );
  if ( dll->FLAC__stream_encoder_set_rice_parameter_search_dist == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_total_samples_estimate
  dll->FLAC__stream_encoder_set_total_samples_estimate = (libFLAC_func_FLAC__stream_encoder_set_total_samples_estimate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_total_samples_estimate" );
  if ( dll->FLAC__stream_encoder_set_total_samples_estimate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_metadata
  dll->FLAC__stream_encoder_set_metadata = (libFLAC_func_FLAC__stream_encoder_set_metadata_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_metadata" );
  if ( dll->FLAC__stream_encoder_set_metadata == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_write_callback
  dll->FLAC__stream_encoder_set_write_callback = (libFLAC_func_FLAC__stream_encoder_set_write_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_write_callback" );
  if ( dll->FLAC__stream_encoder_set_write_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_metadata_callback
  dll->FLAC__stream_encoder_set_metadata_callback = (libFLAC_func_FLAC__stream_encoder_set_metadata_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_metadata_callback" );
  if ( dll->FLAC__stream_encoder_set_metadata_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_client_data
  dll->FLAC__stream_encoder_set_client_data = (libFLAC_func_FLAC__stream_encoder_set_client_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_set_client_data" );
  if ( dll->FLAC__stream_encoder_set_client_data == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_state
  dll->FLAC__stream_encoder_get_state = (libFLAC_func_FLAC__stream_encoder_get_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_state" );
  if ( dll->FLAC__stream_encoder_get_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_state
  dll->FLAC__stream_encoder_get_verify_decoder_state = (libFLAC_func_FLAC__stream_encoder_get_verify_decoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_verify_decoder_state" );
  if ( dll->FLAC__stream_encoder_get_verify_decoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_resolved_state_string
  dll->FLAC__stream_encoder_get_resolved_state_string = (libFLAC_func_FLAC__stream_encoder_get_resolved_state_string_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_resolved_state_string" );
  if ( dll->FLAC__stream_encoder_get_resolved_state_string == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_error_stats
  dll->FLAC__stream_encoder_get_verify_decoder_error_stats = (libFLAC_func_FLAC__stream_encoder_get_verify_decoder_error_stats_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_verify_decoder_error_stats" );
  if ( dll->FLAC__stream_encoder_get_verify_decoder_error_stats == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify
  dll->FLAC__stream_encoder_get_verify = (libFLAC_func_FLAC__stream_encoder_get_verify_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_verify" );
  if ( dll->FLAC__stream_encoder_get_verify == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_streamable_subset
  dll->FLAC__stream_encoder_get_streamable_subset = (libFLAC_func_FLAC__stream_encoder_get_streamable_subset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_streamable_subset" );
  if ( dll->FLAC__stream_encoder_get_streamable_subset == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_mid_side_stereo
  dll->FLAC__stream_encoder_get_do_mid_side_stereo = (libFLAC_func_FLAC__stream_encoder_get_do_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_do_mid_side_stereo" );
  if ( dll->FLAC__stream_encoder_get_do_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_loose_mid_side_stereo
  dll->FLAC__stream_encoder_get_loose_mid_side_stereo = (libFLAC_func_FLAC__stream_encoder_get_loose_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_loose_mid_side_stereo" );
  if ( dll->FLAC__stream_encoder_get_loose_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_channels
  dll->FLAC__stream_encoder_get_channels = (libFLAC_func_FLAC__stream_encoder_get_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_channels" );
  if ( dll->FLAC__stream_encoder_get_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_bits_per_sample
  dll->FLAC__stream_encoder_get_bits_per_sample = (libFLAC_func_FLAC__stream_encoder_get_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_bits_per_sample" );
  if ( dll->FLAC__stream_encoder_get_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_sample_rate
  dll->FLAC__stream_encoder_get_sample_rate = (libFLAC_func_FLAC__stream_encoder_get_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_sample_rate" );
  if ( dll->FLAC__stream_encoder_get_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_blocksize
  dll->FLAC__stream_encoder_get_blocksize = (libFLAC_func_FLAC__stream_encoder_get_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_blocksize" );
  if ( dll->FLAC__stream_encoder_get_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_max_lpc_order
  dll->FLAC__stream_encoder_get_max_lpc_order = (libFLAC_func_FLAC__stream_encoder_get_max_lpc_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_max_lpc_order" );
  if ( dll->FLAC__stream_encoder_get_max_lpc_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_qlp_coeff_precision
  dll->FLAC__stream_encoder_get_qlp_coeff_precision = (libFLAC_func_FLAC__stream_encoder_get_qlp_coeff_precision_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_qlp_coeff_precision" );
  if ( dll->FLAC__stream_encoder_get_qlp_coeff_precision == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_qlp_coeff_prec_search
  dll->FLAC__stream_encoder_get_do_qlp_coeff_prec_search = (libFLAC_func_FLAC__stream_encoder_get_do_qlp_coeff_prec_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_do_qlp_coeff_prec_search" );
  if ( dll->FLAC__stream_encoder_get_do_qlp_coeff_prec_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_escape_coding
  dll->FLAC__stream_encoder_get_do_escape_coding = (libFLAC_func_FLAC__stream_encoder_get_do_escape_coding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_do_escape_coding" );
  if ( dll->FLAC__stream_encoder_get_do_escape_coding == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_exhaustive_model_search
  dll->FLAC__stream_encoder_get_do_exhaustive_model_search = (libFLAC_func_FLAC__stream_encoder_get_do_exhaustive_model_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_do_exhaustive_model_search" );
  if ( dll->FLAC__stream_encoder_get_do_exhaustive_model_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_min_residual_partition_order
  dll->FLAC__stream_encoder_get_min_residual_partition_order = (libFLAC_func_FLAC__stream_encoder_get_min_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_min_residual_partition_order" );
  if ( dll->FLAC__stream_encoder_get_min_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_max_residual_partition_order
  dll->FLAC__stream_encoder_get_max_residual_partition_order = (libFLAC_func_FLAC__stream_encoder_get_max_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_max_residual_partition_order" );
  if ( dll->FLAC__stream_encoder_get_max_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_rice_parameter_search_dist
  dll->FLAC__stream_encoder_get_rice_parameter_search_dist = (libFLAC_func_FLAC__stream_encoder_get_rice_parameter_search_dist_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_rice_parameter_search_dist" );
  if ( dll->FLAC__stream_encoder_get_rice_parameter_search_dist == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_total_samples_estimate
  dll->FLAC__stream_encoder_get_total_samples_estimate = (libFLAC_func_FLAC__stream_encoder_get_total_samples_estimate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_get_total_samples_estimate" );
  if ( dll->FLAC__stream_encoder_get_total_samples_estimate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_init
  dll->FLAC__stream_encoder_init = (libFLAC_func_FLAC__stream_encoder_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_init" );
  if ( dll->FLAC__stream_encoder_init == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_finish
  dll->FLAC__stream_encoder_finish = (libFLAC_func_FLAC__stream_encoder_finish_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_finish" );
  if ( dll->FLAC__stream_encoder_finish == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_process
  dll->FLAC__stream_encoder_process = (libFLAC_func_FLAC__stream_encoder_process_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_process" );
  if ( dll->FLAC__stream_encoder_process == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_process_interleaved
  dll->FLAC__stream_encoder_process_interleaved = (libFLAC_func_FLAC__stream_encoder_process_interleaved_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_encoder_process_interleaved" );
  if ( dll->FLAC__stream_encoder_process_interleaved == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_new
  dll->FLAC__stream_decoder_new = (libFLAC_func_FLAC__stream_decoder_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_new" );
  if ( dll->FLAC__stream_decoder_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_delete
  dll->FLAC__stream_decoder_delete = (libFLAC_func_FLAC__stream_decoder_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_delete" );
  if ( dll->FLAC__stream_decoder_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_read_callback
  dll->FLAC__stream_decoder_set_read_callback = (libFLAC_func_FLAC__stream_decoder_set_read_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_read_callback" );
  if ( dll->FLAC__stream_decoder_set_read_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_write_callback
  dll->FLAC__stream_decoder_set_write_callback = (libFLAC_func_FLAC__stream_decoder_set_write_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_write_callback" );
  if ( dll->FLAC__stream_decoder_set_write_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_callback
  dll->FLAC__stream_decoder_set_metadata_callback = (libFLAC_func_FLAC__stream_decoder_set_metadata_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_metadata_callback" );
  if ( dll->FLAC__stream_decoder_set_metadata_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_error_callback
  dll->FLAC__stream_decoder_set_error_callback = (libFLAC_func_FLAC__stream_decoder_set_error_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_error_callback" );
  if ( dll->FLAC__stream_decoder_set_error_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_client_data
  dll->FLAC__stream_decoder_set_client_data = (libFLAC_func_FLAC__stream_decoder_set_client_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_client_data" );
  if ( dll->FLAC__stream_decoder_set_client_data == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond
  dll->FLAC__stream_decoder_set_metadata_respond = (libFLAC_func_FLAC__stream_decoder_set_metadata_respond_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_metadata_respond" );
  if ( dll->FLAC__stream_decoder_set_metadata_respond == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_application
  dll->FLAC__stream_decoder_set_metadata_respond_application = (libFLAC_func_FLAC__stream_decoder_set_metadata_respond_application_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_metadata_respond_application" );
  if ( dll->FLAC__stream_decoder_set_metadata_respond_application == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_all
  dll->FLAC__stream_decoder_set_metadata_respond_all = (libFLAC_func_FLAC__stream_decoder_set_metadata_respond_all_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_metadata_respond_all" );
  if ( dll->FLAC__stream_decoder_set_metadata_respond_all == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore
  dll->FLAC__stream_decoder_set_metadata_ignore = (libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_metadata_ignore" );
  if ( dll->FLAC__stream_decoder_set_metadata_ignore == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_application
  dll->FLAC__stream_decoder_set_metadata_ignore_application = (libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_application_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_metadata_ignore_application" );
  if ( dll->FLAC__stream_decoder_set_metadata_ignore_application == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_all
  dll->FLAC__stream_decoder_set_metadata_ignore_all = (libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_all_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_set_metadata_ignore_all" );
  if ( dll->FLAC__stream_decoder_set_metadata_ignore_all == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_state
  dll->FLAC__stream_decoder_get_state = (libFLAC_func_FLAC__stream_decoder_get_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_get_state" );
  if ( dll->FLAC__stream_decoder_get_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_channels
  dll->FLAC__stream_decoder_get_channels = (libFLAC_func_FLAC__stream_decoder_get_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_get_channels" );
  if ( dll->FLAC__stream_decoder_get_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_channel_assignment
  dll->FLAC__stream_decoder_get_channel_assignment = (libFLAC_func_FLAC__stream_decoder_get_channel_assignment_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_get_channel_assignment" );
  if ( dll->FLAC__stream_decoder_get_channel_assignment == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_bits_per_sample
  dll->FLAC__stream_decoder_get_bits_per_sample = (libFLAC_func_FLAC__stream_decoder_get_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_get_bits_per_sample" );
  if ( dll->FLAC__stream_decoder_get_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_sample_rate
  dll->FLAC__stream_decoder_get_sample_rate = (libFLAC_func_FLAC__stream_decoder_get_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_get_sample_rate" );
  if ( dll->FLAC__stream_decoder_get_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_blocksize
  dll->FLAC__stream_decoder_get_blocksize = (libFLAC_func_FLAC__stream_decoder_get_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_get_blocksize" );
  if ( dll->FLAC__stream_decoder_get_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_init
  dll->FLAC__stream_decoder_init = (libFLAC_func_FLAC__stream_decoder_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_init" );
  if ( dll->FLAC__stream_decoder_init == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_finish
  dll->FLAC__stream_decoder_finish = (libFLAC_func_FLAC__stream_decoder_finish_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_finish" );
  if ( dll->FLAC__stream_decoder_finish == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_flush
  dll->FLAC__stream_decoder_flush = (libFLAC_func_FLAC__stream_decoder_flush_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_flush" );
  if ( dll->FLAC__stream_decoder_flush == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_reset
  dll->FLAC__stream_decoder_reset = (libFLAC_func_FLAC__stream_decoder_reset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_reset" );
  if ( dll->FLAC__stream_decoder_reset == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_single
  dll->FLAC__stream_decoder_process_single = (libFLAC_func_FLAC__stream_decoder_process_single_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_process_single" );
  if ( dll->FLAC__stream_decoder_process_single == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_metadata
  dll->FLAC__stream_decoder_process_until_end_of_metadata = (libFLAC_func_FLAC__stream_decoder_process_until_end_of_metadata_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_process_until_end_of_metadata" );
  if ( dll->FLAC__stream_decoder_process_until_end_of_metadata == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_stream
  dll->FLAC__stream_decoder_process_until_end_of_stream = (libFLAC_func_FLAC__stream_decoder_process_until_end_of_stream_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__stream_decoder_process_until_end_of_stream" );
  if ( dll->FLAC__stream_decoder_process_until_end_of_stream == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_new
  dll->FLAC__seekable_stream_encoder_new = (libFLAC_func_FLAC__seekable_stream_encoder_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_new" );
  if ( dll->FLAC__seekable_stream_encoder_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_delete
  dll->FLAC__seekable_stream_encoder_delete = (libFLAC_func_FLAC__seekable_stream_encoder_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_delete" );
  if ( dll->FLAC__seekable_stream_encoder_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_verify
  dll->FLAC__seekable_stream_encoder_set_verify = (libFLAC_func_FLAC__seekable_stream_encoder_set_verify_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_verify" );
  if ( dll->FLAC__seekable_stream_encoder_set_verify == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_streamable_subset
  dll->FLAC__seekable_stream_encoder_set_streamable_subset = (libFLAC_func_FLAC__seekable_stream_encoder_set_streamable_subset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_streamable_subset" );
  if ( dll->FLAC__seekable_stream_encoder_set_streamable_subset == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_mid_side_stereo
  dll->FLAC__seekable_stream_encoder_set_do_mid_side_stereo = (libFLAC_func_FLAC__seekable_stream_encoder_set_do_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_do_mid_side_stereo" );
  if ( dll->FLAC__seekable_stream_encoder_set_do_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_loose_mid_side_stereo
  dll->FLAC__seekable_stream_encoder_set_loose_mid_side_stereo = (libFLAC_func_FLAC__seekable_stream_encoder_set_loose_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_loose_mid_side_stereo" );
  if ( dll->FLAC__seekable_stream_encoder_set_loose_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_channels
  dll->FLAC__seekable_stream_encoder_set_channels = (libFLAC_func_FLAC__seekable_stream_encoder_set_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_channels" );
  if ( dll->FLAC__seekable_stream_encoder_set_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_bits_per_sample
  dll->FLAC__seekable_stream_encoder_set_bits_per_sample = (libFLAC_func_FLAC__seekable_stream_encoder_set_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_bits_per_sample" );
  if ( dll->FLAC__seekable_stream_encoder_set_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_sample_rate
  dll->FLAC__seekable_stream_encoder_set_sample_rate = (libFLAC_func_FLAC__seekable_stream_encoder_set_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_sample_rate" );
  if ( dll->FLAC__seekable_stream_encoder_set_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_blocksize
  dll->FLAC__seekable_stream_encoder_set_blocksize = (libFLAC_func_FLAC__seekable_stream_encoder_set_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_blocksize" );
  if ( dll->FLAC__seekable_stream_encoder_set_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_lpc_order
  dll->FLAC__seekable_stream_encoder_set_max_lpc_order = (libFLAC_func_FLAC__seekable_stream_encoder_set_max_lpc_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_max_lpc_order" );
  if ( dll->FLAC__seekable_stream_encoder_set_max_lpc_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_qlp_coeff_precision
  dll->FLAC__seekable_stream_encoder_set_qlp_coeff_precision = (libFLAC_func_FLAC__seekable_stream_encoder_set_qlp_coeff_precision_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_qlp_coeff_precision" );
  if ( dll->FLAC__seekable_stream_encoder_set_qlp_coeff_precision == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search
  dll->FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search = (libFLAC_func_FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search" );
  if ( dll->FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_escape_coding
  dll->FLAC__seekable_stream_encoder_set_do_escape_coding = (libFLAC_func_FLAC__seekable_stream_encoder_set_do_escape_coding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_do_escape_coding" );
  if ( dll->FLAC__seekable_stream_encoder_set_do_escape_coding == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_exhaustive_model_search
  dll->FLAC__seekable_stream_encoder_set_do_exhaustive_model_search = (libFLAC_func_FLAC__seekable_stream_encoder_set_do_exhaustive_model_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_do_exhaustive_model_search" );
  if ( dll->FLAC__seekable_stream_encoder_set_do_exhaustive_model_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_min_residual_partition_order
  dll->FLAC__seekable_stream_encoder_set_min_residual_partition_order = (libFLAC_func_FLAC__seekable_stream_encoder_set_min_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_min_residual_partition_order" );
  if ( dll->FLAC__seekable_stream_encoder_set_min_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_residual_partition_order
  dll->FLAC__seekable_stream_encoder_set_max_residual_partition_order = (libFLAC_func_FLAC__seekable_stream_encoder_set_max_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_max_residual_partition_order" );
  if ( dll->FLAC__seekable_stream_encoder_set_max_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_rice_parameter_search_dist
  dll->FLAC__seekable_stream_encoder_set_rice_parameter_search_dist = (libFLAC_func_FLAC__seekable_stream_encoder_set_rice_parameter_search_dist_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_rice_parameter_search_dist" );
  if ( dll->FLAC__seekable_stream_encoder_set_rice_parameter_search_dist == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_total_samples_estimate
  dll->FLAC__seekable_stream_encoder_set_total_samples_estimate = (libFLAC_func_FLAC__seekable_stream_encoder_set_total_samples_estimate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_total_samples_estimate" );
  if ( dll->FLAC__seekable_stream_encoder_set_total_samples_estimate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_metadata
  dll->FLAC__seekable_stream_encoder_set_metadata = (libFLAC_func_FLAC__seekable_stream_encoder_set_metadata_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_metadata" );
  if ( dll->FLAC__seekable_stream_encoder_set_metadata == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_seek_callback
  dll->FLAC__seekable_stream_encoder_set_seek_callback = (libFLAC_func_FLAC__seekable_stream_encoder_set_seek_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_seek_callback" );
  if ( dll->FLAC__seekable_stream_encoder_set_seek_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_write_callback
  dll->FLAC__seekable_stream_encoder_set_write_callback = (libFLAC_func_FLAC__seekable_stream_encoder_set_write_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_write_callback" );
  if ( dll->FLAC__seekable_stream_encoder_set_write_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_client_data
  dll->FLAC__seekable_stream_encoder_set_client_data = (libFLAC_func_FLAC__seekable_stream_encoder_set_client_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_set_client_data" );
  if ( dll->FLAC__seekable_stream_encoder_set_client_data == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_state
  dll->FLAC__seekable_stream_encoder_get_state = (libFLAC_func_FLAC__seekable_stream_encoder_get_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_state" );
  if ( dll->FLAC__seekable_stream_encoder_get_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_stream_encoder_state
  dll->FLAC__seekable_stream_encoder_get_stream_encoder_state = (libFLAC_func_FLAC__seekable_stream_encoder_get_stream_encoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_stream_encoder_state" );
  if ( dll->FLAC__seekable_stream_encoder_get_stream_encoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_state
  dll->FLAC__seekable_stream_encoder_get_verify_decoder_state = (libFLAC_func_FLAC__seekable_stream_encoder_get_verify_decoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_verify_decoder_state" );
  if ( dll->FLAC__seekable_stream_encoder_get_verify_decoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_resolved_state_string
  dll->FLAC__seekable_stream_encoder_get_resolved_state_string = (libFLAC_func_FLAC__seekable_stream_encoder_get_resolved_state_string_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_resolved_state_string" );
  if ( dll->FLAC__seekable_stream_encoder_get_resolved_state_string == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_error_stats
  dll->FLAC__seekable_stream_encoder_get_verify_decoder_error_stats = (libFLAC_func_FLAC__seekable_stream_encoder_get_verify_decoder_error_stats_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_verify_decoder_error_stats" );
  if ( dll->FLAC__seekable_stream_encoder_get_verify_decoder_error_stats == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify
  dll->FLAC__seekable_stream_encoder_get_verify = (libFLAC_func_FLAC__seekable_stream_encoder_get_verify_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_verify" );
  if ( dll->FLAC__seekable_stream_encoder_get_verify == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_streamable_subset
  dll->FLAC__seekable_stream_encoder_get_streamable_subset = (libFLAC_func_FLAC__seekable_stream_encoder_get_streamable_subset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_streamable_subset" );
  if ( dll->FLAC__seekable_stream_encoder_get_streamable_subset == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_mid_side_stereo
  dll->FLAC__seekable_stream_encoder_get_do_mid_side_stereo = (libFLAC_func_FLAC__seekable_stream_encoder_get_do_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_do_mid_side_stereo" );
  if ( dll->FLAC__seekable_stream_encoder_get_do_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_loose_mid_side_stereo
  dll->FLAC__seekable_stream_encoder_get_loose_mid_side_stereo = (libFLAC_func_FLAC__seekable_stream_encoder_get_loose_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_loose_mid_side_stereo" );
  if ( dll->FLAC__seekable_stream_encoder_get_loose_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_channels
  dll->FLAC__seekable_stream_encoder_get_channels = (libFLAC_func_FLAC__seekable_stream_encoder_get_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_channels" );
  if ( dll->FLAC__seekable_stream_encoder_get_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_bits_per_sample
  dll->FLAC__seekable_stream_encoder_get_bits_per_sample = (libFLAC_func_FLAC__seekable_stream_encoder_get_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_bits_per_sample" );
  if ( dll->FLAC__seekable_stream_encoder_get_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_sample_rate
  dll->FLAC__seekable_stream_encoder_get_sample_rate = (libFLAC_func_FLAC__seekable_stream_encoder_get_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_sample_rate" );
  if ( dll->FLAC__seekable_stream_encoder_get_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_blocksize
  dll->FLAC__seekable_stream_encoder_get_blocksize = (libFLAC_func_FLAC__seekable_stream_encoder_get_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_blocksize" );
  if ( dll->FLAC__seekable_stream_encoder_get_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_lpc_order
  dll->FLAC__seekable_stream_encoder_get_max_lpc_order = (libFLAC_func_FLAC__seekable_stream_encoder_get_max_lpc_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_max_lpc_order" );
  if ( dll->FLAC__seekable_stream_encoder_get_max_lpc_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_qlp_coeff_precision
  dll->FLAC__seekable_stream_encoder_get_qlp_coeff_precision = (libFLAC_func_FLAC__seekable_stream_encoder_get_qlp_coeff_precision_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_qlp_coeff_precision" );
  if ( dll->FLAC__seekable_stream_encoder_get_qlp_coeff_precision == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search
  dll->FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search = (libFLAC_func_FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search" );
  if ( dll->FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_escape_coding
  dll->FLAC__seekable_stream_encoder_get_do_escape_coding = (libFLAC_func_FLAC__seekable_stream_encoder_get_do_escape_coding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_do_escape_coding" );
  if ( dll->FLAC__seekable_stream_encoder_get_do_escape_coding == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_exhaustive_model_search
  dll->FLAC__seekable_stream_encoder_get_do_exhaustive_model_search = (libFLAC_func_FLAC__seekable_stream_encoder_get_do_exhaustive_model_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_do_exhaustive_model_search" );
  if ( dll->FLAC__seekable_stream_encoder_get_do_exhaustive_model_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_min_residual_partition_order
  dll->FLAC__seekable_stream_encoder_get_min_residual_partition_order = (libFLAC_func_FLAC__seekable_stream_encoder_get_min_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_min_residual_partition_order" );
  if ( dll->FLAC__seekable_stream_encoder_get_min_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_residual_partition_order
  dll->FLAC__seekable_stream_encoder_get_max_residual_partition_order = (libFLAC_func_FLAC__seekable_stream_encoder_get_max_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_max_residual_partition_order" );
  if ( dll->FLAC__seekable_stream_encoder_get_max_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_rice_parameter_search_dist
  dll->FLAC__seekable_stream_encoder_get_rice_parameter_search_dist = (libFLAC_func_FLAC__seekable_stream_encoder_get_rice_parameter_search_dist_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_rice_parameter_search_dist" );
  if ( dll->FLAC__seekable_stream_encoder_get_rice_parameter_search_dist == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_total_samples_estimate
  dll->FLAC__seekable_stream_encoder_get_total_samples_estimate = (libFLAC_func_FLAC__seekable_stream_encoder_get_total_samples_estimate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_get_total_samples_estimate" );
  if ( dll->FLAC__seekable_stream_encoder_get_total_samples_estimate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_init
  dll->FLAC__seekable_stream_encoder_init = (libFLAC_func_FLAC__seekable_stream_encoder_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_init" );
  if ( dll->FLAC__seekable_stream_encoder_init == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_finish
  dll->FLAC__seekable_stream_encoder_finish = (libFLAC_func_FLAC__seekable_stream_encoder_finish_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_finish" );
  if ( dll->FLAC__seekable_stream_encoder_finish == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_process
  dll->FLAC__seekable_stream_encoder_process = (libFLAC_func_FLAC__seekable_stream_encoder_process_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_process" );
  if ( dll->FLAC__seekable_stream_encoder_process == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_process_interleaved
  dll->FLAC__seekable_stream_encoder_process_interleaved = (libFLAC_func_FLAC__seekable_stream_encoder_process_interleaved_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_encoder_process_interleaved" );
  if ( dll->FLAC__seekable_stream_encoder_process_interleaved == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_new
  dll->FLAC__seekable_stream_decoder_new = (libFLAC_func_FLAC__seekable_stream_decoder_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_new" );
  if ( dll->FLAC__seekable_stream_decoder_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_delete
  dll->FLAC__seekable_stream_decoder_delete = (libFLAC_func_FLAC__seekable_stream_decoder_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_delete" );
  if ( dll->FLAC__seekable_stream_decoder_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_md5_checking
  dll->FLAC__seekable_stream_decoder_set_md5_checking = (libFLAC_func_FLAC__seekable_stream_decoder_set_md5_checking_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_md5_checking" );
  if ( dll->FLAC__seekable_stream_decoder_set_md5_checking == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_read_callback
  dll->FLAC__seekable_stream_decoder_set_read_callback = (libFLAC_func_FLAC__seekable_stream_decoder_set_read_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_read_callback" );
  if ( dll->FLAC__seekable_stream_decoder_set_read_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_seek_callback
  dll->FLAC__seekable_stream_decoder_set_seek_callback = (libFLAC_func_FLAC__seekable_stream_decoder_set_seek_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_seek_callback" );
  if ( dll->FLAC__seekable_stream_decoder_set_seek_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_tell_callback
  dll->FLAC__seekable_stream_decoder_set_tell_callback = (libFLAC_func_FLAC__seekable_stream_decoder_set_tell_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_tell_callback" );
  if ( dll->FLAC__seekable_stream_decoder_set_tell_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_length_callback
  dll->FLAC__seekable_stream_decoder_set_length_callback = (libFLAC_func_FLAC__seekable_stream_decoder_set_length_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_length_callback" );
  if ( dll->FLAC__seekable_stream_decoder_set_length_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_eof_callback
  dll->FLAC__seekable_stream_decoder_set_eof_callback = (libFLAC_func_FLAC__seekable_stream_decoder_set_eof_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_eof_callback" );
  if ( dll->FLAC__seekable_stream_decoder_set_eof_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_write_callback
  dll->FLAC__seekable_stream_decoder_set_write_callback = (libFLAC_func_FLAC__seekable_stream_decoder_set_write_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_write_callback" );
  if ( dll->FLAC__seekable_stream_decoder_set_write_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_callback
  dll->FLAC__seekable_stream_decoder_set_metadata_callback = (libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_metadata_callback" );
  if ( dll->FLAC__seekable_stream_decoder_set_metadata_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_error_callback
  dll->FLAC__seekable_stream_decoder_set_error_callback = (libFLAC_func_FLAC__seekable_stream_decoder_set_error_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_error_callback" );
  if ( dll->FLAC__seekable_stream_decoder_set_error_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_client_data
  dll->FLAC__seekable_stream_decoder_set_client_data = (libFLAC_func_FLAC__seekable_stream_decoder_set_client_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_client_data" );
  if ( dll->FLAC__seekable_stream_decoder_set_client_data == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond
  dll->FLAC__seekable_stream_decoder_set_metadata_respond = (libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_metadata_respond" );
  if ( dll->FLAC__seekable_stream_decoder_set_metadata_respond == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_application
  dll->FLAC__seekable_stream_decoder_set_metadata_respond_application = (libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_application_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_metadata_respond_application" );
  if ( dll->FLAC__seekable_stream_decoder_set_metadata_respond_application == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_all
  dll->FLAC__seekable_stream_decoder_set_metadata_respond_all = (libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_all_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_metadata_respond_all" );
  if ( dll->FLAC__seekable_stream_decoder_set_metadata_respond_all == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore
  dll->FLAC__seekable_stream_decoder_set_metadata_ignore = (libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_metadata_ignore" );
  if ( dll->FLAC__seekable_stream_decoder_set_metadata_ignore == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_application
  dll->FLAC__seekable_stream_decoder_set_metadata_ignore_application = (libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_application_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_metadata_ignore_application" );
  if ( dll->FLAC__seekable_stream_decoder_set_metadata_ignore_application == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_all
  dll->FLAC__seekable_stream_decoder_set_metadata_ignore_all = (libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_all_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_set_metadata_ignore_all" );
  if ( dll->FLAC__seekable_stream_decoder_set_metadata_ignore_all == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_state
  dll->FLAC__seekable_stream_decoder_get_state = (libFLAC_func_FLAC__seekable_stream_decoder_get_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_state" );
  if ( dll->FLAC__seekable_stream_decoder_get_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_stream_decoder_state
  dll->FLAC__seekable_stream_decoder_get_stream_decoder_state = (libFLAC_func_FLAC__seekable_stream_decoder_get_stream_decoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_stream_decoder_state" );
  if ( dll->FLAC__seekable_stream_decoder_get_stream_decoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_resolved_state_string
  dll->FLAC__seekable_stream_decoder_get_resolved_state_string = (libFLAC_func_FLAC__seekable_stream_decoder_get_resolved_state_string_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_resolved_state_string" );
  if ( dll->FLAC__seekable_stream_decoder_get_resolved_state_string == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_md5_checking
  dll->FLAC__seekable_stream_decoder_get_md5_checking = (libFLAC_func_FLAC__seekable_stream_decoder_get_md5_checking_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_md5_checking" );
  if ( dll->FLAC__seekable_stream_decoder_get_md5_checking == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channels
  dll->FLAC__seekable_stream_decoder_get_channels = (libFLAC_func_FLAC__seekable_stream_decoder_get_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_channels" );
  if ( dll->FLAC__seekable_stream_decoder_get_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channel_assignment
  dll->FLAC__seekable_stream_decoder_get_channel_assignment = (libFLAC_func_FLAC__seekable_stream_decoder_get_channel_assignment_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_channel_assignment" );
  if ( dll->FLAC__seekable_stream_decoder_get_channel_assignment == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_bits_per_sample
  dll->FLAC__seekable_stream_decoder_get_bits_per_sample = (libFLAC_func_FLAC__seekable_stream_decoder_get_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_bits_per_sample" );
  if ( dll->FLAC__seekable_stream_decoder_get_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_sample_rate
  dll->FLAC__seekable_stream_decoder_get_sample_rate = (libFLAC_func_FLAC__seekable_stream_decoder_get_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_sample_rate" );
  if ( dll->FLAC__seekable_stream_decoder_get_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_blocksize
  dll->FLAC__seekable_stream_decoder_get_blocksize = (libFLAC_func_FLAC__seekable_stream_decoder_get_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_blocksize" );
  if ( dll->FLAC__seekable_stream_decoder_get_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_decode_position
  dll->FLAC__seekable_stream_decoder_get_decode_position = (libFLAC_func_FLAC__seekable_stream_decoder_get_decode_position_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_get_decode_position" );
  if ( dll->FLAC__seekable_stream_decoder_get_decode_position == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_init
  dll->FLAC__seekable_stream_decoder_init = (libFLAC_func_FLAC__seekable_stream_decoder_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_init" );
  if ( dll->FLAC__seekable_stream_decoder_init == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_finish
  dll->FLAC__seekable_stream_decoder_finish = (libFLAC_func_FLAC__seekable_stream_decoder_finish_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_finish" );
  if ( dll->FLAC__seekable_stream_decoder_finish == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_flush
  dll->FLAC__seekable_stream_decoder_flush = (libFLAC_func_FLAC__seekable_stream_decoder_flush_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_flush" );
  if ( dll->FLAC__seekable_stream_decoder_flush == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_reset
  dll->FLAC__seekable_stream_decoder_reset = (libFLAC_func_FLAC__seekable_stream_decoder_reset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_reset" );
  if ( dll->FLAC__seekable_stream_decoder_reset == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_single
  dll->FLAC__seekable_stream_decoder_process_single = (libFLAC_func_FLAC__seekable_stream_decoder_process_single_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_process_single" );
  if ( dll->FLAC__seekable_stream_decoder_process_single == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_metadata
  dll->FLAC__seekable_stream_decoder_process_until_end_of_metadata = (libFLAC_func_FLAC__seekable_stream_decoder_process_until_end_of_metadata_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_process_until_end_of_metadata" );
  if ( dll->FLAC__seekable_stream_decoder_process_until_end_of_metadata == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_stream
  dll->FLAC__seekable_stream_decoder_process_until_end_of_stream = (libFLAC_func_FLAC__seekable_stream_decoder_process_until_end_of_stream_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_process_until_end_of_stream" );
  if ( dll->FLAC__seekable_stream_decoder_process_until_end_of_stream == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_seek_absolute
  dll->FLAC__seekable_stream_decoder_seek_absolute = (libFLAC_func_FLAC__seekable_stream_decoder_seek_absolute_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__seekable_stream_decoder_seek_absolute" );
  if ( dll->FLAC__seekable_stream_decoder_seek_absolute == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_get_streaminfo
  dll->FLAC__metadata_get_streaminfo = (libFLAC_func_FLAC__metadata_get_streaminfo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_get_streaminfo" );
  if ( dll->FLAC__metadata_get_streaminfo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_new
  dll->FLAC__metadata_simple_iterator_new = (libFLAC_func_FLAC__metadata_simple_iterator_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_new" );
  if ( dll->FLAC__metadata_simple_iterator_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete
  dll->FLAC__metadata_simple_iterator_delete = (libFLAC_func_FLAC__metadata_simple_iterator_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_delete" );
  if ( dll->FLAC__metadata_simple_iterator_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_status
  dll->FLAC__metadata_simple_iterator_status = (libFLAC_func_FLAC__metadata_simple_iterator_status_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_status" );
  if ( dll->FLAC__metadata_simple_iterator_status == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_init
  dll->FLAC__metadata_simple_iterator_init = (libFLAC_func_FLAC__metadata_simple_iterator_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_init" );
  if ( dll->FLAC__metadata_simple_iterator_init == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_is_writable
  dll->FLAC__metadata_simple_iterator_is_writable = (libFLAC_func_FLAC__metadata_simple_iterator_is_writable_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_is_writable" );
  if ( dll->FLAC__metadata_simple_iterator_is_writable == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_next
  dll->FLAC__metadata_simple_iterator_next = (libFLAC_func_FLAC__metadata_simple_iterator_next_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_next" );
  if ( dll->FLAC__metadata_simple_iterator_next == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_prev
  dll->FLAC__metadata_simple_iterator_prev = (libFLAC_func_FLAC__metadata_simple_iterator_prev_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_prev" );
  if ( dll->FLAC__metadata_simple_iterator_prev == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block_type
  dll->FLAC__metadata_simple_iterator_get_block_type = (libFLAC_func_FLAC__metadata_simple_iterator_get_block_type_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_get_block_type" );
  if ( dll->FLAC__metadata_simple_iterator_get_block_type == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block
  dll->FLAC__metadata_simple_iterator_get_block = (libFLAC_func_FLAC__metadata_simple_iterator_get_block_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_get_block" );
  if ( dll->FLAC__metadata_simple_iterator_get_block == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_set_block
  dll->FLAC__metadata_simple_iterator_set_block = (libFLAC_func_FLAC__metadata_simple_iterator_set_block_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_set_block" );
  if ( dll->FLAC__metadata_simple_iterator_set_block == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_insert_block_after
  dll->FLAC__metadata_simple_iterator_insert_block_after = (libFLAC_func_FLAC__metadata_simple_iterator_insert_block_after_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_insert_block_after" );
  if ( dll->FLAC__metadata_simple_iterator_insert_block_after == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete_block
  dll->FLAC__metadata_simple_iterator_delete_block = (libFLAC_func_FLAC__metadata_simple_iterator_delete_block_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_simple_iterator_delete_block" );
  if ( dll->FLAC__metadata_simple_iterator_delete_block == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_new
  dll->FLAC__metadata_chain_new = (libFLAC_func_FLAC__metadata_chain_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_chain_new" );
  if ( dll->FLAC__metadata_chain_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_delete
  dll->FLAC__metadata_chain_delete = (libFLAC_func_FLAC__metadata_chain_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_chain_delete" );
  if ( dll->FLAC__metadata_chain_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_status
  dll->FLAC__metadata_chain_status = (libFLAC_func_FLAC__metadata_chain_status_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_chain_status" );
  if ( dll->FLAC__metadata_chain_status == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_read
  dll->FLAC__metadata_chain_read = (libFLAC_func_FLAC__metadata_chain_read_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_chain_read" );
  if ( dll->FLAC__metadata_chain_read == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_write
  dll->FLAC__metadata_chain_write = (libFLAC_func_FLAC__metadata_chain_write_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_chain_write" );
  if ( dll->FLAC__metadata_chain_write == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_merge_padding
  dll->FLAC__metadata_chain_merge_padding = (libFLAC_func_FLAC__metadata_chain_merge_padding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_chain_merge_padding" );
  if ( dll->FLAC__metadata_chain_merge_padding == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_sort_padding
  dll->FLAC__metadata_chain_sort_padding = (libFLAC_func_FLAC__metadata_chain_sort_padding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_chain_sort_padding" );
  if ( dll->FLAC__metadata_chain_sort_padding == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_new
  dll->FLAC__metadata_iterator_new = (libFLAC_func_FLAC__metadata_iterator_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_new" );
  if ( dll->FLAC__metadata_iterator_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_delete
  dll->FLAC__metadata_iterator_delete = (libFLAC_func_FLAC__metadata_iterator_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_delete" );
  if ( dll->FLAC__metadata_iterator_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_init
  dll->FLAC__metadata_iterator_init = (libFLAC_func_FLAC__metadata_iterator_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_init" );
  if ( dll->FLAC__metadata_iterator_init == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_next
  dll->FLAC__metadata_iterator_next = (libFLAC_func_FLAC__metadata_iterator_next_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_next" );
  if ( dll->FLAC__metadata_iterator_next == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_prev
  dll->FLAC__metadata_iterator_prev = (libFLAC_func_FLAC__metadata_iterator_prev_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_prev" );
  if ( dll->FLAC__metadata_iterator_prev == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_get_block_type
  dll->FLAC__metadata_iterator_get_block_type = (libFLAC_func_FLAC__metadata_iterator_get_block_type_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_get_block_type" );
  if ( dll->FLAC__metadata_iterator_get_block_type == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_get_block
  dll->FLAC__metadata_iterator_get_block = (libFLAC_func_FLAC__metadata_iterator_get_block_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_get_block" );
  if ( dll->FLAC__metadata_iterator_get_block == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_set_block
  dll->FLAC__metadata_iterator_set_block = (libFLAC_func_FLAC__metadata_iterator_set_block_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_set_block" );
  if ( dll->FLAC__metadata_iterator_set_block == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_delete_block
  dll->FLAC__metadata_iterator_delete_block = (libFLAC_func_FLAC__metadata_iterator_delete_block_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_delete_block" );
  if ( dll->FLAC__metadata_iterator_delete_block == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_before
  dll->FLAC__metadata_iterator_insert_block_before = (libFLAC_func_FLAC__metadata_iterator_insert_block_before_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_insert_block_before" );
  if ( dll->FLAC__metadata_iterator_insert_block_before == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_after
  dll->FLAC__metadata_iterator_insert_block_after = (libFLAC_func_FLAC__metadata_iterator_insert_block_after_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_iterator_insert_block_after" );
  if ( dll->FLAC__metadata_iterator_insert_block_after == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_new
  dll->FLAC__metadata_object_new = (libFLAC_func_FLAC__metadata_object_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_new" );
  if ( dll->FLAC__metadata_object_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_clone
  dll->FLAC__metadata_object_clone = (libFLAC_func_FLAC__metadata_object_clone_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_clone" );
  if ( dll->FLAC__metadata_object_clone == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_delete
  dll->FLAC__metadata_object_delete = (libFLAC_func_FLAC__metadata_object_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_delete" );
  if ( dll->FLAC__metadata_object_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_is_equal
  dll->FLAC__metadata_object_is_equal = (libFLAC_func_FLAC__metadata_object_is_equal_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_is_equal" );
  if ( dll->FLAC__metadata_object_is_equal == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_application_set_data
  dll->FLAC__metadata_object_application_set_data = (libFLAC_func_FLAC__metadata_object_application_set_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_application_set_data" );
  if ( dll->FLAC__metadata_object_application_set_data == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_resize_points
  dll->FLAC__metadata_object_seektable_resize_points = (libFLAC_func_FLAC__metadata_object_seektable_resize_points_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_resize_points" );
  if ( dll->FLAC__metadata_object_seektable_resize_points == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_set_point
  dll->FLAC__metadata_object_seektable_set_point = (libFLAC_func_FLAC__metadata_object_seektable_set_point_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_set_point" );
  if ( dll->FLAC__metadata_object_seektable_set_point == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_insert_point
  dll->FLAC__metadata_object_seektable_insert_point = (libFLAC_func_FLAC__metadata_object_seektable_insert_point_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_insert_point" );
  if ( dll->FLAC__metadata_object_seektable_insert_point == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_delete_point
  dll->FLAC__metadata_object_seektable_delete_point = (libFLAC_func_FLAC__metadata_object_seektable_delete_point_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_delete_point" );
  if ( dll->FLAC__metadata_object_seektable_delete_point == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_is_legal
  dll->FLAC__metadata_object_seektable_is_legal = (libFLAC_func_FLAC__metadata_object_seektable_is_legal_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_is_legal" );
  if ( dll->FLAC__metadata_object_seektable_is_legal == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_placeholders
  dll->FLAC__metadata_object_seektable_template_append_placeholders = (libFLAC_func_FLAC__metadata_object_seektable_template_append_placeholders_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_template_append_placeholders" );
  if ( dll->FLAC__metadata_object_seektable_template_append_placeholders == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_point
  dll->FLAC__metadata_object_seektable_template_append_point = (libFLAC_func_FLAC__metadata_object_seektable_template_append_point_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_template_append_point" );
  if ( dll->FLAC__metadata_object_seektable_template_append_point == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_points
  dll->FLAC__metadata_object_seektable_template_append_points = (libFLAC_func_FLAC__metadata_object_seektable_template_append_points_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_template_append_points" );
  if ( dll->FLAC__metadata_object_seektable_template_append_points == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_spaced_points
  dll->FLAC__metadata_object_seektable_template_append_spaced_points = (libFLAC_func_FLAC__metadata_object_seektable_template_append_spaced_points_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_template_append_spaced_points" );
  if ( dll->FLAC__metadata_object_seektable_template_append_spaced_points == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_sort
  dll->FLAC__metadata_object_seektable_template_sort = (libFLAC_func_FLAC__metadata_object_seektable_template_sort_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_seektable_template_sort" );
  if ( dll->FLAC__metadata_object_seektable_template_sort == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_vendor_string
  dll->FLAC__metadata_object_vorbiscomment_set_vendor_string = (libFLAC_func_FLAC__metadata_object_vorbiscomment_set_vendor_string_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_set_vendor_string" );
  if ( dll->FLAC__metadata_object_vorbiscomment_set_vendor_string == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_resize_comments
  dll->FLAC__metadata_object_vorbiscomment_resize_comments = (libFLAC_func_FLAC__metadata_object_vorbiscomment_resize_comments_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_resize_comments" );
  if ( dll->FLAC__metadata_object_vorbiscomment_resize_comments == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_comment
  dll->FLAC__metadata_object_vorbiscomment_set_comment = (libFLAC_func_FLAC__metadata_object_vorbiscomment_set_comment_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_set_comment" );
  if ( dll->FLAC__metadata_object_vorbiscomment_set_comment == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_insert_comment
  dll->FLAC__metadata_object_vorbiscomment_insert_comment = (libFLAC_func_FLAC__metadata_object_vorbiscomment_insert_comment_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_insert_comment" );
  if ( dll->FLAC__metadata_object_vorbiscomment_insert_comment == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_delete_comment
  dll->FLAC__metadata_object_vorbiscomment_delete_comment = (libFLAC_func_FLAC__metadata_object_vorbiscomment_delete_comment_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_delete_comment" );
  if ( dll->FLAC__metadata_object_vorbiscomment_delete_comment == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_entry_matches
  dll->FLAC__metadata_object_vorbiscomment_entry_matches = (libFLAC_func_FLAC__metadata_object_vorbiscomment_entry_matches_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_entry_matches" );
  if ( dll->FLAC__metadata_object_vorbiscomment_entry_matches == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_find_entry_from
  dll->FLAC__metadata_object_vorbiscomment_find_entry_from = (libFLAC_func_FLAC__metadata_object_vorbiscomment_find_entry_from_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_find_entry_from" );
  if ( dll->FLAC__metadata_object_vorbiscomment_find_entry_from == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entry_matching
  dll->FLAC__metadata_object_vorbiscomment_remove_entry_matching = (libFLAC_func_FLAC__metadata_object_vorbiscomment_remove_entry_matching_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_remove_entry_matching" );
  if ( dll->FLAC__metadata_object_vorbiscomment_remove_entry_matching == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entries_matching
  dll->FLAC__metadata_object_vorbiscomment_remove_entries_matching = (libFLAC_func_FLAC__metadata_object_vorbiscomment_remove_entries_matching_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_vorbiscomment_remove_entries_matching" );
  if ( dll->FLAC__metadata_object_vorbiscomment_remove_entries_matching == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_new
  dll->FLAC__metadata_object_cuesheet_track_new = (libFLAC_func_FLAC__metadata_object_cuesheet_track_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_track_new" );
  if ( dll->FLAC__metadata_object_cuesheet_track_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_clone
  dll->FLAC__metadata_object_cuesheet_track_clone = (libFLAC_func_FLAC__metadata_object_cuesheet_track_clone_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_track_clone" );
  if ( dll->FLAC__metadata_object_cuesheet_track_clone == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete
  dll->FLAC__metadata_object_cuesheet_track_delete = (libFLAC_func_FLAC__metadata_object_cuesheet_track_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_track_delete" );
  if ( dll->FLAC__metadata_object_cuesheet_track_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_resize_indices
  dll->FLAC__metadata_object_cuesheet_track_resize_indices = (libFLAC_func_FLAC__metadata_object_cuesheet_track_resize_indices_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_track_resize_indices" );
  if ( dll->FLAC__metadata_object_cuesheet_track_resize_indices == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_index
  dll->FLAC__metadata_object_cuesheet_track_insert_index = (libFLAC_func_FLAC__metadata_object_cuesheet_track_insert_index_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_track_insert_index" );
  if ( dll->FLAC__metadata_object_cuesheet_track_insert_index == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_blank_index
  dll->FLAC__metadata_object_cuesheet_track_insert_blank_index = (libFLAC_func_FLAC__metadata_object_cuesheet_track_insert_blank_index_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_track_insert_blank_index" );
  if ( dll->FLAC__metadata_object_cuesheet_track_insert_blank_index == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete_index
  dll->FLAC__metadata_object_cuesheet_track_delete_index = (libFLAC_func_FLAC__metadata_object_cuesheet_track_delete_index_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_track_delete_index" );
  if ( dll->FLAC__metadata_object_cuesheet_track_delete_index == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_resize_tracks
  dll->FLAC__metadata_object_cuesheet_resize_tracks = (libFLAC_func_FLAC__metadata_object_cuesheet_resize_tracks_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_resize_tracks" );
  if ( dll->FLAC__metadata_object_cuesheet_resize_tracks == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_set_track
  dll->FLAC__metadata_object_cuesheet_set_track = (libFLAC_func_FLAC__metadata_object_cuesheet_set_track_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_set_track" );
  if ( dll->FLAC__metadata_object_cuesheet_set_track == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_track
  dll->FLAC__metadata_object_cuesheet_insert_track = (libFLAC_func_FLAC__metadata_object_cuesheet_insert_track_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_insert_track" );
  if ( dll->FLAC__metadata_object_cuesheet_insert_track == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_blank_track
  dll->FLAC__metadata_object_cuesheet_insert_blank_track = (libFLAC_func_FLAC__metadata_object_cuesheet_insert_blank_track_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_insert_blank_track" );
  if ( dll->FLAC__metadata_object_cuesheet_insert_blank_track == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_delete_track
  dll->FLAC__metadata_object_cuesheet_delete_track = (libFLAC_func_FLAC__metadata_object_cuesheet_delete_track_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_delete_track" );
  if ( dll->FLAC__metadata_object_cuesheet_delete_track == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_is_legal
  dll->FLAC__metadata_object_cuesheet_is_legal = (libFLAC_func_FLAC__metadata_object_cuesheet_is_legal_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__metadata_object_cuesheet_is_legal" );
  if ( dll->FLAC__metadata_object_cuesheet_is_legal == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__format_sample_rate_is_valid
  dll->FLAC__format_sample_rate_is_valid = (libFLAC_func_FLAC__format_sample_rate_is_valid_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__format_sample_rate_is_valid" );
  if ( dll->FLAC__format_sample_rate_is_valid == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__format_seektable_is_legal
  dll->FLAC__format_seektable_is_legal = (libFLAC_func_FLAC__format_seektable_is_legal_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__format_seektable_is_legal" );
  if ( dll->FLAC__format_seektable_is_legal == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__format_seektable_sort
  dll->FLAC__format_seektable_sort = (libFLAC_func_FLAC__format_seektable_sort_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__format_seektable_sort" );
  if ( dll->FLAC__format_seektable_sort == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__format_cuesheet_is_legal
  dll->FLAC__format_cuesheet_is_legal = (libFLAC_func_FLAC__format_cuesheet_is_legal_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__format_cuesheet_is_legal" );
  if ( dll->FLAC__format_cuesheet_is_legal == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_new
  dll->FLAC__file_encoder_new = (libFLAC_func_FLAC__file_encoder_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_new" );
  if ( dll->FLAC__file_encoder_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_delete
  dll->FLAC__file_encoder_delete = (libFLAC_func_FLAC__file_encoder_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_delete" );
  if ( dll->FLAC__file_encoder_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_verify
  dll->FLAC__file_encoder_set_verify = (libFLAC_func_FLAC__file_encoder_set_verify_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_verify" );
  if ( dll->FLAC__file_encoder_set_verify == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_streamable_subset
  dll->FLAC__file_encoder_set_streamable_subset = (libFLAC_func_FLAC__file_encoder_set_streamable_subset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_streamable_subset" );
  if ( dll->FLAC__file_encoder_set_streamable_subset == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_mid_side_stereo
  dll->FLAC__file_encoder_set_do_mid_side_stereo = (libFLAC_func_FLAC__file_encoder_set_do_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_do_mid_side_stereo" );
  if ( dll->FLAC__file_encoder_set_do_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_loose_mid_side_stereo
  dll->FLAC__file_encoder_set_loose_mid_side_stereo = (libFLAC_func_FLAC__file_encoder_set_loose_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_loose_mid_side_stereo" );
  if ( dll->FLAC__file_encoder_set_loose_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_channels
  dll->FLAC__file_encoder_set_channels = (libFLAC_func_FLAC__file_encoder_set_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_channels" );
  if ( dll->FLAC__file_encoder_set_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_bits_per_sample
  dll->FLAC__file_encoder_set_bits_per_sample = (libFLAC_func_FLAC__file_encoder_set_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_bits_per_sample" );
  if ( dll->FLAC__file_encoder_set_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_sample_rate
  dll->FLAC__file_encoder_set_sample_rate = (libFLAC_func_FLAC__file_encoder_set_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_sample_rate" );
  if ( dll->FLAC__file_encoder_set_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_blocksize
  dll->FLAC__file_encoder_set_blocksize = (libFLAC_func_FLAC__file_encoder_set_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_blocksize" );
  if ( dll->FLAC__file_encoder_set_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_max_lpc_order
  dll->FLAC__file_encoder_set_max_lpc_order = (libFLAC_func_FLAC__file_encoder_set_max_lpc_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_max_lpc_order" );
  if ( dll->FLAC__file_encoder_set_max_lpc_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_qlp_coeff_precision
  dll->FLAC__file_encoder_set_qlp_coeff_precision = (libFLAC_func_FLAC__file_encoder_set_qlp_coeff_precision_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_qlp_coeff_precision" );
  if ( dll->FLAC__file_encoder_set_qlp_coeff_precision == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_qlp_coeff_prec_search
  dll->FLAC__file_encoder_set_do_qlp_coeff_prec_search = (libFLAC_func_FLAC__file_encoder_set_do_qlp_coeff_prec_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_do_qlp_coeff_prec_search" );
  if ( dll->FLAC__file_encoder_set_do_qlp_coeff_prec_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_escape_coding
  dll->FLAC__file_encoder_set_do_escape_coding = (libFLAC_func_FLAC__file_encoder_set_do_escape_coding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_do_escape_coding" );
  if ( dll->FLAC__file_encoder_set_do_escape_coding == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_exhaustive_model_search
  dll->FLAC__file_encoder_set_do_exhaustive_model_search = (libFLAC_func_FLAC__file_encoder_set_do_exhaustive_model_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_do_exhaustive_model_search" );
  if ( dll->FLAC__file_encoder_set_do_exhaustive_model_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_min_residual_partition_order
  dll->FLAC__file_encoder_set_min_residual_partition_order = (libFLAC_func_FLAC__file_encoder_set_min_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_min_residual_partition_order" );
  if ( dll->FLAC__file_encoder_set_min_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_max_residual_partition_order
  dll->FLAC__file_encoder_set_max_residual_partition_order = (libFLAC_func_FLAC__file_encoder_set_max_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_max_residual_partition_order" );
  if ( dll->FLAC__file_encoder_set_max_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_rice_parameter_search_dist
  dll->FLAC__file_encoder_set_rice_parameter_search_dist = (libFLAC_func_FLAC__file_encoder_set_rice_parameter_search_dist_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_rice_parameter_search_dist" );
  if ( dll->FLAC__file_encoder_set_rice_parameter_search_dist == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_total_samples_estimate
  dll->FLAC__file_encoder_set_total_samples_estimate = (libFLAC_func_FLAC__file_encoder_set_total_samples_estimate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_total_samples_estimate" );
  if ( dll->FLAC__file_encoder_set_total_samples_estimate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_metadata
  dll->FLAC__file_encoder_set_metadata = (libFLAC_func_FLAC__file_encoder_set_metadata_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_metadata" );
  if ( dll->FLAC__file_encoder_set_metadata == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_filename
  dll->FLAC__file_encoder_set_filename = (libFLAC_func_FLAC__file_encoder_set_filename_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_filename" );
  if ( dll->FLAC__file_encoder_set_filename == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_progress_callback
  dll->FLAC__file_encoder_set_progress_callback = (libFLAC_func_FLAC__file_encoder_set_progress_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_progress_callback" );
  if ( dll->FLAC__file_encoder_set_progress_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_client_data
  dll->FLAC__file_encoder_set_client_data = (libFLAC_func_FLAC__file_encoder_set_client_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_set_client_data" );
  if ( dll->FLAC__file_encoder_set_client_data == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_state
  dll->FLAC__file_encoder_get_state = (libFLAC_func_FLAC__file_encoder_get_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_state" );
  if ( dll->FLAC__file_encoder_get_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_seekable_stream_encoder_state
  dll->FLAC__file_encoder_get_seekable_stream_encoder_state = (libFLAC_func_FLAC__file_encoder_get_seekable_stream_encoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_seekable_stream_encoder_state" );
  if ( dll->FLAC__file_encoder_get_seekable_stream_encoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_stream_encoder_state
  dll->FLAC__file_encoder_get_stream_encoder_state = (libFLAC_func_FLAC__file_encoder_get_stream_encoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_stream_encoder_state" );
  if ( dll->FLAC__file_encoder_get_stream_encoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_state
  dll->FLAC__file_encoder_get_verify_decoder_state = (libFLAC_func_FLAC__file_encoder_get_verify_decoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_verify_decoder_state" );
  if ( dll->FLAC__file_encoder_get_verify_decoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_resolved_state_string
  dll->FLAC__file_encoder_get_resolved_state_string = (libFLAC_func_FLAC__file_encoder_get_resolved_state_string_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_resolved_state_string" );
  if ( dll->FLAC__file_encoder_get_resolved_state_string == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_error_stats
  dll->FLAC__file_encoder_get_verify_decoder_error_stats = (libFLAC_func_FLAC__file_encoder_get_verify_decoder_error_stats_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_verify_decoder_error_stats" );
  if ( dll->FLAC__file_encoder_get_verify_decoder_error_stats == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify
  dll->FLAC__file_encoder_get_verify = (libFLAC_func_FLAC__file_encoder_get_verify_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_verify" );
  if ( dll->FLAC__file_encoder_get_verify == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_streamable_subset
  dll->FLAC__file_encoder_get_streamable_subset = (libFLAC_func_FLAC__file_encoder_get_streamable_subset_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_streamable_subset" );
  if ( dll->FLAC__file_encoder_get_streamable_subset == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_mid_side_stereo
  dll->FLAC__file_encoder_get_do_mid_side_stereo = (libFLAC_func_FLAC__file_encoder_get_do_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_do_mid_side_stereo" );
  if ( dll->FLAC__file_encoder_get_do_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_loose_mid_side_stereo
  dll->FLAC__file_encoder_get_loose_mid_side_stereo = (libFLAC_func_FLAC__file_encoder_get_loose_mid_side_stereo_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_loose_mid_side_stereo" );
  if ( dll->FLAC__file_encoder_get_loose_mid_side_stereo == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_channels
  dll->FLAC__file_encoder_get_channels = (libFLAC_func_FLAC__file_encoder_get_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_channels" );
  if ( dll->FLAC__file_encoder_get_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_bits_per_sample
  dll->FLAC__file_encoder_get_bits_per_sample = (libFLAC_func_FLAC__file_encoder_get_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_bits_per_sample" );
  if ( dll->FLAC__file_encoder_get_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_sample_rate
  dll->FLAC__file_encoder_get_sample_rate = (libFLAC_func_FLAC__file_encoder_get_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_sample_rate" );
  if ( dll->FLAC__file_encoder_get_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_blocksize
  dll->FLAC__file_encoder_get_blocksize = (libFLAC_func_FLAC__file_encoder_get_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_blocksize" );
  if ( dll->FLAC__file_encoder_get_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_max_lpc_order
  dll->FLAC__file_encoder_get_max_lpc_order = (libFLAC_func_FLAC__file_encoder_get_max_lpc_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_max_lpc_order" );
  if ( dll->FLAC__file_encoder_get_max_lpc_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_qlp_coeff_precision
  dll->FLAC__file_encoder_get_qlp_coeff_precision = (libFLAC_func_FLAC__file_encoder_get_qlp_coeff_precision_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_qlp_coeff_precision" );
  if ( dll->FLAC__file_encoder_get_qlp_coeff_precision == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_qlp_coeff_prec_search
  dll->FLAC__file_encoder_get_do_qlp_coeff_prec_search = (libFLAC_func_FLAC__file_encoder_get_do_qlp_coeff_prec_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_do_qlp_coeff_prec_search" );
  if ( dll->FLAC__file_encoder_get_do_qlp_coeff_prec_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_escape_coding
  dll->FLAC__file_encoder_get_do_escape_coding = (libFLAC_func_FLAC__file_encoder_get_do_escape_coding_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_do_escape_coding" );
  if ( dll->FLAC__file_encoder_get_do_escape_coding == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_exhaustive_model_search
  dll->FLAC__file_encoder_get_do_exhaustive_model_search = (libFLAC_func_FLAC__file_encoder_get_do_exhaustive_model_search_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_do_exhaustive_model_search" );
  if ( dll->FLAC__file_encoder_get_do_exhaustive_model_search == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_min_residual_partition_order
  dll->FLAC__file_encoder_get_min_residual_partition_order = (libFLAC_func_FLAC__file_encoder_get_min_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_min_residual_partition_order" );
  if ( dll->FLAC__file_encoder_get_min_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_max_residual_partition_order
  dll->FLAC__file_encoder_get_max_residual_partition_order = (libFLAC_func_FLAC__file_encoder_get_max_residual_partition_order_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_max_residual_partition_order" );
  if ( dll->FLAC__file_encoder_get_max_residual_partition_order == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_rice_parameter_search_dist
  dll->FLAC__file_encoder_get_rice_parameter_search_dist = (libFLAC_func_FLAC__file_encoder_get_rice_parameter_search_dist_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_rice_parameter_search_dist" );
  if ( dll->FLAC__file_encoder_get_rice_parameter_search_dist == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_total_samples_estimate
  dll->FLAC__file_encoder_get_total_samples_estimate = (libFLAC_func_FLAC__file_encoder_get_total_samples_estimate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_get_total_samples_estimate" );
  if ( dll->FLAC__file_encoder_get_total_samples_estimate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_init
  dll->FLAC__file_encoder_init = (libFLAC_func_FLAC__file_encoder_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_init" );
  if ( dll->FLAC__file_encoder_init == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_finish
  dll->FLAC__file_encoder_finish = (libFLAC_func_FLAC__file_encoder_finish_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_finish" );
  if ( dll->FLAC__file_encoder_finish == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_process
  dll->FLAC__file_encoder_process = (libFLAC_func_FLAC__file_encoder_process_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_process" );
  if ( dll->FLAC__file_encoder_process == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_process_interleaved
  dll->FLAC__file_encoder_process_interleaved = (libFLAC_func_FLAC__file_encoder_process_interleaved_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_encoder_process_interleaved" );
  if ( dll->FLAC__file_encoder_process_interleaved == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_new
  dll->FLAC__file_decoder_new = (libFLAC_func_FLAC__file_decoder_new_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_new" );
  if ( dll->FLAC__file_decoder_new == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_delete
  dll->FLAC__file_decoder_delete = (libFLAC_func_FLAC__file_decoder_delete_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_delete" );
  if ( dll->FLAC__file_decoder_delete == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_md5_checking
  dll->FLAC__file_decoder_set_md5_checking = (libFLAC_func_FLAC__file_decoder_set_md5_checking_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_md5_checking" );
  if ( dll->FLAC__file_decoder_set_md5_checking == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_filename
  dll->FLAC__file_decoder_set_filename = (libFLAC_func_FLAC__file_decoder_set_filename_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_filename" );
  if ( dll->FLAC__file_decoder_set_filename == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_write_callback
  dll->FLAC__file_decoder_set_write_callback = (libFLAC_func_FLAC__file_decoder_set_write_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_write_callback" );
  if ( dll->FLAC__file_decoder_set_write_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_callback
  dll->FLAC__file_decoder_set_metadata_callback = (libFLAC_func_FLAC__file_decoder_set_metadata_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_metadata_callback" );
  if ( dll->FLAC__file_decoder_set_metadata_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_error_callback
  dll->FLAC__file_decoder_set_error_callback = (libFLAC_func_FLAC__file_decoder_set_error_callback_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_error_callback" );
  if ( dll->FLAC__file_decoder_set_error_callback == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_client_data
  dll->FLAC__file_decoder_set_client_data = (libFLAC_func_FLAC__file_decoder_set_client_data_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_client_data" );
  if ( dll->FLAC__file_decoder_set_client_data == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond
  dll->FLAC__file_decoder_set_metadata_respond = (libFLAC_func_FLAC__file_decoder_set_metadata_respond_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_metadata_respond" );
  if ( dll->FLAC__file_decoder_set_metadata_respond == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_application
  dll->FLAC__file_decoder_set_metadata_respond_application = (libFLAC_func_FLAC__file_decoder_set_metadata_respond_application_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_metadata_respond_application" );
  if ( dll->FLAC__file_decoder_set_metadata_respond_application == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_all
  dll->FLAC__file_decoder_set_metadata_respond_all = (libFLAC_func_FLAC__file_decoder_set_metadata_respond_all_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_metadata_respond_all" );
  if ( dll->FLAC__file_decoder_set_metadata_respond_all == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore
  dll->FLAC__file_decoder_set_metadata_ignore = (libFLAC_func_FLAC__file_decoder_set_metadata_ignore_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_metadata_ignore" );
  if ( dll->FLAC__file_decoder_set_metadata_ignore == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_application
  dll->FLAC__file_decoder_set_metadata_ignore_application = (libFLAC_func_FLAC__file_decoder_set_metadata_ignore_application_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_metadata_ignore_application" );
  if ( dll->FLAC__file_decoder_set_metadata_ignore_application == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_all
  dll->FLAC__file_decoder_set_metadata_ignore_all = (libFLAC_func_FLAC__file_decoder_set_metadata_ignore_all_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_set_metadata_ignore_all" );
  if ( dll->FLAC__file_decoder_set_metadata_ignore_all == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_state
  dll->FLAC__file_decoder_get_state = (libFLAC_func_FLAC__file_decoder_get_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_state" );
  if ( dll->FLAC__file_decoder_get_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_seekable_stream_decoder_state
  dll->FLAC__file_decoder_get_seekable_stream_decoder_state = (libFLAC_func_FLAC__file_decoder_get_seekable_stream_decoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_seekable_stream_decoder_state" );
  if ( dll->FLAC__file_decoder_get_seekable_stream_decoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_stream_decoder_state
  dll->FLAC__file_decoder_get_stream_decoder_state = (libFLAC_func_FLAC__file_decoder_get_stream_decoder_state_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_stream_decoder_state" );
  if ( dll->FLAC__file_decoder_get_stream_decoder_state == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_resolved_state_string
  dll->FLAC__file_decoder_get_resolved_state_string = (libFLAC_func_FLAC__file_decoder_get_resolved_state_string_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_resolved_state_string" );
  if ( dll->FLAC__file_decoder_get_resolved_state_string == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_md5_checking
  dll->FLAC__file_decoder_get_md5_checking = (libFLAC_func_FLAC__file_decoder_get_md5_checking_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_md5_checking" );
  if ( dll->FLAC__file_decoder_get_md5_checking == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_channels
  dll->FLAC__file_decoder_get_channels = (libFLAC_func_FLAC__file_decoder_get_channels_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_channels" );
  if ( dll->FLAC__file_decoder_get_channels == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_channel_assignment
  dll->FLAC__file_decoder_get_channel_assignment = (libFLAC_func_FLAC__file_decoder_get_channel_assignment_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_channel_assignment" );
  if ( dll->FLAC__file_decoder_get_channel_assignment == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_bits_per_sample
  dll->FLAC__file_decoder_get_bits_per_sample = (libFLAC_func_FLAC__file_decoder_get_bits_per_sample_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_bits_per_sample" );
  if ( dll->FLAC__file_decoder_get_bits_per_sample == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_sample_rate
  dll->FLAC__file_decoder_get_sample_rate = (libFLAC_func_FLAC__file_decoder_get_sample_rate_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_sample_rate" );
  if ( dll->FLAC__file_decoder_get_sample_rate == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_blocksize
  dll->FLAC__file_decoder_get_blocksize = (libFLAC_func_FLAC__file_decoder_get_blocksize_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_blocksize" );
  if ( dll->FLAC__file_decoder_get_blocksize == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_decode_position
  dll->FLAC__file_decoder_get_decode_position = (libFLAC_func_FLAC__file_decoder_get_decode_position_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_get_decode_position" );
  if ( dll->FLAC__file_decoder_get_decode_position == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_init
  dll->FLAC__file_decoder_init = (libFLAC_func_FLAC__file_decoder_init_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_init" );
  if ( dll->FLAC__file_decoder_init == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_finish
  dll->FLAC__file_decoder_finish = (libFLAC_func_FLAC__file_decoder_finish_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_finish" );
  if ( dll->FLAC__file_decoder_finish == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_single
  dll->FLAC__file_decoder_process_single = (libFLAC_func_FLAC__file_decoder_process_single_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_process_single" );
  if ( dll->FLAC__file_decoder_process_single == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_metadata
  dll->FLAC__file_decoder_process_until_end_of_metadata = (libFLAC_func_FLAC__file_decoder_process_until_end_of_metadata_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_process_until_end_of_metadata" );
  if ( dll->FLAC__file_decoder_process_until_end_of_metadata == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_file
  dll->FLAC__file_decoder_process_until_end_of_file = (libFLAC_func_FLAC__file_decoder_process_until_end_of_file_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_process_until_end_of_file" );
  if ( dll->FLAC__file_decoder_process_until_end_of_file == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_seek_absolute
  dll->FLAC__file_decoder_seek_absolute = (libFLAC_func_FLAC__file_decoder_seek_absolute_t) GetProcAddress ( (HINSTANCE) dll->__h_dll, "FLAC__file_decoder_seek_absolute" );
  if ( dll->FLAC__file_decoder_seek_absolute == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderStateString
  dll->FLAC__StreamEncoderStateString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__StreamEncoderStateString" );
  if ( dll->FLAC__StreamEncoderStateString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderWriteStatusString
  dll->FLAC__StreamEncoderWriteStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__StreamEncoderWriteStatusString" );
  if ( dll->FLAC__StreamEncoderWriteStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderStateString
  dll->FLAC__StreamDecoderStateString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__StreamDecoderStateString" );
  if ( dll->FLAC__StreamDecoderStateString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderReadStatusString
  dll->FLAC__StreamDecoderReadStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__StreamDecoderReadStatusString" );
  if ( dll->FLAC__StreamDecoderReadStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderWriteStatusString
  dll->FLAC__StreamDecoderWriteStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__StreamDecoderWriteStatusString" );
  if ( dll->FLAC__StreamDecoderWriteStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderErrorStatusString
  dll->FLAC__StreamDecoderErrorStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__StreamDecoderErrorStatusString" );
  if ( dll->FLAC__StreamDecoderErrorStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderStateString
  dll->FLAC__SeekableStreamEncoderStateString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__SeekableStreamEncoderStateString" );
  if ( dll->FLAC__SeekableStreamEncoderStateString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString
  dll->FLAC__SeekableStreamEncoderSeekStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__SeekableStreamEncoderSeekStatusString" );
  if ( dll->FLAC__SeekableStreamEncoderSeekStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderStateString
  dll->FLAC__SeekableStreamDecoderStateString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__SeekableStreamDecoderStateString" );
  if ( dll->FLAC__SeekableStreamDecoderStateString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderReadStatusString
  dll->FLAC__SeekableStreamDecoderReadStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__SeekableStreamDecoderReadStatusString" );
  if ( dll->FLAC__SeekableStreamDecoderReadStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString
  dll->FLAC__SeekableStreamDecoderSeekStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__SeekableStreamDecoderSeekStatusString" );
  if ( dll->FLAC__SeekableStreamDecoderSeekStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderTellStatusString
  dll->FLAC__SeekableStreamDecoderTellStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__SeekableStreamDecoderTellStatusString" );
  if ( dll->FLAC__SeekableStreamDecoderTellStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString
  dll->FLAC__SeekableStreamDecoderLengthStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__SeekableStreamDecoderLengthStatusString" );
  if ( dll->FLAC__SeekableStreamDecoderLengthStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_SimpleIteratorStatusString
  dll->FLAC__Metadata_SimpleIteratorStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__Metadata_SimpleIteratorStatusString" );
  if ( dll->FLAC__Metadata_SimpleIteratorStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_ChainStatusString
  dll->FLAC__Metadata_ChainStatusString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__Metadata_ChainStatusString" );
  if ( dll->FLAC__Metadata_ChainStatusString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__VERSION_STRING
  dll->FLAC__VERSION_STRING = (FLAC_API const char * *) GetProcAddress ( dll->__h_dll, "FLAC__VERSION_STRING" );
  if ( dll->FLAC__VERSION_STRING == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__VENDOR_STRING
  dll->FLAC__VENDOR_STRING = (FLAC_API const char * *) GetProcAddress ( dll->__h_dll, "FLAC__VENDOR_STRING" );
  if ( dll->FLAC__VENDOR_STRING == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_STRING
  dll->FLAC__STREAM_SYNC_STRING = (FLAC_API const FLAC__byte* ) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_SYNC_STRING" );
  if ( dll->FLAC__STREAM_SYNC_STRING == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC
  dll->FLAC__STREAM_SYNC = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_SYNC" );
  if ( dll->FLAC__STREAM_SYNC == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_LEN
  dll->FLAC__STREAM_SYNC_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_SYNC_LEN" );
  if ( dll->FLAC__STREAM_SYNC_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__EntropyCodingMethodTypeString
  dll->FLAC__EntropyCodingMethodTypeString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__EntropyCodingMethodTypeString" );
  if ( dll->FLAC__EntropyCodingMethodTypeString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN
  dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN" );
  if ( dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN
  dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN" );
  if ( dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN
  dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN" );
  if ( dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER
  dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER" );
  if ( dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN
  dll->FLAC__ENTROPY_CODING_METHOD_TYPE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__ENTROPY_CODING_METHOD_TYPE_LEN" );
  if ( dll->FLAC__ENTROPY_CODING_METHOD_TYPE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SubframeTypeString
  dll->FLAC__SubframeTypeString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__SubframeTypeString" );
  if ( dll->FLAC__SubframeTypeString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN
  dll->FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN" );
  if ( dll->FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN
  dll->FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN" );
  if ( dll->FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN
  dll->FLAC__SUBFRAME_ZERO_PAD_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_ZERO_PAD_LEN" );
  if ( dll->FLAC__SUBFRAME_ZERO_PAD_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LEN
  dll->FLAC__SUBFRAME_TYPE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_TYPE_LEN" );
  if ( dll->FLAC__SUBFRAME_TYPE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN
  dll->FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN" );
  if ( dll->FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK
  dll->FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK" );
  if ( dll->FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK
  dll->FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK" );
  if ( dll->FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK
  dll->FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK" );
  if ( dll->FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK
  dll->FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK" );
  if ( dll->FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__ChannelAssignmentString
  dll->FLAC__ChannelAssignmentString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__ChannelAssignmentString" );
  if ( dll->FLAC__ChannelAssignmentString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FrameNumberTypeString
  dll->FLAC__FrameNumberTypeString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__FrameNumberTypeString" );
  if ( dll->FLAC__FrameNumberTypeString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC
  dll->FLAC__FRAME_HEADER_SYNC = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_SYNC" );
  if ( dll->FLAC__FRAME_HEADER_SYNC == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN
  dll->FLAC__FRAME_HEADER_SYNC_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_SYNC_LEN" );
  if ( dll->FLAC__FRAME_HEADER_SYNC_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN
  dll->FLAC__FRAME_HEADER_RESERVED_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_RESERVED_LEN" );
  if ( dll->FLAC__FRAME_HEADER_RESERVED_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN
  dll->FLAC__FRAME_HEADER_BLOCK_SIZE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_BLOCK_SIZE_LEN" );
  if ( dll->FLAC__FRAME_HEADER_BLOCK_SIZE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN
  dll->FLAC__FRAME_HEADER_SAMPLE_RATE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_SAMPLE_RATE_LEN" );
  if ( dll->FLAC__FRAME_HEADER_SAMPLE_RATE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN
  dll->FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN" );
  if ( dll->FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN
  dll->FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN" );
  if ( dll->FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN
  dll->FLAC__FRAME_HEADER_ZERO_PAD_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_ZERO_PAD_LEN" );
  if ( dll->FLAC__FRAME_HEADER_ZERO_PAD_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CRC_LEN
  dll->FLAC__FRAME_HEADER_CRC_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_HEADER_CRC_LEN" );
  if ( dll->FLAC__FRAME_HEADER_CRC_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN
  dll->FLAC__FRAME_FOOTER_CRC_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__FRAME_FOOTER_CRC_LEN" );
  if ( dll->FLAC__FRAME_FOOTER_CRC_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__MetadataTypeString
  dll->FLAC__MetadataTypeString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__MetadataTypeString" );
  if ( dll->FLAC__MetadataTypeString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN
  dll->FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN" );
  if ( dll->FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN
  dll->FLAC__STREAM_METADATA_APPLICATION_ID_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_APPLICATION_ID_LEN" );
  if ( dll->FLAC__STREAM_METADATA_APPLICATION_ID_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN
  dll->FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN" );
  if ( dll->FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN
  dll->FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN" );
  if ( dll->FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN
  dll->FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN" );
  if ( dll->FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER
  dll->FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER = (FLAC_API const FLAC__uint64 *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER" );
  if ( dll->FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN
  dll->FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN" );
  if ( dll->FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN
  dll->FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN" );
  if ( dll->FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
  dll->FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN" );
  if ( dll->FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN
  dll->FLAC__STREAM_METADATA_IS_LAST_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_IS_LAST_LEN" );
  if ( dll->FLAC__STREAM_METADATA_IS_LAST_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN
  dll->FLAC__STREAM_METADATA_TYPE_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_TYPE_LEN" );
  if ( dll->FLAC__STREAM_METADATA_TYPE_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN
  dll->FLAC__STREAM_METADATA_LENGTH_LEN = (FLAC_API const unsigned *) GetProcAddress ( dll->__h_dll, "FLAC__STREAM_METADATA_LENGTH_LEN" );
  if ( dll->FLAC__STREAM_METADATA_LENGTH_LEN == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FileEncoderStateString
  dll->FLAC__FileEncoderStateString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__FileEncoderStateString" );
  if ( dll->FLAC__FileEncoderStateString == NULL ) err++;
#endif
#ifndef IGNORE_libFLAC_FLAC__FileDecoderStateString
  dll->FLAC__FileDecoderStateString = (FLAC_API const char * const* ) GetProcAddress ( dll->__h_dll, "FLAC__FileDecoderStateString" );
  if ( dll->FLAC__FileDecoderStateString == NULL ) err++;
#endif
  if ( err > 0 ) { free ( dll ); return NULL; }
  return dll;
}

void free_libFLAC_dll ( libFLAC_dll_t *dll )
{
  FreeLibrary ( (HMODULE) dll->__h_dll );
	free ( dll );
}


#ifndef IGNORE_libFLAC_FLAC__StreamEncoderStateString
FLAC_API const char * const* * g_libFLAC_FLAC__StreamEncoderStateString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderWriteStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__StreamEncoderWriteStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderStateString
FLAC_API const char * const* * g_libFLAC_FLAC__StreamDecoderStateString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderReadStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__StreamDecoderReadStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderWriteStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__StreamDecoderWriteStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderErrorStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__StreamDecoderErrorStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderStateString
FLAC_API const char * const* * g_libFLAC_FLAC__SeekableStreamEncoderStateString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderStateString
FLAC_API const char * const* * g_libFLAC_FLAC__SeekableStreamDecoderStateString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderReadStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__SeekableStreamDecoderReadStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderTellStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__SeekableStreamDecoderTellStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_SimpleIteratorStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__Metadata_SimpleIteratorStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_ChainStatusString
FLAC_API const char * const* * g_libFLAC_FLAC__Metadata_ChainStatusString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__VERSION_STRING
FLAC_API const char * * g_libFLAC_FLAC__VERSION_STRING = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__VENDOR_STRING
FLAC_API const char * * g_libFLAC_FLAC__VENDOR_STRING = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_STRING
FLAC_API const FLAC__byte* * g_libFLAC_FLAC__STREAM_SYNC_STRING = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_SYNC = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_SYNC_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__EntropyCodingMethodTypeString
FLAC_API const char * const* * g_libFLAC_FLAC__EntropyCodingMethodTypeString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER
FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SubframeTypeString
FLAC_API const char * const* * g_libFLAC_FLAC__SubframeTypeString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK
FLAC_API const unsigned * g_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__ChannelAssignmentString
FLAC_API const char * const* * g_libFLAC_FLAC__ChannelAssignmentString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FrameNumberTypeString
FLAC_API const char * const* * g_libFLAC_FLAC__FrameNumberTypeString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_SYNC = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CRC_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_HEADER_CRC_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__MetadataTypeString
FLAC_API const char * const* * g_libFLAC_FLAC__MetadataTypeString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER
FLAC_API const FLAC__uint64 * g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN
FLAC_API const unsigned * g_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FileEncoderStateString
FLAC_API const char * const* * g_libFLAC_FLAC__FileEncoderStateString = NULL;
#endif
#ifndef IGNORE_libFLAC_FLAC__FileDecoderStateString
FLAC_API const char * const* * g_libFLAC_FLAC__FileDecoderStateString = NULL;
#endif

static libFLAC_dll_t* volatile g_libFLAC_dll = NULL;
int g_load_libFLAC_dll ( char *path )
{
	if ( g_libFLAC_dll != NULL ) return 0;
	g_libFLAC_dll = load_libFLAC_dll ( path );
	if ( g_libFLAC_dll == NULL ) return -1;
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderStateString
	g_libFLAC_FLAC__StreamEncoderStateString = g_libFLAC_dll->FLAC__StreamEncoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderWriteStatusString
	g_libFLAC_FLAC__StreamEncoderWriteStatusString = g_libFLAC_dll->FLAC__StreamEncoderWriteStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderStateString
	g_libFLAC_FLAC__StreamDecoderStateString = g_libFLAC_dll->FLAC__StreamDecoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderReadStatusString
	g_libFLAC_FLAC__StreamDecoderReadStatusString = g_libFLAC_dll->FLAC__StreamDecoderReadStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderWriteStatusString
	g_libFLAC_FLAC__StreamDecoderWriteStatusString = g_libFLAC_dll->FLAC__StreamDecoderWriteStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderErrorStatusString
	g_libFLAC_FLAC__StreamDecoderErrorStatusString = g_libFLAC_dll->FLAC__StreamDecoderErrorStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderStateString
	g_libFLAC_FLAC__SeekableStreamEncoderStateString = g_libFLAC_dll->FLAC__SeekableStreamEncoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString
	g_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString = g_libFLAC_dll->FLAC__SeekableStreamEncoderSeekStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderStateString
	g_libFLAC_FLAC__SeekableStreamDecoderStateString = g_libFLAC_dll->FLAC__SeekableStreamDecoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderReadStatusString
	g_libFLAC_FLAC__SeekableStreamDecoderReadStatusString = g_libFLAC_dll->FLAC__SeekableStreamDecoderReadStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString
	g_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString = g_libFLAC_dll->FLAC__SeekableStreamDecoderSeekStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderTellStatusString
	g_libFLAC_FLAC__SeekableStreamDecoderTellStatusString = g_libFLAC_dll->FLAC__SeekableStreamDecoderTellStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString
	g_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString = g_libFLAC_dll->FLAC__SeekableStreamDecoderLengthStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_SimpleIteratorStatusString
	g_libFLAC_FLAC__Metadata_SimpleIteratorStatusString = g_libFLAC_dll->FLAC__Metadata_SimpleIteratorStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_ChainStatusString
	g_libFLAC_FLAC__Metadata_ChainStatusString = g_libFLAC_dll->FLAC__Metadata_ChainStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__VERSION_STRING
	g_libFLAC_FLAC__VERSION_STRING = g_libFLAC_dll->FLAC__VERSION_STRING;
#endif
#ifndef IGNORE_libFLAC_FLAC__VENDOR_STRING
	g_libFLAC_FLAC__VENDOR_STRING = g_libFLAC_dll->FLAC__VENDOR_STRING;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_STRING
	g_libFLAC_FLAC__STREAM_SYNC_STRING = g_libFLAC_dll->FLAC__STREAM_SYNC_STRING;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC
	g_libFLAC_FLAC__STREAM_SYNC = g_libFLAC_dll->FLAC__STREAM_SYNC;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_LEN
	g_libFLAC_FLAC__STREAM_SYNC_LEN = g_libFLAC_dll->FLAC__STREAM_SYNC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__EntropyCodingMethodTypeString
	g_libFLAC_FLAC__EntropyCodingMethodTypeString = g_libFLAC_dll->FLAC__EntropyCodingMethodTypeString;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN
	g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN = g_libFLAC_dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN
	g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN = g_libFLAC_dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN
	g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN = g_libFLAC_dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER
	g_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER = g_libFLAC_dll->FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN
	g_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN = g_libFLAC_dll->FLAC__ENTROPY_CODING_METHOD_TYPE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SubframeTypeString
	g_libFLAC_FLAC__SubframeTypeString = g_libFLAC_dll->FLAC__SubframeTypeString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN
	g_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN = g_libFLAC_dll->FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN
	g_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN = g_libFLAC_dll->FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN
	g_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN = g_libFLAC_dll->FLAC__SUBFRAME_ZERO_PAD_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LEN
	g_libFLAC_FLAC__SUBFRAME_TYPE_LEN = g_libFLAC_dll->FLAC__SUBFRAME_TYPE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN
	g_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN = g_libFLAC_dll->FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK
	g_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK = g_libFLAC_dll->FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK
	g_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK = g_libFLAC_dll->FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK
	g_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK = g_libFLAC_dll->FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK
	g_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK = g_libFLAC_dll->FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK;
#endif
#ifndef IGNORE_libFLAC_FLAC__ChannelAssignmentString
	g_libFLAC_FLAC__ChannelAssignmentString = g_libFLAC_dll->FLAC__ChannelAssignmentString;
#endif
#ifndef IGNORE_libFLAC_FLAC__FrameNumberTypeString
	g_libFLAC_FLAC__FrameNumberTypeString = g_libFLAC_dll->FLAC__FrameNumberTypeString;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC
	g_libFLAC_FLAC__FRAME_HEADER_SYNC = g_libFLAC_dll->FLAC__FRAME_HEADER_SYNC;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN
	g_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN = g_libFLAC_dll->FLAC__FRAME_HEADER_SYNC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN
	g_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN = g_libFLAC_dll->FLAC__FRAME_HEADER_RESERVED_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN
	g_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN = g_libFLAC_dll->FLAC__FRAME_HEADER_BLOCK_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN
	g_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN = g_libFLAC_dll->FLAC__FRAME_HEADER_SAMPLE_RATE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN
	g_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN = g_libFLAC_dll->FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN
	g_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN = g_libFLAC_dll->FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN
	g_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN = g_libFLAC_dll->FLAC__FRAME_HEADER_ZERO_PAD_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CRC_LEN
	g_libFLAC_FLAC__FRAME_HEADER_CRC_LEN = g_libFLAC_dll->FLAC__FRAME_HEADER_CRC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN
	g_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN = g_libFLAC_dll->FLAC__FRAME_FOOTER_CRC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__MetadataTypeString
	g_libFLAC_FLAC__MetadataTypeString = g_libFLAC_dll->FLAC__MetadataTypeString;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN
	g_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN
	g_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_APPLICATION_ID_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN
	g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN
	g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN
	g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER
	g_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER = g_libFLAC_dll->FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN
	g_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN
	g_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
	g_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN
	g_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_IS_LAST_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN
	g_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_TYPE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN
	g_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN = g_libFLAC_dll->FLAC__STREAM_METADATA_LENGTH_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FileEncoderStateString
	g_libFLAC_FLAC__FileEncoderStateString = g_libFLAC_dll->FLAC__FileEncoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__FileDecoderStateString
	g_libFLAC_FLAC__FileDecoderStateString = g_libFLAC_dll->FLAC__FileDecoderStateString;
#endif
	return 0;
}
void g_free_libFLAC_dll ( void )
{
	if ( g_libFLAC_dll != NULL ) {
		free_libFLAC_dll ( g_libFLAC_dll );
		g_libFLAC_dll = NULL;
	}
}

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_new
FLAC_API FLAC__StreamEncoder * FLAC__stream_encoder_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_new))();
	}
	return (FLAC_API FLAC__StreamEncoder *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_delete
FLAC_API void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__stream_encoder_delete))(encoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_verify
FLAC_API FLAC__bool FLAC__stream_encoder_set_verify(FLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_verify))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_streamable_subset
FLAC_API FLAC__bool FLAC__stream_encoder_set_streamable_subset(FLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_streamable_subset))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_mid_side_stereo
FLAC_API FLAC__bool FLAC__stream_encoder_set_do_mid_side_stereo(FLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_do_mid_side_stereo))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_loose_mid_side_stereo
FLAC_API FLAC__bool FLAC__stream_encoder_set_loose_mid_side_stereo(FLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_loose_mid_side_stereo))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_channels
FLAC_API FLAC__bool FLAC__stream_encoder_set_channels(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_channels))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_bits_per_sample
FLAC_API FLAC__bool FLAC__stream_encoder_set_bits_per_sample(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_bits_per_sample))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_sample_rate
FLAC_API FLAC__bool FLAC__stream_encoder_set_sample_rate(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_sample_rate))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_blocksize
FLAC_API FLAC__bool FLAC__stream_encoder_set_blocksize(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_blocksize))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_max_lpc_order
FLAC_API FLAC__bool FLAC__stream_encoder_set_max_lpc_order(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_max_lpc_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_qlp_coeff_precision
FLAC_API FLAC__bool FLAC__stream_encoder_set_qlp_coeff_precision(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_qlp_coeff_precision))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_qlp_coeff_prec_search
FLAC_API FLAC__bool FLAC__stream_encoder_set_do_qlp_coeff_prec_search(FLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_do_qlp_coeff_prec_search))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_escape_coding
FLAC_API FLAC__bool FLAC__stream_encoder_set_do_escape_coding(FLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_do_escape_coding))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_exhaustive_model_search
FLAC_API FLAC__bool FLAC__stream_encoder_set_do_exhaustive_model_search(FLAC__StreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_do_exhaustive_model_search))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_min_residual_partition_order
FLAC_API FLAC__bool FLAC__stream_encoder_set_min_residual_partition_order(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_min_residual_partition_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_max_residual_partition_order
FLAC_API FLAC__bool FLAC__stream_encoder_set_max_residual_partition_order(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_max_residual_partition_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_rice_parameter_search_dist
FLAC_API FLAC__bool FLAC__stream_encoder_set_rice_parameter_search_dist(FLAC__StreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_rice_parameter_search_dist))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_total_samples_estimate
FLAC_API FLAC__bool FLAC__stream_encoder_set_total_samples_estimate(FLAC__StreamEncoder *encoder, FLAC__uint64 value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_total_samples_estimate))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_metadata
FLAC_API FLAC__bool FLAC__stream_encoder_set_metadata(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_metadata))(encoder,metadata,num_blocks);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_write_callback
FLAC_API FLAC__bool FLAC__stream_encoder_set_write_callback(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_write_callback))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_metadata_callback
FLAC_API FLAC__bool FLAC__stream_encoder_set_metadata_callback(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderMetadataCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_metadata_callback))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_client_data
FLAC_API FLAC__bool FLAC__stream_encoder_set_client_data(FLAC__StreamEncoder *encoder, void *value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_set_client_data))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_state
FLAC_API FLAC__StreamEncoderState FLAC__stream_encoder_get_state(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_state))(encoder);
	}
	return (FLAC_API FLAC__StreamEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_state
FLAC_API FLAC__StreamDecoderState FLAC__stream_encoder_get_verify_decoder_state(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_verify_decoder_state))(encoder);
	}
	return (FLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_resolved_state_string
FLAC_API const char * FLAC__stream_encoder_get_resolved_state_string(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_resolved_state_string))(encoder);
	}
	return (FLAC_API const char *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_error_stats
FLAC_API void FLAC__stream_encoder_get_verify_decoder_error_stats(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__stream_encoder_get_verify_decoder_error_stats))(encoder,absolute_sample,frame_number,channel,sample,expected,got);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify
FLAC_API FLAC__bool FLAC__stream_encoder_get_verify(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_verify))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_streamable_subset
FLAC_API FLAC__bool FLAC__stream_encoder_get_streamable_subset(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_streamable_subset))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_mid_side_stereo
FLAC_API FLAC__bool FLAC__stream_encoder_get_do_mid_side_stereo(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_do_mid_side_stereo))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_loose_mid_side_stereo
FLAC_API FLAC__bool FLAC__stream_encoder_get_loose_mid_side_stereo(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_loose_mid_side_stereo))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_channels
FLAC_API unsigned FLAC__stream_encoder_get_channels(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_channels))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_bits_per_sample
FLAC_API unsigned FLAC__stream_encoder_get_bits_per_sample(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_bits_per_sample))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_sample_rate
FLAC_API unsigned FLAC__stream_encoder_get_sample_rate(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_sample_rate))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_blocksize
FLAC_API unsigned FLAC__stream_encoder_get_blocksize(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_blocksize))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_max_lpc_order
FLAC_API unsigned FLAC__stream_encoder_get_max_lpc_order(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_max_lpc_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_qlp_coeff_precision
FLAC_API unsigned FLAC__stream_encoder_get_qlp_coeff_precision(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_qlp_coeff_precision))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_qlp_coeff_prec_search
FLAC_API FLAC__bool FLAC__stream_encoder_get_do_qlp_coeff_prec_search(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_do_qlp_coeff_prec_search))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_escape_coding
FLAC_API FLAC__bool FLAC__stream_encoder_get_do_escape_coding(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_do_escape_coding))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_exhaustive_model_search
FLAC_API FLAC__bool FLAC__stream_encoder_get_do_exhaustive_model_search(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_do_exhaustive_model_search))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_min_residual_partition_order
FLAC_API unsigned FLAC__stream_encoder_get_min_residual_partition_order(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_min_residual_partition_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_max_residual_partition_order
FLAC_API unsigned FLAC__stream_encoder_get_max_residual_partition_order(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_max_residual_partition_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_rice_parameter_search_dist
FLAC_API unsigned FLAC__stream_encoder_get_rice_parameter_search_dist(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_rice_parameter_search_dist))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_total_samples_estimate
FLAC_API FLAC__uint64 FLAC__stream_encoder_get_total_samples_estimate(const FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_get_total_samples_estimate))(encoder);
	}
	return (FLAC_API FLAC__uint64)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_init
FLAC_API FLAC__StreamEncoderState FLAC__stream_encoder_init(FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_init))(encoder);
	}
	return (FLAC_API FLAC__StreamEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_finish
FLAC_API void FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__stream_encoder_finish))(encoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_process
FLAC_API FLAC__bool FLAC__stream_encoder_process(FLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_process))(encoder,buffer,samples);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_encoder_process_interleaved
FLAC_API FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_encoder_process_interleaved))(encoder,buffer,samples);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_new
FLAC_API FLAC__StreamDecoder * FLAC__stream_decoder_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_new))();
	}
	return (FLAC_API FLAC__StreamDecoder *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_delete
FLAC_API void FLAC__stream_decoder_delete(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__stream_decoder_delete))(decoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_read_callback
FLAC_API FLAC__bool FLAC__stream_decoder_set_read_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_read_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_write_callback
FLAC_API FLAC__bool FLAC__stream_decoder_set_write_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderWriteCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_write_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_callback
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderMetadataCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_metadata_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_error_callback
FLAC_API FLAC__bool FLAC__stream_decoder_set_error_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_error_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_client_data
FLAC_API FLAC__bool FLAC__stream_decoder_set_client_data(FLAC__StreamDecoder *decoder, void *value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_client_data))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_respond(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_metadata_respond))(decoder,type);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_application
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_respond_application(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_metadata_respond_application))(decoder,id);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_all
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_respond_all(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_metadata_respond_all))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_ignore(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_metadata_ignore))(decoder,type);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_application
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_ignore_application(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_metadata_ignore_application))(decoder,id);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_all
FLAC_API FLAC__bool FLAC__stream_decoder_set_metadata_ignore_all(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_set_metadata_ignore_all))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_state
FLAC_API FLAC__StreamDecoderState FLAC__stream_decoder_get_state(const FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_get_state))(decoder);
	}
	return (FLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_channels
FLAC_API unsigned FLAC__stream_decoder_get_channels(const FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_get_channels))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_channel_assignment
FLAC_API FLAC__ChannelAssignment FLAC__stream_decoder_get_channel_assignment(const FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_get_channel_assignment))(decoder);
	}
	return (FLAC_API FLAC__ChannelAssignment)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_bits_per_sample
FLAC_API unsigned FLAC__stream_decoder_get_bits_per_sample(const FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_get_bits_per_sample))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_sample_rate
FLAC_API unsigned FLAC__stream_decoder_get_sample_rate(const FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_get_sample_rate))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_blocksize
FLAC_API unsigned FLAC__stream_decoder_get_blocksize(const FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_get_blocksize))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_init
FLAC_API FLAC__StreamDecoderState FLAC__stream_decoder_init(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_init))(decoder);
	}
	return (FLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_finish
FLAC_API void FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__stream_decoder_finish))(decoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_flush
FLAC_API FLAC__bool FLAC__stream_decoder_flush(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_flush))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_reset
FLAC_API FLAC__bool FLAC__stream_decoder_reset(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_reset))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_single
FLAC_API FLAC__bool FLAC__stream_decoder_process_single(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_process_single))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_metadata
FLAC_API FLAC__bool FLAC__stream_decoder_process_until_end_of_metadata(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_process_until_end_of_metadata))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_stream
FLAC_API FLAC__bool FLAC__stream_decoder_process_until_end_of_stream(FLAC__StreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__stream_decoder_process_until_end_of_stream))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_new
FLAC_API FLAC__SeekableStreamEncoder * FLAC__seekable_stream_encoder_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_new))();
	}
	return (FLAC_API FLAC__SeekableStreamEncoder *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_delete
FLAC_API void FLAC__seekable_stream_encoder_delete(FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__seekable_stream_encoder_delete))(encoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_verify
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_verify(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_verify))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_streamable_subset
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_streamable_subset(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_streamable_subset))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_mid_side_stereo
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_do_mid_side_stereo))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_loose_mid_side_stereo
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_loose_mid_side_stereo))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_channels
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_channels(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_channels))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_bits_per_sample
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_bits_per_sample(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_bits_per_sample))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_sample_rate
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_sample_rate(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_sample_rate))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_blocksize
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_blocksize(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_blocksize))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_lpc_order
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_max_lpc_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_max_lpc_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_qlp_coeff_precision
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_qlp_coeff_precision(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_qlp_coeff_precision))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_escape_coding
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_escape_coding(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_do_escape_coding))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_exhaustive_model_search
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_exhaustive_model_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_do_exhaustive_model_search))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_min_residual_partition_order
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_min_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_min_residual_partition_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_residual_partition_order
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_max_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_max_residual_partition_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_rice_parameter_search_dist
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_rice_parameter_search_dist(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_rice_parameter_search_dist))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_total_samples_estimate
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_total_samples_estimate(FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_total_samples_estimate))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_metadata
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_metadata(FLAC__SeekableStreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_metadata))(encoder,metadata,num_blocks);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_seek_callback
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_seek_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderSeekCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_seek_callback))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_write_callback
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_write_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderWriteCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_write_callback))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_client_data
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_client_data(FLAC__SeekableStreamEncoder *encoder, void *value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_set_client_data))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_state
FLAC_API FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_get_state(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_state))(encoder);
	}
	return (FLAC_API FLAC__SeekableStreamEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_stream_encoder_state
FLAC_API FLAC__StreamEncoderState FLAC__seekable_stream_encoder_get_stream_encoder_state(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_stream_encoder_state))(encoder);
	}
	return (FLAC_API FLAC__StreamEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_state
FLAC_API FLAC__StreamDecoderState FLAC__seekable_stream_encoder_get_verify_decoder_state(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_verify_decoder_state))(encoder);
	}
	return (FLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_resolved_state_string
FLAC_API const char * FLAC__seekable_stream_encoder_get_resolved_state_string(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_resolved_state_string))(encoder);
	}
	return (FLAC_API const char *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_error_stats
FLAC_API void FLAC__seekable_stream_encoder_get_verify_decoder_error_stats(const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_verify_decoder_error_stats))(encoder,absolute_sample,frame_number,channel,sample,expected,got);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_verify(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_verify))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_streamable_subset
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_streamable_subset(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_streamable_subset))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_mid_side_stereo
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_do_mid_side_stereo))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_loose_mid_side_stereo
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_loose_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_loose_mid_side_stereo))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_channels
FLAC_API unsigned FLAC__seekable_stream_encoder_get_channels(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_channels))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_bits_per_sample
FLAC_API unsigned FLAC__seekable_stream_encoder_get_bits_per_sample(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_bits_per_sample))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_sample_rate
FLAC_API unsigned FLAC__seekable_stream_encoder_get_sample_rate(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_sample_rate))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_blocksize
FLAC_API unsigned FLAC__seekable_stream_encoder_get_blocksize(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_blocksize))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_lpc_order
FLAC_API unsigned FLAC__seekable_stream_encoder_get_max_lpc_order(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_max_lpc_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_qlp_coeff_precision
FLAC_API unsigned FLAC__seekable_stream_encoder_get_qlp_coeff_precision(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_qlp_coeff_precision))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_escape_coding
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_escape_coding(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_do_escape_coding))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_exhaustive_model_search
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_exhaustive_model_search(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_do_exhaustive_model_search))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_min_residual_partition_order
FLAC_API unsigned FLAC__seekable_stream_encoder_get_min_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_min_residual_partition_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_residual_partition_order
FLAC_API unsigned FLAC__seekable_stream_encoder_get_max_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_max_residual_partition_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_rice_parameter_search_dist
FLAC_API unsigned FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_rice_parameter_search_dist))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_total_samples_estimate
FLAC_API FLAC__uint64 FLAC__seekable_stream_encoder_get_total_samples_estimate(const FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_get_total_samples_estimate))(encoder);
	}
	return (FLAC_API FLAC__uint64)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_init
FLAC_API FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_init(FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_init))(encoder);
	}
	return (FLAC_API FLAC__SeekableStreamEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_finish
FLAC_API void FLAC__seekable_stream_encoder_finish(FLAC__SeekableStreamEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__seekable_stream_encoder_finish))(encoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_process
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_process(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_process))(encoder,buffer,samples);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_process_interleaved
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_process_interleaved(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_encoder_process_interleaved))(encoder,buffer,samples);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_new
FLAC_API FLAC__SeekableStreamDecoder * FLAC__seekable_stream_decoder_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_new))();
	}
	return (FLAC_API FLAC__SeekableStreamDecoder *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_delete
FLAC_API void FLAC__seekable_stream_decoder_delete(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__seekable_stream_decoder_delete))(decoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_md5_checking
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_md5_checking(FLAC__SeekableStreamDecoder *decoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_md5_checking))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_read_callback
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_read_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_read_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_seek_callback
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_seek_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_seek_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_tell_callback
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_tell_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_tell_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_length_callback
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_length_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_length_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_eof_callback
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_eof_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderEofCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_eof_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_write_callback
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_write_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderWriteCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_write_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_callback
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderMetadataCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_metadata_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_error_callback
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_error_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderErrorCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_error_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_client_data
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_client_data(FLAC__SeekableStreamDecoder *decoder, void *value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_client_data))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_metadata_respond))(decoder,type);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_application
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4])
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_metadata_respond_application))(decoder,id);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_all
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_all(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_metadata_respond_all))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_metadata_ignore))(decoder,type);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_application
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4])
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_metadata_ignore_application))(decoder,id);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_all
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_all(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_set_metadata_ignore_all))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_state
FLAC_API FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_get_state(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_state))(decoder);
	}
	return (FLAC_API FLAC__SeekableStreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_stream_decoder_state
FLAC_API FLAC__StreamDecoderState FLAC__seekable_stream_decoder_get_stream_decoder_state(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_stream_decoder_state))(decoder);
	}
	return (FLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_resolved_state_string
FLAC_API const char * FLAC__seekable_stream_decoder_get_resolved_state_string(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_resolved_state_string))(decoder);
	}
	return (FLAC_API const char *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_md5_checking
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_get_md5_checking(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_md5_checking))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channels
FLAC_API unsigned FLAC__seekable_stream_decoder_get_channels(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_channels))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channel_assignment
FLAC_API FLAC__ChannelAssignment FLAC__seekable_stream_decoder_get_channel_assignment(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_channel_assignment))(decoder);
	}
	return (FLAC_API FLAC__ChannelAssignment)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_bits_per_sample
FLAC_API unsigned FLAC__seekable_stream_decoder_get_bits_per_sample(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_bits_per_sample))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_sample_rate
FLAC_API unsigned FLAC__seekable_stream_decoder_get_sample_rate(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_sample_rate))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_blocksize
FLAC_API unsigned FLAC__seekable_stream_decoder_get_blocksize(const FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_blocksize))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_decode_position
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_get_decode_position(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *position)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_get_decode_position))(decoder,position);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_init
FLAC_API FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_init(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_init))(decoder);
	}
	return (FLAC_API FLAC__SeekableStreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_finish
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_finish(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_finish))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_flush
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_flush(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_flush))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_reset
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_reset(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_reset))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_single
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_process_single(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_process_single))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_metadata
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_process_until_end_of_metadata(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_process_until_end_of_metadata))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_stream
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_process_until_end_of_stream(FLAC__SeekableStreamDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_process_until_end_of_stream))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_seek_absolute
FLAC_API FLAC__bool FLAC__seekable_stream_decoder_seek_absolute(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__seekable_stream_decoder_seek_absolute))(decoder,sample);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_get_streaminfo
FLAC_API FLAC__bool FLAC__metadata_get_streaminfo(const char *filename, FLAC__StreamMetadata *streaminfo)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_get_streaminfo))(filename,streaminfo);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_new
FLAC_API FLAC__Metadata_SimpleIterator * FLAC__metadata_simple_iterator_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_new))();
	}
	return (FLAC_API FLAC__Metadata_SimpleIterator *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete
FLAC_API void FLAC__metadata_simple_iterator_delete(FLAC__Metadata_SimpleIterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_simple_iterator_delete))(iterator);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_status
FLAC_API FLAC__Metadata_SimpleIteratorStatus FLAC__metadata_simple_iterator_status(FLAC__Metadata_SimpleIterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_status))(iterator);
	}
	return (FLAC_API FLAC__Metadata_SimpleIteratorStatus)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_init
FLAC_API FLAC__bool FLAC__metadata_simple_iterator_init(FLAC__Metadata_SimpleIterator *iterator, const char *filename, FLAC__bool read_only, FLAC__bool preserve_file_stats)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_init))(iterator,filename,read_only,preserve_file_stats);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_is_writable
FLAC_API FLAC__bool FLAC__metadata_simple_iterator_is_writable(const FLAC__Metadata_SimpleIterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_is_writable))(iterator);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_next
FLAC_API FLAC__bool FLAC__metadata_simple_iterator_next(FLAC__Metadata_SimpleIterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_next))(iterator);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_prev
FLAC_API FLAC__bool FLAC__metadata_simple_iterator_prev(FLAC__Metadata_SimpleIterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_prev))(iterator);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block_type
FLAC_API FLAC__MetadataType FLAC__metadata_simple_iterator_get_block_type(const FLAC__Metadata_SimpleIterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_get_block_type))(iterator);
	}
	return (FLAC_API FLAC__MetadataType)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block
FLAC_API FLAC__StreamMetadata * FLAC__metadata_simple_iterator_get_block(FLAC__Metadata_SimpleIterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_get_block))(iterator);
	}
	return (FLAC_API FLAC__StreamMetadata *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_set_block
FLAC_API FLAC__bool FLAC__metadata_simple_iterator_set_block(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_set_block))(iterator,block,use_padding);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_insert_block_after
FLAC_API FLAC__bool FLAC__metadata_simple_iterator_insert_block_after(FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_insert_block_after))(iterator,block,use_padding);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete_block
FLAC_API FLAC__bool FLAC__metadata_simple_iterator_delete_block(FLAC__Metadata_SimpleIterator *iterator, FLAC__bool use_padding)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_simple_iterator_delete_block))(iterator,use_padding);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_chain_new
FLAC_API FLAC__Metadata_Chain * FLAC__metadata_chain_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_chain_new))();
	}
	return (FLAC_API FLAC__Metadata_Chain *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_chain_delete
FLAC_API void FLAC__metadata_chain_delete(FLAC__Metadata_Chain *chain)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_chain_delete))(chain);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_chain_status
FLAC_API FLAC__Metadata_ChainStatus FLAC__metadata_chain_status(FLAC__Metadata_Chain *chain)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_chain_status))(chain);
	}
	return (FLAC_API FLAC__Metadata_ChainStatus)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_chain_read
FLAC_API FLAC__bool FLAC__metadata_chain_read(FLAC__Metadata_Chain *chain, const char *filename)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_chain_read))(chain,filename);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_chain_write
FLAC_API FLAC__bool FLAC__metadata_chain_write(FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_chain_write))(chain,use_padding,preserve_file_stats);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_chain_merge_padding
FLAC_API void FLAC__metadata_chain_merge_padding(FLAC__Metadata_Chain *chain)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_chain_merge_padding))(chain);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_chain_sort_padding
FLAC_API void FLAC__metadata_chain_sort_padding(FLAC__Metadata_Chain *chain)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_chain_sort_padding))(chain);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_new
FLAC_API FLAC__Metadata_Iterator * FLAC__metadata_iterator_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_new))();
	}
	return (FLAC_API FLAC__Metadata_Iterator *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_delete
FLAC_API void FLAC__metadata_iterator_delete(FLAC__Metadata_Iterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_iterator_delete))(iterator);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_init
FLAC_API void FLAC__metadata_iterator_init(FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Chain *chain)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_iterator_init))(iterator,chain);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_next
FLAC_API FLAC__bool FLAC__metadata_iterator_next(FLAC__Metadata_Iterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_next))(iterator);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_prev
FLAC_API FLAC__bool FLAC__metadata_iterator_prev(FLAC__Metadata_Iterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_prev))(iterator);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_get_block_type
FLAC_API FLAC__MetadataType FLAC__metadata_iterator_get_block_type(const FLAC__Metadata_Iterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_get_block_type))(iterator);
	}
	return (FLAC_API FLAC__MetadataType)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_get_block
FLAC_API FLAC__StreamMetadata * FLAC__metadata_iterator_get_block(FLAC__Metadata_Iterator *iterator)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_get_block))(iterator);
	}
	return (FLAC_API FLAC__StreamMetadata *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_set_block
FLAC_API FLAC__bool FLAC__metadata_iterator_set_block(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_set_block))(iterator,block);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_delete_block
FLAC_API FLAC__bool FLAC__metadata_iterator_delete_block(FLAC__Metadata_Iterator *iterator, FLAC__bool replace_with_padding)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_delete_block))(iterator,replace_with_padding);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_before
FLAC_API FLAC__bool FLAC__metadata_iterator_insert_block_before(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_insert_block_before))(iterator,block);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_after
FLAC_API FLAC__bool FLAC__metadata_iterator_insert_block_after(FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_iterator_insert_block_after))(iterator,block);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_new
FLAC_API FLAC__StreamMetadata * FLAC__metadata_object_new(FLAC__MetadataType type)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_new))(type);
	}
	return (FLAC_API FLAC__StreamMetadata *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_clone
FLAC_API FLAC__StreamMetadata * FLAC__metadata_object_clone(const FLAC__StreamMetadata *object)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_clone))(object);
	}
	return (FLAC_API FLAC__StreamMetadata *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_delete
FLAC_API void FLAC__metadata_object_delete(FLAC__StreamMetadata *object)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_object_delete))(object);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_is_equal
FLAC_API FLAC__bool FLAC__metadata_object_is_equal(const FLAC__StreamMetadata *block1, const FLAC__StreamMetadata *block2)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_is_equal))(block1,block2);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_application_set_data
FLAC_API FLAC__bool FLAC__metadata_object_application_set_data(FLAC__StreamMetadata *object, FLAC__byte *data, unsigned length, FLAC__bool copy)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_application_set_data))(object,data,length,copy);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_resize_points
FLAC_API FLAC__bool FLAC__metadata_object_seektable_resize_points(FLAC__StreamMetadata *object, unsigned new_num_points)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_resize_points))(object,new_num_points);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_set_point
FLAC_API void FLAC__metadata_object_seektable_set_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_object_seektable_set_point))(object,point_num,point);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_insert_point
FLAC_API FLAC__bool FLAC__metadata_object_seektable_insert_point(FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_insert_point))(object,point_num,point);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_delete_point
FLAC_API FLAC__bool FLAC__metadata_object_seektable_delete_point(FLAC__StreamMetadata *object, unsigned point_num)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_delete_point))(object,point_num);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_is_legal
FLAC_API FLAC__bool FLAC__metadata_object_seektable_is_legal(const FLAC__StreamMetadata *object)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_is_legal))(object);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_placeholders
FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_append_placeholders(FLAC__StreamMetadata *object, unsigned num)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_template_append_placeholders))(object,num);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_point
FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_append_point(FLAC__StreamMetadata *object, FLAC__uint64 sample_number)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_template_append_point))(object,sample_number);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_points
FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_append_points(FLAC__StreamMetadata *object, FLAC__uint64 sample_numbers[], unsigned num)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_template_append_points))(object,sample_numbers,num);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_spaced_points
FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_append_spaced_points(FLAC__StreamMetadata *object, unsigned num, FLAC__uint64 total_samples)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_template_append_spaced_points))(object,num,total_samples);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_sort
FLAC_API FLAC__bool FLAC__metadata_object_seektable_template_sort(FLAC__StreamMetadata *object, FLAC__bool compact)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_seektable_template_sort))(object,compact);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_vendor_string
FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_set_vendor_string(FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_set_vendor_string))(object,entry,copy);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_resize_comments
FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_resize_comments(FLAC__StreamMetadata *object, unsigned new_num_comments)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_resize_comments))(object,new_num_comments);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_comment
FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_set_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_set_comment))(object,comment_num,entry,copy);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_insert_comment
FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_insert_comment(FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_insert_comment))(object,comment_num,entry,copy);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_delete_comment
FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_delete_comment(FLAC__StreamMetadata *object, unsigned comment_num)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_delete_comment))(object,comment_num);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_entry_matches
FLAC_API FLAC__bool FLAC__metadata_object_vorbiscomment_entry_matches(const FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, unsigned field_name_length)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_entry_matches))(entry,field_name,field_name_length);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_find_entry_from
FLAC_API int FLAC__metadata_object_vorbiscomment_find_entry_from(const FLAC__StreamMetadata *object, unsigned offset, const char *field_name)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_find_entry_from))(object,offset,field_name);
	}
	return (FLAC_API int)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entry_matching
FLAC_API int FLAC__metadata_object_vorbiscomment_remove_entry_matching(FLAC__StreamMetadata *object, const char *field_name)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_remove_entry_matching))(object,field_name);
	}
	return (FLAC_API int)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entries_matching
FLAC_API int FLAC__metadata_object_vorbiscomment_remove_entries_matching(FLAC__StreamMetadata *object, const char *field_name)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_vorbiscomment_remove_entries_matching))(object,field_name);
	}
	return (FLAC_API int)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_new
FLAC_API FLAC__StreamMetadata_CueSheet_Track * FLAC__metadata_object_cuesheet_track_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_track_new))();
	}
	return (FLAC_API FLAC__StreamMetadata_CueSheet_Track *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_clone
FLAC_API FLAC__StreamMetadata_CueSheet_Track * FLAC__metadata_object_cuesheet_track_clone(const FLAC__StreamMetadata_CueSheet_Track *object)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_track_clone))(object);
	}
	return (FLAC_API FLAC__StreamMetadata_CueSheet_Track *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete
FLAC_API void FLAC__metadata_object_cuesheet_track_delete(FLAC__StreamMetadata_CueSheet_Track *object)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_track_delete))(object);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_resize_indices
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_track_resize_indices(FLAC__StreamMetadata *object, unsigned track_num, unsigned new_num_indices)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_track_resize_indices))(object,track_num,new_num_indices);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_index
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_track_insert_index(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num, FLAC__StreamMetadata_CueSheet_Index index)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_track_insert_index))(object,track_num,index_num,index);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_blank_index
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_track_insert_blank_index(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_track_insert_blank_index))(object,track_num,index_num);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete_index
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_track_delete_index(FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_track_delete_index))(object,track_num,index_num);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_resize_tracks
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_resize_tracks(FLAC__StreamMetadata *object, unsigned new_num_tracks)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_resize_tracks))(object,new_num_tracks);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_set_track
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_set_track(FLAC__StreamMetadata *object, unsigned track_num, FLAC__StreamMetadata_CueSheet_Track *track, FLAC__bool copy)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_set_track))(object,track_num,track,copy);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_track
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_insert_track(FLAC__StreamMetadata *object, unsigned track_num, FLAC__StreamMetadata_CueSheet_Track *track, FLAC__bool copy)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_insert_track))(object,track_num,track,copy);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_blank_track
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_insert_blank_track(FLAC__StreamMetadata *object, unsigned track_num)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_insert_blank_track))(object,track_num);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_delete_track
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_delete_track(FLAC__StreamMetadata *object, unsigned track_num)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_delete_track))(object,track_num);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_is_legal
FLAC_API FLAC__bool FLAC__metadata_object_cuesheet_is_legal(const FLAC__StreamMetadata *object, FLAC__bool check_cd_da_subset, const char **violation)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__metadata_object_cuesheet_is_legal))(object,check_cd_da_subset,violation);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__format_sample_rate_is_valid
FLAC_API FLAC__bool FLAC__format_sample_rate_is_valid(unsigned sample_rate)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__format_sample_rate_is_valid))(sample_rate);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__format_seektable_is_legal
FLAC_API FLAC__bool FLAC__format_seektable_is_legal(const FLAC__StreamMetadata_SeekTable *seek_table)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__format_seektable_is_legal))(seek_table);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__format_seektable_sort
FLAC_API unsigned FLAC__format_seektable_sort(FLAC__StreamMetadata_SeekTable *seek_table)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__format_seektable_sort))(seek_table);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__format_cuesheet_is_legal
FLAC_API FLAC__bool FLAC__format_cuesheet_is_legal(const FLAC__StreamMetadata_CueSheet *cue_sheet, FLAC__bool check_cd_da_subset, const char **violation)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__format_cuesheet_is_legal))(cue_sheet,check_cd_da_subset,violation);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_new
FLAC_API FLAC__FileEncoder * FLAC__file_encoder_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_new))();
	}
	return (FLAC_API FLAC__FileEncoder *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_delete
FLAC_API void FLAC__file_encoder_delete(FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__file_encoder_delete))(encoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_verify
FLAC_API FLAC__bool FLAC__file_encoder_set_verify(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_verify))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_streamable_subset
FLAC_API FLAC__bool FLAC__file_encoder_set_streamable_subset(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_streamable_subset))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_mid_side_stereo
FLAC_API FLAC__bool FLAC__file_encoder_set_do_mid_side_stereo(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_do_mid_side_stereo))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_loose_mid_side_stereo
FLAC_API FLAC__bool FLAC__file_encoder_set_loose_mid_side_stereo(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_loose_mid_side_stereo))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_channels
FLAC_API FLAC__bool FLAC__file_encoder_set_channels(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_channels))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_bits_per_sample
FLAC_API FLAC__bool FLAC__file_encoder_set_bits_per_sample(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_bits_per_sample))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_sample_rate
FLAC_API FLAC__bool FLAC__file_encoder_set_sample_rate(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_sample_rate))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_blocksize
FLAC_API FLAC__bool FLAC__file_encoder_set_blocksize(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_blocksize))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_max_lpc_order
FLAC_API FLAC__bool FLAC__file_encoder_set_max_lpc_order(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_max_lpc_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_qlp_coeff_precision
FLAC_API FLAC__bool FLAC__file_encoder_set_qlp_coeff_precision(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_qlp_coeff_precision))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_qlp_coeff_prec_search
FLAC_API FLAC__bool FLAC__file_encoder_set_do_qlp_coeff_prec_search(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_do_qlp_coeff_prec_search))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_escape_coding
FLAC_API FLAC__bool FLAC__file_encoder_set_do_escape_coding(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_do_escape_coding))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_exhaustive_model_search
FLAC_API FLAC__bool FLAC__file_encoder_set_do_exhaustive_model_search(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_do_exhaustive_model_search))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_min_residual_partition_order
FLAC_API FLAC__bool FLAC__file_encoder_set_min_residual_partition_order(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_min_residual_partition_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_max_residual_partition_order
FLAC_API FLAC__bool FLAC__file_encoder_set_max_residual_partition_order(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_max_residual_partition_order))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_rice_parameter_search_dist
FLAC_API FLAC__bool FLAC__file_encoder_set_rice_parameter_search_dist(FLAC__FileEncoder *encoder, unsigned value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_rice_parameter_search_dist))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_total_samples_estimate
FLAC_API FLAC__bool FLAC__file_encoder_set_total_samples_estimate(FLAC__FileEncoder *encoder, FLAC__uint64 value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_total_samples_estimate))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_metadata
FLAC_API FLAC__bool FLAC__file_encoder_set_metadata(FLAC__FileEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_metadata))(encoder,metadata,num_blocks);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_filename
FLAC_API FLAC__bool FLAC__file_encoder_set_filename(FLAC__FileEncoder *encoder, const char *value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_filename))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_progress_callback
FLAC_API FLAC__bool FLAC__file_encoder_set_progress_callback(FLAC__FileEncoder *encoder, FLAC__FileEncoderProgressCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_progress_callback))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_client_data
FLAC_API FLAC__bool FLAC__file_encoder_set_client_data(FLAC__FileEncoder *encoder, void *value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_set_client_data))(encoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_state
FLAC_API FLAC__FileEncoderState FLAC__file_encoder_get_state(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_state))(encoder);
	}
	return (FLAC_API FLAC__FileEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_seekable_stream_encoder_state
FLAC_API FLAC__SeekableStreamEncoderState FLAC__file_encoder_get_seekable_stream_encoder_state(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_seekable_stream_encoder_state))(encoder);
	}
	return (FLAC_API FLAC__SeekableStreamEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_stream_encoder_state
FLAC_API FLAC__StreamEncoderState FLAC__file_encoder_get_stream_encoder_state(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_stream_encoder_state))(encoder);
	}
	return (FLAC_API FLAC__StreamEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_state
FLAC_API FLAC__StreamDecoderState FLAC__file_encoder_get_verify_decoder_state(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_verify_decoder_state))(encoder);
	}
	return (FLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_resolved_state_string
FLAC_API const char * FLAC__file_encoder_get_resolved_state_string(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_resolved_state_string))(encoder);
	}
	return (FLAC_API const char *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_error_stats
FLAC_API void FLAC__file_encoder_get_verify_decoder_error_stats(const FLAC__FileEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__file_encoder_get_verify_decoder_error_stats))(encoder,absolute_sample,frame_number,channel,sample,expected,got);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify
FLAC_API FLAC__bool FLAC__file_encoder_get_verify(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_verify))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_streamable_subset
FLAC_API FLAC__bool FLAC__file_encoder_get_streamable_subset(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_streamable_subset))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_mid_side_stereo
FLAC_API FLAC__bool FLAC__file_encoder_get_do_mid_side_stereo(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_do_mid_side_stereo))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_loose_mid_side_stereo
FLAC_API FLAC__bool FLAC__file_encoder_get_loose_mid_side_stereo(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_loose_mid_side_stereo))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_channels
FLAC_API unsigned FLAC__file_encoder_get_channels(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_channels))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_bits_per_sample
FLAC_API unsigned FLAC__file_encoder_get_bits_per_sample(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_bits_per_sample))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_sample_rate
FLAC_API unsigned FLAC__file_encoder_get_sample_rate(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_sample_rate))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_blocksize
FLAC_API unsigned FLAC__file_encoder_get_blocksize(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_blocksize))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_max_lpc_order
FLAC_API unsigned FLAC__file_encoder_get_max_lpc_order(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_max_lpc_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_qlp_coeff_precision
FLAC_API unsigned FLAC__file_encoder_get_qlp_coeff_precision(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_qlp_coeff_precision))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_qlp_coeff_prec_search
FLAC_API FLAC__bool FLAC__file_encoder_get_do_qlp_coeff_prec_search(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_do_qlp_coeff_prec_search))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_escape_coding
FLAC_API FLAC__bool FLAC__file_encoder_get_do_escape_coding(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_do_escape_coding))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_exhaustive_model_search
FLAC_API FLAC__bool FLAC__file_encoder_get_do_exhaustive_model_search(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_do_exhaustive_model_search))(encoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_min_residual_partition_order
FLAC_API unsigned FLAC__file_encoder_get_min_residual_partition_order(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_min_residual_partition_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_max_residual_partition_order
FLAC_API unsigned FLAC__file_encoder_get_max_residual_partition_order(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_max_residual_partition_order))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_rice_parameter_search_dist
FLAC_API unsigned FLAC__file_encoder_get_rice_parameter_search_dist(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_rice_parameter_search_dist))(encoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_total_samples_estimate
FLAC_API FLAC__uint64 FLAC__file_encoder_get_total_samples_estimate(const FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_get_total_samples_estimate))(encoder);
	}
	return (FLAC_API FLAC__uint64)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_init
FLAC_API FLAC__FileEncoderState FLAC__file_encoder_init(FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_init))(encoder);
	}
	return (FLAC_API FLAC__FileEncoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_finish
FLAC_API void FLAC__file_encoder_finish(FLAC__FileEncoder *encoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__file_encoder_finish))(encoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_process
FLAC_API FLAC__bool FLAC__file_encoder_process(FLAC__FileEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_process))(encoder,buffer,samples);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_encoder_process_interleaved
FLAC_API FLAC__bool FLAC__file_encoder_process_interleaved(FLAC__FileEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_encoder_process_interleaved))(encoder,buffer,samples);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_new
FLAC_API FLAC__FileDecoder * FLAC__file_decoder_new()
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_new))();
	}
	return (FLAC_API FLAC__FileDecoder *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_delete
FLAC_API void FLAC__file_decoder_delete(FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		(*(g_libFLAC_dll->FLAC__file_decoder_delete))(decoder);
	}
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_md5_checking
FLAC_API FLAC__bool FLAC__file_decoder_set_md5_checking(FLAC__FileDecoder *decoder, FLAC__bool value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_md5_checking))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_filename
FLAC_API FLAC__bool FLAC__file_decoder_set_filename(FLAC__FileDecoder *decoder, const char *value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_filename))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_write_callback
FLAC_API FLAC__bool FLAC__file_decoder_set_write_callback(FLAC__FileDecoder *decoder, FLAC__FileDecoderWriteCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_write_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_callback
FLAC_API FLAC__bool FLAC__file_decoder_set_metadata_callback(FLAC__FileDecoder *decoder, FLAC__FileDecoderMetadataCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_metadata_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_error_callback
FLAC_API FLAC__bool FLAC__file_decoder_set_error_callback(FLAC__FileDecoder *decoder, FLAC__FileDecoderErrorCallback value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_error_callback))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_client_data
FLAC_API FLAC__bool FLAC__file_decoder_set_client_data(FLAC__FileDecoder *decoder, void *value)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_client_data))(decoder,value);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond
FLAC_API FLAC__bool FLAC__file_decoder_set_metadata_respond(FLAC__FileDecoder *decoder, FLAC__MetadataType type)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_metadata_respond))(decoder,type);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_application
FLAC_API FLAC__bool FLAC__file_decoder_set_metadata_respond_application(FLAC__FileDecoder *decoder, const FLAC__byte id[4])
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_metadata_respond_application))(decoder,id);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_all
FLAC_API FLAC__bool FLAC__file_decoder_set_metadata_respond_all(FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_metadata_respond_all))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore
FLAC_API FLAC__bool FLAC__file_decoder_set_metadata_ignore(FLAC__FileDecoder *decoder, FLAC__MetadataType type)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_metadata_ignore))(decoder,type);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_application
FLAC_API FLAC__bool FLAC__file_decoder_set_metadata_ignore_application(FLAC__FileDecoder *decoder, const FLAC__byte id[4])
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_metadata_ignore_application))(decoder,id);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_all
FLAC_API FLAC__bool FLAC__file_decoder_set_metadata_ignore_all(FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_set_metadata_ignore_all))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_state
FLAC_API FLAC__FileDecoderState FLAC__file_decoder_get_state(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_state))(decoder);
	}
	return (FLAC_API FLAC__FileDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_seekable_stream_decoder_state
FLAC_API FLAC__SeekableStreamDecoderState FLAC__file_decoder_get_seekable_stream_decoder_state(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_seekable_stream_decoder_state))(decoder);
	}
	return (FLAC_API FLAC__SeekableStreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_stream_decoder_state
FLAC_API FLAC__StreamDecoderState FLAC__file_decoder_get_stream_decoder_state(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_stream_decoder_state))(decoder);
	}
	return (FLAC_API FLAC__StreamDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_resolved_state_string
FLAC_API const char * FLAC__file_decoder_get_resolved_state_string(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_resolved_state_string))(decoder);
	}
	return (FLAC_API const char *)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_md5_checking
FLAC_API FLAC__bool FLAC__file_decoder_get_md5_checking(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_md5_checking))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_channels
FLAC_API unsigned FLAC__file_decoder_get_channels(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_channels))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_channel_assignment
FLAC_API FLAC__ChannelAssignment FLAC__file_decoder_get_channel_assignment(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_channel_assignment))(decoder);
	}
	return (FLAC_API FLAC__ChannelAssignment)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_bits_per_sample
FLAC_API unsigned FLAC__file_decoder_get_bits_per_sample(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_bits_per_sample))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_sample_rate
FLAC_API unsigned FLAC__file_decoder_get_sample_rate(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_sample_rate))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_blocksize
FLAC_API unsigned FLAC__file_decoder_get_blocksize(const FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_blocksize))(decoder);
	}
	return (FLAC_API unsigned)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_decode_position
FLAC_API FLAC__bool FLAC__file_decoder_get_decode_position(const FLAC__FileDecoder *decoder, FLAC__uint64 *position)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_get_decode_position))(decoder,position);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_init
FLAC_API FLAC__FileDecoderState FLAC__file_decoder_init(FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_init))(decoder);
	}
	return (FLAC_API FLAC__FileDecoderState)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_finish
FLAC_API FLAC__bool FLAC__file_decoder_finish(FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_finish))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_single
FLAC_API FLAC__bool FLAC__file_decoder_process_single(FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_process_single))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_metadata
FLAC_API FLAC__bool FLAC__file_decoder_process_until_end_of_metadata(FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_process_until_end_of_metadata))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_file
FLAC_API FLAC__bool FLAC__file_decoder_process_until_end_of_file(FLAC__FileDecoder *decoder)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_process_until_end_of_file))(decoder);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

#ifndef IGNORE_libFLAC_FLAC__file_decoder_seek_absolute
FLAC_API FLAC__bool FLAC__file_decoder_seek_absolute(FLAC__FileDecoder *decoder, FLAC__uint64 sample)
{
	if ( g_libFLAC_dll != NULL ) {
		return (*(g_libFLAC_dll->FLAC__file_decoder_seek_absolute))(decoder,sample);
	}
	return (FLAC_API FLAC__bool)0;
}
#endif

/*
  NOT IMPORT LIST(9)
    FLAC__file_encoder_disable_constant_subframes
    FLAC__file_encoder_disable_fixed_subframes
    FLAC__file_encoder_disable_verbatim_subframes
    FLAC__seekable_stream_encoder_disable_constant_subframes
    FLAC__seekable_stream_encoder_disable_fixed_subframes
    FLAC__seekable_stream_encoder_disable_verbatim_subframes
    FLAC__stream_encoder_disable_constant_subframes
    FLAC__stream_encoder_disable_fixed_subframes
    FLAC__stream_encoder_disable_verbatim_subframes
*/

#if defined(__cplusplus)
}  /* extern "C" { */
#endif

#endif /* AU_FLAC_DLL */

