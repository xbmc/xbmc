/**
   Primary implementation of gpu assisted h.264 functions
*/
#include "h264gpu.h"
static textureParameters tp_3d, tp_2d;
#include <inttypes.h>

H264Context *g_h;
static void render_mbs();
static void render_one_block(int x, int y, int mv_x, int mv_y, int x_pix, int y_pix,
			     int x_off, int y_off, int ref, int dpb_pos);
static void setup_shaders(GPUH264Context *h);
static enum shader {fpel, hpel1, hpel2, qpel1, qpel2, qpel3};
static char* shaderfiles[6][2] = { {"shaders/fullpel.vert", "shaders/fullpel.frag"},
				   {"shaders/halfpel1.vert", "shaders/halfpel1.frag"},
				   {"shaders/halfpel2.vert", "shaders/halfpel2.frag"},
				   {"shaders/qpel1.vert", "shaders/qpel1.frag"},
				   {"shaders/qpel2.vert", "shaders/qpel2.frag"},
				   {"shaders/qpel3.vert", "shaders/qpel3.frag"} };

// TODO: Move this into GPUContext
static GLuint dispList;

//Utility functions for the DPB bitmap
/**
 * Returns the first free slice in the DPB
 */
int alloc_dpb(GPUH264Context *g)
{
  int i;
  for(i = 0; i < 16; i++)
  {
    if((g->dpb_free >> i) & 0x1)
    {
      g->dpb_free &= ~(1 << i);
      return i;
    }
  }
  printf("ERROR: DPB is full\n");
  assert(0); // DPB should never be full
  return -1;
}

/**
 * Frees a slice in the DPB
 */
void free_dpb(GPUH264Context *g, int i)
{
  assert(i < 16);
  g->dpb_free |= (1 << i);
}

void clear_dpb(GPUH264Context *g)
{
  g->dpb_free = ~0x0;
}

void gpu_init(H264Context *h)
{
  GPUH264Context * const g = &h->gpu;
  MpegEncContext * const s = &h->s;
  int pic_width = 16*s->mb_width, pic_height = 16*s->mb_height;
  int i;
  tp_3d.texTarget		= GL_TEXTURE_3D;
  tp_3d.texInternalFormat       = GL_LUMINANCE8;
  tp_3d.texFormat	        = GL_LUMINANCE;

  if(g->init) {
      printf("gpu_init called twice (Why?)\n");
      return;
    }
  //screenWidth = 1920, screenHeight = 1088;
  screenWidth = pic_width, screenHeight = pic_height;
  printf("Initializing GPU Context\n");
  initGPU(screenWidth, screenHeight);
  initGPGPU(screenWidth, screenHeight);
  glEnable(tp_3d.texTarget);

  //RUDD TEMP DPB is fixed at 64 for now
  //Nearest Power of 2?
  g->dpb_tex = createTexture(screenWidth, screenHeight, 16, tp_3d); 
  g->dpb_free = ~0x0;

  //RUDD TEST for comparison
  g->lum_residual = av_mallocz(s->linesize   * s->mb_height * 16 * sizeof(short));
  g->cr_residual  = av_mallocz(s->uvlinesize * s->mb_height * 8  * sizeof(short));
  g->cb_residual  = av_mallocz(s->uvlinesize * s->mb_height * 8  * sizeof(short));

  //RUDD TODO size?
  g->block_buffer = av_mallocz(9000*sizeof(H264mb));
  g->init = 1;

  setup_shaders(g);
}

void upload_references(H264Context *h)
{
  GPUH264Context * const g = &h->gpu;
  MpegEncContext * const s = &h->s;
  int i, list;

  // Go through reference list, uploading reference pictures as necessary
  // Note: Only I frames should be not resident on the card. and seeing as
  // I Frames are usually accompanied by an IDR, only the first pic of
  // of ref list 0 should need to be uploaded per GOP. However, for hte
  // sake of this preliminary implementation i'll keep this as is

  for(list = 0; list < 2; list++)
  {
    for(i = 0; i < h->ref_count[list]; i++)
    {
      Picture *pic = &h->ref_list[list][i];
      if(!pic->gpu_dpb && pic->data[0])
      {
        int pic_width = 16*s->mb_width, pic_height = 16*s->mb_height;
        int z, j;
        printf("Picture: %8x(Type: %d)(res: %dx%x) at index %d is not resident\n",
               pic, pic_width, pic_height,  pic->pict_type, i);
        z = alloc_dpb(g);
        transferTo3DTexture(pic_width, pic_height, z, tp_3d, g->dpb_tex,
                            GL_UNSIGNED_BYTE, pic->data[0], s->linesize);
        printf("Inserting into location %d of the DPB\n", z);
        textureWidth = pic_width, textureHeight = pic_height;
        textureDepth = z;
        glBindTexture(tp_3d.texTarget, h->gpu.dpb_tex);
      }
    }
  }
}

void draw_mbs()
{
  H264Context *h = g_h;
  GPUH264Context * const g = &h->gpu;
  MpegEncContext * const s = &h->s;
  int pic_width = 16*s->mb_width, pic_height = 16*s->mb_height;
  
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);

  GLuint refParam;
  int i;
  for(i = 0; i < 6; i++)
  {
    //g->shaders[i] = createGLSLProgram("shaders/base.vert", "shaders/fullpel.frag");
    g->shaders[i] = createGLSLProgram(shaderfiles[i][0], shaderfiles[i][1]);
    refParam = glGetUniformLocation(g->shaders[i], "dpb");
    setupUniformInt(g->shaders[i], pic_width, "tex_width");
    setupUniformInt(g->shaders[i], pic_height, "tex_height");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(tp_3d.texTarget, g->dpb_tex);
    glUniform1i(refParam, 1);
    checkGLErrors("Uniform setup");
    glCallList(dispList);
    glFinish();
  }
}

void gpu_motion(H264Context *h)
{
  printf("Beginning GPU Motion Compensation\n");
  printf("What is the size of a picture: %d\n", sizeof(Picture));
  upload_references(h);
  g_h = h;
  dispList = glGenLists(1);
  render_mbs();
  glutDisplayFunc(draw_mbs);
  glutMainLoop();
}

void setup_shaders(GPUH264Context *g)
{

}

void render_one_block(int x, int y, int mv_x, int mv_y, int xPix, int yPix,
			int x_off, int y_off, int ref,int dpb_pos)
{
  x *= 16;
  y *= 16;  
  x += x_off;
  y -= y_off;

  // Motion Vector and ref frame is stored in texcoord 0
  glTexCoord3i(mv_x, mv_y, dpb_pos);
  glVertex2f(x, y);
  glVertex2f(x, y+yPix);
  glVertex2f(x+xPix, y+yPix);
  glVertex2f(x+xPix, y);
 
}

void render_mbs()
{
  H264Context *h = g_h;
  GPUH264Context * const g = &h->gpu;
  MpegEncContext * const s = &h->s;
  H264mb* blockStore = g->block_buffer;
  int i, l;
  int lists = (h->slice_type==FF_B_TYPE)?2:1;
  int dpb_pos = s->current_picture.gpu_dpb;
  printf("Attempting to motion compensate %d blocks\n", (g->end-g->start+1));

  glNewList(dispList, GL_COMPILE);
  for(l=0; l < lists; l++)
  {
    glBegin(GL_QUADS);
    for(i= g->start; i <= g->end; i++)
    {
      const int mb_x = blockStore[i].mb_x;
      const int mb_y = blockStore[i].mb_y;
      const int mb_xy = mb_x + mb_y*s->mb_stride;
      const int mb_type = s->current_picture.mb_type[mb_xy];
      int mv_x, mv_y, j;
      
      //RUDD TODO ignoring Intra blocks for now
      if(IS_INTER(mb_type))
      {
        if(IS_16X16(mb_type) && IS_DIR(mb_type, 0, l))
          {
            mv_x = blockStore[i].mv_cache[0][ scan8[0]][0];
            mv_y = blockStore[i].mv_cache[0][ scan8[0]][1];
            render_one_block(mb_x, mb_y, mv_x, mv_y, 16, 16, 0, 0,
                             h->ref_list[l][h->ref_cache[l][ scan8[0] ]].gpu_dpb, dpb_pos);
          }
	else if(IS_16X8(mb_type))
        {
	  if(IS_DIR(mb_type, 0, l))
	  {
	      mv_x = blockStore[i].mv_cache[l][ scan8[0]][0];
	      mv_y = blockStore[i].mv_cache[l][ scan8[0]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 16, 8, 0, 0,
			       h->ref_list[l][h->ref_cache[l][ scan8[0] ]].gpu_dpb, dpb_pos);
	  }
	  if(IS_DIR(mb_type, 1, l))
	  {
	    mv_x = blockStore[i].mv_cache[0][ scan8[8]][0];
	    mv_y = blockStore[i].mv_cache[0][ scan8[8]][1];
	    render_one_block(mb_x, mb_y, mv_x, mv_y, 16, 8, 0, -8,
			     h->ref_list[l][h->ref_cache[l][ scan8[8] ]].gpu_dpb, dpb_pos);	
	  }
	}
	else if(IS_8X16(mb_type))
	{
	  if(IS_DIR(mb_type, 0, l))
          {
	    mv_x = blockStore[i].mv_cache[0][ scan8[0]][0];
	    mv_y = blockStore[i].mv_cache[0][ scan8[0]][1];
	    render_one_block(mb_x, mb_y, mv_x, mv_y, 8, 16, 0, 0,
			     h->ref_list[l][h->ref_cache[l][ scan8[0] ]].gpu_dpb, dpb_pos);
	  }
	  if(IS_DIR(mb_type, 1, l))
	  {
	      mv_x = blockStore[i].mv_cache[0][ scan8[4]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[4]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 8, 16, 8, 0,
			       h->ref_list[l][h->ref_cache[l][ scan8[4] ]].gpu_dpb, dpb_pos);
	  }
	}
	else
        {
	  assert(IS_8X8(mb_type));
	  int j;
	  for(j=0;j<4;j++)
	  {
	    const int sub_mb_type= h->sub_mb_type[j];
	    const int n= 4*j;
	    int x_offset= (j&1);
	    int y_offset= (j&2)>>1;
	    if(!IS_DIR(sub_mb_type, 0, l))
              continue;

	    if(IS_SUB_8X8(sub_mb_type))
	    {
		mv_x = blockStore[i].mv_cache[0][ scan8[n]][0];
		mv_y = blockStore[i].mv_cache[0][ scan8[n]][1];
		render_one_block(mb_x, mb_y, mv_x, mv_y, 8, 8, 8*x_offset,-8*y_offset,
			      h->ref_list[l][h->ref_cache[l][ scan8[n] ]].gpu_dpb, dpb_pos);
	    }
	    else if(IS_SUB_8X4(sub_mb_type))
	    {
	      mv_x = blockStore[i].mv_cache[0][ scan8[n]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[n]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 8, 4, 8*x_offset,-8*y_offset,
			      h->ref_list[l][h->ref_cache[l][ scan8[n] ]].gpu_dpb, dpb_pos);

	      mv_x = blockStore[i].mv_cache[0][ scan8[n+2]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[n+2]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 8, 4, 8*x_offset,-8*y_offset-4,
			    h->ref_list[l][h->ref_cache[l][ scan8[n+2] ]].gpu_dpb, dpb_pos);
	    }
	    else if(IS_SUB_4X8(sub_mb_type))
            {
	      mv_x = blockStore[i].mv_cache[0][ scan8[n]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[n]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 4, 8, 8*x_offset,-8*y_offset,
			    h->ref_list[l][h->ref_cache[l][ scan8[n] ]].gpu_dpb, dpb_pos);

	      mv_x = blockStore[i].mv_cache[0][ scan8[n+1]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[n+1]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 4, 8, 8*x_offset+4,-8*y_offset,
			    h->ref_list[l][h->ref_cache[l][ scan8[n+1] ]].gpu_dpb, dpb_pos);
	    }
            else
	    {
	      mv_x = blockStore[i].mv_cache[0][ scan8[n]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[n]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 4, 4, 8*x_offset,-8*y_offset,
			    h->ref_list[l][h->ref_cache[l][ scan8[n] ]].gpu_dpb, dpb_pos);

	      mv_x = blockStore[i].mv_cache[0][ scan8[n+1]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[n+1]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 4, 4, 8*x_offset+4,-8*y_offset,
			    h->ref_list[l][h->ref_cache[l][ scan8[n+1] ]].gpu_dpb, dpb_pos);

	      mv_x = blockStore[i].mv_cache[0][ scan8[n+2]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[n+2]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 4, 4, 8*x_offset,-8*y_offset-4,
			    h->ref_list[l][h->ref_cache[l][ scan8[n+2] ]].gpu_dpb, dpb_pos);

	      mv_x = blockStore[i].mv_cache[0][ scan8[n+3]][0];
	      mv_y = blockStore[i].mv_cache[0][ scan8[n+3]][1];
	      render_one_block(mb_x, mb_y, mv_x, mv_y, 4, 4, 8*x_offset+4,-8*y_offset-4,
			    h->ref_list[l][h->ref_cache[l][ scan8[n+3] ]].gpu_dpb, dpb_pos);
	    }
	  }
	}
      }
    }
  }
  glEnd();
  glEndList();
}
