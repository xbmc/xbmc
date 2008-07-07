/**
 * Basic utilities for openGL stuff. Just a preliminary implementation
 **/

#include "gpu_utils.h"

int screenWidth = 1280, screenHeight = 768;
float textureWidth = 800, textureHeight = 600, textureDepth = 0;

void debug_draw_func()
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  
  glClear(GL_COLOR_BUFFER_BIT);
  glBegin(GL_QUADS);
  glTexCoord3f(0,0, textureDepth);
  glVertex2i(0, 0);
  glTexCoord3f(textureWidth,0, textureDepth); 
  glVertex2i(screenWidth, 0);
  glTexCoord3f(textureWidth,textureHeight, textureDepth); 
  glVertex2i(screenWidth, screenHeight);
  glTexCoord3f(0,textureHeight, textureDepth);
  glVertex2i(0, screenHeight);
  glEnd();
  glFlush();
}

//Initalizes GLUT, GLEW, and sets up window
//Needed for testing
void initGPU(int width, int height)
{
    int argc = 1;
    char *argv[] = {"GPU Testing", NULL};
    int err = 1;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutInitWindowPosition(300, 300);
    glutCreateWindow("GPU Frame");
    //init GLEW
    err = glewInit();
    if (GLEW_OK != err) 
      {
        printf((char*)glewGetErrorString(err));
        exit(-1);
      }
}

//Initializes GPU for GPGPU computations
//Saves previous GPU state
//Viewing frustum goes from -1.0 to 1.0, texenv mode is replace
//\returns handle to fb
GLuint initGPGPU(int width, int height)
{

  //TODO: For error checking, i'm not using an FBO, I'll render to screen.
  glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0, width, height, 0.0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glViewport(0, 0, width, height);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);
  checkGLErrors("initGPGPU, Initializing and saving matrices");
}

// Deinitialized GPU for GPGPU computations
// returns the openGL state to what it was before
// initGPGPU
void deinitGPGPU()
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
  checkGLErrors("deinitGPGPU, Is this an unpaired deinit call?");
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

// Creates and sets up a texture according to the texture parameters
// Textures will use NN interpolation , no wrapping, and will use
// floats. if Depth is 0 creates a 2d texture, otherwise creates
// a 3d texture
// \param tp - struct containing parameters for the texture
// \returns handles to texture
GLuint createTexture(int width, int height, int depth, textureParameters tp)
{
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(tp.texTarget,tex);

  glTexParameteri(tp.texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(tp.texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(tp.texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(tp.texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);

  if(depth)
  {
    glTexParameteri(tp.texTarget, GL_TEXTURE_WRAP_R, GL_CLAMP);
    glTexImage3D(tp.texTarget,0,tp.texInternalFormat,width,height,depth,0,tp.texFormat,GL_FLOAT,0);
  }
  else
  {
    glTexImage2D(tp.texTarget,0,tp.texInternalFormat,width,height,0,tp.texFormat,GL_FLOAT,0);
  }

  // check if that worked
  if (glGetError() != GL_NO_ERROR) {
	printf("%s: Error creating texture", __FUNCTION__);
	exit (ERROR_TEXTURE);
  }
  
  return tex;
}

void transferTo2DTexture(int width, int height, textureParameters tp, GLuint tex, GLenum type, void *data, int stride)
{
  //TODO: optimized(?) for NVIDIA, apparently there's a better way for ATI
  glBindTexture(tp.texTarget, tex);
  glTexSubImage2D(tp.texTarget,0,0,0,width,height,tp.texFormat,type,data);
  checkGLErrors("transferTo2DTexture");
}

void transferTo3DTexture(int width, int height, int depth, textureParameters tp, GLuint tex, GLenum type, void *data, int stride)
{
  glBindTexture(tp.texTarget, tex);  
  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
  glTexSubImage3D(tp.texTarget,0,0,0,depth,width,height,1,tp.texFormat,type,data);
  checkGLErrors("transferTo3DTexture");
}

//Transfers data from a texture
void transferFromTexture(textureParameters tp, GLuint tex, GLenum type, void *data)
{
  glBindTexture(tp.texTarget, tex);
  glGetTexImage(tp.texTarget, 0, tp.texFormat, type, data);
  checkGLErrors("transferFromTexture");

}

// Creates GLSL program from one vertex shader and one fragement shader
// I prefer to code them in different files so it takes in file names
// and reads them in
GLuint createGLSLProgram(const char* vertFile,const char* fragFile)
{

  GLuint prog = glCreateProgram();
  GLhandleARB frag, vert;
  char *vertSrc, *fragSrc;

  vert = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
  frag = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

  vertSrc = textFileRead(vertFile);
  fragSrc = textFileRead(fragFile);

  const char* vs = vertSrc;
  const char* fs = fragSrc;

  //RUDD DEBUG
  printf("%s\n", fs);

  glShaderSourceARB(vert, 1, &vs, NULL);
  glShaderSourceARB(frag, 1, &fs, NULL);

  free(vertSrc);free(fragSrc);

  glCompileShaderARB(vert);
  glCompileShaderARB(frag);
  glAttachObjectARB(prog, vert);
  glAttachObjectARB(prog, frag);

  glLinkProgramARB(prog);
  glUseProgramObjectARB(prog);

  printShaderInfoLog(vert);
  printShaderInfoLog(frag);
  printProgramInfoLog(prog);

  checkGLErrors("Shader setup");

  return prog;
}

/**
    Error handling functions. Thanks to Dominik Göddeke of
    www.mathematik.uni-dortmund.de/~goeddeke/gpgpu for this
    code
**/


/**
 * Checks framebuffer status.
 * Copied directly out of the spec, modified to deliver a return value.
 */
int checkFramebufferStatus() {
    GLenum status;
    status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return 1;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
	    printf("Framebuffer incomplete, incomplete attachment\n");
            return 0;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
	    printf("Unsupported framebuffer format\n");
            return 0;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
	    printf("Framebuffer incomplete, missing attachment\n");
            return 0;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
	    printf("Framebuffer incomplete, attached images must have same dimensions\n");
            return 0;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
	    printf("Framebuffer incomplete, attached images must have same format\n");
            return 0;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
	    printf("Framebuffer incomplete, missing draw buffer\n");
            return 0;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
	    printf("Framebuffer incomplete, missing read buffer\n");
            return 0;
    }
    return 0;
}



/**
 * Checks for OpenGL errors.
 * Extremely useful debugging function: When developing, 
 * make sure to call this after almost every GL call.
 */
void checkGLErrors (const char *label) {
    GLenum errCode;
    const GLubyte *errStr;
    
    if ((errCode = glGetError()) != GL_NO_ERROR) {
	errStr = gluErrorString(errCode);
	printf("OpenGL ERROR: ");
	printf((char*)errStr);
	printf("(Label: ");
	printf(label);
	printf(")\n.");
    }
}



/**
 * error checking for GLSL
 */
void printProgramInfoLog(GLuint obj) {
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 1) {
        infoLog = (char *)malloc(infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf(infoLog);
        printf("\n");
        free(infoLog);
    }
}



void printShaderInfoLog(GLuint obj) {
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
    if (infologLength > 1) {
        infoLog = (char *)malloc(infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        printf(infoLog);
        printf("\n");
        free(infoLog);
    }
}


/**
 * Prints out given vector for debugging purposes.
 */
void printVector (const float *p, const int N) {
  int i;
  printf("printVector: \t");
  for (i=0; i<N; i++) 
    printf("  %f ",p[i]);
  printf("\n");
}

void printMatrix (const float *p, const int stride, const int height)
{
  int i, j;
  printf("printMatrix: \t");
  for(i=0; i<height; i++)
  {
    for(j=0; j<stride; j++)
    {
      printf(" %f ", p[j+i*stride]);
    }
    printf("\n");
  }
}
    

/**
   Some generic functions for reading and writing text files
**/

char *textFileRead(const char *fn) {


	FILE *fp;
	char *content = NULL;

	int f,count;
	f = open(fn, O_RDONLY);

	count = lseek(f, 0, SEEK_END);

	close(f);

	if (fn != NULL) {
		fp = fopen(fn,"rt");

		if (fp != NULL) {


			if (count > 0) {
				content = (char *)malloc(sizeof(char) * (count+1));
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
	}
	return content;
}

int textFileWrite(char *fn, char *s) {

	FILE *fp;
	int status = 0;

	if (fn != NULL) {
		fp = fopen(fn,"w");

		if (fp != NULL) {
			
			if (fwrite(s,sizeof(char),strlen(s),fp) == strlen(s))
				status = 1;
			fclose(fp);
		}
	}
	return(status);
}



