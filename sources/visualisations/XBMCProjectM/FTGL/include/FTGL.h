#ifndef     __FTGL__
#define     __FTGL__


typedef double   FTGL_DOUBLE;
typedef float    FTGL_FLOAT;

// Fixes for deprecated identifiers in 2.1.5
#ifndef FT_OPEN_MEMORY
    #define FT_OPEN_MEMORY (FT_Open_Flags)1
#endif

#ifndef FT_RENDER_MODE_MONO
    #define FT_RENDER_MODE_MONO ft_render_mode_mono
#endif

#ifndef FT_RENDER_MODE_NORMAL
    #define FT_RENDER_MODE_NORMAL ft_render_mode_normal
#endif

  
#ifdef WIN32

    // Under windows avoid including <windows.h> is overrated. 
    // Sure, it can be avoided and "name space pollution" can be
    // avoided, but why? It really doesn't make that much difference
    // these days.
    #define  WIN32_LEAN_AND_MEAN
    #include <windows.h>

    #ifndef __gl_h_
        #include <GL/gl.h>
        #include <GL/glu.h>
    #endif

#else

    // Non windows platforms - don't require nonsense as seen above :-)    
    #ifndef __gl_h_
        #ifdef __APPLE_CC__
            #include <OpenGL/gl.h>
            #include <OpenGL/glu.h>
        #else
            #include <GL/gl.h>
            #include <GL/glu.h>
        #endif                

    #endif

    // Required for compatibility with glext.h style function definitions of 
    // OpenGL extensions, such as in src/osg/Point.cpp.
    #ifndef APIENTRY
        #define APIENTRY
    #endif
#endif

// Compiler-specific conditional compilation
#ifdef _MSC_VER // MS Visual C++ 

    // Disable various warning.
    // 4786: template name too long
    #pragma warning( disable : 4251 )
    #pragma warning( disable : 4275 )
    #pragma warning( disable : 4786 )

    // The following definitions control how symbols are exported.
    // If the target is a static library ensure that FTGL_LIBRARY_STATIC
    // is defined. If building a dynamic library (ie DLL) ensure the
    // FTGL_LIBRARY macro is defined, as it will mark symbols for 
    // export. If compiling a project to _use_ the _dynamic_ library 
    // version of the library, no definition is required. 
    #ifdef FTGL_LIBRARY_STATIC      // static lib - no special export required
    #  define FTGL_EXPORT
    #elif FTGL_LIBRARY              // dynamic lib - must export/import symbols appropriately.
    #  define FTGL_EXPORT   __declspec(dllexport)
    #else
    #  define FTGL_EXPORT   __declspec(dllimport)
    #endif 

#else
    // Compiler that is not MS Visual C++.
    // Ensure that the export symbol is defined (and blank)
    #define FTGL_EXPORT
#endif  

#endif  //  __FTGL__
