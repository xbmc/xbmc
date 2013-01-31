// TEMPLATE for DirectX implementation
// check out fishbmc_opengl.hpp
// contact marcel@26elf.at for questions
// thanks for porting!

#include <any headers you may need>

// declare a global texture
// declare other globals you may need

void init (VIS_PROPS* vis_props)
{
    // take anything from the VIS_PROPS struct
}

inline void init_texture (int width, int height, uint32_t* pixels)
{
    // initialize the global texture of size (width x height) with data from pixels
}

inline void replace_texture (int width, int height, uint32_t* pixels)
{
    // replace the texture data with pixels
}

inline void delete_texture()
{
    // free memory held by global texture
}

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
    // draw a textured quad, sized (width x height), centered at (center_x, center_y),
    // rotated by angle (this is in degrees) around an axis defined as (axis, 1 - axis, 0),
    // scaled by (1 - sin (angle / 360 * M_PI) / 3)
    // texture coordinates are given for all corners
}

inline void start_render()
{
    // save state
    // enable blending
    // disable depth testing
    // paint both sides of polygons

    // setup coordinate system:
    //     screen top left: (-1, -1)
    //     screen bottom right: (1, 1)
    //     screen distance from eye: 3
    //     screen depth clipping: 3 to 15

    // bind global texture
    // move 6 units into the screen
    // rotate by global variable g_angle, around axis (0, 1, 0)
}

inline void finish_render()
{
    // restore original state
}
