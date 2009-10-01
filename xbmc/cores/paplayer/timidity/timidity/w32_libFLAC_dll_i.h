#ifndef __libFLAC_dll_i_h__
#define __libFLAC_dll_i_h__

/* #define IGNORE_libFLAC_FLAC__StreamEncoderStateString  /* */
#define IGNORE_libFLAC_FLAC__StreamEncoderWriteStatusString  /* */
/* #define IGNORE_libFLAC_FLAC__StreamDecoderStateString  /* */
#define IGNORE_libFLAC_FLAC__StreamDecoderReadStatusString  /* */
#define IGNORE_libFLAC_FLAC__StreamDecoderWriteStatusString  /* */
#define IGNORE_libFLAC_FLAC__StreamDecoderErrorStatusString  /* */
/* #define IGNORE_libFLAC_FLAC__SeekableStreamEncoderStateString  /* */
#define IGNORE_libFLAC_FLAC__SeekableStreamEncoderSeekStatusString  /* */
/* #define IGNORE_libFLAC_FLAC__SeekableStreamDecoderStateString  /* */
#define IGNORE_libFLAC_FLAC__SeekableStreamDecoderReadStatusString  /* */
#define IGNORE_libFLAC_FLAC__SeekableStreamDecoderSeekStatusString  /* */
#define IGNORE_libFLAC_FLAC__SeekableStreamDecoderTellStatusString  /* */
#define IGNORE_libFLAC_FLAC__SeekableStreamDecoderLengthStatusString  /* */
#define IGNORE_libFLAC_FLAC__Metadata_SimpleIteratorStatusString  /* */
#define IGNORE_libFLAC_FLAC__Metadata_ChainStatusString  /* */
#define IGNORE_libFLAC_FLAC__VERSION_STRING  /* */
#define IGNORE_libFLAC_FLAC__VENDOR_STRING  /* */
#define IGNORE_libFLAC_FLAC__STREAM_SYNC_STRING  /* */
#define IGNORE_libFLAC_FLAC__STREAM_SYNC  /* */
#define IGNORE_libFLAC_FLAC__STREAM_SYNC_LEN  /* */
#define IGNORE_libFLAC_FLAC__EntropyCodingMethodTypeString  /* */
#define IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN  /* */
#define IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN  /* */
#define IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN  /* */
#define IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER  /* */
#define IGNORE_libFLAC_FLAC__ENTROPY_CODING_METHOD_TYPE_LEN  /* */
#define IGNORE_libFLAC_FLAC__SubframeTypeString  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_ZERO_PAD_LEN  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LEN  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_WASTED_BITS_FLAG_LEN  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_CONSTANT_BYTE_ALIGNED_MASK  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_VERBATIM_BYTE_ALIGNED_MASK  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_FIXED_BYTE_ALIGNED_MASK  /* */
#define IGNORE_libFLAC_FLAC__SUBFRAME_TYPE_LPC_BYTE_ALIGNED_MASK  /* */
#define IGNORE_libFLAC_FLAC__ChannelAssignmentString  /* */
#define IGNORE_libFLAC_FLAC__FrameNumberTypeString  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_SYNC_LEN  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_RESERVED_LEN  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_BLOCK_SIZE_LEN  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_SAMPLE_RATE_LEN  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_CHANNEL_ASSIGNMENT_LEN  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_BITS_PER_SAMPLE_LEN  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_ZERO_PAD_LEN  /* */
#define IGNORE_libFLAC_FLAC__FRAME_HEADER_CRC_LEN  /* */
#define IGNORE_libFLAC_FLAC__FRAME_FOOTER_CRC_LEN  /* */
#define IGNORE_libFLAC_FLAC__MetadataTypeString  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_STREAMINFO_MD5SUM_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_APPLICATION_ID_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_ENTRY_LENGTH_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_VORBIS_COMMENT_NUM_COMMENTS_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_OFFSET_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_NUMBER_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_INDEX_RESERVED_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_OFFSET_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUMBER_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_ISRC_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_TYPE_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_PRE_EMPHASIS_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_RESERVED_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_TRACK_NUM_INDICES_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_MEDIA_CATALOG_NUMBER_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_LEAD_IN_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_IS_CD_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_RESERVED_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_CUESHEET_NUM_TRACKS_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_IS_LAST_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_TYPE_LEN  /* */
#define IGNORE_libFLAC_FLAC__STREAM_METADATA_LENGTH_LEN  /* */
#define IGNORE_libFLAC_FLAC__FileEncoderStateString  /* */
#define IGNORE_libFLAC_FLAC__FileDecoderStateString  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_new  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_delete  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_verify  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_set_streamable_subset  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_do_mid_side_stereo  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_loose_mid_side_stereo  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_channels  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_bits_per_sample  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_sample_rate  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_blocksize  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_max_lpc_order  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_qlp_coeff_precision  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_do_qlp_coeff_prec_search  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_set_do_escape_coding  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_set_do_exhaustive_model_search  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_min_residual_partition_order  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_max_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_set_rice_parameter_search_dist  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_set_total_samples_estimate  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_metadata  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_write_callback  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_metadata_callback  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_set_client_data  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_get_state  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_state  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_resolved_state_string  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_verify_decoder_error_stats  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_get_verify  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_streamable_subset  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_do_mid_side_stereo  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_loose_mid_side_stereo  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_channels  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_bits_per_sample  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_sample_rate  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_blocksize  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_max_lpc_order  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_qlp_coeff_precision  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_do_qlp_coeff_prec_search  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_do_escape_coding  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_do_exhaustive_model_search  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_min_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_max_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_rice_parameter_search_dist  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_get_total_samples_estimate  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_init  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_finish  /* */
#define IGNORE_libFLAC_FLAC__stream_encoder_process  /* */
/* #define IGNORE_libFLAC_FLAC__stream_encoder_process_interleaved  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_new  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_delete  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_read_callback  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_write_callback  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_callback  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_error_callback  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_client_data  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_application  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_respond_all  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_application  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_set_metadata_ignore_all  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_get_state  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_get_channels  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_get_channel_assignment  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_get_bits_per_sample  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_get_sample_rate  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_get_blocksize  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_init  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_finish  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_flush  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_reset  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_process_single  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_metadata  /* */
#define IGNORE_libFLAC_FLAC__stream_decoder_process_until_end_of_stream  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_new  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_delete  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_verify  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_streamable_subset  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_mid_side_stereo  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_loose_mid_side_stereo  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_channels  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_bits_per_sample  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_sample_rate  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_blocksize  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_lpc_order  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_qlp_coeff_precision  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_escape_coding  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_do_exhaustive_model_search  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_min_residual_partition_order  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_max_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_rice_parameter_search_dist  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_total_samples_estimate  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_metadata  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_seek_callback  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_write_callback  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_set_client_data  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_state  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_stream_encoder_state  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_state  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_resolved_state_string  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify_decoder_error_stats  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_verify  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_streamable_subset  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_mid_side_stereo  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_loose_mid_side_stereo  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_channels  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_bits_per_sample  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_sample_rate  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_blocksize  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_lpc_order  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_qlp_coeff_precision  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_escape_coding  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_do_exhaustive_model_search  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_min_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_max_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_rice_parameter_search_dist  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_get_total_samples_estimate  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_init  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_finish  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_encoder_process  /* */
/* #define IGNORE_libFLAC_FLAC__seekable_stream_encoder_process_interleaved  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_new  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_delete  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_md5_checking  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_read_callback  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_seek_callback  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_tell_callback  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_length_callback  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_eof_callback  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_write_callback  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_callback  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_error_callback  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_client_data  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_application  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_respond_all  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_application  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_set_metadata_ignore_all  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_state  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_stream_decoder_state  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_resolved_state_string  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_md5_checking  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channels  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_channel_assignment  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_bits_per_sample  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_sample_rate  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_blocksize  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_get_decode_position  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_init  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_finish  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_flush  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_reset  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_single  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_metadata  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_process_until_end_of_stream  /* */
#define IGNORE_libFLAC_FLAC__seekable_stream_decoder_seek_absolute  /* */
#define IGNORE_libFLAC_FLAC__metadata_get_streaminfo  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_new  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_status  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_init  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_is_writable  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_next  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_prev  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block_type  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_get_block  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_set_block  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_insert_block_after  /* */
#define IGNORE_libFLAC_FLAC__metadata_simple_iterator_delete_block  /* */
#define IGNORE_libFLAC_FLAC__metadata_chain_new  /* */
#define IGNORE_libFLAC_FLAC__metadata_chain_delete  /* */
#define IGNORE_libFLAC_FLAC__metadata_chain_status  /* */
#define IGNORE_libFLAC_FLAC__metadata_chain_read  /* */
#define IGNORE_libFLAC_FLAC__metadata_chain_write  /* */
#define IGNORE_libFLAC_FLAC__metadata_chain_merge_padding  /* */
#define IGNORE_libFLAC_FLAC__metadata_chain_sort_padding  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_new  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_delete  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_init  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_next  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_prev  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_get_block_type  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_get_block  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_set_block  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_delete_block  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_before  /* */
#define IGNORE_libFLAC_FLAC__metadata_iterator_insert_block_after  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_new  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_clone  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_delete  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_is_equal  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_application_set_data  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_resize_points  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_set_point  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_insert_point  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_delete_point  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_is_legal  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_placeholders  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_point  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_points  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_template_append_spaced_points  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_seektable_template_sort  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_vendor_string  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_resize_comments  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_set_comment  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_insert_comment  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_delete_comment  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_entry_matches  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_find_entry_from  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entry_matching  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_vorbiscomment_remove_entries_matching  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_new  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_clone  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_resize_indices  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_index  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_insert_blank_index  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_track_delete_index  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_resize_tracks  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_set_track  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_track  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_insert_blank_track  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_delete_track  /* */
#define IGNORE_libFLAC_FLAC__metadata_object_cuesheet_is_legal  /* */
/* #define IGNORE_libFLAC_FLAC__format_sample_rate_is_valid  /* */
#define IGNORE_libFLAC_FLAC__format_seektable_is_legal  /* */
#define IGNORE_libFLAC_FLAC__format_seektable_sort  /* */
#define IGNORE_libFLAC_FLAC__format_cuesheet_is_legal  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_new  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_delete  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_verify  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_streamable_subset  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_do_mid_side_stereo  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_loose_mid_side_stereo  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_channels  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_bits_per_sample  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_sample_rate  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_blocksize  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_max_lpc_order  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_qlp_coeff_precision  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_do_qlp_coeff_prec_search  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_do_escape_coding  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_do_exhaustive_model_search  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_min_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_max_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_rice_parameter_search_dist  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_total_samples_estimate  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_metadata  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_filename  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_progress_callback  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_set_client_data  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_state  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_seekable_stream_encoder_state  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_stream_encoder_state  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_state  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_resolved_state_string  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_verify_decoder_error_stats  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_verify  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_streamable_subset  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_do_mid_side_stereo  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_loose_mid_side_stereo  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_channels  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_bits_per_sample  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_sample_rate  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_blocksize  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_max_lpc_order  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_qlp_coeff_precision  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_do_qlp_coeff_prec_search  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_do_escape_coding  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_do_exhaustive_model_search  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_min_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_max_residual_partition_order  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_rice_parameter_search_dist  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_get_total_samples_estimate  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_init  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_finish  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_process  /* */
#define IGNORE_libFLAC_FLAC__file_encoder_process_interleaved  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_new  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_delete  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_md5_checking  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_filename  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_write_callback  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_metadata_callback  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_error_callback  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_client_data  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_application  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_metadata_respond_all  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_application  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_set_metadata_ignore_all  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_state  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_seekable_stream_decoder_state  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_stream_decoder_state  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_resolved_state_string  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_md5_checking  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_channels  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_channel_assignment  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_bits_per_sample  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_sample_rate  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_blocksize  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_get_decode_position  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_init  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_finish  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_process_single  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_metadata  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_process_until_end_of_file  /* */
#define IGNORE_libFLAC_FLAC__file_decoder_seek_absolute  /* */

#endif  /* __libFLAC_dll_i_h__ */
