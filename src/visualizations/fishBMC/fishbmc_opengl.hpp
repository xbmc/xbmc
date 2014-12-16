#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else // __APPLE__
#include <GL/gl.h>
#include <GL/glu.h>
#endif // __APPLE__


// additional global variable
GLuint      g_texture;


// OpenGL: ignores VIS_PROPS
void init (VIS_PROPS*) {}

// OpenGL: texture initialization
inline void init_texture (int width, int height, uint32_t* pixels)
{
    // generate a texture for drawing into
    glEnable (GL_TEXTURE_2D);
    glGenTextures (1, &g_texture);
    glBindTexture (GL_TEXTURE_2D, g_texture);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

// OpenGL: replace texture
inline void replace_texture (int width, int height, uint32_t* pixels)
{
    glBindTexture (GL_TEXTURE_2D, g_texture);
    glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

// OpenGL: delete texture
inline void delete_texture()
{
    glDeleteTextures (1, &g_texture);
}

// OpenGL: paint a textured quad
void textured_quad (double center_x,
                    double center_y,
                    double angle,
                    double axis,
                    double width,
                    double height,
                    double tex_left,
                    double tex_right,
                    double tex_top,
                    double tex_bottom)
{
    glPushMatrix();

    glTranslatef (center_x, center_y, 0);
    glRotatef (angle, axis, 1 - axis, 0);

    double scale = 1 - sin (angle / 360 * M_PI) / 3;
    glScalef (scale, scale, scale);

    glBegin (GL_QUADS);

    glTexCoord2d (tex_left, tex_top);
    glVertex3d (- width / 2, - height / 2, 0);

    glTexCoord2d (tex_right, tex_top);
    glVertex3d (width / 2, - height / 2, 0);

    glTexCoord2d (tex_right, tex_bottom);
    glVertex3d (width / 2, height / 2, 0);

    glTexCoord2d (tex_left, tex_bottom);
    glVertex3d (- width / 2, height / 2, 0);

    glEnd();

    glPopMatrix();
}

// OpenGL: setup to start rendering
inline void start_render()
{
    // save state
    glPushAttrib (GL_ENABLE_BIT | GL_TEXTURE_BIT);

    // enable blending
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // enable texturing
    glEnable (GL_TEXTURE_2D);

    // disable depth testing
    glDisable (GL_DEPTH_TEST);

    // paint both sides of polygons
    glPolygonMode (GL_FRONT, GL_FILL);

    // OpenGL projection matrix setup
    glMatrixMode (GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // coordinate system:
    //     screen top left: (-1, -1)
    //     screen bottom right: (1, 1)
    //     screen depth clipping: 3 to 15
    glFrustum (-1, 1, 1, -1, 3, 15);

    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // bind global texture
    glBindTexture (GL_TEXTURE_2D, g_texture);

    // move 6 units into the screen
    glTranslatef (0, 0, -6.0f);

    // rotate
    glRotatef (g_angle, 0, 1, 0);
}

// OpenGL: done rendering
inline void finish_render()
{
    // return OpenGL matrices to original state
    glPopMatrix();
    glMatrixMode (GL_PROJECTION);
    glPopMatrix();

    // restore OpenGl original state
    glPopAttrib();
}
