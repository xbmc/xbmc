//
// Hack to access SDL's privates, specific to the SDL version 1.2.13 and 
// should NOT be assumed compatible with any other version.
// -d4rk (XBMC)
//
// TODO: sync to SDL at build time


#ifndef XBMC_SDL_PRIVATE
#define XBMC_SDL_PRIVATE

// APPLE only since we handle fullscreen management manually
#ifdef __APPLE__ 

#ifndef _THIS
#define _THIS void*
#endif

#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

struct SDL_VideoDevice {
	/* * * */
	/* The name of this video driver */
	const char *name;

	/* * * */
	/* Initialization/Query functions */

	/* Initialize the native video subsystem, filling 'vformat' with the 
	   "best" display pixel format, returning 0 or -1 if there's an error.
	 */
	int (*VideoInit)(_THIS, SDL_PixelFormat *vformat);

	/* List the available video modes for the given pixel format, sorted
	   from largest to smallest.
	 */
	SDL_Rect **(*ListModes)(_THIS, SDL_PixelFormat *format, Uint32 flags);

	/* Set the requested video mode, returning a surface which will be
	   set to the SDL_VideoSurface.  The width and height will already
	   be verified by ListModes(), and the video subsystem is free to
	   set the mode to a supported bit depth different from the one
	   specified -- the desired bpp will be emulated with a shadow
	   surface if necessary.  If a new mode is returned, this function
	   should take care of cleaning up the current mode.
	 */
	SDL_Surface *(*SetVideoMode)(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags);

	/* Toggle the fullscreen mode */
	int (*ToggleFullScreen)(_THIS, int on);

	/* This is called after the video mode has been set, to get the
	   initial mouse state.  It should queue events as necessary to
	   properly represent the current mouse focus and position.
	 */
	void (*UpdateMouse)(_THIS);

	/* Create a YUV video surface (possibly overlay) of the given
	   format.  The hardware should be able to perform at least 2x
	   scaling on display.
	 */
	SDL_Overlay *(*CreateYUVOverlay)(_THIS, int width, int height,
	                                 Uint32 format, SDL_Surface *display);

        /* Sets the color entries { firstcolor .. (firstcolor+ncolors-1) }
	   of the physical palette to those in 'colors'. If the device is
	   using a software palette (SDL_HWPALETTE not set), then the
	   changes are reflected in the logical palette of the screen
	   as well.
	   The return value is 1 if all entries could be set properly
	   or 0 otherwise.
	*/
	int (*SetColors)(_THIS, int firstcolor, int ncolors,
			 SDL_Color *colors);

	/* This pointer should exist in the native video subsystem and should
	   point to an appropriate update function for the current video mode
	 */
	void (*UpdateRects)(_THIS, int numrects, SDL_Rect *rects);

	/* Reverse the effects VideoInit() -- called if VideoInit() fails
	   or if the application is shutting down the video subsystem.
	*/
	void (*VideoQuit)(_THIS);

	/* * * */
	/* Hardware acceleration functions */

	/* Information about the video hardware */
	SDL_VideoInfo info;

	/* The pixel format used when SDL_CreateRGBSurface creates SDL_HWSURFACEs with alpha */
	SDL_PixelFormat* displayformatalphapixel;
	
	/* Allocates a surface in video memory */
	int (*AllocHWSurface)(_THIS, SDL_Surface *surface);

	/* Sets the hardware accelerated blit function, if any, based
	   on the current flags of the surface (colorkey, alpha, etc.)
	 */
	int (*CheckHWBlit)(_THIS, SDL_Surface *src, SDL_Surface *dst);

	/* Fills a surface rectangle with the given color */
	int (*FillHWRect)(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color);

	/* Sets video mem colorkey and accelerated blit function */
	int (*SetHWColorKey)(_THIS, SDL_Surface *surface, Uint32 key);

	/* Sets per surface hardware alpha value */
	int (*SetHWAlpha)(_THIS, SDL_Surface *surface, Uint8 value);

	/* Returns a readable/writable surface */
	int (*LockHWSurface)(_THIS, SDL_Surface *surface);
	void (*UnlockHWSurface)(_THIS, SDL_Surface *surface);

	/* Performs hardware flipping */
	int (*FlipHWSurface)(_THIS, SDL_Surface *surface);

	/* Frees a previously allocated video surface */
	void (*FreeHWSurface)(_THIS, SDL_Surface *surface);

	/* * * */
	/* Gamma support */

	Uint16 *gamma;

	/* Set the gamma correction directly (emulated with gamma ramps) */
	int (*SetGamma)(_THIS, float red, float green, float blue);

	/* Get the gamma correction directly (emulated with gamma ramps) */
	int (*GetGamma)(_THIS, float *red, float *green, float *blue);

	/* Set the gamma ramp */
	int (*SetGammaRamp)(_THIS, Uint16 *ramp);

	/* Get the gamma ramp */
	int (*GetGammaRamp)(_THIS, Uint16 *ramp);

	/* * * */
	/* OpenGL support */

	/* Sets the dll to use for OpenGL and loads it */
	int (*GL_LoadLibrary)(_THIS, const char *path);

	/* Retrieves the address of a function in the gl library */
	void* (*GL_GetProcAddress)(_THIS, const char *proc);

	/* Get attribute information from the windowing system. */
	int (*GL_GetAttribute)(_THIS, SDL_GLattr attrib, int* value);

	/* Make the context associated with this driver current */
	int (*GL_MakeCurrent)(_THIS);

	/* Swap the current buffers in double buffer mode. */
	void (*GL_SwapBuffers)(_THIS);

  	/* OpenGL functions for SDL_OPENGLBLIT */
#if SDL_VIDEO_OPENGL
#if !defined(__WIN32__)
#ifdef WINAPI
#undef WINAPI
#define WINAPI
#endif
#endif
#define SDL_PROC(ret,func,params) ret (WINAPI *func) params;
#define SDL_PROC_UNUSED(ret,func,params)
SDL_PROC_UNUSED(void,glAccum,(GLenum,GLfloat))
SDL_PROC_UNUSED(void,glAlphaFunc,(GLenum,GLclampf))
SDL_PROC_UNUSED(GLboolean,glAreTexturesResident,(GLsizei,const GLuint*,GLboolean*))
SDL_PROC_UNUSED(void,glArrayElement,(GLint))
SDL_PROC(void,glBegin,(GLenum))
SDL_PROC(void,glBindTexture,(GLenum,GLuint))
SDL_PROC_UNUSED(void,glBitmap,(GLsizei,GLsizei,GLfloat,GLfloat,GLfloat,GLfloat,const GLubyte*))
SDL_PROC(void,glBlendFunc,(GLenum,GLenum))
SDL_PROC_UNUSED(void,glCallList,(GLuint))
SDL_PROC_UNUSED(void,glCallLists,(GLsizei,GLenum,const GLvoid*))
SDL_PROC_UNUSED(void,glClear,(GLbitfield))
SDL_PROC_UNUSED(void,glClearAccum,(GLfloat,GLfloat,GLfloat,GLfloat))
SDL_PROC_UNUSED(void,glClearColor,(GLclampf,GLclampf,GLclampf,GLclampf))
SDL_PROC_UNUSED(void,glClearDepth,(GLclampd))
SDL_PROC_UNUSED(void,glClearIndex,(GLfloat))
SDL_PROC_UNUSED(void,glClearStencil,(GLint))
SDL_PROC_UNUSED(void,glClipPlane,(GLenum,const GLdouble*))
SDL_PROC_UNUSED(void,glColor3b,(GLbyte,GLbyte,GLbyte))
SDL_PROC_UNUSED(void,glColor3bv,(const GLbyte*))
SDL_PROC_UNUSED(void,glColor3d,(GLdouble,GLdouble,GLdouble))
SDL_PROC_UNUSED(void,glColor3dv,(const GLdouble*))
SDL_PROC_UNUSED(void,glColor3f,(GLfloat,GLfloat,GLfloat))
SDL_PROC_UNUSED(void,glColor3fv,(const GLfloat*))
SDL_PROC_UNUSED(void,glColor3i,(GLint,GLint,GLint))
SDL_PROC_UNUSED(void,glColor3iv,(const GLint*))
SDL_PROC_UNUSED(void,glColor3s,(GLshort,GLshort,GLshort))
SDL_PROC_UNUSED(void,glColor3sv,(const GLshort*))
SDL_PROC_UNUSED(void,glColor3ub,(GLubyte,GLubyte,GLubyte))
SDL_PROC_UNUSED(void,glColor3ubv,(const GLubyte*))
SDL_PROC_UNUSED(void,glColor3ui,(GLuint,GLuint,GLuint))
SDL_PROC_UNUSED(void,glColor3uiv,(const GLuint*))
SDL_PROC_UNUSED(void,glColor3us,(GLushort,GLushort,GLushort))
SDL_PROC_UNUSED(void,glColor3usv,(const GLushort*))
SDL_PROC_UNUSED(void,glColor4b,(GLbyte,GLbyte,GLbyte,GLbyte))
SDL_PROC_UNUSED(void,glColor4bv,(const GLbyte*))
SDL_PROC_UNUSED(void,glColor4d,(GLdouble,GLdouble,GLdouble,GLdouble))
SDL_PROC_UNUSED(void,glColor4dv,(const GLdouble*))
SDL_PROC(void,glColor4f,(GLfloat,GLfloat,GLfloat,GLfloat))
SDL_PROC_UNUSED(void,glColor4fv,(const GLfloat*))
SDL_PROC_UNUSED(void,glColor4i,(GLint,GLint,GLint,GLint))
SDL_PROC_UNUSED(void,glColor4iv,(const GLint*))
SDL_PROC_UNUSED(void,glColor4s,(GLshort,GLshort,GLshort,GLshort))
SDL_PROC_UNUSED(void,glColor4sv,(const GLshort*))
SDL_PROC_UNUSED(void,glColor4ub,(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha))
SDL_PROC_UNUSED(void,glColor4ubv,(const GLubyte *v))
SDL_PROC_UNUSED(void,glColor4ui,(GLuint red, GLuint green, GLuint blue, GLuint alpha))
SDL_PROC_UNUSED(void,glColor4uiv,(const GLuint *v))
SDL_PROC_UNUSED(void,glColor4us,(GLushort red, GLushort green, GLushort blue, GLushort alpha))
SDL_PROC_UNUSED(void,glColor4usv,(const GLushort *v))
SDL_PROC_UNUSED(void,glColorMask,(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))
SDL_PROC_UNUSED(void,glColorMaterial,(GLenum face, GLenum mode))
SDL_PROC_UNUSED(void,glColorPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC_UNUSED(void,glCopyPixels,(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type))
SDL_PROC_UNUSED(void,glCopyTexImage1D,(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border))
SDL_PROC_UNUSED(void,glCopyTexImage2D,(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border))
SDL_PROC_UNUSED(void,glCopyTexSubImage1D,(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width))
SDL_PROC_UNUSED(void,glCopyTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height))
SDL_PROC_UNUSED(void,glCullFace,(GLenum mode))
SDL_PROC_UNUSED(void,glDeleteLists,(GLuint list, GLsizei range))
SDL_PROC_UNUSED(void,glDeleteTextures,(GLsizei n, const GLuint *textures))
SDL_PROC_UNUSED(void,glDepthFunc,(GLenum func))
SDL_PROC_UNUSED(void,glDepthMask,(GLboolean flag))
SDL_PROC_UNUSED(void,glDepthRange,(GLclampd zNear, GLclampd zFar))
SDL_PROC(void,glDisable,(GLenum cap))
SDL_PROC_UNUSED(void,glDisableClientState,(GLenum array))
SDL_PROC_UNUSED(void,glDrawArrays,(GLenum mode, GLint first, GLsizei count))
SDL_PROC_UNUSED(void,glDrawBuffer,(GLenum mode))
SDL_PROC_UNUSED(void,glDrawElements,(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices))
SDL_PROC_UNUSED(void,glDrawPixels,(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC_UNUSED(void,glEdgeFlag,(GLboolean flag))
SDL_PROC_UNUSED(void,glEdgeFlagPointer,(GLsizei stride, const GLvoid *pointer))
SDL_PROC_UNUSED(void,glEdgeFlagv,(const GLboolean *flag))
SDL_PROC(void,glEnable,(GLenum cap))
SDL_PROC_UNUSED(void,glEnableClientState,(GLenum array))
SDL_PROC(void,glEnd,(void))
SDL_PROC_UNUSED(void,glEndList,(void))
SDL_PROC_UNUSED(void,glEvalCoord1d,(GLdouble u))
SDL_PROC_UNUSED(void,glEvalCoord1dv,(const GLdouble *u))
SDL_PROC_UNUSED(void,glEvalCoord1f,(GLfloat u))
SDL_PROC_UNUSED(void,glEvalCoord1fv,(const GLfloat *u))
SDL_PROC_UNUSED(void,glEvalCoord2d,(GLdouble u, GLdouble v))
SDL_PROC_UNUSED(void,glEvalCoord2dv,(const GLdouble *u))
SDL_PROC_UNUSED(void,glEvalCoord2f,(GLfloat u, GLfloat v))
SDL_PROC_UNUSED(void,glEvalCoord2fv,(const GLfloat *u))
SDL_PROC_UNUSED(void,glEvalMesh1,(GLenum mode, GLint i1, GLint i2))
SDL_PROC_UNUSED(void,glEvalMesh2,(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2))
SDL_PROC_UNUSED(void,glEvalPoint1,(GLint i))
SDL_PROC_UNUSED(void,glEvalPoint2,(GLint i, GLint j))
SDL_PROC_UNUSED(void,glFeedbackBuffer,(GLsizei size, GLenum type, GLfloat *buffer))
SDL_PROC_UNUSED(void,glFinish,(void))
SDL_PROC(void,glFlush,(void))
SDL_PROC_UNUSED(void,glFogf,(GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glFogfv,(GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glFogi,(GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glFogiv,(GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glFrontFace,(GLenum mode))
SDL_PROC_UNUSED(void,glFrustum,(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))
SDL_PROC_UNUSED(GLuint,glGenLists,(GLsizei range))
SDL_PROC(void,glGenTextures,(GLsizei n, GLuint *textures))
SDL_PROC_UNUSED(void,glGetBooleanv,(GLenum pname, GLboolean *params))
SDL_PROC_UNUSED(void,glGetClipPlane,(GLenum plane, GLdouble *equation))
SDL_PROC_UNUSED(void,glGetDoublev,(GLenum pname, GLdouble *params))
SDL_PROC_UNUSED(GLenum,glGetError,(void))
SDL_PROC_UNUSED(void,glGetFloatv,(GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetIntegerv,(GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetLightfv,(GLenum light, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetLightiv,(GLenum light, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetMapdv,(GLenum target, GLenum query, GLdouble *v))
SDL_PROC_UNUSED(void,glGetMapfv,(GLenum target, GLenum query, GLfloat *v))
SDL_PROC_UNUSED(void,glGetMapiv,(GLenum target, GLenum query, GLint *v))
SDL_PROC_UNUSED(void,glGetMaterialfv,(GLenum face, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetMaterialiv,(GLenum face, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetPixelMapfv,(GLenum map, GLfloat *values))
SDL_PROC_UNUSED(void,glGetPixelMapuiv,(GLenum map, GLuint *values))
SDL_PROC_UNUSED(void,glGetPixelMapusv,(GLenum map, GLushort *values))
SDL_PROC_UNUSED(void,glGetPointerv,(GLenum pname, GLvoid* *params))
SDL_PROC_UNUSED(void,glGetPolygonStipple,(GLubyte *mask))
SDL_PROC(const GLubyte *,glGetString,(GLenum name))
SDL_PROC_UNUSED(void,glGetTexEnvfv,(GLenum target, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetTexEnviv,(GLenum target, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetTexGendv,(GLenum coord, GLenum pname, GLdouble *params))
SDL_PROC_UNUSED(void,glGetTexGenfv,(GLenum coord, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetTexGeniv,(GLenum coord, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetTexImage,(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels))
SDL_PROC_UNUSED(void,glGetTexLevelParameterfv,(GLenum target, GLint level, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetTexLevelParameteriv,(GLenum target, GLint level, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetTexParameterfv,(GLenum target, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetTexParameteriv,(GLenum target, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glHint,(GLenum target, GLenum mode))
SDL_PROC_UNUSED(void,glIndexMask,(GLuint mask))
SDL_PROC_UNUSED(void,glIndexPointer,(GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC_UNUSED(void,glIndexd,(GLdouble c))
SDL_PROC_UNUSED(void,glIndexdv,(const GLdouble *c))
SDL_PROC_UNUSED(void,glIndexf,(GLfloat c))
SDL_PROC_UNUSED(void,glIndexfv,(const GLfloat *c))
SDL_PROC_UNUSED(void,glIndexi,(GLint c))
SDL_PROC_UNUSED(void,glIndexiv,(const GLint *c))
SDL_PROC_UNUSED(void,glIndexs,(GLshort c))
SDL_PROC_UNUSED(void,glIndexsv,(const GLshort *c))
SDL_PROC_UNUSED(void,glIndexub,(GLubyte c))
SDL_PROC_UNUSED(void,glIndexubv,(const GLubyte *c))
SDL_PROC_UNUSED(void,glInitNames,(void))
SDL_PROC_UNUSED(void,glInterleavedArrays,(GLenum format, GLsizei stride, const GLvoid *pointer))
SDL_PROC_UNUSED(GLboolean,glIsEnabled,(GLenum cap))
SDL_PROC_UNUSED(GLboolean,glIsList,(GLuint list))
SDL_PROC_UNUSED(GLboolean,glIsTexture,(GLuint texture))
SDL_PROC_UNUSED(void,glLightModelf,(GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glLightModelfv,(GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glLightModeli,(GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glLightModeliv,(GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glLightf,(GLenum light, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glLightfv,(GLenum light, GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glLighti,(GLenum light, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glLightiv,(GLenum light, GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glLineStipple,(GLint factor, GLushort pattern))
SDL_PROC_UNUSED(void,glLineWidth,(GLfloat width))
SDL_PROC_UNUSED(void,glListBase,(GLuint base))
SDL_PROC(void,glLoadIdentity,(void))
SDL_PROC_UNUSED(void,glLoadMatrixd,(const GLdouble *m))
SDL_PROC_UNUSED(void,glLoadMatrixf,(const GLfloat *m))
SDL_PROC_UNUSED(void,glLoadName,(GLuint name))
SDL_PROC_UNUSED(void,glLogicOp,(GLenum opcode))
SDL_PROC_UNUSED(void,glMap1d,(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points))
SDL_PROC_UNUSED(void,glMap1f,(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points))
SDL_PROC_UNUSED(void,glMap2d,(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points))
SDL_PROC_UNUSED(void,glMap2f,(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points))
SDL_PROC_UNUSED(void,glMapGrid1d,(GLint un, GLdouble u1, GLdouble u2))
SDL_PROC_UNUSED(void,glMapGrid1f,(GLint un, GLfloat u1, GLfloat u2))
SDL_PROC_UNUSED(void,glMapGrid2d,(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2))
SDL_PROC_UNUSED(void,glMapGrid2f,(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2))
SDL_PROC_UNUSED(void,glMaterialf,(GLenum face, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glMaterialfv,(GLenum face, GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glMateriali,(GLenum face, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glMaterialiv,(GLenum face, GLenum pname, const GLint *params))
SDL_PROC(void,glMatrixMode,(GLenum mode))
SDL_PROC_UNUSED(void,glMultMatrixd,(const GLdouble *m))
SDL_PROC_UNUSED(void,glMultMatrixf,(const GLfloat *m))
SDL_PROC_UNUSED(void,glNewList,(GLuint list, GLenum mode))
SDL_PROC_UNUSED(void,glNormal3b,(GLbyte nx, GLbyte ny, GLbyte nz))
SDL_PROC_UNUSED(void,glNormal3bv,(const GLbyte *v))
SDL_PROC_UNUSED(void,glNormal3d,(GLdouble nx, GLdouble ny, GLdouble nz))
SDL_PROC_UNUSED(void,glNormal3dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glNormal3f,(GLfloat nx, GLfloat ny, GLfloat nz))
SDL_PROC_UNUSED(void,glNormal3fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glNormal3i,(GLint nx, GLint ny, GLint nz))
SDL_PROC_UNUSED(void,glNormal3iv,(const GLint *v))
SDL_PROC_UNUSED(void,glNormal3s,(GLshort nx, GLshort ny, GLshort nz))
SDL_PROC_UNUSED(void,glNormal3sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glNormalPointer,(GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC(void,glOrtho,(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))
SDL_PROC_UNUSED(void,glPassThrough,(GLfloat token))
SDL_PROC_UNUSED(void,glPixelMapfv,(GLenum map, GLsizei mapsize, const GLfloat *values))
SDL_PROC_UNUSED(void,glPixelMapuiv,(GLenum map, GLsizei mapsize, const GLuint *values))
SDL_PROC_UNUSED(void,glPixelMapusv,(GLenum map, GLsizei mapsize, const GLushort *values))
SDL_PROC_UNUSED(void,glPixelStoref,(GLenum pname, GLfloat param))
SDL_PROC(void,glPixelStorei,(GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glPixelTransferf,(GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glPixelTransferi,(GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glPixelZoom,(GLfloat xfactor, GLfloat yfactor))
SDL_PROC_UNUSED(void,glPointSize,(GLfloat size))
SDL_PROC_UNUSED(void,glPolygonMode,(GLenum face, GLenum mode))
SDL_PROC_UNUSED(void,glPolygonOffset,(GLfloat factor, GLfloat units))
SDL_PROC_UNUSED(void,glPolygonStipple,(const GLubyte *mask))
SDL_PROC(void,glPopAttrib,(void))
SDL_PROC(void,glPopClientAttrib,(void))
SDL_PROC(void,glPopMatrix,(void))
SDL_PROC_UNUSED(void,glPopName,(void))
SDL_PROC_UNUSED(void,glPrioritizeTextures,(GLsizei n, const GLuint *textures, const GLclampf *priorities))
SDL_PROC(void,glPushAttrib,(GLbitfield mask))
SDL_PROC(void,glPushClientAttrib,(GLbitfield mask))
SDL_PROC(void,glPushMatrix,(void))
SDL_PROC_UNUSED(void,glPushName,(GLuint name))
SDL_PROC_UNUSED(void,glRasterPos2d,(GLdouble x, GLdouble y))
SDL_PROC_UNUSED(void,glRasterPos2dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glRasterPos2f,(GLfloat x, GLfloat y))
SDL_PROC_UNUSED(void,glRasterPos2fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glRasterPos2i,(GLint x, GLint y))
SDL_PROC_UNUSED(void,glRasterPos2iv,(const GLint *v))
SDL_PROC_UNUSED(void,glRasterPos2s,(GLshort x, GLshort y))
SDL_PROC_UNUSED(void,glRasterPos2sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glRasterPos3d,(GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glRasterPos3dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glRasterPos3f,(GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glRasterPos3fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glRasterPos3i,(GLint x, GLint y, GLint z))
SDL_PROC_UNUSED(void,glRasterPos3iv,(const GLint *v))
SDL_PROC_UNUSED(void,glRasterPos3s,(GLshort x, GLshort y, GLshort z))
SDL_PROC_UNUSED(void,glRasterPos3sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glRasterPos4d,(GLdouble x, GLdouble y, GLdouble z, GLdouble w))
SDL_PROC_UNUSED(void,glRasterPos4dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glRasterPos4f,(GLfloat x, GLfloat y, GLfloat z, GLfloat w))
SDL_PROC_UNUSED(void,glRasterPos4fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glRasterPos4i,(GLint x, GLint y, GLint z, GLint w))
SDL_PROC_UNUSED(void,glRasterPos4iv,(const GLint *v))
SDL_PROC_UNUSED(void,glRasterPos4s,(GLshort x, GLshort y, GLshort z, GLshort w))
SDL_PROC_UNUSED(void,glRasterPos4sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glReadBuffer,(GLenum mode))
SDL_PROC_UNUSED(void,glReadPixels,(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels))
SDL_PROC_UNUSED(void,glRectd,(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2))
SDL_PROC_UNUSED(void,glRectdv,(const GLdouble *v1, const GLdouble *v2))
SDL_PROC_UNUSED(void,glRectf,(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2))
SDL_PROC_UNUSED(void,glRectfv,(const GLfloat *v1, const GLfloat *v2))
SDL_PROC_UNUSED(void,glRecti,(GLint x1, GLint y1, GLint x2, GLint y2))
SDL_PROC_UNUSED(void,glRectiv,(const GLint *v1, const GLint *v2))
SDL_PROC_UNUSED(void,glRects,(GLshort x1, GLshort y1, GLshort x2, GLshort y2))
SDL_PROC_UNUSED(void,glRectsv,(const GLshort *v1, const GLshort *v2))
SDL_PROC_UNUSED(GLint,glRenderMode,(GLenum mode))
SDL_PROC_UNUSED(void,glRotated,(GLdouble angle, GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glRotatef,(GLfloat angle, GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glScaled,(GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glScalef,(GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glScissor,(GLint x, GLint y, GLsizei width, GLsizei height))
SDL_PROC_UNUSED(void,glSelectBuffer,(GLsizei size, GLuint *buffer))
SDL_PROC_UNUSED(void,glShadeModel,(GLenum mode))
SDL_PROC_UNUSED(void,glStencilFunc,(GLenum func, GLint ref, GLuint mask))
SDL_PROC_UNUSED(void,glStencilMask,(GLuint mask))
SDL_PROC_UNUSED(void,glStencilOp,(GLenum fail, GLenum zfail, GLenum zpass))
SDL_PROC_UNUSED(void,glTexCoord1d,(GLdouble s))
SDL_PROC_UNUSED(void,glTexCoord1dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glTexCoord1f,(GLfloat s))
SDL_PROC_UNUSED(void,glTexCoord1fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glTexCoord1i,(GLint s))
SDL_PROC_UNUSED(void,glTexCoord1iv,(const GLint *v))
SDL_PROC_UNUSED(void,glTexCoord1s,(GLshort s))
SDL_PROC_UNUSED(void,glTexCoord1sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glTexCoord2d,(GLdouble s, GLdouble t))
SDL_PROC_UNUSED(void,glTexCoord2dv,(const GLdouble *v))
SDL_PROC(void,glTexCoord2f,(GLfloat s, GLfloat t))
SDL_PROC_UNUSED(void,glTexCoord2fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glTexCoord2i,(GLint s, GLint t))
SDL_PROC_UNUSED(void,glTexCoord2iv,(const GLint *v))
SDL_PROC_UNUSED(void,glTexCoord2s,(GLshort s, GLshort t))
SDL_PROC_UNUSED(void,glTexCoord2sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glTexCoord3d,(GLdouble s, GLdouble t, GLdouble r))
SDL_PROC_UNUSED(void,glTexCoord3dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glTexCoord3f,(GLfloat s, GLfloat t, GLfloat r))
SDL_PROC_UNUSED(void,glTexCoord3fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glTexCoord3i,(GLint s, GLint t, GLint r))
SDL_PROC_UNUSED(void,glTexCoord3iv,(const GLint *v))
SDL_PROC_UNUSED(void,glTexCoord3s,(GLshort s, GLshort t, GLshort r))
SDL_PROC_UNUSED(void,glTexCoord3sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glTexCoord4d,(GLdouble s, GLdouble t, GLdouble r, GLdouble q))
SDL_PROC_UNUSED(void,glTexCoord4dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glTexCoord4f,(GLfloat s, GLfloat t, GLfloat r, GLfloat q))
SDL_PROC_UNUSED(void,glTexCoord4fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glTexCoord4i,(GLint s, GLint t, GLint r, GLint q))
SDL_PROC_UNUSED(void,glTexCoord4iv,(const GLint *v))
SDL_PROC_UNUSED(void,glTexCoord4s,(GLshort s, GLshort t, GLshort r, GLshort q))
SDL_PROC_UNUSED(void,glTexCoord4sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glTexCoordPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC(void,glTexEnvf,(GLenum target, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glTexEnvfv,(GLenum target, GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glTexEnvi,(GLenum target, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glTexEnviv,(GLenum target, GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glTexGend,(GLenum coord, GLenum pname, GLdouble param))
SDL_PROC_UNUSED(void,glTexGendv,(GLenum coord, GLenum pname, const GLdouble *params))
SDL_PROC_UNUSED(void,glTexGenf,(GLenum coord, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glTexGenfv,(GLenum coord, GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glTexGeni,(GLenum coord, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glTexGeniv,(GLenum coord, GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glTexImage1D,(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC(void,glTexImage2D,(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC_UNUSED(void,glTexParameterf,(GLenum target, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glTexParameterfv,(GLenum target, GLenum pname, const GLfloat *params))
SDL_PROC(void,glTexParameteri,(GLenum target, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glTexParameteriv,(GLenum target, GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glTexSubImage1D,(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC(void,glTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC_UNUSED(void,glTranslated,(GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glTranslatef,(GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glVertex2d,(GLdouble x, GLdouble y))
SDL_PROC_UNUSED(void,glVertex2dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glVertex2f,(GLfloat x, GLfloat y))
SDL_PROC_UNUSED(void,glVertex2fv,(const GLfloat *v))
SDL_PROC(void,glVertex2i,(GLint x, GLint y))
SDL_PROC_UNUSED(void,glVertex2iv,(const GLint *v))
SDL_PROC_UNUSED(void,glVertex2s,(GLshort x, GLshort y))
SDL_PROC_UNUSED(void,glVertex2sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glVertex3d,(GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glVertex3dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glVertex3f,(GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glVertex3fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glVertex3i,(GLint x, GLint y, GLint z))
SDL_PROC_UNUSED(void,glVertex3iv,(const GLint *v))
SDL_PROC_UNUSED(void,glVertex3s,(GLshort x, GLshort y, GLshort z))
SDL_PROC_UNUSED(void,glVertex3sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glVertex4d,(GLdouble x, GLdouble y, GLdouble z, GLdouble w))
SDL_PROC_UNUSED(void,glVertex4dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glVertex4f,(GLfloat x, GLfloat y, GLfloat z, GLfloat w))
SDL_PROC_UNUSED(void,glVertex4fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glVertex4i,(GLint x, GLint y, GLint z, GLint w))
SDL_PROC_UNUSED(void,glVertex4iv,(const GLint *v))
SDL_PROC_UNUSED(void,glVertex4s,(GLshort x, GLshort y, GLshort z, GLshort w))
SDL_PROC_UNUSED(void,glVertex4sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glVertexPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC(void,glViewport,(GLint x, GLint y, GLsizei width, GLsizei height))
#undef SDL_PROC

	/* Texture id */
	GLuint texture;
#endif
	int is_32bit;
 
	/* * * */
	/* Window manager functions */

	/* Set the title and icon text */
	void (*SetCaption)(_THIS, const char *title, const char *icon);

	/* Set the window icon image */
	void (*SetIcon)(_THIS, SDL_Surface *icon, Uint8 *mask);

	/* Iconify the window.
	   This function returns 1 if there is a window manager and the
	   window was actually iconified, it returns 0 otherwise.
	*/
	int (*IconifyWindow)(_THIS);

	/* Grab or ungrab keyboard and mouse input */
	SDL_GrabMode (*GrabInput)(_THIS, SDL_GrabMode mode);

	/* Get some platform dependent window information */
	int (*GetWMInfo)(_THIS, SDL_SysWMinfo *info);

	/* * * */
	/* Cursor manager functions */

	/* Free a window manager cursor
	   This function can be NULL if CreateWMCursor is also NULL.
	 */
	void (*FreeWMCursor)(_THIS, WMcursor *cursor);

	/* If not NULL, create a black/white window manager cursor */
	WMcursor *(*CreateWMCursor)(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);

	/* Show the specified cursor, or hide if cursor is NULL */
	int (*ShowWMCursor)(_THIS, WMcursor *cursor);

	/* Warp the window manager cursor to (x,y)
	   If NULL, a mouse motion event is posted internally.
	 */
	void (*WarpWMCursor)(_THIS, Uint16 x, Uint16 y);

	/* If not NULL, this is called when a mouse motion event occurs */
	void (*MoveWMCursor)(_THIS, int x, int y);

	/* Determine whether the mouse should be in relative mode or not.
	   This function is called when the input grab state or cursor
	   visibility state changes.
	   If the cursor is not visible, and the input is grabbed, the
	   driver can place the mouse in relative mode, which may result
	   in higher accuracy sampling of the pointer motion.
	*/
	void (*CheckMouseMode)(_THIS);

	/* * * */
	/* Event manager functions */

	/* Initialize keyboard mapping for this driver */
	void (*InitOSKeymap)(_THIS);

	/* Handle any queued OS events */
	void (*PumpEvents)(_THIS);

	/* * * */
	/* Data common to all drivers */
	SDL_Surface *screen;
	SDL_Surface *shadow;
	SDL_Surface *visible;
        SDL_Palette *physpal;	/* physical palette, if != logical palette */
        SDL_Color *gammacols;	/* gamma-corrected colours, or NULL */
	char *wm_title;
	char *wm_icon;
	int offset_x;
	int offset_y;
	SDL_GrabMode input_grab;

	/* Driver information flags */
	int handles_any_size;	/* Driver handles any size video mode */

	/* * * */
	/* Data used by the GL drivers */
	struct {
		int red_size;
		int green_size;
		int blue_size;
		int alpha_size;
		int depth_size;
		int buffer_size;
		int stencil_size;
		int double_buffer;
		int accum_red_size;
		int accum_green_size;
		int accum_blue_size;
		int accum_alpha_size;
		int stereo;
		int multisamplebuffers;
		int multisamplesamples;
		int accelerated;
		int swap_control;
		int driver_loaded;
		char driver_path[256];
		void* dll_handle;
	} gl_config;

	/* * * */
	/* Data private to this driver */
	struct SDL_PrivateVideoData *hidden;
	struct SDL_PrivateGLData *gl_data;

	/* * * */
	/* The function used to dispose of this structure */
	void (*free)(_THIS);
};

// don't care about these structures, but know that they're sizeof(void*) for now.
// -d4rk (XBMC)

#define CGDirectDisplayID int
#define CFDictionaryRef int
#define CFArrayRef int
#define CGDirectPaletteRef int
#define NSOpenGLContext int
#define NSWindow int
#define NSQuickDrawView int
#define io_connect_t int
#define NSText int
#define NSPoint int
#define SInt32 int
#define ImageDescriptionHandle int
#define MatrixRecordPtr int
#define DecompressorComponent int
#define ImageSequence int
#define CGrafPtr int
#define PlanarPixmapInfoYUV420 int

/* Main driver structure to store required state information */

typedef struct SDL_PrivateVideoData {

    BOOL               allow_screensaver;  /* 0 == disable screensaver */
    CGDirectDisplayID  display;            /* 0 == main display (only support single display) */
    CFDictionaryRef    mode;               /* current mode of the display */
    CFDictionaryRef    save_mode;          /* original mode of the display */
    CFArrayRef         mode_list;          /* list of available fullscreen modes */
    CGDirectPaletteRef palette;            /* palette of an 8-bit display */
    NSOpenGLContext    *gl_context;        /* OpenGL rendering context */
    Uint32             width, height, bpp; /* frequently used data about the display */
    Uint32             flags;              /* flags for current mode, for teardown purposes */
    Uint32             video_set;          /* boolean; indicates if video was set correctly */
    Uint32             warp_flag;          /* boolean; notify to event loop that a warp just occured */
    Uint32             warp_ticks;         /* timestamp when the warp occured */
    NSWindow           *window;            /* Cocoa window to implement the SDL window */
    NSQuickDrawView    *view;              /* the window's view; draw 2D and OpenGL into this view */
    SDL_Surface        *resize_icon;       /* icon for the resize badge, we have to draw it by hand */
    SDL_GrabMode       current_grab_mode;  /* default value is SDL_GRAB_OFF */
    SDL_Rect           **client_mode_list; /* resolution list to pass back to client */
    SDLKey             keymap[256];        /* Mac OS X to SDL key mapping */
    Uint32             current_mods;       /* current keyboard modifiers, to track modifier state */
    NSText             *field_edit;        /* a field editor for keyboard composition processing */
    Uint32             last_virtual_button;/* last virtual mouse button pressed */
    io_connect_t       power_connection;   /* used with IOKit to detect wake from sleep */
    Uint8              expect_mouse_up;    /* used to determine when to send mouse up events */
    Uint8              grab_state;         /* used to manage grab behavior */
    NSPoint            cursor_loc;         /* saved cursor coords, for activate/deactivate when grabbed */
    BOOL               cursor_should_be_visible;     /* tells if cursor is supposed to be visible (SDL_ShowCursor) */
    BOOL               cursor_visible;     /* tells if cursor is *actually* visible or not */
    Uint8*             sw_buffers[2];      /* pointers to the two software buffers for double-buffer emulation */
    SDL_Thread         *thread;            /* thread for async updates to the screen */
    SDL_sem            *sem1, *sem2;       /* synchronization for async screen updates */
    Uint8              *current_buffer;    /* the buffer being copied to the screen */
    BOOL               quit_thread;        /* used to quit the async blitting thread */
    SInt32             system_version;     /* used to dis-/enable workarounds depending on the system version */
    
    ImageDescriptionHandle yuv_idh;
    MatrixRecordPtr        yuv_matrix;
    DecompressorComponent  yuv_codec;
    ImageSequence          yuv_seq;
    PlanarPixmapInfoYUV420 *yuv_pixmap;
    Sint16                  yuv_width, yuv_height;
    CGrafPtr                yuv_port;

    void *opengl_library;    /* dynamically loaded OpenGL library. */
} SDL_PrivateVideoData;

#undef CGDirectDisplayID 
#undef CFDictionaryRef 
#undef CFArrayRef 
#undef CGDirectPaletteRef 
#undef NSOpenGLContext 
#undef NSWindow 
#undef NSQuickDrawView 
#undef io_connect_t 
#undef NSText 
#undef NSPoint 
#undef SInt32 
#undef ImageDescriptionHandle 
#undef MatrixRecordPtr 
#undef DecompressorComponent 
#undef ImageSequence 
#undef CGrafPtr 
#undef PlanarPixmapInfoYUV420 

#endif

#endif
#ifndef XBMC_SDL_PRIVATE
#define XBMC_SDL_PRIVATE
#ifdef __APPLE__
#ifndef _THIS
#define _THIS void*
#endif

struct SDL_VideoDevice {
	/* * * */
	/* The name of this video driver */
	const char *name;

	/* * * */
	/* Initialization/Query functions */

	/* Initialize the native video subsystem, filling 'vformat' with the 
	   "best" display pixel format, returning 0 or -1 if there's an error.
	 */
	int (*VideoInit)(_THIS, SDL_PixelFormat *vformat);

	/* List the available video modes for the given pixel format, sorted
	   from largest to smallest.
	 */
	SDL_Rect **(*ListModes)(_THIS, SDL_PixelFormat *format, Uint32 flags);

	/* Set the requested video mode, returning a surface which will be
	   set to the SDL_VideoSurface.  The width and height will already
	   be verified by ListModes(), and the video subsystem is free to
	   set the mode to a supported bit depth different from the one
	   specified -- the desired bpp will be emulated with a shadow
	   surface if necessary.  If a new mode is returned, this function
	   should take care of cleaning up the current mode.
	 */
	SDL_Surface *(*SetVideoMode)(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags);

	/* Toggle the fullscreen mode */
	int (*ToggleFullScreen)(_THIS, int on);

	/* This is called after the video mode has been set, to get the
	   initial mouse state.  It should queue events as necessary to
	   properly represent the current mouse focus and position.
	 */
	void (*UpdateMouse)(_THIS);

	/* Create a YUV video surface (possibly overlay) of the given
	   format.  The hardware should be able to perform at least 2x
	   scaling on display.
	 */
	SDL_Overlay *(*CreateYUVOverlay)(_THIS, int width, int height,
	                                 Uint32 format, SDL_Surface *display);

        /* Sets the color entries { firstcolor .. (firstcolor+ncolors-1) }
	   of the physical palette to those in 'colors'. If the device is
	   using a software palette (SDL_HWPALETTE not set), then the
	   changes are reflected in the logical palette of the screen
	   as well.
	   The return value is 1 if all entries could be set properly
	   or 0 otherwise.
	*/
	int (*SetColors)(_THIS, int firstcolor, int ncolors,
			 SDL_Color *colors);

	/* This pointer should exist in the native video subsystem and should
	   point to an appropriate update function for the current video mode
	 */
	void (*UpdateRects)(_THIS, int numrects, SDL_Rect *rects);

	/* Reverse the effects VideoInit() -- called if VideoInit() fails
	   or if the application is shutting down the video subsystem.
	*/
	void (*VideoQuit)(_THIS);

	/* * * */
	/* Hardware acceleration functions */

	/* Information about the video hardware */
	SDL_VideoInfo info;

	/* The pixel format used when SDL_CreateRGBSurface creates SDL_HWSURFACEs with alpha */
	SDL_PixelFormat* displayformatalphapixel;
	
	/* Allocates a surface in video memory */
	int (*AllocHWSurface)(_THIS, SDL_Surface *surface);

	/* Sets the hardware accelerated blit function, if any, based
	   on the current flags of the surface (colorkey, alpha, etc.)
	 */
	int (*CheckHWBlit)(_THIS, SDL_Surface *src, SDL_Surface *dst);

	/* Fills a surface rectangle with the given color */
	int (*FillHWRect)(_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color);

	/* Sets video mem colorkey and accelerated blit function */
	int (*SetHWColorKey)(_THIS, SDL_Surface *surface, Uint32 key);

	/* Sets per surface hardware alpha value */
	int (*SetHWAlpha)(_THIS, SDL_Surface *surface, Uint8 value);

	/* Returns a readable/writable surface */
	int (*LockHWSurface)(_THIS, SDL_Surface *surface);
	void (*UnlockHWSurface)(_THIS, SDL_Surface *surface);

	/* Performs hardware flipping */
	int (*FlipHWSurface)(_THIS, SDL_Surface *surface);

	/* Frees a previously allocated video surface */
	void (*FreeHWSurface)(_THIS, SDL_Surface *surface);

	/* * * */
	/* Gamma support */

	Uint16 *gamma;

	/* Set the gamma correction directly (emulated with gamma ramps) */
	int (*SetGamma)(_THIS, float red, float green, float blue);

	/* Get the gamma correction directly (emulated with gamma ramps) */
	int (*GetGamma)(_THIS, float *red, float *green, float *blue);

	/* Set the gamma ramp */
	int (*SetGammaRamp)(_THIS, Uint16 *ramp);

	/* Get the gamma ramp */
	int (*GetGammaRamp)(_THIS, Uint16 *ramp);

	/* * * */
	/* OpenGL support */

	/* Sets the dll to use for OpenGL and loads it */
	int (*GL_LoadLibrary)(_THIS, const char *path);

	/* Retrieves the address of a function in the gl library */
	void* (*GL_GetProcAddress)(_THIS, const char *proc);

	/* Get attribute information from the windowing system. */
	int (*GL_GetAttribute)(_THIS, SDL_GLattr attrib, int* value);

	/* Make the context associated with this driver current */
	int (*GL_MakeCurrent)(_THIS);

	/* Swap the current buffers in double buffer mode. */
	void (*GL_SwapBuffers)(_THIS);

  	/* OpenGL functions for SDL_OPENGLBLIT */
#if SDL_VIDEO_OPENGL
#if !defined(__WIN32__)
#define WINAPI
#endif
#define SDL_PROC(ret,func,params) ret (WINAPI *func) params;
#define SDL_PROC_UNUSED(ret,func,params)
SDL_PROC_UNUSED(void,glAccum,(GLenum,GLfloat))
SDL_PROC_UNUSED(void,glAlphaFunc,(GLenum,GLclampf))
SDL_PROC_UNUSED(GLboolean,glAreTexturesResident,(GLsizei,const GLuint*,GLboolean*))
SDL_PROC_UNUSED(void,glArrayElement,(GLint))
SDL_PROC(void,glBegin,(GLenum))
SDL_PROC(void,glBindTexture,(GLenum,GLuint))
SDL_PROC_UNUSED(void,glBitmap,(GLsizei,GLsizei,GLfloat,GLfloat,GLfloat,GLfloat,const GLubyte*))
SDL_PROC(void,glBlendFunc,(GLenum,GLenum))
SDL_PROC_UNUSED(void,glCallList,(GLuint))
SDL_PROC_UNUSED(void,glCallLists,(GLsizei,GLenum,const GLvoid*))
SDL_PROC_UNUSED(void,glClear,(GLbitfield))
SDL_PROC_UNUSED(void,glClearAccum,(GLfloat,GLfloat,GLfloat,GLfloat))
SDL_PROC_UNUSED(void,glClearColor,(GLclampf,GLclampf,GLclampf,GLclampf))
SDL_PROC_UNUSED(void,glClearDepth,(GLclampd))
SDL_PROC_UNUSED(void,glClearIndex,(GLfloat))
SDL_PROC_UNUSED(void,glClearStencil,(GLint))
SDL_PROC_UNUSED(void,glClipPlane,(GLenum,const GLdouble*))
SDL_PROC_UNUSED(void,glColor3b,(GLbyte,GLbyte,GLbyte))
SDL_PROC_UNUSED(void,glColor3bv,(const GLbyte*))
SDL_PROC_UNUSED(void,glColor3d,(GLdouble,GLdouble,GLdouble))
SDL_PROC_UNUSED(void,glColor3dv,(const GLdouble*))
SDL_PROC_UNUSED(void,glColor3f,(GLfloat,GLfloat,GLfloat))
SDL_PROC_UNUSED(void,glColor3fv,(const GLfloat*))
SDL_PROC_UNUSED(void,glColor3i,(GLint,GLint,GLint))
SDL_PROC_UNUSED(void,glColor3iv,(const GLint*))
SDL_PROC_UNUSED(void,glColor3s,(GLshort,GLshort,GLshort))
SDL_PROC_UNUSED(void,glColor3sv,(const GLshort*))
SDL_PROC_UNUSED(void,glColor3ub,(GLubyte,GLubyte,GLubyte))
SDL_PROC_UNUSED(void,glColor3ubv,(const GLubyte*))
SDL_PROC_UNUSED(void,glColor3ui,(GLuint,GLuint,GLuint))
SDL_PROC_UNUSED(void,glColor3uiv,(const GLuint*))
SDL_PROC_UNUSED(void,glColor3us,(GLushort,GLushort,GLushort))
SDL_PROC_UNUSED(void,glColor3usv,(const GLushort*))
SDL_PROC_UNUSED(void,glColor4b,(GLbyte,GLbyte,GLbyte,GLbyte))
SDL_PROC_UNUSED(void,glColor4bv,(const GLbyte*))
SDL_PROC_UNUSED(void,glColor4d,(GLdouble,GLdouble,GLdouble,GLdouble))
SDL_PROC_UNUSED(void,glColor4dv,(const GLdouble*))
SDL_PROC(void,glColor4f,(GLfloat,GLfloat,GLfloat,GLfloat))
SDL_PROC_UNUSED(void,glColor4fv,(const GLfloat*))
SDL_PROC_UNUSED(void,glColor4i,(GLint,GLint,GLint,GLint))
SDL_PROC_UNUSED(void,glColor4iv,(const GLint*))
SDL_PROC_UNUSED(void,glColor4s,(GLshort,GLshort,GLshort,GLshort))
SDL_PROC_UNUSED(void,glColor4sv,(const GLshort*))
SDL_PROC_UNUSED(void,glColor4ub,(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha))
SDL_PROC_UNUSED(void,glColor4ubv,(const GLubyte *v))
SDL_PROC_UNUSED(void,glColor4ui,(GLuint red, GLuint green, GLuint blue, GLuint alpha))
SDL_PROC_UNUSED(void,glColor4uiv,(const GLuint *v))
SDL_PROC_UNUSED(void,glColor4us,(GLushort red, GLushort green, GLushort blue, GLushort alpha))
SDL_PROC_UNUSED(void,glColor4usv,(const GLushort *v))
SDL_PROC_UNUSED(void,glColorMask,(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))
SDL_PROC_UNUSED(void,glColorMaterial,(GLenum face, GLenum mode))
SDL_PROC_UNUSED(void,glColorPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC_UNUSED(void,glCopyPixels,(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type))
SDL_PROC_UNUSED(void,glCopyTexImage1D,(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border))
SDL_PROC_UNUSED(void,glCopyTexImage2D,(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border))
SDL_PROC_UNUSED(void,glCopyTexSubImage1D,(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width))
SDL_PROC_UNUSED(void,glCopyTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height))
SDL_PROC_UNUSED(void,glCullFace,(GLenum mode))
SDL_PROC_UNUSED(void,glDeleteLists,(GLuint list, GLsizei range))
SDL_PROC_UNUSED(void,glDeleteTextures,(GLsizei n, const GLuint *textures))
SDL_PROC_UNUSED(void,glDepthFunc,(GLenum func))
SDL_PROC_UNUSED(void,glDepthMask,(GLboolean flag))
SDL_PROC_UNUSED(void,glDepthRange,(GLclampd zNear, GLclampd zFar))
SDL_PROC(void,glDisable,(GLenum cap))
SDL_PROC_UNUSED(void,glDisableClientState,(GLenum array))
SDL_PROC_UNUSED(void,glDrawArrays,(GLenum mode, GLint first, GLsizei count))
SDL_PROC_UNUSED(void,glDrawBuffer,(GLenum mode))
SDL_PROC_UNUSED(void,glDrawElements,(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices))
SDL_PROC_UNUSED(void,glDrawPixels,(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC_UNUSED(void,glEdgeFlag,(GLboolean flag))
SDL_PROC_UNUSED(void,glEdgeFlagPointer,(GLsizei stride, const GLvoid *pointer))
SDL_PROC_UNUSED(void,glEdgeFlagv,(const GLboolean *flag))
SDL_PROC(void,glEnable,(GLenum cap))
SDL_PROC_UNUSED(void,glEnableClientState,(GLenum array))
SDL_PROC(void,glEnd,(void))
SDL_PROC_UNUSED(void,glEndList,(void))
SDL_PROC_UNUSED(void,glEvalCoord1d,(GLdouble u))
SDL_PROC_UNUSED(void,glEvalCoord1dv,(const GLdouble *u))
SDL_PROC_UNUSED(void,glEvalCoord1f,(GLfloat u))
SDL_PROC_UNUSED(void,glEvalCoord1fv,(const GLfloat *u))
SDL_PROC_UNUSED(void,glEvalCoord2d,(GLdouble u, GLdouble v))
SDL_PROC_UNUSED(void,glEvalCoord2dv,(const GLdouble *u))
SDL_PROC_UNUSED(void,glEvalCoord2f,(GLfloat u, GLfloat v))
SDL_PROC_UNUSED(void,glEvalCoord2fv,(const GLfloat *u))
SDL_PROC_UNUSED(void,glEvalMesh1,(GLenum mode, GLint i1, GLint i2))
SDL_PROC_UNUSED(void,glEvalMesh2,(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2))
SDL_PROC_UNUSED(void,glEvalPoint1,(GLint i))
SDL_PROC_UNUSED(void,glEvalPoint2,(GLint i, GLint j))
SDL_PROC_UNUSED(void,glFeedbackBuffer,(GLsizei size, GLenum type, GLfloat *buffer))
SDL_PROC_UNUSED(void,glFinish,(void))
SDL_PROC(void,glFlush,(void))
SDL_PROC_UNUSED(void,glFogf,(GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glFogfv,(GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glFogi,(GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glFogiv,(GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glFrontFace,(GLenum mode))
SDL_PROC_UNUSED(void,glFrustum,(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))
SDL_PROC_UNUSED(GLuint,glGenLists,(GLsizei range))
SDL_PROC(void,glGenTextures,(GLsizei n, GLuint *textures))
SDL_PROC_UNUSED(void,glGetBooleanv,(GLenum pname, GLboolean *params))
SDL_PROC_UNUSED(void,glGetClipPlane,(GLenum plane, GLdouble *equation))
SDL_PROC_UNUSED(void,glGetDoublev,(GLenum pname, GLdouble *params))
SDL_PROC_UNUSED(GLenum,glGetError,(void))
SDL_PROC_UNUSED(void,glGetFloatv,(GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetIntegerv,(GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetLightfv,(GLenum light, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetLightiv,(GLenum light, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetMapdv,(GLenum target, GLenum query, GLdouble *v))
SDL_PROC_UNUSED(void,glGetMapfv,(GLenum target, GLenum query, GLfloat *v))
SDL_PROC_UNUSED(void,glGetMapiv,(GLenum target, GLenum query, GLint *v))
SDL_PROC_UNUSED(void,glGetMaterialfv,(GLenum face, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetMaterialiv,(GLenum face, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetPixelMapfv,(GLenum map, GLfloat *values))
SDL_PROC_UNUSED(void,glGetPixelMapuiv,(GLenum map, GLuint *values))
SDL_PROC_UNUSED(void,glGetPixelMapusv,(GLenum map, GLushort *values))
SDL_PROC_UNUSED(void,glGetPointerv,(GLenum pname, GLvoid* *params))
SDL_PROC_UNUSED(void,glGetPolygonStipple,(GLubyte *mask))
SDL_PROC(const GLubyte *,glGetString,(GLenum name))
SDL_PROC_UNUSED(void,glGetTexEnvfv,(GLenum target, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetTexEnviv,(GLenum target, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetTexGendv,(GLenum coord, GLenum pname, GLdouble *params))
SDL_PROC_UNUSED(void,glGetTexGenfv,(GLenum coord, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetTexGeniv,(GLenum coord, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetTexImage,(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels))
SDL_PROC_UNUSED(void,glGetTexLevelParameterfv,(GLenum target, GLint level, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetTexLevelParameteriv,(GLenum target, GLint level, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glGetTexParameterfv,(GLenum target, GLenum pname, GLfloat *params))
SDL_PROC_UNUSED(void,glGetTexParameteriv,(GLenum target, GLenum pname, GLint *params))
SDL_PROC_UNUSED(void,glHint,(GLenum target, GLenum mode))
SDL_PROC_UNUSED(void,glIndexMask,(GLuint mask))
SDL_PROC_UNUSED(void,glIndexPointer,(GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC_UNUSED(void,glIndexd,(GLdouble c))
SDL_PROC_UNUSED(void,glIndexdv,(const GLdouble *c))
SDL_PROC_UNUSED(void,glIndexf,(GLfloat c))
SDL_PROC_UNUSED(void,glIndexfv,(const GLfloat *c))
SDL_PROC_UNUSED(void,glIndexi,(GLint c))
SDL_PROC_UNUSED(void,glIndexiv,(const GLint *c))
SDL_PROC_UNUSED(void,glIndexs,(GLshort c))
SDL_PROC_UNUSED(void,glIndexsv,(const GLshort *c))
SDL_PROC_UNUSED(void,glIndexub,(GLubyte c))
SDL_PROC_UNUSED(void,glIndexubv,(const GLubyte *c))
SDL_PROC_UNUSED(void,glInitNames,(void))
SDL_PROC_UNUSED(void,glInterleavedArrays,(GLenum format, GLsizei stride, const GLvoid *pointer))
SDL_PROC_UNUSED(GLboolean,glIsEnabled,(GLenum cap))
SDL_PROC_UNUSED(GLboolean,glIsList,(GLuint list))
SDL_PROC_UNUSED(GLboolean,glIsTexture,(GLuint texture))
SDL_PROC_UNUSED(void,glLightModelf,(GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glLightModelfv,(GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glLightModeli,(GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glLightModeliv,(GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glLightf,(GLenum light, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glLightfv,(GLenum light, GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glLighti,(GLenum light, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glLightiv,(GLenum light, GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glLineStipple,(GLint factor, GLushort pattern))
SDL_PROC_UNUSED(void,glLineWidth,(GLfloat width))
SDL_PROC_UNUSED(void,glListBase,(GLuint base))
SDL_PROC(void,glLoadIdentity,(void))
SDL_PROC_UNUSED(void,glLoadMatrixd,(const GLdouble *m))
SDL_PROC_UNUSED(void,glLoadMatrixf,(const GLfloat *m))
SDL_PROC_UNUSED(void,glLoadName,(GLuint name))
SDL_PROC_UNUSED(void,glLogicOp,(GLenum opcode))
SDL_PROC_UNUSED(void,glMap1d,(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points))
SDL_PROC_UNUSED(void,glMap1f,(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points))
SDL_PROC_UNUSED(void,glMap2d,(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points))
SDL_PROC_UNUSED(void,glMap2f,(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points))
SDL_PROC_UNUSED(void,glMapGrid1d,(GLint un, GLdouble u1, GLdouble u2))
SDL_PROC_UNUSED(void,glMapGrid1f,(GLint un, GLfloat u1, GLfloat u2))
SDL_PROC_UNUSED(void,glMapGrid2d,(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2))
SDL_PROC_UNUSED(void,glMapGrid2f,(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2))
SDL_PROC_UNUSED(void,glMaterialf,(GLenum face, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glMaterialfv,(GLenum face, GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glMateriali,(GLenum face, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glMaterialiv,(GLenum face, GLenum pname, const GLint *params))
SDL_PROC(void,glMatrixMode,(GLenum mode))
SDL_PROC_UNUSED(void,glMultMatrixd,(const GLdouble *m))
SDL_PROC_UNUSED(void,glMultMatrixf,(const GLfloat *m))
SDL_PROC_UNUSED(void,glNewList,(GLuint list, GLenum mode))
SDL_PROC_UNUSED(void,glNormal3b,(GLbyte nx, GLbyte ny, GLbyte nz))
SDL_PROC_UNUSED(void,glNormal3bv,(const GLbyte *v))
SDL_PROC_UNUSED(void,glNormal3d,(GLdouble nx, GLdouble ny, GLdouble nz))
SDL_PROC_UNUSED(void,glNormal3dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glNormal3f,(GLfloat nx, GLfloat ny, GLfloat nz))
SDL_PROC_UNUSED(void,glNormal3fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glNormal3i,(GLint nx, GLint ny, GLint nz))
SDL_PROC_UNUSED(void,glNormal3iv,(const GLint *v))
SDL_PROC_UNUSED(void,glNormal3s,(GLshort nx, GLshort ny, GLshort nz))
SDL_PROC_UNUSED(void,glNormal3sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glNormalPointer,(GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC(void,glOrtho,(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar))
SDL_PROC_UNUSED(void,glPassThrough,(GLfloat token))
SDL_PROC_UNUSED(void,glPixelMapfv,(GLenum map, GLsizei mapsize, const GLfloat *values))
SDL_PROC_UNUSED(void,glPixelMapuiv,(GLenum map, GLsizei mapsize, const GLuint *values))
SDL_PROC_UNUSED(void,glPixelMapusv,(GLenum map, GLsizei mapsize, const GLushort *values))
SDL_PROC_UNUSED(void,glPixelStoref,(GLenum pname, GLfloat param))
SDL_PROC(void,glPixelStorei,(GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glPixelTransferf,(GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glPixelTransferi,(GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glPixelZoom,(GLfloat xfactor, GLfloat yfactor))
SDL_PROC_UNUSED(void,glPointSize,(GLfloat size))
SDL_PROC_UNUSED(void,glPolygonMode,(GLenum face, GLenum mode))
SDL_PROC_UNUSED(void,glPolygonOffset,(GLfloat factor, GLfloat units))
SDL_PROC_UNUSED(void,glPolygonStipple,(const GLubyte *mask))
SDL_PROC(void,glPopAttrib,(void))
SDL_PROC(void,glPopClientAttrib,(void))
SDL_PROC(void,glPopMatrix,(void))
SDL_PROC_UNUSED(void,glPopName,(void))
SDL_PROC_UNUSED(void,glPrioritizeTextures,(GLsizei n, const GLuint *textures, const GLclampf *priorities))
SDL_PROC(void,glPushAttrib,(GLbitfield mask))
SDL_PROC(void,glPushClientAttrib,(GLbitfield mask))
SDL_PROC(void,glPushMatrix,(void))
SDL_PROC_UNUSED(void,glPushName,(GLuint name))
SDL_PROC_UNUSED(void,glRasterPos2d,(GLdouble x, GLdouble y))
SDL_PROC_UNUSED(void,glRasterPos2dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glRasterPos2f,(GLfloat x, GLfloat y))
SDL_PROC_UNUSED(void,glRasterPos2fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glRasterPos2i,(GLint x, GLint y))
SDL_PROC_UNUSED(void,glRasterPos2iv,(const GLint *v))
SDL_PROC_UNUSED(void,glRasterPos2s,(GLshort x, GLshort y))
SDL_PROC_UNUSED(void,glRasterPos2sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glRasterPos3d,(GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glRasterPos3dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glRasterPos3f,(GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glRasterPos3fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glRasterPos3i,(GLint x, GLint y, GLint z))
SDL_PROC_UNUSED(void,glRasterPos3iv,(const GLint *v))
SDL_PROC_UNUSED(void,glRasterPos3s,(GLshort x, GLshort y, GLshort z))
SDL_PROC_UNUSED(void,glRasterPos3sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glRasterPos4d,(GLdouble x, GLdouble y, GLdouble z, GLdouble w))
SDL_PROC_UNUSED(void,glRasterPos4dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glRasterPos4f,(GLfloat x, GLfloat y, GLfloat z, GLfloat w))
SDL_PROC_UNUSED(void,glRasterPos4fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glRasterPos4i,(GLint x, GLint y, GLint z, GLint w))
SDL_PROC_UNUSED(void,glRasterPos4iv,(const GLint *v))
SDL_PROC_UNUSED(void,glRasterPos4s,(GLshort x, GLshort y, GLshort z, GLshort w))
SDL_PROC_UNUSED(void,glRasterPos4sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glReadBuffer,(GLenum mode))
SDL_PROC_UNUSED(void,glReadPixels,(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels))
SDL_PROC_UNUSED(void,glRectd,(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2))
SDL_PROC_UNUSED(void,glRectdv,(const GLdouble *v1, const GLdouble *v2))
SDL_PROC_UNUSED(void,glRectf,(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2))
SDL_PROC_UNUSED(void,glRectfv,(const GLfloat *v1, const GLfloat *v2))
SDL_PROC_UNUSED(void,glRecti,(GLint x1, GLint y1, GLint x2, GLint y2))
SDL_PROC_UNUSED(void,glRectiv,(const GLint *v1, const GLint *v2))
SDL_PROC_UNUSED(void,glRects,(GLshort x1, GLshort y1, GLshort x2, GLshort y2))
SDL_PROC_UNUSED(void,glRectsv,(const GLshort *v1, const GLshort *v2))
SDL_PROC_UNUSED(GLint,glRenderMode,(GLenum mode))
SDL_PROC_UNUSED(void,glRotated,(GLdouble angle, GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glRotatef,(GLfloat angle, GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glScaled,(GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glScalef,(GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glScissor,(GLint x, GLint y, GLsizei width, GLsizei height))
SDL_PROC_UNUSED(void,glSelectBuffer,(GLsizei size, GLuint *buffer))
SDL_PROC_UNUSED(void,glShadeModel,(GLenum mode))
SDL_PROC_UNUSED(void,glStencilFunc,(GLenum func, GLint ref, GLuint mask))
SDL_PROC_UNUSED(void,glStencilMask,(GLuint mask))
SDL_PROC_UNUSED(void,glStencilOp,(GLenum fail, GLenum zfail, GLenum zpass))
SDL_PROC_UNUSED(void,glTexCoord1d,(GLdouble s))
SDL_PROC_UNUSED(void,glTexCoord1dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glTexCoord1f,(GLfloat s))
SDL_PROC_UNUSED(void,glTexCoord1fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glTexCoord1i,(GLint s))
SDL_PROC_UNUSED(void,glTexCoord1iv,(const GLint *v))
SDL_PROC_UNUSED(void,glTexCoord1s,(GLshort s))
SDL_PROC_UNUSED(void,glTexCoord1sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glTexCoord2d,(GLdouble s, GLdouble t))
SDL_PROC_UNUSED(void,glTexCoord2dv,(const GLdouble *v))
SDL_PROC(void,glTexCoord2f,(GLfloat s, GLfloat t))
SDL_PROC_UNUSED(void,glTexCoord2fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glTexCoord2i,(GLint s, GLint t))
SDL_PROC_UNUSED(void,glTexCoord2iv,(const GLint *v))
SDL_PROC_UNUSED(void,glTexCoord2s,(GLshort s, GLshort t))
SDL_PROC_UNUSED(void,glTexCoord2sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glTexCoord3d,(GLdouble s, GLdouble t, GLdouble r))
SDL_PROC_UNUSED(void,glTexCoord3dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glTexCoord3f,(GLfloat s, GLfloat t, GLfloat r))
SDL_PROC_UNUSED(void,glTexCoord3fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glTexCoord3i,(GLint s, GLint t, GLint r))
SDL_PROC_UNUSED(void,glTexCoord3iv,(const GLint *v))
SDL_PROC_UNUSED(void,glTexCoord3s,(GLshort s, GLshort t, GLshort r))
SDL_PROC_UNUSED(void,glTexCoord3sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glTexCoord4d,(GLdouble s, GLdouble t, GLdouble r, GLdouble q))
SDL_PROC_UNUSED(void,glTexCoord4dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glTexCoord4f,(GLfloat s, GLfloat t, GLfloat r, GLfloat q))
SDL_PROC_UNUSED(void,glTexCoord4fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glTexCoord4i,(GLint s, GLint t, GLint r, GLint q))
SDL_PROC_UNUSED(void,glTexCoord4iv,(const GLint *v))
SDL_PROC_UNUSED(void,glTexCoord4s,(GLshort s, GLshort t, GLshort r, GLshort q))
SDL_PROC_UNUSED(void,glTexCoord4sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glTexCoordPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC(void,glTexEnvf,(GLenum target, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glTexEnvfv,(GLenum target, GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glTexEnvi,(GLenum target, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glTexEnviv,(GLenum target, GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glTexGend,(GLenum coord, GLenum pname, GLdouble param))
SDL_PROC_UNUSED(void,glTexGendv,(GLenum coord, GLenum pname, const GLdouble *params))
SDL_PROC_UNUSED(void,glTexGenf,(GLenum coord, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glTexGenfv,(GLenum coord, GLenum pname, const GLfloat *params))
SDL_PROC_UNUSED(void,glTexGeni,(GLenum coord, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glTexGeniv,(GLenum coord, GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glTexImage1D,(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC(void,glTexImage2D,(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC_UNUSED(void,glTexParameterf,(GLenum target, GLenum pname, GLfloat param))
SDL_PROC_UNUSED(void,glTexParameterfv,(GLenum target, GLenum pname, const GLfloat *params))
SDL_PROC(void,glTexParameteri,(GLenum target, GLenum pname, GLint param))
SDL_PROC_UNUSED(void,glTexParameteriv,(GLenum target, GLenum pname, const GLint *params))
SDL_PROC_UNUSED(void,glTexSubImage1D,(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC(void,glTexSubImage2D,(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels))
SDL_PROC_UNUSED(void,glTranslated,(GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glTranslatef,(GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glVertex2d,(GLdouble x, GLdouble y))
SDL_PROC_UNUSED(void,glVertex2dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glVertex2f,(GLfloat x, GLfloat y))
SDL_PROC_UNUSED(void,glVertex2fv,(const GLfloat *v))
SDL_PROC(void,glVertex2i,(GLint x, GLint y))
SDL_PROC_UNUSED(void,glVertex2iv,(const GLint *v))
SDL_PROC_UNUSED(void,glVertex2s,(GLshort x, GLshort y))
SDL_PROC_UNUSED(void,glVertex2sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glVertex3d,(GLdouble x, GLdouble y, GLdouble z))
SDL_PROC_UNUSED(void,glVertex3dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glVertex3f,(GLfloat x, GLfloat y, GLfloat z))
SDL_PROC_UNUSED(void,glVertex3fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glVertex3i,(GLint x, GLint y, GLint z))
SDL_PROC_UNUSED(void,glVertex3iv,(const GLint *v))
SDL_PROC_UNUSED(void,glVertex3s,(GLshort x, GLshort y, GLshort z))
SDL_PROC_UNUSED(void,glVertex3sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glVertex4d,(GLdouble x, GLdouble y, GLdouble z, GLdouble w))
SDL_PROC_UNUSED(void,glVertex4dv,(const GLdouble *v))
SDL_PROC_UNUSED(void,glVertex4f,(GLfloat x, GLfloat y, GLfloat z, GLfloat w))
SDL_PROC_UNUSED(void,glVertex4fv,(const GLfloat *v))
SDL_PROC_UNUSED(void,glVertex4i,(GLint x, GLint y, GLint z, GLint w))
SDL_PROC_UNUSED(void,glVertex4iv,(const GLint *v))
SDL_PROC_UNUSED(void,glVertex4s,(GLshort x, GLshort y, GLshort z, GLshort w))
SDL_PROC_UNUSED(void,glVertex4sv,(const GLshort *v))
SDL_PROC_UNUSED(void,glVertexPointer,(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
SDL_PROC(void,glViewport,(GLint x, GLint y, GLsizei width, GLsizei height))
#undef SDL_PROC

	/* Texture id */
	GLuint texture;
#endif
	int is_32bit;
 
	/* * * */
	/* Window manager functions */

	/* Set the title and icon text */
	void (*SetCaption)(_THIS, const char *title, const char *icon);

	/* Set the window icon image */
	void (*SetIcon)(_THIS, SDL_Surface *icon, Uint8 *mask);

	/* Iconify the window.
	   This function returns 1 if there is a window manager and the
	   window was actually iconified, it returns 0 otherwise.
	*/
	int (*IconifyWindow)(_THIS);

	/* Grab or ungrab keyboard and mouse input */
	SDL_GrabMode (*GrabInput)(_THIS, SDL_GrabMode mode);

	/* Get some platform dependent window information */
	int (*GetWMInfo)(_THIS, SDL_SysWMinfo *info);

	/* * * */
	/* Cursor manager functions */

	/* Free a window manager cursor
	   This function can be NULL if CreateWMCursor is also NULL.
	 */
	void (*FreeWMCursor)(_THIS, WMcursor *cursor);

	/* If not NULL, create a black/white window manager cursor */
	WMcursor *(*CreateWMCursor)(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);

	/* Show the specified cursor, or hide if cursor is NULL */
	int (*ShowWMCursor)(_THIS, WMcursor *cursor);

	/* Warp the window manager cursor to (x,y)
	   If NULL, a mouse motion event is posted internally.
	 */
	void (*WarpWMCursor)(_THIS, Uint16 x, Uint16 y);

	/* If not NULL, this is called when a mouse motion event occurs */
	void (*MoveWMCursor)(_THIS, int x, int y);

	/* Determine whether the mouse should be in relative mode or not.
	   This function is called when the input grab state or cursor
	   visibility state changes.
	   If the cursor is not visible, and the input is grabbed, the
	   driver can place the mouse in relative mode, which may result
	   in higher accuracy sampling of the pointer motion.
	*/
	void (*CheckMouseMode)(_THIS);

	/* * * */
	/* Event manager functions */

	/* Initialize keyboard mapping for this driver */
	void (*InitOSKeymap)(_THIS);

	/* Handle any queued OS events */
	void (*PumpEvents)(_THIS);

	/* * * */
	/* Data common to all drivers */
	SDL_Surface *screen;
	SDL_Surface *shadow;
	SDL_Surface *visible;
        SDL_Palette *physpal;	/* physical palette, if != logical palette */
        SDL_Color *gammacols;	/* gamma-corrected colours, or NULL */
	char *wm_title;
	char *wm_icon;
	int offset_x;
	int offset_y;
	SDL_GrabMode input_grab;

	/* Driver information flags */
	int handles_any_size;	/* Driver handles any size video mode */

	/* * * */
	/* Data used by the GL drivers */
	struct {
		int red_size;
		int green_size;
		int blue_size;
		int alpha_size;
		int depth_size;
		int buffer_size;
		int stencil_size;
		int double_buffer;
		int accum_red_size;
		int accum_green_size;
		int accum_blue_size;
		int accum_alpha_size;
		int stereo;
		int multisamplebuffers;
		int multisamplesamples;
		int accelerated;
		int swap_control;
		int driver_loaded;
		char driver_path[256];
		void* dll_handle;
	} gl_config;

	/* * * */
	/* Data private to this driver */
	struct SDL_PrivateVideoData *hidden;
	struct SDL_PrivateGLData *gl_data;

	/* * * */
	/* The function used to dispose of this structure */
	void (*free)(_THIS);
};

/* Main driver structure to store required state information */
#define CGDirectDisplayID int
#define CFDictionaryRef int
#define CFArrayRef int
#define CGDirectPaletteRef int
#define NSOpenGLContext int
#define NSWindow int
#define NSQuickDrawView int
#define io_connect_t int
#define NSText int
#define NSPoint int
#define SInt32 int
#define ImageDescriptionHandle int
#define MatrixRecordPtr int
#define DecompressorComponent int
#define ImageSequence int
#define CGrafPtr int
#define PlanarPixmapInfoYUV420 int

typedef struct SDL_PrivateVideoData {

    BOOL               allow_screensaver;  /* 0 == disable screensaver */
    CGDirectDisplayID  display;            /* 0 == main display (only support single display) */
    CFDictionaryRef    mode;               /* current mode of the display */
    CFDictionaryRef    save_mode;          /* original mode of the display */
    CFArrayRef         mode_list;          /* list of available fullscreen modes */
    CGDirectPaletteRef palette;            /* palette of an 8-bit display */
    NSOpenGLContext    *gl_context;        /* OpenGL rendering context */
    Uint32             width, height, bpp; /* frequently used data about the display */
    Uint32             flags;              /* flags for current mode, for teardown purposes */
    Uint32             video_set;          /* boolean; indicates if video was set correctly */
    Uint32             warp_flag;          /* boolean; notify to event loop that a warp just occured */
    Uint32             warp_ticks;         /* timestamp when the warp occured */
    NSWindow           *window;            /* Cocoa window to implement the SDL window */
    NSQuickDrawView    *view;              /* the window's view; draw 2D and OpenGL into this view */
    SDL_Surface        *resize_icon;       /* icon for the resize badge, we have to draw it by hand */
    SDL_GrabMode       current_grab_mode;  /* default value is SDL_GRAB_OFF */
    SDL_Rect           **client_mode_list; /* resolution list to pass back to client */
    SDLKey             keymap[256];        /* Mac OS X to SDL key mapping */
    Uint32             current_mods;       /* current keyboard modifiers, to track modifier state */
    NSText             *field_edit;        /* a field editor for keyboard composition processing */
    Uint32             last_virtual_button;/* last virtual mouse button pressed */
    io_connect_t       power_connection;   /* used with IOKit to detect wake from sleep */
    Uint8              expect_mouse_up;    /* used to determine when to send mouse up events */
    Uint8              grab_state;         /* used to manage grab behavior */
    NSPoint            cursor_loc;         /* saved cursor coords, for activate/deactivate when grabbed */
    BOOL               cursor_should_be_visible;     /* tells if cursor is supposed to be visible (SDL_ShowCursor) */
    BOOL               cursor_visible;     /* tells if cursor is *actually* visible or not */
    Uint8*             sw_buffers[2];      /* pointers to the two software buffers for double-buffer emulation */
    SDL_Thread         *thread;            /* thread for async updates to the screen */
    SDL_sem            *sem1, *sem2;       /* synchronization for async screen updates */
    Uint8              *current_buffer;    /* the buffer being copied to the screen */
    BOOL               quit_thread;        /* used to quit the async blitting thread */
    SInt32             system_version;     /* used to dis-/enable workarounds depending on the system version */
    
    ImageDescriptionHandle yuv_idh;
    MatrixRecordPtr        yuv_matrix;
    DecompressorComponent  yuv_codec;
    ImageSequence          yuv_seq;
    PlanarPixmapInfoYUV420 *yuv_pixmap;
    Sint16                  yuv_width, yuv_height;
    CGrafPtr                yuv_port;

    void *opengl_library;    /* dynamically loaded OpenGL library. */
} SDL_PrivateVideoData;

#endif

#endif
