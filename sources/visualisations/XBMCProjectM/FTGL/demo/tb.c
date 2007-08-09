/*
 *  Simple trackball-like motion adapted (ripped off) from projtex.c
 *  (written by David Yu and David Blythe).  See the SIGGRAPH '96
 *  Advanced OpenGL course notes.
 */


/* includes */
#include <math.h>
#include <assert.h>
#ifdef __APPLE_CC__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif
#include "tb.h"
#include "trackball.h"

/* globals */
static GLuint    tb_lasttime;

float curquat[4];
float lastquat[4];
int beginx, beginy;

static GLuint    tb_width;
static GLuint    tb_height;

static GLint     tb_button = -1;
static GLboolean tb_tracking = GL_FALSE;
static GLboolean tb_animate = GL_TRUE;

static void
_tbAnimate(void)
{
  add_quats(lastquat, curquat, curquat);
  glutPostRedisplay();
}

void
_tbStartMotion(int x, int y, int time)
{
  assert(tb_button != -1);

  glutIdleFunc(0);
  tb_tracking = GL_TRUE;
  tb_lasttime = time;
  beginx = x;
  beginy = y;
}

void
_tbStopMotion(unsigned time)
{
  assert(tb_button != -1);

  tb_tracking = GL_FALSE;

  if (time == tb_lasttime && tb_animate) {
    glutIdleFunc(_tbAnimate);
  } else {
    if (tb_animate) {
      glutIdleFunc(0);
    }
  }
}

void
tbAnimate(GLboolean animate)
{
  tb_animate = animate;
}

void
tbInit(GLuint button)
{
  tb_button = button;
  trackball(curquat, 0.0, 0.0, 0.0, 0.0);
}

void
tbMatrix(void)
{
  GLfloat m[4][4];

  assert(tb_button != -1);
  build_rotmatrix(m, curquat);
  glMultMatrixf(&m[0][0]);
}

void
tbReshape(int width, int height)
{
  assert(tb_button != -1);

  tb_width  = width;
  tb_height = height;
}

void
tbMouse(int button, int state, int x, int y)
{
  assert(tb_button != -1);

  if (state == GLUT_DOWN && button == tb_button)
    _tbStartMotion(x, y, glutGet(GLUT_ELAPSED_TIME));
  else if (state == GLUT_UP && button == tb_button)
    _tbStopMotion(glutGet(GLUT_ELAPSED_TIME));
}

void
tbMotion(int x, int y)
{
  if (tb_tracking) {
    trackball(lastquat,
      (2.0 * beginx - tb_width) / tb_width,
      (tb_height - 2.0 * beginy) / tb_height,
      (2.0 * x - tb_width) / tb_width,
      (tb_height - 2.0 * y) / tb_height
      );
    beginx = x;
    beginy = y;
    tb_animate = 1;
    tb_lasttime = glutGet(GLUT_ELAPSED_TIME);
    _tbAnimate();
  }
}
