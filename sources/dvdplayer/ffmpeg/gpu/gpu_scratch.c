//File so I can try things out
#include "gpu_utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
float* texture, *outTexture;
int i, j;
#include "../libavcodec/h264data.h"
#include "../libavutil/common.h"
#include <GL/glew.h>
#include <GL/glut.h>

#define MAX_NEG_CROP 1024
uint8_t ff_cropTbl[256 + 2 * MAX_NEG_CROP] = {0, };

static inline long long read_time2(void)
{
    long long l;
    asm volatile("rdtsc\n\t"
                 : "=A" (l));
    return l;
}
#define GPU_STOP(id)							\
  tend = read_time2();							\
  if(tcount < 2 || tend - tstart < FFMAX(8*tsum/tcount, 2000)) {	\
    tsum += tend-tstart;						\
    tcount++;								\
  }									\
  printf("%qd cycles in %qd runs \n", tsum/tcount, tcount);	\
  tstart = read_time2(); 

#define GPU_START				\
  long long tstart, tend, tsum, tcount;		\
  tstart = read_time2();			\



void ff_h264_idct8_add_c(uint8_t *dst, DCTELEM *block, int stride);
void ff_h264_idct_add_c(uint8_t *dst, DCTELEM *block, int stride);
void ff_h264_idct8_dc_add_c(uint8_t *dst, DCTELEM *block, int stride);
void ff_h264_idct_dc_add_c(uint8_t *dst, DCTELEM *block, int stride);
void ff_h264_lowres_idct_add_c(uint8_t *dst, int stride, DCTELEM *block);
void ff_h264_lowres_idct_put_c(uint8_t *dst, int stride, DCTELEM *block);

GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, 
		     GL_COLOR_ATTACHMENT1_EXT, 
		     GL_COLOR_ATTACHMENT2_EXT, 
		     GL_COLOR_ATTACHMENT3_EXT };


DCTELEM test4x4[16] = { 55, 116, 109, 210,
			 153, 183, 148, 112,
			 166, 137, 1, 204,
			 219, 154, 114, 27};

DCTELEM test8x8[64] = { 55, 116, 109, 210, 61, 54, 74, 21,
			153, 183, 148, 112,51, 62, 123, 51,
			166, 137, 1, 204, 51, 578, 123, 67,
			219, 154, 114, 231, 64, 132, 74, 12,
			166, 137, 1, 204, 51, 578, 123, 67,
			166, 137, 1, 204, 51, 578, 123, 67,
			55, 116, 109, 210, 61, 54, 74, 21,
			55, 116, 109, 210, 61, 54, 74, 21};

DCTELEM ident8x8[64] = {1, 0, 0, 0, 0, 0, 0, 0,
			0, 1, 0, 0, 0, 0, 0, 0,
			0, 0, 1, 0, 0, 0, 0, 0,
			0, 0, 0, 1, 0, 0, 0, 0,
			0, 0, 0, 0, 1, 0, 0, 0,
			0, 0, 0, 0, 0, 1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0,
			0, 0, 0, 0, 0, 0, 0, 1};


    
void print_matrix(uint8_t* mat,  int height, int width)
{
  int i,j;

  for(i=0; i<height;i++)
    {
      for(j=0;j<width;j++)
        {
          printf(" %d ", mat[j+i*width]);
        }
      printf("\n");
    }
}

void print_dct_matrix(DCTELEM* mat,  int height, int width)
{
  int i,j;

  for(i=0; i<height;i++)    {
      for(j=0;j<width;j++)
        {
          printf(" %d ", mat[j+i*width]);
        }
      printf(";\n");
    }
}


void draw()
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  
  glClear(GL_COLOR_BUFFER_BIT);
  glBegin(GL_QUADS);
  glColor4f(1.0, 0.0, 0.0, 0.0);
  //  glTexCoord2i(0,0);
  glVertex2i(0, 0);
  //glTexCoord2i(screenWidth,0); 
  glVertex2i(screenWidth, 0);
  // glTexCoord2i(screenWidth,screenHeight); 
  glVertex2i(screenWidth, screenHeight);
  //glTexCoord2i(0,screenHeight); 
  glVertex2i(0, screenHeight);
  glEnd();
  glFlush();
  glFinish();
}

int main(void)
{
  //Create an all white texture
  textureParameters tp;
  int texWidth, texHeight;
  GLuint tex, outTex, out[4], prog, inParam, fb;
  int8_t test[16], testBig[64];
  float* outData[4];
  struct stat fileStat;

  for(i=0;i<16;i++)
    test[i] = 0;
  for(i=0;i<64;i++)
    testBig[i] = 0;
  for(i=0;i<256;i++) ff_cropTbl[i + MAX_NEG_CROP] = i;
  for(i=0;i<MAX_NEG_CROP;i++) {
    ff_cropTbl[i] = 0;
    ff_cropTbl[i + MAX_NEG_CROP + 256] = 255;
  }

  tp.texTarget		= GL_TEXTURE_RECTANGLE_ARB;
  tp.texInternalFormat	= GL_RGBA32F_ARB;
  tp.texFormat		= GL_RGBA;

  screenWidth = 4; screenHeight = 4;
  initGPU(screenWidth, screenHeight);
  initGPGPU(screenWidth, screenHeight);

  texture = calloc(1, screenWidth*screenHeight*sizeof(float));
  outTexture =  calloc(1, screenWidth*screenHeight*sizeof(float));

  texWidth = screenWidth/4; texHeight = screenHeight;

  for(i=0; i < 4; i++)
    {
      outData[i] = calloc(1, screenWidth*screenHeight/4*sizeof(float));
      out[i] = createTexture(texWidth, texHeight/4, 0, tp);
    }

  for(i=0; i<screenHeight;i++)
  {
    for(j=0; j<screenWidth; j++)
      {
	texture[j+(screenHeight-1-i)*screenWidth] = test4x4[j+i*screenWidth];
      }
  }
  tex = createTexture(texWidth, texHeight, 0, tp);
  outTex = createTexture(texWidth, texHeight, 0, tp);
  transferTo2DTexture(screenWidth/4, screenHeight, tp, tex, GL_FLOAT, texture, screenWidth/4);
  
  //set up shader
  prog = createGLSLProgram("/home/rudd/coding/gpu/ffmpeg/gpu/shaders/dct.vert", "/home/rudd/coding/gpu/ffmpeg/gpu/shaders/dct4_dc.frag");
  glUseProgram(prog);
  inParam = glGetUniformLocation(prog, "dctelems");
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(tp.texTarget, tex);
  glUniform1i(inParam, 1);
  checkGLErrors("Uniform Setup");
  glEnable( tp.texTarget );
  
  glGenFramebuffersEXT(1, &fb);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
  
  // Set up colr_tex and depth_rb for render-to-texture
  for(i=0; i < 4; i++)
    {
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
			    buffers[i],
			    tp.texTarget, out[i], 0);
    }
  //  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
  //		      GL_COLOR_ATTACHMENT1_EXT,
  //		      tp.texTarget, outTex, 0);
  //  glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
  glDrawBuffers(4, buffers);
  checkFramebufferStatus();

  GPU_START
  print_dct_matrix(test4x4, 4, 4);
  for(i = 0; i < 100; i++)
    {
      ff_h264_idct_dc_add_c(test, test4x4, 4);
      GPU_STOP("test");
    }
      print_matrix(test, 4, 4);


  print_dct_matrix(ident8x8, 8, 8);
  ff_h264_idct8_add_c(testBig, ident8x8, 8);
  printf("\n\n");
  print_matrix(testBig, 8, 8);
  printf("\n\n");
  
  // Check framebuffer completeness at the end of initialization.

  draw();
  glFinish();
 

#if 1
  for(i=0; i<4; i++)
    {
      transferFromTexture(tp, out[i], GL_FLOAT, outTexture);
      for(j=0; j < screenWidth; j++)
	printf(" %f ",  outTexture[j]);
      printf("\n");
    }
#endif

#if 0
  for(i=0; i<screenHeight/4;i++)
  {
    for(j=0; j<screenWidth; j++)
      {
	printf(" %f ", outTexture[j+i*screenWidth]);
      }
    printf("\n");
  }
#endif


  glutDisplayFunc(draw);
  glutMainLoop();  
return 0;
}
