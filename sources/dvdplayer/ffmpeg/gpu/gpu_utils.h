#include <GL/glew.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef GPU_UTILS_H
#define GPU_UTILS_H

// error codes
#define  ERROR_GLSL        -1
#define  ERROR_GLEW        -2
#define  ERROR_TEXTURE     -3
#define  ERROR_BINDFBO     -4
#define  ERROR_FBOTEXTURE  -5
#define  ERROR_PARAMS      -6

// a default draw function that will draw a single quad depending 
// on {screen,texture}{Height,Width}
extern int screenWidth, screenHeight;
extern float textureWidth,textureHeight, textureDepth;
void debug_draw_func();


typedef struct textureParameters {
  GLenum texTarget;
  GLenum texInternalFormat;
  GLenum texFormat;
}textureParameters;

void initGPU(int width, int height);
GLuint initGPGPU(int width, int height);
void adjustMatrices(int width, int height);
void deinitGPGPU();
GLuint createTexture(int width, int height, int depth, textureParameters tp);
void transferTo2DTexture(int width, int height,  textureParameters tp, GLuint tex,
			 GLenum type, void *data, int stride);
void transferTo3Dtexture(int width, int height, int depth, textureParameters tp,
			 GLuint tex, GLenum type, void *data, int stride);
void transferFromTexture(textureParameters tp, GLuint tex, GLenum type, void *data);
GLuint createGLSLProgram(const char* vertFile,const char* fragFile);
void setupUniformInt(GLuint prog, int in, char* param);
int checkFramebufferStatus();
void checkGLErrors (const char *label);
void printProgramInfoLog(GLuint obj);
void printShaderInfoLog(GLuint obj);
void printVector (const uint8_t *p, const int N);
void printMatrix (const uint8_t *p, const int stride, const int height);
char *textFileRead(const char *fn) ;
int textFileWrite(char *fn, char *s);
#endif /* GPU_UTILS_H */
