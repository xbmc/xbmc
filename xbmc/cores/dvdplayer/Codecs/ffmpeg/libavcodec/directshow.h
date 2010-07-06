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
    const uint8_t         *bitstream;
    unsigned              bitstream_size;
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
