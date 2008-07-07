/**
   Test suite for GPU assisted h264 decoding. Contains
   functions for comparing the output of the normal
   ffmpeg decoding path with the GPU assisted path

**/

#include "../libavcodec/h264.h"
#include "gpu_utils.h"


void debug_draw(H264Context *h)
{
  MpegEncContext * const s = &h->s;
  int i, j, k;
  textureParameters tp;
  float* texture;
  GLuint tex;
  tp.texTarget		= GL_TEXTURE_RECTANGLE_ARB;
  tp.texInternalFormat	= GL_LUMINANCE32F_ARB;
  tp.texFormat		= GL_LUMINANCE;
  glEnable( tp.texTarget );
  //  tex = createTexture(s->linesize, s->height, 0, tp);
  //transferTo2DTexture(s->linesize, s->height, tp, tex, GL_UNSIGNED_BYTE, h->mo_comp->data[0]);
  //  transferTo2DTexture(s->linesize, s->height, tp, tex, GL_UNSIGNED_BYTE, h->ref_list[0][0].data[0]);
  glutDisplayFunc(debug_draw_func);
  glutMainLoop();
}

void gpu_cmp_motion(H264Context *h)
{
  MpegEncContext * const s = &h->s;
  int i,j, pic_width = 16*s->mb_width, pic_height = 16*s->mb_height, err = 0;
  screenWidth = 1280; screenHeight = 768;

  if(h->slice_type == FF_I_TYPE)
  {
    printf("Ignoring I slice\n");
    return;
  }
  
#if 1
  textureWidth = pic_width;
  textureHeight = pic_height;
  debug_draw(h);
#else

  for(i = 0; i < pic_height; i++)
  {
    for(j = 0; j < pic_width; j++)
    {
      if(h->mo_comp->data[0][j+i*s->linesize] != h->gpu.mo_comp.data[0][j+i*s->linesize])
	{
	  err = 1;
	  printf("Mismatch at (%d, %d). Expected: %d - Actual: %d\n", j, i,h->mo_comp->data[0][j+i*s->linesize], h->gpu.mo_comp.data[0][j+i*s->linesize]);
	}
    }
  }

  if(err)
    printf("GPU motion capture : [FAILED]\n");
  else
    printf("GPU motion capture : [PASSED]\n");
#endif
}
