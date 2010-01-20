

#ifndef __libFLAC_dll_h__
#define __libFLAC_dll_h__

#include "w32_libFLAC_dll_i.h"

/***************************************************************
   for header file
 ***************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif



/***************************************************************
   for definition of function type
 ***************************************************************/
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_new
typedef FLAC_API FLAC__StreamEncoder * (* libFLAC_func_FLAC__stream_encoder_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_delete
typedef FLAC_API void (* libFLAC_func_FLAC__stream_encoder_delete_t) (FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_verify
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_verify_t) (FLAC__StreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_streamable_subset
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_streamable_subset_t) (FLAC__StreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_do_mid_side_stereo_t) (FLAC__StreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_loose_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_loose_mid_side_stereo_t) (FLAC__StreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_channels
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_channels_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_bits_per_sample
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_bits_per_sample_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_sample_rate
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_sample_rate_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_blocksize
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_blocksize_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_max_lpc_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_max_lpc_order_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_qlp_coeff_precision
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_qlp_coeff_precision_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_qlp_coeff_prec_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_do_qlp_coeff_prec_search_t) (FLAC__StreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_escape_coding
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_do_escape_coding_t) (FLAC__StreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_exhaustive_model_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_do_exhaustive_model_search_t) (FLAC__StreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_min_residual_partition_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_min_residual_partition_order_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_max_residual_partition_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_max_residual_partition_order_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_rice_parameter_search_dist
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_rice_parameter_search_dist_t) (FLAC__StreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_total_samples_estimate
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_total_samples_estimate_t) (FLAC__StreamEncoder *encoder, FLAC__uint64 value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_metadata
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_metadata_t) (FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_write_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_write_callback_t) (FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_metadata_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_metadata_callback_t) (FLAC__StreamEncoder *encoder, FLAC__StreamEncoderMetadataCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_client_data
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_set_client_data_t) (FLAC__StreamEncoder *encoder, void *value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_state
typedef FLAC_API FLAC__StreamEncoderState (* libFLAC_func_FLAC__stream_encoder_get_state_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_state
typedef FLAC_API FLAC__StreamDecoderState (* libFLAC_func_FLAC__stream_encoder_get_verify_decoder_state_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_resolved_state_string
typedef FLAC_API const char * (* libFLAC_func_FLAC__stream_encoder_get_resolved_state_string_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_error_stats
typedef FLAC_API void (* libFLAC_func_FLAC__stream_encoder_get_verify_decoder_error_stats_t) (const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_get_verify_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_streamable_subset
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_get_streamable_subset_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_get_do_mid_side_stereo_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_loose_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_get_loose_mid_side_stereo_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_channels
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_channels_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_bits_per_sample
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_bits_per_sample_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_sample_rate
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_sample_rate_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_blocksize
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_blocksize_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_max_lpc_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_max_lpc_order_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_qlp_coeff_precision
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_qlp_coeff_precision_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_qlp_coeff_prec_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_get_do_qlp_coeff_prec_search_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_escape_coding
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_get_do_escape_coding_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_exhaustive_model_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_get_do_exhaustive_model_search_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_min_residual_partition_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_min_residual_partition_order_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_max_residual_partition_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_max_residual_partition_order_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_rice_parameter_search_dist
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_encoder_get_rice_parameter_search_dist_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_total_samples_estimate
typedef FLAC_API FLAC__uint64 (* libFLAC_func_FLAC__stream_encoder_get_total_samples_estimate_t) (const FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_init
typedef FLAC_API FLAC__StreamEncoderState (* libFLAC_func_FLAC__stream_encoder_init_t) (FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_finish
typedef FLAC_API void (* libFLAC_func_FLAC__stream_encoder_finish_t) (FLAC__StreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_process
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_process_t) (FLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_process_interleaved
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_encoder_process_interleaved_t) (FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_new
typedef FLAC_API FLAC__StreamDecoder * (* libFLAC_func_FLAC__stream_decoder_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_delete
typedef FLAC_API void (* libFLAC_func_FLAC__stream_decoder_delete_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_read_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_read_callback_t) (FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_write_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_write_callback_t) (FLAC__StreamDecoder *decoder, FLAC__StreamDecoderWriteCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_metadata_callback_t) (FLAC__StreamDecoder *decoder, FLAC__StreamDecoderMetadataCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_error_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_error_callback_t) (FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_client_data
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_client_data_t) (FLAC__StreamDecoder *decoder, void *value);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_metadata_respond_t) (FLAC__StreamDecoder *decoder, FLAC__MetadataType type);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_application
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_metadata_respond_application_t) (FLAC__StreamDecoder *decoder, const FLAC__byte id[4]);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_all
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_metadata_respond_all_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_t) (FLAC__StreamDecoder *decoder, FLAC__MetadataType type);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_application
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_application_t) (FLAC__StreamDecoder *decoder, const FLAC__byte id[4]);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_all
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_all_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_state
typedef FLAC_API FLAC__StreamDecoderState (* libFLAC_func_FLAC__stream_decoder_get_state_t) (const FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_channels
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_decoder_get_channels_t) (const FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_channel_assignment
typedef FLAC_API FLAC__ChannelAssignment (* libFLAC_func_FLAC__stream_decoder_get_channel_assignment_t) (const FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_bits_per_sample
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_decoder_get_bits_per_sample_t) (const FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_sample_rate
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_decoder_get_sample_rate_t) (const FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_blocksize
typedef FLAC_API unsigned (* libFLAC_func_FLAC__stream_decoder_get_blocksize_t) (const FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_init
typedef FLAC_API FLAC__StreamDecoderState (* libFLAC_func_FLAC__stream_decoder_init_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_finish
typedef FLAC_API void (* libFLAC_func_FLAC__stream_decoder_finish_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_flush
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_flush_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_reset
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_reset_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_single
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_process_single_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_metadata
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_process_until_end_of_metadata_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_stream
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__stream_decoder_process_until_end_of_stream_t) (FLAC__StreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_new
typedef FLAC_API FLAC__SeekableStreamEncoder * (* libFLAC_func_FLAC__seekable_stream_encoder_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_delete
typedef FLAC_API void (* libFLAC_func_FLAC__seekable_stream_encoder_delete_t) (FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_verify
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_verify_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_streamable_subset
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_streamable_subset_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_do_mid_side_stereo_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_loose_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_loose_mid_side_stereo_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_channels
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_channels_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_bits_per_sample
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_bits_per_sample_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_sample_rate
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_sample_rate_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_blocksize
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_blocksize_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_lpc_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_max_lpc_order_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_qlp_coeff_precision
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_qlp_coeff_precision_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_escape_coding
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_do_escape_coding_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_exhaustive_model_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_do_exhaustive_model_search_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_min_residual_partition_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_min_residual_partition_order_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_residual_partition_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_max_residual_partition_order_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_rice_parameter_search_dist
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_rice_parameter_search_dist_t) (FLAC__SeekableStreamEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_total_samples_estimate
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_total_samples_estimate_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_metadata
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_metadata_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_seek_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_seek_callback_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderSeekCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_write_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_write_callback_t) (FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderWriteCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_client_data
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_set_client_data_t) (FLAC__SeekableStreamEncoder *encoder, void *value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_state
typedef FLAC_API FLAC__SeekableStreamEncoderState (* libFLAC_func_FLAC__seekable_stream_encoder_get_state_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_stream_encoder_state
typedef FLAC_API FLAC__StreamEncoderState (* libFLAC_func_FLAC__seekable_stream_encoder_get_stream_encoder_state_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_state
typedef FLAC_API FLAC__StreamDecoderState (* libFLAC_func_FLAC__seekable_stream_encoder_get_verify_decoder_state_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_resolved_state_string
typedef FLAC_API const char * (* libFLAC_func_FLAC__seekable_stream_encoder_get_resolved_state_string_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_error_stats
typedef FLAC_API void (* libFLAC_func_FLAC__seekable_stream_encoder_get_verify_decoder_error_stats_t) (const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_get_verify_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_streamable_subset
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_get_streamable_subset_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_get_do_mid_side_stereo_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_loose_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_get_loose_mid_side_stereo_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_channels
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_channels_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_bits_per_sample
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_bits_per_sample_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_sample_rate
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_sample_rate_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_blocksize
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_blocksize_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_lpc_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_max_lpc_order_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_qlp_coeff_precision
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_qlp_coeff_precision_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_escape_coding
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_get_do_escape_coding_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_exhaustive_model_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_get_do_exhaustive_model_search_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_min_residual_partition_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_min_residual_partition_order_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_residual_partition_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_max_residual_partition_order_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_rice_parameter_search_dist
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_encoder_get_rice_parameter_search_dist_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_total_samples_estimate
typedef FLAC_API FLAC__uint64 (* libFLAC_func_FLAC__seekable_stream_encoder_get_total_samples_estimate_t) (const FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_init
typedef FLAC_API FLAC__SeekableStreamEncoderState (* libFLAC_func_FLAC__seekable_stream_encoder_init_t) (FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_finish
typedef FLAC_API void (* libFLAC_func_FLAC__seekable_stream_encoder_finish_t) (FLAC__SeekableStreamEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_process
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_process_t) (FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_process_interleaved
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_encoder_process_interleaved_t) (FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_new
typedef FLAC_API FLAC__SeekableStreamDecoder * (* libFLAC_func_FLAC__seekable_stream_decoder_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_delete
typedef FLAC_API void (* libFLAC_func_FLAC__seekable_stream_decoder_delete_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_md5_checking
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_md5_checking_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_read_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_read_callback_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_seek_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_seek_callback_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_tell_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_tell_callback_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_length_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_length_callback_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_eof_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_eof_callback_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderEofCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_write_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_write_callback_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderWriteCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_callback_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderMetadataCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_error_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_error_callback_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderErrorCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_client_data
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_client_data_t) (FLAC__SeekableStreamDecoder *decoder, void *value);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_application
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_application_t) (FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_all
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_all_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_application
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_application_t) (FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_all
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_all_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_state
typedef FLAC_API FLAC__SeekableStreamDecoderState (* libFLAC_func_FLAC__seekable_stream_decoder_get_state_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_stream_decoder_state
typedef FLAC_API FLAC__StreamDecoderState (* libFLAC_func_FLAC__seekable_stream_decoder_get_stream_decoder_state_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_resolved_state_string
typedef FLAC_API const char * (* libFLAC_func_FLAC__seekable_stream_decoder_get_resolved_state_string_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_md5_checking
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_get_md5_checking_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channels
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_decoder_get_channels_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channel_assignment
typedef FLAC_API FLAC__ChannelAssignment (* libFLAC_func_FLAC__seekable_stream_decoder_get_channel_assignment_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_bits_per_sample
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_decoder_get_bits_per_sample_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_sample_rate
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_decoder_get_sample_rate_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_blocksize
typedef FLAC_API unsigned (* libFLAC_func_FLAC__seekable_stream_decoder_get_blocksize_t) (const FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_decode_position
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_get_decode_position_t) (const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *position);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_init
typedef FLAC_API FLAC__SeekableStreamDecoderState (* libFLAC_func_FLAC__seekable_stream_decoder_init_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_finish
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_finish_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_flush
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_flush_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_reset
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_reset_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_single
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_process_single_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_metadata
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_process_until_end_of_metadata_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_stream
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_process_until_end_of_stream_t) (FLAC__SeekableStreamDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_seek_absolute
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__seekable_stream_decoder_seek_absolute_t) (FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_get_streaminfo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_get_streaminfo_t) (const char *filename, FLAC__StreamMetadata *streaminfo);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_new
typedef FLAC_API FLAC__Metadata_SimpleIterator * (* libFLAC_func_FLAC__metadata_simple_iterator_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_simple_iterator_delete_t) (FLAC__Metadata_SimpleIterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_status
typedef FLAC_API FLAC__Metadata_SimpleIteratorStatus (* libFLAC_func_FLAC__metadata_simple_iterator_status_t) (FLAC__Metadata_SimpleIterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_init
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_simple_iterator_init_t) (FLAC__Metadata_SimpleIterator *iterator, const char *filename, FLAC__bool read_only, FLAC__bool preserve_file_stats);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_is_writable
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_simple_iterator_is_writable_t) (const FLAC__Metadata_SimpleIterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_next
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_simple_iterator_next_t) (FLAC__Metadata_SimpleIterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_prev
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_simple_iterator_prev_t) (FLAC__Metadata_SimpleIterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block_type
typedef FLAC_API FLAC__MetadataType (* libFLAC_func_FLAC__metadata_simple_iterator_get_block_type_t) (const FLAC__Metadata_SimpleIterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block
typedef FLAC_API FLAC__StreamMetadata * (* libFLAC_func_FLAC__metadata_simple_iterator_get_block_t) (FLAC__Metadata_SimpleIterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_set_block
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_simple_iterator_set_block_t) (FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_insert_block_after
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_simple_iterator_insert_block_after_t) (FLAC__Metadata_SimpleIterator *iterator, FLAC__StreamMetadata *block, FLAC__bool use_padding);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete_block
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_simple_iterator_delete_block_t) (FLAC__Metadata_SimpleIterator *iterator, FLAC__bool use_padding);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_new
typedef FLAC_API FLAC__Metadata_Chain * (* libFLAC_func_FLAC__metadata_chain_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_delete
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_chain_delete_t) (FLAC__Metadata_Chain *chain);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_status
typedef FLAC_API FLAC__Metadata_ChainStatus (* libFLAC_func_FLAC__metadata_chain_status_t) (FLAC__Metadata_Chain *chain);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_read
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_chain_read_t) (FLAC__Metadata_Chain *chain, const char *filename);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_write
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_chain_write_t) (FLAC__Metadata_Chain *chain, FLAC__bool use_padding, FLAC__bool preserve_file_stats);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_merge_padding
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_chain_merge_padding_t) (FLAC__Metadata_Chain *chain);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_sort_padding
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_chain_sort_padding_t) (FLAC__Metadata_Chain *chain);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_new
typedef FLAC_API FLAC__Metadata_Iterator * (* libFLAC_func_FLAC__metadata_iterator_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_delete
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_iterator_delete_t) (FLAC__Metadata_Iterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_init
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_iterator_init_t) (FLAC__Metadata_Iterator *iterator, FLAC__Metadata_Chain *chain);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_next
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_iterator_next_t) (FLAC__Metadata_Iterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_prev
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_iterator_prev_t) (FLAC__Metadata_Iterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_get_block_type
typedef FLAC_API FLAC__MetadataType (* libFLAC_func_FLAC__metadata_iterator_get_block_type_t) (const FLAC__Metadata_Iterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_get_block
typedef FLAC_API FLAC__StreamMetadata * (* libFLAC_func_FLAC__metadata_iterator_get_block_t) (FLAC__Metadata_Iterator *iterator);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_set_block
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_iterator_set_block_t) (FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_delete_block
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_iterator_delete_block_t) (FLAC__Metadata_Iterator *iterator, FLAC__bool replace_with_padding);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_before
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_iterator_insert_block_before_t) (FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_after
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_iterator_insert_block_after_t) (FLAC__Metadata_Iterator *iterator, FLAC__StreamMetadata *block);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_new
typedef FLAC_API FLAC__StreamMetadata * (* libFLAC_func_FLAC__metadata_object_new_t) (FLAC__MetadataType type);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_clone
typedef FLAC_API FLAC__StreamMetadata * (* libFLAC_func_FLAC__metadata_object_clone_t) (const FLAC__StreamMetadata *object);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_delete
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_object_delete_t) (FLAC__StreamMetadata *object);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_is_equal
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_is_equal_t) (const FLAC__StreamMetadata *block1, const FLAC__StreamMetadata *block2);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_application_set_data
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_application_set_data_t) (FLAC__StreamMetadata *object, FLAC__byte *data, unsigned length, FLAC__bool copy);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_resize_points
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_resize_points_t) (FLAC__StreamMetadata *object, unsigned new_num_points);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_set_point
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_object_seektable_set_point_t) (FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_insert_point
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_insert_point_t) (FLAC__StreamMetadata *object, unsigned point_num, FLAC__StreamMetadata_SeekPoint point);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_delete_point
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_delete_point_t) (FLAC__StreamMetadata *object, unsigned point_num);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_is_legal
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_is_legal_t) (const FLAC__StreamMetadata *object);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_placeholders
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_template_append_placeholders_t) (FLAC__StreamMetadata *object, unsigned num);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_point
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_template_append_point_t) (FLAC__StreamMetadata *object, FLAC__uint64 sample_number);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_points
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_template_append_points_t) (FLAC__StreamMetadata *object, FLAC__uint64 sample_numbers[], unsigned num);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_spaced_points
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_template_append_spaced_points_t) (FLAC__StreamMetadata *object, unsigned num, FLAC__uint64 total_samples);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_sort
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_seektable_template_sort_t) (FLAC__StreamMetadata *object, FLAC__bool compact);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_vendor_string
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_vorbiscomment_set_vendor_string_t) (FLAC__StreamMetadata *object, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_resize_comments
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_vorbiscomment_resize_comments_t) (FLAC__StreamMetadata *object, unsigned new_num_comments);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_comment
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_vorbiscomment_set_comment_t) (FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_insert_comment
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_vorbiscomment_insert_comment_t) (FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_delete_comment
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_vorbiscomment_delete_comment_t) (FLAC__StreamMetadata *object, unsigned comment_num);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_entry_matches
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_vorbiscomment_entry_matches_t) (const FLAC__StreamMetadata_VorbisComment_Entry *entry, const char *field_name, unsigned field_name_length);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_find_entry_from
typedef FLAC_API int (* libFLAC_func_FLAC__metadata_object_vorbiscomment_find_entry_from_t) (const FLAC__StreamMetadata *object, unsigned offset, const char *field_name);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entry_matching
typedef FLAC_API int (* libFLAC_func_FLAC__metadata_object_vorbiscomment_remove_entry_matching_t) (FLAC__StreamMetadata *object, const char *field_name);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entries_matching
typedef FLAC_API int (* libFLAC_func_FLAC__metadata_object_vorbiscomment_remove_entries_matching_t) (FLAC__StreamMetadata *object, const char *field_name);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_new
typedef FLAC_API FLAC__StreamMetadata_CueSheet_Track * (* libFLAC_func_FLAC__metadata_object_cuesheet_track_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_clone
typedef FLAC_API FLAC__StreamMetadata_CueSheet_Track * (* libFLAC_func_FLAC__metadata_object_cuesheet_track_clone_t) (const FLAC__StreamMetadata_CueSheet_Track *object);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete
typedef FLAC_API void (* libFLAC_func_FLAC__metadata_object_cuesheet_track_delete_t) (FLAC__StreamMetadata_CueSheet_Track *object);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_resize_indices
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_track_resize_indices_t) (FLAC__StreamMetadata *object, unsigned track_num, unsigned new_num_indices);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_index
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_track_insert_index_t) (FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num, FLAC__StreamMetadata_CueSheet_Index index);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_blank_index
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_track_insert_blank_index_t) (FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete_index
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_track_delete_index_t) (FLAC__StreamMetadata *object, unsigned track_num, unsigned index_num);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_resize_tracks
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_resize_tracks_t) (FLAC__StreamMetadata *object, unsigned new_num_tracks);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_set_track
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_set_track_t) (FLAC__StreamMetadata *object, unsigned track_num, FLAC__StreamMetadata_CueSheet_Track *track, FLAC__bool copy);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_track
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_insert_track_t) (FLAC__StreamMetadata *object, unsigned track_num, FLAC__StreamMetadata_CueSheet_Track *track, FLAC__bool copy);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_blank_track
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_insert_blank_track_t) (FLAC__StreamMetadata *object, unsigned track_num);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_delete_track
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_delete_track_t) (FLAC__StreamMetadata *object, unsigned track_num);
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_is_legal
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__metadata_object_cuesheet_is_legal_t) (const FLAC__StreamMetadata *object, FLAC__bool check_cd_da_subset, const char **violation);
#endif
#ifndef IGNORE_libFLAC_FLAC__format_sample_rate_is_valid
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__format_sample_rate_is_valid_t) (unsigned sample_rate);
#endif
#ifndef IGNORE_libFLAC_FLAC__format_seektable_is_legal
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__format_seektable_is_legal_t) (const FLAC__StreamMetadata_SeekTable *seek_table);
#endif
#ifndef IGNORE_libFLAC_FLAC__format_seektable_sort
typedef FLAC_API unsigned (* libFLAC_func_FLAC__format_seektable_sort_t) (FLAC__StreamMetadata_SeekTable *seek_table);
#endif
#ifndef IGNORE_libFLAC_FLAC__format_cuesheet_is_legal
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__format_cuesheet_is_legal_t) (const FLAC__StreamMetadata_CueSheet *cue_sheet, FLAC__bool check_cd_da_subset, const char **violation);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_new
typedef FLAC_API FLAC__FileEncoder * (* libFLAC_func_FLAC__file_encoder_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_delete
typedef FLAC_API void (* libFLAC_func_FLAC__file_encoder_delete_t) (FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_verify
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_verify_t) (FLAC__FileEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_streamable_subset
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_streamable_subset_t) (FLAC__FileEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_do_mid_side_stereo_t) (FLAC__FileEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_loose_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_loose_mid_side_stereo_t) (FLAC__FileEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_channels
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_channels_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_bits_per_sample
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_bits_per_sample_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_sample_rate
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_sample_rate_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_blocksize
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_blocksize_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_max_lpc_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_max_lpc_order_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_qlp_coeff_precision
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_qlp_coeff_precision_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_qlp_coeff_prec_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_do_qlp_coeff_prec_search_t) (FLAC__FileEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_escape_coding
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_do_escape_coding_t) (FLAC__FileEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_exhaustive_model_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_do_exhaustive_model_search_t) (FLAC__FileEncoder *encoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_min_residual_partition_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_min_residual_partition_order_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_max_residual_partition_order
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_max_residual_partition_order_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_rice_parameter_search_dist
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_rice_parameter_search_dist_t) (FLAC__FileEncoder *encoder, unsigned value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_total_samples_estimate
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_total_samples_estimate_t) (FLAC__FileEncoder *encoder, FLAC__uint64 value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_metadata
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_metadata_t) (FLAC__FileEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_filename
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_filename_t) (FLAC__FileEncoder *encoder, const char *value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_progress_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_progress_callback_t) (FLAC__FileEncoder *encoder, FLAC__FileEncoderProgressCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_client_data
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_set_client_data_t) (FLAC__FileEncoder *encoder, void *value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_state
typedef FLAC_API FLAC__FileEncoderState (* libFLAC_func_FLAC__file_encoder_get_state_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_seekable_stream_encoder_state
typedef FLAC_API FLAC__SeekableStreamEncoderState (* libFLAC_func_FLAC__file_encoder_get_seekable_stream_encoder_state_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_stream_encoder_state
typedef FLAC_API FLAC__StreamEncoderState (* libFLAC_func_FLAC__file_encoder_get_stream_encoder_state_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_state
typedef FLAC_API FLAC__StreamDecoderState (* libFLAC_func_FLAC__file_encoder_get_verify_decoder_state_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_resolved_state_string
typedef FLAC_API const char * (* libFLAC_func_FLAC__file_encoder_get_resolved_state_string_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_error_stats
typedef FLAC_API void (* libFLAC_func_FLAC__file_encoder_get_verify_decoder_error_stats_t) (const FLAC__FileEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_get_verify_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_streamable_subset
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_get_streamable_subset_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_get_do_mid_side_stereo_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_loose_mid_side_stereo
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_get_loose_mid_side_stereo_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_channels
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_channels_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_bits_per_sample
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_bits_per_sample_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_sample_rate
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_sample_rate_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_blocksize
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_blocksize_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_max_lpc_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_max_lpc_order_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_qlp_coeff_precision
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_qlp_coeff_precision_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_qlp_coeff_prec_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_get_do_qlp_coeff_prec_search_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_escape_coding
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_get_do_escape_coding_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_exhaustive_model_search
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_get_do_exhaustive_model_search_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_min_residual_partition_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_min_residual_partition_order_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_max_residual_partition_order
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_max_residual_partition_order_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_rice_parameter_search_dist
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_encoder_get_rice_parameter_search_dist_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_total_samples_estimate
typedef FLAC_API FLAC__uint64 (* libFLAC_func_FLAC__file_encoder_get_total_samples_estimate_t) (const FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_init
typedef FLAC_API FLAC__FileEncoderState (* libFLAC_func_FLAC__file_encoder_init_t) (FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_finish
typedef FLAC_API void (* libFLAC_func_FLAC__file_encoder_finish_t) (FLAC__FileEncoder *encoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_process
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_process_t) (FLAC__FileEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_process_interleaved
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_encoder_process_interleaved_t) (FLAC__FileEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_new
typedef FLAC_API FLAC__FileDecoder * (* libFLAC_func_FLAC__file_decoder_new_t) ();
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_delete
typedef FLAC_API void (* libFLAC_func_FLAC__file_decoder_delete_t) (FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_md5_checking
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_md5_checking_t) (FLAC__FileDecoder *decoder, FLAC__bool value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_filename
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_filename_t) (FLAC__FileDecoder *decoder, const char *value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_write_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_write_callback_t) (FLAC__FileDecoder *decoder, FLAC__FileDecoderWriteCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_metadata_callback_t) (FLAC__FileDecoder *decoder, FLAC__FileDecoderMetadataCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_error_callback
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_error_callback_t) (FLAC__FileDecoder *decoder, FLAC__FileDecoderErrorCallback value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_client_data
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_client_data_t) (FLAC__FileDecoder *decoder, void *value);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_metadata_respond_t) (FLAC__FileDecoder *decoder, FLAC__MetadataType type);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_application
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_metadata_respond_application_t) (FLAC__FileDecoder *decoder, const FLAC__byte id[4]);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_all
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_metadata_respond_all_t) (FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_metadata_ignore_t) (FLAC__FileDecoder *decoder, FLAC__MetadataType type);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_application
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_metadata_ignore_application_t) (FLAC__FileDecoder *decoder, const FLAC__byte id[4]);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_all
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_set_metadata_ignore_all_t) (FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_state
typedef FLAC_API FLAC__FileDecoderState (* libFLAC_func_FLAC__file_decoder_get_state_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_seekable_stream_decoder_state
typedef FLAC_API FLAC__SeekableStreamDecoderState (* libFLAC_func_FLAC__file_decoder_get_seekable_stream_decoder_state_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_stream_decoder_state
typedef FLAC_API FLAC__StreamDecoderState (* libFLAC_func_FLAC__file_decoder_get_stream_decoder_state_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_resolved_state_string
typedef FLAC_API const char * (* libFLAC_func_FLAC__file_decoder_get_resolved_state_string_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_md5_checking
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_get_md5_checking_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_channels
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_decoder_get_channels_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_channel_assignment
typedef FLAC_API FLAC__ChannelAssignment (* libFLAC_func_FLAC__file_decoder_get_channel_assignment_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_bits_per_sample
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_decoder_get_bits_per_sample_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_sample_rate
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_decoder_get_sample_rate_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_blocksize
typedef FLAC_API unsigned (* libFLAC_func_FLAC__file_decoder_get_blocksize_t) (const FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_decode_position
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_get_decode_position_t) (const FLAC__FileDecoder *decoder, FLAC__uint64 *position);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_init
typedef FLAC_API FLAC__FileDecoderState (* libFLAC_func_FLAC__file_decoder_init_t) (FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_finish
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_finish_t) (FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_single
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_process_single_t) (FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_metadata
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_process_until_end_of_metadata_t) (FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_file
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_process_until_end_of_file_t) (FLAC__FileDecoder *decoder);
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_seek_absolute
typedef FLAC_API FLAC__bool (* libFLAC_func_FLAC__file_decoder_seek_absolute_t) (FLAC__FileDecoder *decoder, FLAC__uint64 sample);
#endif


typedef struct libFLAC_dll_t_ {
  HANDLE __h_dll;
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_new
  libFLAC_func_FLAC__stream_encoder_new_t  FLAC__stream_encoder_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_delete
  libFLAC_func_FLAC__stream_encoder_delete_t  FLAC__stream_encoder_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_verify
  libFLAC_func_FLAC__stream_encoder_set_verify_t  FLAC__stream_encoder_set_verify;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_streamable_subset
  libFLAC_func_FLAC__stream_encoder_set_streamable_subset_t  FLAC__stream_encoder_set_streamable_subset;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_mid_side_stereo
  libFLAC_func_FLAC__stream_encoder_set_do_mid_side_stereo_t  FLAC__stream_encoder_set_do_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_loose_mid_side_stereo
  libFLAC_func_FLAC__stream_encoder_set_loose_mid_side_stereo_t  FLAC__stream_encoder_set_loose_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_channels
  libFLAC_func_FLAC__stream_encoder_set_channels_t  FLAC__stream_encoder_set_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_bits_per_sample
  libFLAC_func_FLAC__stream_encoder_set_bits_per_sample_t  FLAC__stream_encoder_set_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_sample_rate
  libFLAC_func_FLAC__stream_encoder_set_sample_rate_t  FLAC__stream_encoder_set_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_blocksize
  libFLAC_func_FLAC__stream_encoder_set_blocksize_t  FLAC__stream_encoder_set_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_max_lpc_order
  libFLAC_func_FLAC__stream_encoder_set_max_lpc_order_t  FLAC__stream_encoder_set_max_lpc_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_qlp_coeff_precision
  libFLAC_func_FLAC__stream_encoder_set_qlp_coeff_precision_t  FLAC__stream_encoder_set_qlp_coeff_precision;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_qlp_coeff_prec_search
  libFLAC_func_FLAC__stream_encoder_set_do_qlp_coeff_prec_search_t  FLAC__stream_encoder_set_do_qlp_coeff_prec_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_escape_coding
  libFLAC_func_FLAC__stream_encoder_set_do_escape_coding_t  FLAC__stream_encoder_set_do_escape_coding;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_do_exhaustive_model_search
  libFLAC_func_FLAC__stream_encoder_set_do_exhaustive_model_search_t  FLAC__stream_encoder_set_do_exhaustive_model_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_min_residual_partition_order
  libFLAC_func_FLAC__stream_encoder_set_min_residual_partition_order_t  FLAC__stream_encoder_set_min_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_max_residual_partition_order
  libFLAC_func_FLAC__stream_encoder_set_max_residual_partition_order_t  FLAC__stream_encoder_set_max_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_rice_parameter_search_dist
  libFLAC_func_FLAC__stream_encoder_set_rice_parameter_search_dist_t  FLAC__stream_encoder_set_rice_parameter_search_dist;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_total_samples_estimate
  libFLAC_func_FLAC__stream_encoder_set_total_samples_estimate_t  FLAC__stream_encoder_set_total_samples_estimate;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_metadata
  libFLAC_func_FLAC__stream_encoder_set_metadata_t  FLAC__stream_encoder_set_metadata;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_write_callback
  libFLAC_func_FLAC__stream_encoder_set_write_callback_t  FLAC__stream_encoder_set_write_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_metadata_callback
  libFLAC_func_FLAC__stream_encoder_set_metadata_callback_t  FLAC__stream_encoder_set_metadata_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_set_client_data
  libFLAC_func_FLAC__stream_encoder_set_client_data_t  FLAC__stream_encoder_set_client_data;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_state
  libFLAC_func_FLAC__stream_encoder_get_state_t  FLAC__stream_encoder_get_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_state
  libFLAC_func_FLAC__stream_encoder_get_verify_decoder_state_t  FLAC__stream_encoder_get_verify_decoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_resolved_state_string
  libFLAC_func_FLAC__stream_encoder_get_resolved_state_string_t  FLAC__stream_encoder_get_resolved_state_string;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_error_stats
  libFLAC_func_FLAC__stream_encoder_get_verify_decoder_error_stats_t  FLAC__stream_encoder_get_verify_decoder_error_stats;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_verify
  libFLAC_func_FLAC__stream_encoder_get_verify_t  FLAC__stream_encoder_get_verify;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_streamable_subset
  libFLAC_func_FLAC__stream_encoder_get_streamable_subset_t  FLAC__stream_encoder_get_streamable_subset;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_mid_side_stereo
  libFLAC_func_FLAC__stream_encoder_get_do_mid_side_stereo_t  FLAC__stream_encoder_get_do_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_loose_mid_side_stereo
  libFLAC_func_FLAC__stream_encoder_get_loose_mid_side_stereo_t  FLAC__stream_encoder_get_loose_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_channels
  libFLAC_func_FLAC__stream_encoder_get_channels_t  FLAC__stream_encoder_get_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_bits_per_sample
  libFLAC_func_FLAC__stream_encoder_get_bits_per_sample_t  FLAC__stream_encoder_get_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_sample_rate
  libFLAC_func_FLAC__stream_encoder_get_sample_rate_t  FLAC__stream_encoder_get_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_blocksize
  libFLAC_func_FLAC__stream_encoder_get_blocksize_t  FLAC__stream_encoder_get_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_max_lpc_order
  libFLAC_func_FLAC__stream_encoder_get_max_lpc_order_t  FLAC__stream_encoder_get_max_lpc_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_qlp_coeff_precision
  libFLAC_func_FLAC__stream_encoder_get_qlp_coeff_precision_t  FLAC__stream_encoder_get_qlp_coeff_precision;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_qlp_coeff_prec_search
  libFLAC_func_FLAC__stream_encoder_get_do_qlp_coeff_prec_search_t  FLAC__stream_encoder_get_do_qlp_coeff_prec_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_escape_coding
  libFLAC_func_FLAC__stream_encoder_get_do_escape_coding_t  FLAC__stream_encoder_get_do_escape_coding;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_do_exhaustive_model_search
  libFLAC_func_FLAC__stream_encoder_get_do_exhaustive_model_search_t  FLAC__stream_encoder_get_do_exhaustive_model_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_min_residual_partition_order
  libFLAC_func_FLAC__stream_encoder_get_min_residual_partition_order_t  FLAC__stream_encoder_get_min_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_max_residual_partition_order
  libFLAC_func_FLAC__stream_encoder_get_max_residual_partition_order_t  FLAC__stream_encoder_get_max_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_rice_parameter_search_dist
  libFLAC_func_FLAC__stream_encoder_get_rice_parameter_search_dist_t  FLAC__stream_encoder_get_rice_parameter_search_dist;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_get_total_samples_estimate
  libFLAC_func_FLAC__stream_encoder_get_total_samples_estimate_t  FLAC__stream_encoder_get_total_samples_estimate;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_init
  libFLAC_func_FLAC__stream_encoder_init_t  FLAC__stream_encoder_init;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_finish
  libFLAC_func_FLAC__stream_encoder_finish_t  FLAC__stream_encoder_finish;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_process
  libFLAC_func_FLAC__stream_encoder_process_t  FLAC__stream_encoder_process;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_encoder_process_interleaved
  libFLAC_func_FLAC__stream_encoder_process_interleaved_t  FLAC__stream_encoder_process_interleaved;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_new
  libFLAC_func_FLAC__stream_decoder_new_t  FLAC__stream_decoder_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_delete
  libFLAC_func_FLAC__stream_decoder_delete_t  FLAC__stream_decoder_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_read_callback
  libFLAC_func_FLAC__stream_decoder_set_read_callback_t  FLAC__stream_decoder_set_read_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_write_callback
  libFLAC_func_FLAC__stream_decoder_set_write_callback_t  FLAC__stream_decoder_set_write_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_callback
  libFLAC_func_FLAC__stream_decoder_set_metadata_callback_t  FLAC__stream_decoder_set_metadata_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_error_callback
  libFLAC_func_FLAC__stream_decoder_set_error_callback_t  FLAC__stream_decoder_set_error_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_client_data
  libFLAC_func_FLAC__stream_decoder_set_client_data_t  FLAC__stream_decoder_set_client_data;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond
  libFLAC_func_FLAC__stream_decoder_set_metadata_respond_t  FLAC__stream_decoder_set_metadata_respond;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_application
  libFLAC_func_FLAC__stream_decoder_set_metadata_respond_application_t  FLAC__stream_decoder_set_metadata_respond_application;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_all
  libFLAC_func_FLAC__stream_decoder_set_metadata_respond_all_t  FLAC__stream_decoder_set_metadata_respond_all;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore
  libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_t  FLAC__stream_decoder_set_metadata_ignore;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_application
  libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_application_t  FLAC__stream_decoder_set_metadata_ignore_application;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_all
  libFLAC_func_FLAC__stream_decoder_set_metadata_ignore_all_t  FLAC__stream_decoder_set_metadata_ignore_all;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_state
  libFLAC_func_FLAC__stream_decoder_get_state_t  FLAC__stream_decoder_get_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_channels
  libFLAC_func_FLAC__stream_decoder_get_channels_t  FLAC__stream_decoder_get_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_channel_assignment
  libFLAC_func_FLAC__stream_decoder_get_channel_assignment_t  FLAC__stream_decoder_get_channel_assignment;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_bits_per_sample
  libFLAC_func_FLAC__stream_decoder_get_bits_per_sample_t  FLAC__stream_decoder_get_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_sample_rate
  libFLAC_func_FLAC__stream_decoder_get_sample_rate_t  FLAC__stream_decoder_get_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_get_blocksize
  libFLAC_func_FLAC__stream_decoder_get_blocksize_t  FLAC__stream_decoder_get_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_init
  libFLAC_func_FLAC__stream_decoder_init_t  FLAC__stream_decoder_init;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_finish
  libFLAC_func_FLAC__stream_decoder_finish_t  FLAC__stream_decoder_finish;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_flush
  libFLAC_func_FLAC__stream_decoder_flush_t  FLAC__stream_decoder_flush;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_reset
  libFLAC_func_FLAC__stream_decoder_reset_t  FLAC__stream_decoder_reset;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_single
  libFLAC_func_FLAC__stream_decoder_process_single_t  FLAC__stream_decoder_process_single;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_metadata
  libFLAC_func_FLAC__stream_decoder_process_until_end_of_metadata_t  FLAC__stream_decoder_process_until_end_of_metadata;
#endif
#ifndef IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_stream
  libFLAC_func_FLAC__stream_decoder_process_until_end_of_stream_t  FLAC__stream_decoder_process_until_end_of_stream;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_new
  libFLAC_func_FLAC__seekable_stream_encoder_new_t  FLAC__seekable_stream_encoder_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_delete
  libFLAC_func_FLAC__seekable_stream_encoder_delete_t  FLAC__seekable_stream_encoder_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_verify
  libFLAC_func_FLAC__seekable_stream_encoder_set_verify_t  FLAC__seekable_stream_encoder_set_verify;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_streamable_subset
  libFLAC_func_FLAC__seekable_stream_encoder_set_streamable_subset_t  FLAC__seekable_stream_encoder_set_streamable_subset;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_mid_side_stereo
  libFLAC_func_FLAC__seekable_stream_encoder_set_do_mid_side_stereo_t  FLAC__seekable_stream_encoder_set_do_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_loose_mid_side_stereo
  libFLAC_func_FLAC__seekable_stream_encoder_set_loose_mid_side_stereo_t  FLAC__seekable_stream_encoder_set_loose_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_channels
  libFLAC_func_FLAC__seekable_stream_encoder_set_channels_t  FLAC__seekable_stream_encoder_set_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_bits_per_sample
  libFLAC_func_FLAC__seekable_stream_encoder_set_bits_per_sample_t  FLAC__seekable_stream_encoder_set_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_sample_rate
  libFLAC_func_FLAC__seekable_stream_encoder_set_sample_rate_t  FLAC__seekable_stream_encoder_set_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_blocksize
  libFLAC_func_FLAC__seekable_stream_encoder_set_blocksize_t  FLAC__seekable_stream_encoder_set_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_lpc_order
  libFLAC_func_FLAC__seekable_stream_encoder_set_max_lpc_order_t  FLAC__seekable_stream_encoder_set_max_lpc_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_qlp_coeff_precision
  libFLAC_func_FLAC__seekable_stream_encoder_set_qlp_coeff_precision_t  FLAC__seekable_stream_encoder_set_qlp_coeff_precision;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search
  libFLAC_func_FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search_t  FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_escape_coding
  libFLAC_func_FLAC__seekable_stream_encoder_set_do_escape_coding_t  FLAC__seekable_stream_encoder_set_do_escape_coding;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_exhaustive_model_search
  libFLAC_func_FLAC__seekable_stream_encoder_set_do_exhaustive_model_search_t  FLAC__seekable_stream_encoder_set_do_exhaustive_model_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_min_residual_partition_order
  libFLAC_func_FLAC__seekable_stream_encoder_set_min_residual_partition_order_t  FLAC__seekable_stream_encoder_set_min_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_residual_partition_order
  libFLAC_func_FLAC__seekable_stream_encoder_set_max_residual_partition_order_t  FLAC__seekable_stream_encoder_set_max_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_rice_parameter_search_dist
  libFLAC_func_FLAC__seekable_stream_encoder_set_rice_parameter_search_dist_t  FLAC__seekable_stream_encoder_set_rice_parameter_search_dist;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_total_samples_estimate
  libFLAC_func_FLAC__seekable_stream_encoder_set_total_samples_estimate_t  FLAC__seekable_stream_encoder_set_total_samples_estimate;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_metadata
  libFLAC_func_FLAC__seekable_stream_encoder_set_metadata_t  FLAC__seekable_stream_encoder_set_metadata;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_seek_callback
  libFLAC_func_FLAC__seekable_stream_encoder_set_seek_callback_t  FLAC__seekable_stream_encoder_set_seek_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_write_callback
  libFLAC_func_FLAC__seekable_stream_encoder_set_write_callback_t  FLAC__seekable_stream_encoder_set_write_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_client_data
  libFLAC_func_FLAC__seekable_stream_encoder_set_client_data_t  FLAC__seekable_stream_encoder_set_client_data;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_state
  libFLAC_func_FLAC__seekable_stream_encoder_get_state_t  FLAC__seekable_stream_encoder_get_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_stream_encoder_state
  libFLAC_func_FLAC__seekable_stream_encoder_get_stream_encoder_state_t  FLAC__seekable_stream_encoder_get_stream_encoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_state
  libFLAC_func_FLAC__seekable_stream_encoder_get_verify_decoder_state_t  FLAC__seekable_stream_encoder_get_verify_decoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_resolved_state_string
  libFLAC_func_FLAC__seekable_stream_encoder_get_resolved_state_string_t  FLAC__seekable_stream_encoder_get_resolved_state_string;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_error_stats
  libFLAC_func_FLAC__seekable_stream_encoder_get_verify_decoder_error_stats_t  FLAC__seekable_stream_encoder_get_verify_decoder_error_stats;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify
  libFLAC_func_FLAC__seekable_stream_encoder_get_verify_t  FLAC__seekable_stream_encoder_get_verify;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_streamable_subset
  libFLAC_func_FLAC__seekable_stream_encoder_get_streamable_subset_t  FLAC__seekable_stream_encoder_get_streamable_subset;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_mid_side_stereo
  libFLAC_func_FLAC__seekable_stream_encoder_get_do_mid_side_stereo_t  FLAC__seekable_stream_encoder_get_do_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_loose_mid_side_stereo
  libFLAC_func_FLAC__seekable_stream_encoder_get_loose_mid_side_stereo_t  FLAC__seekable_stream_encoder_get_loose_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_channels
  libFLAC_func_FLAC__seekable_stream_encoder_get_channels_t  FLAC__seekable_stream_encoder_get_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_bits_per_sample
  libFLAC_func_FLAC__seekable_stream_encoder_get_bits_per_sample_t  FLAC__seekable_stream_encoder_get_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_sample_rate
  libFLAC_func_FLAC__seekable_stream_encoder_get_sample_rate_t  FLAC__seekable_stream_encoder_get_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_blocksize
  libFLAC_func_FLAC__seekable_stream_encoder_get_blocksize_t  FLAC__seekable_stream_encoder_get_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_lpc_order
  libFLAC_func_FLAC__seekable_stream_encoder_get_max_lpc_order_t  FLAC__seekable_stream_encoder_get_max_lpc_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_qlp_coeff_precision
  libFLAC_func_FLAC__seekable_stream_encoder_get_qlp_coeff_precision_t  FLAC__seekable_stream_encoder_get_qlp_coeff_precision;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search
  libFLAC_func_FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search_t  FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_escape_coding
  libFLAC_func_FLAC__seekable_stream_encoder_get_do_escape_coding_t  FLAC__seekable_stream_encoder_get_do_escape_coding;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_exhaustive_model_search
  libFLAC_func_FLAC__seekable_stream_encoder_get_do_exhaustive_model_search_t  FLAC__seekable_stream_encoder_get_do_exhaustive_model_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_min_residual_partition_order
  libFLAC_func_FLAC__seekable_stream_encoder_get_min_residual_partition_order_t  FLAC__seekable_stream_encoder_get_min_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_residual_partition_order
  libFLAC_func_FLAC__seekable_stream_encoder_get_max_residual_partition_order_t  FLAC__seekable_stream_encoder_get_max_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_rice_parameter_search_dist
  libFLAC_func_FLAC__seekable_stream_encoder_get_rice_parameter_search_dist_t  FLAC__seekable_stream_encoder_get_rice_parameter_search_dist;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_total_samples_estimate
  libFLAC_func_FLAC__seekable_stream_encoder_get_total_samples_estimate_t  FLAC__seekable_stream_encoder_get_total_samples_estimate;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_init
  libFLAC_func_FLAC__seekable_stream_encoder_init_t  FLAC__seekable_stream_encoder_init;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_finish
  libFLAC_func_FLAC__seekable_stream_encoder_finish_t  FLAC__seekable_stream_encoder_finish;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_process
  libFLAC_func_FLAC__seekable_stream_encoder_process_t  FLAC__seekable_stream_encoder_process;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_encoder_process_interleaved
  libFLAC_func_FLAC__seekable_stream_encoder_process_interleaved_t  FLAC__seekable_stream_encoder_process_interleaved;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_new
  libFLAC_func_FLAC__seekable_stream_decoder_new_t  FLAC__seekable_stream_decoder_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_delete
  libFLAC_func_FLAC__seekable_stream_decoder_delete_t  FLAC__seekable_stream_decoder_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_md5_checking
  libFLAC_func_FLAC__seekable_stream_decoder_set_md5_checking_t  FLAC__seekable_stream_decoder_set_md5_checking;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_read_callback
  libFLAC_func_FLAC__seekable_stream_decoder_set_read_callback_t  FLAC__seekable_stream_decoder_set_read_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_seek_callback
  libFLAC_func_FLAC__seekable_stream_decoder_set_seek_callback_t  FLAC__seekable_stream_decoder_set_seek_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_tell_callback
  libFLAC_func_FLAC__seekable_stream_decoder_set_tell_callback_t  FLAC__seekable_stream_decoder_set_tell_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_length_callback
  libFLAC_func_FLAC__seekable_stream_decoder_set_length_callback_t  FLAC__seekable_stream_decoder_set_length_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_eof_callback
  libFLAC_func_FLAC__seekable_stream_decoder_set_eof_callback_t  FLAC__seekable_stream_decoder_set_eof_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_write_callback
  libFLAC_func_FLAC__seekable_stream_decoder_set_write_callback_t  FLAC__seekable_stream_decoder_set_write_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_callback
  libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_callback_t  FLAC__seekable_stream_decoder_set_metadata_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_error_callback
  libFLAC_func_FLAC__seekable_stream_decoder_set_error_callback_t  FLAC__seekable_stream_decoder_set_error_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_client_data
  libFLAC_func_FLAC__seekable_stream_decoder_set_client_data_t  FLAC__seekable_stream_decoder_set_client_data;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond
  libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_t  FLAC__seekable_stream_decoder_set_metadata_respond;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_application
  libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_application_t  FLAC__seekable_stream_decoder_set_metadata_respond_application;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_all
  libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_respond_all_t  FLAC__seekable_stream_decoder_set_metadata_respond_all;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore
  libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_t  FLAC__seekable_stream_decoder_set_metadata_ignore;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_application
  libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_application_t  FLAC__seekable_stream_decoder_set_metadata_ignore_application;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_all
  libFLAC_func_FLAC__seekable_stream_decoder_set_metadata_ignore_all_t  FLAC__seekable_stream_decoder_set_metadata_ignore_all;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_state
  libFLAC_func_FLAC__seekable_stream_decoder_get_state_t  FLAC__seekable_stream_decoder_get_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_stream_decoder_state
  libFLAC_func_FLAC__seekable_stream_decoder_get_stream_decoder_state_t  FLAC__seekable_stream_decoder_get_stream_decoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_resolved_state_string
  libFLAC_func_FLAC__seekable_stream_decoder_get_resolved_state_string_t  FLAC__seekable_stream_decoder_get_resolved_state_string;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_md5_checking
  libFLAC_func_FLAC__seekable_stream_decoder_get_md5_checking_t  FLAC__seekable_stream_decoder_get_md5_checking;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channels
  libFLAC_func_FLAC__seekable_stream_decoder_get_channels_t  FLAC__seekable_stream_decoder_get_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channel_assignment
  libFLAC_func_FLAC__seekable_stream_decoder_get_channel_assignment_t  FLAC__seekable_stream_decoder_get_channel_assignment;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_bits_per_sample
  libFLAC_func_FLAC__seekable_stream_decoder_get_bits_per_sample_t  FLAC__seekable_stream_decoder_get_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_sample_rate
  libFLAC_func_FLAC__seekable_stream_decoder_get_sample_rate_t  FLAC__seekable_stream_decoder_get_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_blocksize
  libFLAC_func_FLAC__seekable_stream_decoder_get_blocksize_t  FLAC__seekable_stream_decoder_get_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_decode_position
  libFLAC_func_FLAC__seekable_stream_decoder_get_decode_position_t  FLAC__seekable_stream_decoder_get_decode_position;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_init
  libFLAC_func_FLAC__seekable_stream_decoder_init_t  FLAC__seekable_stream_decoder_init;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_finish
  libFLAC_func_FLAC__seekable_stream_decoder_finish_t  FLAC__seekable_stream_decoder_finish;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_flush
  libFLAC_func_FLAC__seekable_stream_decoder_flush_t  FLAC__seekable_stream_decoder_flush;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_reset
  libFLAC_func_FLAC__seekable_stream_decoder_reset_t  FLAC__seekable_stream_decoder_reset;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_single
  libFLAC_func_FLAC__seekable_stream_decoder_process_single_t  FLAC__seekable_stream_decoder_process_single;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_metadata
  libFLAC_func_FLAC__seekable_stream_decoder_process_until_end_of_metadata_t  FLAC__seekable_stream_decoder_process_until_end_of_metadata;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_stream
  libFLAC_func_FLAC__seekable_stream_decoder_process_until_end_of_stream_t  FLAC__seekable_stream_decoder_process_until_end_of_stream;
#endif
#ifndef IGNORE_libFLAC_FLAC__seekable_stream_decoder_seek_absolute
  libFLAC_func_FLAC__seekable_stream_decoder_seek_absolute_t  FLAC__seekable_stream_decoder_seek_absolute;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_get_streaminfo
  libFLAC_func_FLAC__metadata_get_streaminfo_t  FLAC__metadata_get_streaminfo;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_new
  libFLAC_func_FLAC__metadata_simple_iterator_new_t  FLAC__metadata_simple_iterator_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete
  libFLAC_func_FLAC__metadata_simple_iterator_delete_t  FLAC__metadata_simple_iterator_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_status
  libFLAC_func_FLAC__metadata_simple_iterator_status_t  FLAC__metadata_simple_iterator_status;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_init
  libFLAC_func_FLAC__metadata_simple_iterator_init_t  FLAC__metadata_simple_iterator_init;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_is_writable
  libFLAC_func_FLAC__metadata_simple_iterator_is_writable_t  FLAC__metadata_simple_iterator_is_writable;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_next
  libFLAC_func_FLAC__metadata_simple_iterator_next_t  FLAC__metadata_simple_iterator_next;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_prev
  libFLAC_func_FLAC__metadata_simple_iterator_prev_t  FLAC__metadata_simple_iterator_prev;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block_type
  libFLAC_func_FLAC__metadata_simple_iterator_get_block_type_t  FLAC__metadata_simple_iterator_get_block_type;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block
  libFLAC_func_FLAC__metadata_simple_iterator_get_block_t  FLAC__metadata_simple_iterator_get_block;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_set_block
  libFLAC_func_FLAC__metadata_simple_iterator_set_block_t  FLAC__metadata_simple_iterator_set_block;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_insert_block_after
  libFLAC_func_FLAC__metadata_simple_iterator_insert_block_after_t  FLAC__metadata_simple_iterator_insert_block_after;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete_block
  libFLAC_func_FLAC__metadata_simple_iterator_delete_block_t  FLAC__metadata_simple_iterator_delete_block;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_new
  libFLAC_func_FLAC__metadata_chain_new_t  FLAC__metadata_chain_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_delete
  libFLAC_func_FLAC__metadata_chain_delete_t  FLAC__metadata_chain_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_status
  libFLAC_func_FLAC__metadata_chain_status_t  FLAC__metadata_chain_status;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_read
  libFLAC_func_FLAC__metadata_chain_read_t  FLAC__metadata_chain_read;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_write
  libFLAC_func_FLAC__metadata_chain_write_t  FLAC__metadata_chain_write;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_merge_padding
  libFLAC_func_FLAC__metadata_chain_merge_padding_t  FLAC__metadata_chain_merge_padding;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_chain_sort_padding
  libFLAC_func_FLAC__metadata_chain_sort_padding_t  FLAC__metadata_chain_sort_padding;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_new
  libFLAC_func_FLAC__metadata_iterator_new_t  FLAC__metadata_iterator_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_delete
  libFLAC_func_FLAC__metadata_iterator_delete_t  FLAC__metadata_iterator_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_init
  libFLAC_func_FLAC__metadata_iterator_init_t  FLAC__metadata_iterator_init;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_next
  libFLAC_func_FLAC__metadata_iterator_next_t  FLAC__metadata_iterator_next;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_prev
  libFLAC_func_FLAC__metadata_iterator_prev_t  FLAC__metadata_iterator_prev;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_get_block_type
  libFLAC_func_FLAC__metadata_iterator_get_block_type_t  FLAC__metadata_iterator_get_block_type;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_get_block
  libFLAC_func_FLAC__metadata_iterator_get_block_t  FLAC__metadata_iterator_get_block;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_set_block
  libFLAC_func_FLAC__metadata_iterator_set_block_t  FLAC__metadata_iterator_set_block;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_delete_block
  libFLAC_func_FLAC__metadata_iterator_delete_block_t  FLAC__metadata_iterator_delete_block;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_before
  libFLAC_func_FLAC__metadata_iterator_insert_block_before_t  FLAC__metadata_iterator_insert_block_before;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_after
  libFLAC_func_FLAC__metadata_iterator_insert_block_after_t  FLAC__metadata_iterator_insert_block_after;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_new
  libFLAC_func_FLAC__metadata_object_new_t  FLAC__metadata_object_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_clone
  libFLAC_func_FLAC__metadata_object_clone_t  FLAC__metadata_object_clone;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_delete
  libFLAC_func_FLAC__metadata_object_delete_t  FLAC__metadata_object_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_is_equal
  libFLAC_func_FLAC__metadata_object_is_equal_t  FLAC__metadata_object_is_equal;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_application_set_data
  libFLAC_func_FLAC__metadata_object_application_set_data_t  FLAC__metadata_object_application_set_data;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_resize_points
  libFLAC_func_FLAC__metadata_object_seektable_resize_points_t  FLAC__metadata_object_seektable_resize_points;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_set_point
  libFLAC_func_FLAC__metadata_object_seektable_set_point_t  FLAC__metadata_object_seektable_set_point;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_insert_point
  libFLAC_func_FLAC__metadata_object_seektable_insert_point_t  FLAC__metadata_object_seektable_insert_point;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_delete_point
  libFLAC_func_FLAC__metadata_object_seektable_delete_point_t  FLAC__metadata_object_seektable_delete_point;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_is_legal
  libFLAC_func_FLAC__metadata_object_seektable_is_legal_t  FLAC__metadata_object_seektable_is_legal;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_placeholders
  libFLAC_func_FLAC__metadata_object_seektable_template_append_placeholders_t  FLAC__metadata_object_seektable_template_append_placeholders;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_point
  libFLAC_func_FLAC__metadata_object_seektable_template_append_point_t  FLAC__metadata_object_seektable_template_append_point;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_points
  libFLAC_func_FLAC__metadata_object_seektable_template_append_points_t  FLAC__metadata_object_seektable_template_append_points;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_spaced_points
  libFLAC_func_FLAC__metadata_object_seektable_template_append_spaced_points_t  FLAC__metadata_object_seektable_template_append_spaced_points;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_seektable_template_sort
  libFLAC_func_FLAC__metadata_object_seektable_template_sort_t  FLAC__metadata_object_seektable_template_sort;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_vendor_string
  libFLAC_func_FLAC__metadata_object_vorbiscomment_set_vendor_string_t  FLAC__metadata_object_vorbiscomment_set_vendor_string;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_resize_comments
  libFLAC_func_FLAC__metadata_object_vorbiscomment_resize_comments_t  FLAC__metadata_object_vorbiscomment_resize_comments;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_comment
  libFLAC_func_FLAC__metadata_object_vorbiscomment_set_comment_t  FLAC__metadata_object_vorbiscomment_set_comment;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_insert_comment
  libFLAC_func_FLAC__metadata_object_vorbiscomment_insert_comment_t  FLAC__metadata_object_vorbiscomment_insert_comment;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_delete_comment
  libFLAC_func_FLAC__metadata_object_vorbiscomment_delete_comment_t  FLAC__metadata_object_vorbiscomment_delete_comment;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_entry_matches
  libFLAC_func_FLAC__metadata_object_vorbiscomment_entry_matches_t  FLAC__metadata_object_vorbiscomment_entry_matches;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_find_entry_from
  libFLAC_func_FLAC__metadata_object_vorbiscomment_find_entry_from_t  FLAC__metadata_object_vorbiscomment_find_entry_from;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entry_matching
  libFLAC_func_FLAC__metadata_object_vorbiscomment_remove_entry_matching_t  FLAC__metadata_object_vorbiscomment_remove_entry_matching;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entries_matching
  libFLAC_func_FLAC__metadata_object_vorbiscomment_remove_entries_matching_t  FLAC__metadata_object_vorbiscomment_remove_entries_matching;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_new
  libFLAC_func_FLAC__metadata_object_cuesheet_track_new_t  FLAC__metadata_object_cuesheet_track_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_clone
  libFLAC_func_FLAC__metadata_object_cuesheet_track_clone_t  FLAC__metadata_object_cuesheet_track_clone;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete
  libFLAC_func_FLAC__metadata_object_cuesheet_track_delete_t  FLAC__metadata_object_cuesheet_track_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_resize_indices
  libFLAC_func_FLAC__metadata_object_cuesheet_track_resize_indices_t  FLAC__metadata_object_cuesheet_track_resize_indices;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_index
  libFLAC_func_FLAC__metadata_object_cuesheet_track_insert_index_t  FLAC__metadata_object_cuesheet_track_insert_index;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_blank_index
  libFLAC_func_FLAC__metadata_object_cuesheet_track_insert_blank_index_t  FLAC__metadata_object_cuesheet_track_insert_blank_index;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete_index
  libFLAC_func_FLAC__metadata_object_cuesheet_track_delete_index_t  FLAC__metadata_object_cuesheet_track_delete_index;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_resize_tracks
  libFLAC_func_FLAC__metadata_object_cuesheet_resize_tracks_t  FLAC__metadata_object_cuesheet_resize_tracks;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_set_track
  libFLAC_func_FLAC__metadata_object_cuesheet_set_track_t  FLAC__metadata_object_cuesheet_set_track;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_track
  libFLAC_func_FLAC__metadata_object_cuesheet_insert_track_t  FLAC__metadata_object_cuesheet_insert_track;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_blank_track
  libFLAC_func_FLAC__metadata_object_cuesheet_insert_blank_track_t  FLAC__metadata_object_cuesheet_insert_blank_track;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_delete_track
  libFLAC_func_FLAC__metadata_object_cuesheet_delete_track_t  FLAC__metadata_object_cuesheet_delete_track;
#endif
#ifndef IGNORE_libFLAC_FLAC__metadata_object_cuesheet_is_legal
  libFLAC_func_FLAC__metadata_object_cuesheet_is_legal_t  FLAC__metadata_object_cuesheet_is_legal;
#endif
#ifndef IGNORE_libFLAC_FLAC__format_sample_rate_is_valid
  libFLAC_func_FLAC__format_sample_rate_is_valid_t  FLAC__format_sample_rate_is_valid;
#endif
#ifndef IGNORE_libFLAC_FLAC__format_seektable_is_legal
  libFLAC_func_FLAC__format_seektable_is_legal_t  FLAC__format_seektable_is_legal;
#endif
#ifndef IGNORE_libFLAC_FLAC__format_seektable_sort
  libFLAC_func_FLAC__format_seektable_sort_t  FLAC__format_seektable_sort;
#endif
#ifndef IGNORE_libFLAC_FLAC__format_cuesheet_is_legal
  libFLAC_func_FLAC__format_cuesheet_is_legal_t  FLAC__format_cuesheet_is_legal;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_new
  libFLAC_func_FLAC__file_encoder_new_t  FLAC__file_encoder_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_delete
  libFLAC_func_FLAC__file_encoder_delete_t  FLAC__file_encoder_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_verify
  libFLAC_func_FLAC__file_encoder_set_verify_t  FLAC__file_encoder_set_verify;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_streamable_subset
  libFLAC_func_FLAC__file_encoder_set_streamable_subset_t  FLAC__file_encoder_set_streamable_subset;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_mid_side_stereo
  libFLAC_func_FLAC__file_encoder_set_do_mid_side_stereo_t  FLAC__file_encoder_set_do_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_loose_mid_side_stereo
  libFLAC_func_FLAC__file_encoder_set_loose_mid_side_stereo_t  FLAC__file_encoder_set_loose_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_channels
  libFLAC_func_FLAC__file_encoder_set_channels_t  FLAC__file_encoder_set_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_bits_per_sample
  libFLAC_func_FLAC__file_encoder_set_bits_per_sample_t  FLAC__file_encoder_set_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_sample_rate
  libFLAC_func_FLAC__file_encoder_set_sample_rate_t  FLAC__file_encoder_set_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_blocksize
  libFLAC_func_FLAC__file_encoder_set_blocksize_t  FLAC__file_encoder_set_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_max_lpc_order
  libFLAC_func_FLAC__file_encoder_set_max_lpc_order_t  FLAC__file_encoder_set_max_lpc_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_qlp_coeff_precision
  libFLAC_func_FLAC__file_encoder_set_qlp_coeff_precision_t  FLAC__file_encoder_set_qlp_coeff_precision;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_qlp_coeff_prec_search
  libFLAC_func_FLAC__file_encoder_set_do_qlp_coeff_prec_search_t  FLAC__file_encoder_set_do_qlp_coeff_prec_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_escape_coding
  libFLAC_func_FLAC__file_encoder_set_do_escape_coding_t  FLAC__file_encoder_set_do_escape_coding;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_do_exhaustive_model_search
  libFLAC_func_FLAC__file_encoder_set_do_exhaustive_model_search_t  FLAC__file_encoder_set_do_exhaustive_model_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_min_residual_partition_order
  libFLAC_func_FLAC__file_encoder_set_min_residual_partition_order_t  FLAC__file_encoder_set_min_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_max_residual_partition_order
  libFLAC_func_FLAC__file_encoder_set_max_residual_partition_order_t  FLAC__file_encoder_set_max_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_rice_parameter_search_dist
  libFLAC_func_FLAC__file_encoder_set_rice_parameter_search_dist_t  FLAC__file_encoder_set_rice_parameter_search_dist;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_total_samples_estimate
  libFLAC_func_FLAC__file_encoder_set_total_samples_estimate_t  FLAC__file_encoder_set_total_samples_estimate;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_metadata
  libFLAC_func_FLAC__file_encoder_set_metadata_t  FLAC__file_encoder_set_metadata;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_filename
  libFLAC_func_FLAC__file_encoder_set_filename_t  FLAC__file_encoder_set_filename;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_progress_callback
  libFLAC_func_FLAC__file_encoder_set_progress_callback_t  FLAC__file_encoder_set_progress_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_set_client_data
  libFLAC_func_FLAC__file_encoder_set_client_data_t  FLAC__file_encoder_set_client_data;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_state
  libFLAC_func_FLAC__file_encoder_get_state_t  FLAC__file_encoder_get_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_seekable_stream_encoder_state
  libFLAC_func_FLAC__file_encoder_get_seekable_stream_encoder_state_t  FLAC__file_encoder_get_seekable_stream_encoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_stream_encoder_state
  libFLAC_func_FLAC__file_encoder_get_stream_encoder_state_t  FLAC__file_encoder_get_stream_encoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_state
  libFLAC_func_FLAC__file_encoder_get_verify_decoder_state_t  FLAC__file_encoder_get_verify_decoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_resolved_state_string
  libFLAC_func_FLAC__file_encoder_get_resolved_state_string_t  FLAC__file_encoder_get_resolved_state_string;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_error_stats
  libFLAC_func_FLAC__file_encoder_get_verify_decoder_error_stats_t  FLAC__file_encoder_get_verify_decoder_error_stats;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_verify
  libFLAC_func_FLAC__file_encoder_get_verify_t  FLAC__file_encoder_get_verify;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_streamable_subset
  libFLAC_func_FLAC__file_encoder_get_streamable_subset_t  FLAC__file_encoder_get_streamable_subset;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_mid_side_stereo
  libFLAC_func_FLAC__file_encoder_get_do_mid_side_stereo_t  FLAC__file_encoder_get_do_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_loose_mid_side_stereo
  libFLAC_func_FLAC__file_encoder_get_loose_mid_side_stereo_t  FLAC__file_encoder_get_loose_mid_side_stereo;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_channels
  libFLAC_func_FLAC__file_encoder_get_channels_t  FLAC__file_encoder_get_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_bits_per_sample
  libFLAC_func_FLAC__file_encoder_get_bits_per_sample_t  FLAC__file_encoder_get_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_sample_rate
  libFLAC_func_FLAC__file_encoder_get_sample_rate_t  FLAC__file_encoder_get_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_blocksize
  libFLAC_func_FLAC__file_encoder_get_blocksize_t  FLAC__file_encoder_get_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_max_lpc_order
  libFLAC_func_FLAC__file_encoder_get_max_lpc_order_t  FLAC__file_encoder_get_max_lpc_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_qlp_coeff_precision
  libFLAC_func_FLAC__file_encoder_get_qlp_coeff_precision_t  FLAC__file_encoder_get_qlp_coeff_precision;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_qlp_coeff_prec_search
  libFLAC_func_FLAC__file_encoder_get_do_qlp_coeff_prec_search_t  FLAC__file_encoder_get_do_qlp_coeff_prec_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_escape_coding
  libFLAC_func_FLAC__file_encoder_get_do_escape_coding_t  FLAC__file_encoder_get_do_escape_coding;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_do_exhaustive_model_search
  libFLAC_func_FLAC__file_encoder_get_do_exhaustive_model_search_t  FLAC__file_encoder_get_do_exhaustive_model_search;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_min_residual_partition_order
  libFLAC_func_FLAC__file_encoder_get_min_residual_partition_order_t  FLAC__file_encoder_get_min_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_max_residual_partition_order
  libFLAC_func_FLAC__file_encoder_get_max_residual_partition_order_t  FLAC__file_encoder_get_max_residual_partition_order;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_rice_parameter_search_dist
  libFLAC_func_FLAC__file_encoder_get_rice_parameter_search_dist_t  FLAC__file_encoder_get_rice_parameter_search_dist;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_get_total_samples_estimate
  libFLAC_func_FLAC__file_encoder_get_total_samples_estimate_t  FLAC__file_encoder_get_total_samples_estimate;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_init
  libFLAC_func_FLAC__file_encoder_init_t  FLAC__file_encoder_init;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_finish
  libFLAC_func_FLAC__file_encoder_finish_t  FLAC__file_encoder_finish;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_process
  libFLAC_func_FLAC__file_encoder_process_t  FLAC__file_encoder_process;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_encoder_process_interleaved
  libFLAC_func_FLAC__file_encoder_process_interleaved_t  FLAC__file_encoder_process_interleaved;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_new
  libFLAC_func_FLAC__file_decoder_new_t  FLAC__file_decoder_new;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_delete
  libFLAC_func_FLAC__file_decoder_delete_t  FLAC__file_decoder_delete;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_md5_checking
  libFLAC_func_FLAC__file_decoder_set_md5_checking_t  FLAC__file_decoder_set_md5_checking;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_filename
  libFLAC_func_FLAC__file_decoder_set_filename_t  FLAC__file_decoder_set_filename;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_write_callback
  libFLAC_func_FLAC__file_decoder_set_write_callback_t  FLAC__file_decoder_set_write_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_callback
  libFLAC_func_FLAC__file_decoder_set_metadata_callback_t  FLAC__file_decoder_set_metadata_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_error_callback
  libFLAC_func_FLAC__file_decoder_set_error_callback_t  FLAC__file_decoder_set_error_callback;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_client_data
  libFLAC_func_FLAC__file_decoder_set_client_data_t  FLAC__file_decoder_set_client_data;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond
  libFLAC_func_FLAC__file_decoder_set_metadata_respond_t  FLAC__file_decoder_set_metadata_respond;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_application
  libFLAC_func_FLAC__file_decoder_set_metadata_respond_application_t  FLAC__file_decoder_set_metadata_respond_application;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_all
  libFLAC_func_FLAC__file_decoder_set_metadata_respond_all_t  FLAC__file_decoder_set_metadata_respond_all;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore
  libFLAC_func_FLAC__file_decoder_set_metadata_ignore_t  FLAC__file_decoder_set_metadata_ignore;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_application
  libFLAC_func_FLAC__file_decoder_set_metadata_ignore_application_t  FLAC__file_decoder_set_metadata_ignore_application;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_all
  libFLAC_func_FLAC__file_decoder_set_metadata_ignore_all_t  FLAC__file_decoder_set_metadata_ignore_all;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_state
  libFLAC_func_FLAC__file_decoder_get_state_t  FLAC__file_decoder_get_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_seekable_stream_decoder_state
  libFLAC_func_FLAC__file_decoder_get_seekable_stream_decoder_state_t  FLAC__file_decoder_get_seekable_stream_decoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_stream_decoder_state
  libFLAC_func_FLAC__file_decoder_get_stream_decoder_state_t  FLAC__file_decoder_get_stream_decoder_state;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_resolved_state_string
  libFLAC_func_FLAC__file_decoder_get_resolved_state_string_t  FLAC__file_decoder_get_resolved_state_string;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_md5_checking
  libFLAC_func_FLAC__file_decoder_get_md5_checking_t  FLAC__file_decoder_get_md5_checking;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_channels
  libFLAC_func_FLAC__file_decoder_get_channels_t  FLAC__file_decoder_get_channels;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_channel_assignment
  libFLAC_func_FLAC__file_decoder_get_channel_assignment_t  FLAC__file_decoder_get_channel_assignment;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_bits_per_sample
  libFLAC_func_FLAC__file_decoder_get_bits_per_sample_t  FLAC__file_decoder_get_bits_per_sample;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_sample_rate
  libFLAC_func_FLAC__file_decoder_get_sample_rate_t  FLAC__file_decoder_get_sample_rate;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_blocksize
  libFLAC_func_FLAC__file_decoder_get_blocksize_t  FLAC__file_decoder_get_blocksize;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_get_decode_position
  libFLAC_func_FLAC__file_decoder_get_decode_position_t  FLAC__file_decoder_get_decode_position;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_init
  libFLAC_func_FLAC__file_decoder_init_t  FLAC__file_decoder_init;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_finish
  libFLAC_func_FLAC__file_decoder_finish_t  FLAC__file_decoder_finish;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_single
  libFLAC_func_FLAC__file_decoder_process_single_t  FLAC__file_decoder_process_single;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_metadata
  libFLAC_func_FLAC__file_decoder_process_until_end_of_metadata_t  FLAC__file_decoder_process_until_end_of_metadata;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_file
  libFLAC_func_FLAC__file_decoder_process_until_end_of_file_t  FLAC__file_decoder_process_until_end_of_file;
#endif
#ifndef IGNORE_libFLAC_FLAC__file_decoder_seek_absolute
  libFLAC_func_FLAC__file_decoder_seek_absolute_t  FLAC__file_decoder_seek_absolute;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderStateString
  FLAC_API const char * const*   FLAC__StreamEncoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamEncoderWriteStatusString
  FLAC_API const char * const*   FLAC__StreamEncoderWriteStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderStateString
  FLAC_API const char * const*   FLAC__StreamDecoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderReadStatusString
  FLAC_API const char * const*   FLAC__StreamDecoderReadStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderWriteStatusString
  FLAC_API const char * const*   FLAC__StreamDecoderWriteStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__StreamDecoderErrorStatusString
  FLAC_API const char * const*   FLAC__StreamDecoderErrorStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderStateString
  FLAC_API const char * const*   FLAC__SeekableStreamEncoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString
  FLAC_API const char * const*   FLAC__SeekableStreamEncoderSeekStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderStateString
  FLAC_API const char * const*   FLAC__SeekableStreamDecoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderReadStatusString
  FLAC_API const char * const*   FLAC__SeekableStreamDecoderReadStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString
  FLAC_API const char * const*   FLAC__SeekableStreamDecoderSeekStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderTellStatusString
  FLAC_API const char * const*   FLAC__SeekableStreamDecoderTellStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString
  FLAC_API const char * const*   FLAC__SeekableStreamDecoderLengthStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_SimpleIteratorStatusString
  FLAC_API const char * const*   FLAC__Metadata_SimpleIteratorStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__Metadata_ChainStatusString
  FLAC_API const char * const*   FLAC__Metadata_ChainStatusString;
#endif
#ifndef IGNORE_libFLAC_FLAC__VERSION_STRING
  FLAC_API const char * *  FLAC__VERSION_STRING;
#endif
#ifndef IGNORE_libFLAC_FLAC__VENDOR_STRING
  FLAC_API const char * *  FLAC__VENDOR_STRING;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_STRING
  FLAC_API const FLAC__byte*   FLAC__STREAM_SYNC_STRING;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC
  FLAC_API const unsigned *  FLAC__STREAM_SYNC;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_SYNC_LEN
  FLAC_API const unsigned *  FLAC__STREAM_SYNC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__EntropyCodingMethodTypeString
  FLAC_API const char * const*   FLAC__EntropyCodingMethodTypeString;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN
  FLAC_API const unsigned *  FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN
  FLAC_API const unsigned *  FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN
  FLAC_API const unsigned *  FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER
  FLAC_API const unsigned *  FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER;
#endif
#ifndef IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN
  FLAC_API const unsigned *  FLAC__ENTROPY_CODING_METHOD_TYPE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SubframeTypeString
  FLAC_API const char * const*   FLAC__SubframeTypeString;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN
  FLAC_API const unsigned *  FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN
  FLAC_API const unsigned *  FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN
  FLAC_API const unsigned *  FLAC__SUBFRAME_ZERO_PAD_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LEN
  FLAC_API const unsigned *  FLAC__SUBFRAME_TYPE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN
  FLAC_API const unsigned *  FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK
  FLAC_API const unsigned *  FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK
  FLAC_API const unsigned *  FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK
  FLAC_API const unsigned *  FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK;
#endif
#ifndef IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK
  FLAC_API const unsigned *  FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK;
#endif
#ifndef IGNORE_libFLAC_FLAC__ChannelAssignmentString
  FLAC_API const char * const*   FLAC__ChannelAssignmentString;
#endif
#ifndef IGNORE_libFLAC_FLAC__FrameNumberTypeString
  FLAC_API const char * const*   FLAC__FrameNumberTypeString;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_SYNC;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_SYNC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_RESERVED_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_BLOCK_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_SAMPLE_RATE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_ZERO_PAD_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_HEADER_CRC_LEN
  FLAC_API const unsigned *  FLAC__FRAME_HEADER_CRC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN
  FLAC_API const unsigned *  FLAC__FRAME_FOOTER_CRC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__MetadataTypeString
  FLAC_API const char * const*   FLAC__MetadataTypeString;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_APPLICATION_ID_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER
  FLAC_API const FLAC__uint64 *  FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_IS_LAST_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_TYPE_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN
  FLAC_API const unsigned *  FLAC__STREAM_METADATA_LENGTH_LEN;
#endif
#ifndef IGNORE_libFLAC_FLAC__FileEncoderStateString
  FLAC_API const char * const*   FLAC__FileEncoderStateString;
#endif
#ifndef IGNORE_libFLAC_FLAC__FileDecoderStateString
  FLAC_API const char * const*   FLAC__FileDecoderStateString;
#endif
} libFLAC_dll_t;

extern libFLAC_dll_t *load_libFLAC_dll ( char *path );
extern void free_libFLAC_dll ( libFLAC_dll_t *dll );


#if defined(__cplusplus)
}  /* extern "C" { */
#endif

/***************************************************************/

#endif  /* __libFLAC_dll_h__ */

