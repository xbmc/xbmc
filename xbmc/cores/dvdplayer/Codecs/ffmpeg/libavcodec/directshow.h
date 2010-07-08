#ifndef _DIRECTSHOW_DXVA_H
#define _DIRECTSHOW_DXVA_H

#include <stdint.h>
#include <dxva2api.h>



#define MAX_SLICES 16
typedef struct directshow_dxva_h264{
    DXVA_PicParams_H264   picture_params;
    DXVA_Qmatrix_H264     picture_qmatrix;

    unsigned              slice_count;
    DXVA_Slice_H264_Short slice_short[MAX_SLICES];
    DXVA_Slice_H264_Long  slice_long[MAX_SLICES];

    int                   short_ref_opaque[32]; /* maximum of 32 picture with h264_context*/
	  int                   short_ref_count; /* maximum of 32 picture with h264_context*/

    int                   long_ref_opaque[32]; /* maximum of 32 picture with h264_context*/
	  int                   long_ref_count; /* maximum of 32 picture with h264_context*/

    int                   field_type;
	  int                   slice_type;
	  int                   frame_poc;
    int64_t               frame_start;
    
    
}directshow_dxva_h264;


/*typedef struct {
    DXVA_PictureParameters pp;
    DXVA_SliceInfo         si;

    const uint8_t          *bitstream;
    unsigned               bitstream_size;
} DirectShowDxva_VC1;

typedef struct {
    DXVA_PictureParameters pp;
    DXVA_QmatrixData       qm;
    unsigned               slice_count;
    DXVA_SliceInfo         slice[MAX_SLICES];

    const uint8_t          *bitstream;
    unsigned               bitstream_size;
} DirectShowDxva_MPEG2;*/

#endif
