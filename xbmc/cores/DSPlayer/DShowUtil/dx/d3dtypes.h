/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3dtypes.h
 *  Content:    Direct3D types include file
 *
 ***************************************************************************/

#ifndef _D3DTYPES_H_
#define _D3DTYPES_H_

#ifndef DIRECT3D_VERSION
#define DIRECT3D_VERSION         0x0700
#endif

#if (DIRECT3D_VERSION >= 0x0800)
#pragma message("should not include d3dtypes.h when compiling for DX8 or newer interfaces")
#endif

#include <windows.h>

#include <float.h>
#include "ddraw.h"

#pragma warning(disable:4201) // anonymous unions warning
#if defined(_X86_) || defined(_IA64_)
#pragma pack(4)
#endif


/* D3DVALUE is the fundamental Direct3D fractional data type */

#define D3DVALP(val, prec) ((float)(val))
#define D3DVAL(val) ((float)(val))

#ifndef DX_SHARED_DEFINES

/*
 * This definition is shared with other DirectX components whose header files
 * might already have defined it. Therefore, we don't define this type if
 * someone else already has (as indicated by the definition of
 * DX_SHARED_DEFINES). We don't set DX_SHARED_DEFINES here as there are
 * other types in this header that are also shared. The last of these
 * shared defines in this file will set DX_SHARED_DEFINES.
 */
typedef float D3DVALUE, *LPD3DVALUE;

#endif /* DX_SHARED_DEFINES */

#define D3DDivide(a, b)    (float)((double) (a) / (double) (b))
#define D3DMultiply(a, b)    ((a) * (b))

typedef LONG D3DFIXED;

#ifndef RGB_MAKE
/*
 * Format of CI colors is
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    alpha      |         color index           |   fraction    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define CI_GETALPHA(ci)    ((ci) >> 24)
#define CI_GETINDEX(ci)    (((ci) >> 8) & 0xffff)
#define CI_GETFRACTION(ci) ((ci) & 0xff)
#define CI_ROUNDINDEX(ci)  CI_GETINDEX((ci) + 0x80)
#define CI_MASKALPHA(ci)   ((ci) & 0xffffff)
#define CI_MAKE(a, i, f)    (((a) << 24) | ((i) << 8) | (f))

/*
 * Format of RGBA colors is
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    alpha      |      red      |     green     |     blue      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define RGBA_GETALPHA(rgb)      ((rgb) >> 24)
#define RGBA_GETRED(rgb)        (((rgb) >> 16) & 0xff)
#define RGBA_GETGREEN(rgb)      (((rgb) >> 8) & 0xff)
#define RGBA_GETBLUE(rgb)       ((rgb) & 0xff)
#define RGBA_MAKE(r, g, b, a)   ((D3DCOLOR) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

/* D3DRGB and D3DRGBA may be used as initialisers for D3DCOLORs
 * The float values must be in the range 0..1
 */
#define D3DRGB(r, g, b) \
    (0xff000000L | ( ((long)((r) * 255)) << 16) | (((long)((g) * 255)) << 8) | (long)((b) * 255))
#define D3DRGBA(r, g, b, a) \
    (   (((long)((a) * 255)) << 24) | (((long)((r) * 255)) << 16) \
    |   (((long)((g) * 255)) << 8) | (long)((b) * 255) \
    )

/*
 * Format of RGB colors is
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |    ignored    |      red      |     green     |     blue      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#define RGB_GETRED(rgb)         (((rgb) >> 16) & 0xff)
#define RGB_GETGREEN(rgb)       (((rgb) >> 8) & 0xff)
#define RGB_GETBLUE(rgb)        ((rgb) & 0xff)
#define RGBA_SETALPHA(rgba, x) (((x) << 24) | ((rgba) & 0x00ffffff))
#define RGB_MAKE(r, g, b)       ((D3DCOLOR) (((r) << 16) | ((g) << 8) | (b)))
#define RGBA_TORGB(rgba)       ((D3DCOLOR) ((rgba) & 0xffffff))
#define RGB_TORGBA(rgb)        ((D3DCOLOR) ((rgb) | 0xff000000))

#endif

/*
 * Flags for Enumerate functions
 */

/*
 * Stop the enumeration
 */
#define D3DENUMRET_CANCEL                        DDENUMRET_CANCEL

/*
 * Continue the enumeration
 */
#define D3DENUMRET_OK                            DDENUMRET_OK

typedef HRESULT (CALLBACK* LPD3DVALIDATECALLBACK)(LPVOID lpUserArg, DWORD dwOffset);
typedef HRESULT (CALLBACK* LPD3DENUMTEXTUREFORMATSCALLBACK)(LPDDSURFACEDESC lpDdsd, LPVOID lpContext);
typedef HRESULT (CALLBACK* LPD3DENUMPIXELFORMATSCALLBACK)(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext);

#ifndef DX_SHARED_DEFINES

/*
 * This definition is shared with other DirectX components whose header files
 * might already have defined it. Therefore, we don't define this type if
 * someone else already has (as indicated by the definition of
 * DX_SHARED_DEFINES). We don't set DX_SHARED_DEFINES here as there are
 * other types in this header that are also shared. The last of these
 * shared defines in this file will set DX_SHARED_DEFINES.
 */
#ifndef D3DCOLOR_DEFINED
typedef DWORD D3DCOLOR;
#define D3DCOLOR_DEFINED
#endif
typedef DWORD *LPD3DCOLOR;

#endif /* DX_SHARED_DEFINES */

typedef DWORD D3DMATERIALHANDLE, *LPD3DMATERIALHANDLE;
typedef DWORD D3DTEXTUREHANDLE, *LPD3DTEXTUREHANDLE;
typedef DWORD D3DMATRIXHANDLE, *LPD3DMATRIXHANDLE;

#ifndef D3DCOLORVALUE_DEFINED
typedef struct _D3DCOLORVALUE {
    union {
    D3DVALUE r;
    D3DVALUE dvR;
    };
    union {
    D3DVALUE g;
    D3DVALUE dvG;
    };
    union {
    D3DVALUE b;
    D3DVALUE dvB;
    };
    union {
    D3DVALUE a;
    D3DVALUE dvA;
    };
} D3DCOLORVALUE;
#define D3DCOLORVALUE_DEFINED
#endif
typedef struct _D3DCOLORVALUE *LPD3DCOLORVALUE;

#ifndef D3DRECT_DEFINED
typedef struct _D3DRECT {
    union {
    LONG x1;
    LONG lX1;
    };
    union {
    LONG y1;
    LONG lY1;
    };
    union {
    LONG x2;
    LONG lX2;
    };
    union {
    LONG y2;
    LONG lY2;
    };
} D3DRECT;
#define D3DRECT_DEFINED
#endif
typedef struct _D3DRECT *LPD3DRECT;

#ifndef DX_SHARED_DEFINES

/*
 * This definition is shared with other DirectX components whose header files
 * might already have defined it. Therefore, we don't define this type if
 * someone else already has (as indicated by the definition of
 * DX_SHARED_DEFINES).
 */

#ifndef D3DVECTOR_DEFINED
typedef struct _D3DVECTOR {
    union {
    D3DVALUE x;
    D3DVALUE dvX;
    };
    union {
    D3DVALUE y;
    D3DVALUE dvY;
    };
    union {
    D3DVALUE z;
    D3DVALUE dvZ;
    };
#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)

public:

    // =====================================
    // Constructors
    // =====================================

    _D3DVECTOR() { }
    _D3DVECTOR(D3DVALUE f);
    _D3DVECTOR(D3DVALUE _x, D3DVALUE _y, D3DVALUE _z);
    _D3DVECTOR(const D3DVALUE f[3]);

    // =====================================
    // Access grants
    // =====================================

    const D3DVALUE&operator[](int i) const;
    D3DVALUE&operator[](int i);

    // =====================================
    // Assignment operators
    // =====================================

    _D3DVECTOR& operator += (const _D3DVECTOR& v);
    _D3DVECTOR& operator -= (const _D3DVECTOR& v);
    _D3DVECTOR& operator *= (const _D3DVECTOR& v);
    _D3DVECTOR& operator /= (const _D3DVECTOR& v);
    _D3DVECTOR& operator *= (D3DVALUE s);
    _D3DVECTOR& operator /= (D3DVALUE s);

    // =====================================
    // Unary operators
    // =====================================

    friend _D3DVECTOR operator + (const _D3DVECTOR& v);
    friend _D3DVECTOR operator - (const _D3DVECTOR& v);


    // =====================================
    // Binary operators
    // =====================================

    // Addition and subtraction
        friend _D3DVECTOR operator + (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
        friend _D3DVECTOR operator - (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
    // Scalar multiplication and division
        friend _D3DVECTOR operator * (const _D3DVECTOR& v, D3DVALUE s);
        friend _D3DVECTOR operator * (D3DVALUE s, const _D3DVECTOR& v);
        friend _D3DVECTOR operator / (const _D3DVECTOR& v, D3DVALUE s);
    // Memberwise multiplication and division
        friend _D3DVECTOR operator * (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
        friend _D3DVECTOR operator / (const _D3DVECTOR& v1, const _D3DVECTOR& v2);

    // Vector dominance
        friend int operator < (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
        friend int operator <= (const _D3DVECTOR& v1, const _D3DVECTOR& v2);

    // Bitwise equality
        friend int operator == (const _D3DVECTOR& v1, const _D3DVECTOR& v2);

    // Length-related functions
        friend D3DVALUE SquareMagnitude (const _D3DVECTOR& v);
        friend D3DVALUE Magnitude (const _D3DVECTOR& v);

    // Returns vector with same direction and unit length
        friend _D3DVECTOR Normalize (const _D3DVECTOR& v);

    // Return min/max component of the input vector
        friend D3DVALUE Min (const _D3DVECTOR& v);
        friend D3DVALUE Max (const _D3DVECTOR& v);

    // Return memberwise min/max of input vectors
        friend _D3DVECTOR Minimize (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
        friend _D3DVECTOR Maximize (const _D3DVECTOR& v1, const _D3DVECTOR& v2);

    // Dot and cross product
        friend D3DVALUE DotProduct (const _D3DVECTOR& v1, const _D3DVECTOR& v2);
        friend _D3DVECTOR CrossProduct (const _D3DVECTOR& v1, const _D3DVECTOR& v2);

#endif
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DVECTOR;
#define D3DVECTOR_DEFINED
#endif
typedef struct _D3DVECTOR *LPD3DVECTOR;

/*
 * As this is the last of the shared defines to be defined we now set
 * D3D_SHARED_DEFINES to flag that fact that this header has defined these
 * types.
 */
#define DX_SHARED_DEFINES

#endif /* DX_SHARED_DEFINES */

/*
 * Vertex data types supported in an ExecuteBuffer.
 */

/*
 * Homogeneous vertices
 */

typedef struct _D3DHVERTEX {
    DWORD           dwFlags;        /* Homogeneous clipping flags */
    union {
    D3DVALUE    hx;
    D3DVALUE    dvHX;
    };
    union {
    D3DVALUE    hy;
    D3DVALUE    dvHY;
    };
    union {
    D3DVALUE    hz;
    D3DVALUE    dvHZ;
    };
} D3DHVERTEX, *LPD3DHVERTEX;

/*
 * Transformed/lit vertices
 */
typedef struct _D3DTLVERTEX {
    union {
    D3DVALUE    sx;             /* Screen coordinates */
    D3DVALUE    dvSX;
    };
    union {
    D3DVALUE    sy;
    D3DVALUE    dvSY;
    };
    union {
    D3DVALUE    sz;
    D3DVALUE    dvSZ;
    };
    union {
    D3DVALUE    rhw;        /* Reciprocal of homogeneous w */
    D3DVALUE    dvRHW;
    };
    union {
    D3DCOLOR    color;          /* Vertex color */
    D3DCOLOR    dcColor;
    };
    union {
    D3DCOLOR    specular;       /* Specular component of vertex */
    D3DCOLOR    dcSpecular;
    };
    union {
    D3DVALUE    tu;             /* Texture coordinates */
    D3DVALUE    dvTU;
    };
    union {
    D3DVALUE    tv;
    D3DVALUE    dvTV;
    };
#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)
    _D3DTLVERTEX() { }
    _D3DTLVERTEX(const D3DVECTOR& v, float _rhw,
                 D3DCOLOR _color, D3DCOLOR _specular,
                 float _tu, float _tv)
        { sx = v.x; sy = v.y; sz = v.z; rhw = _rhw;
          color = _color; specular = _specular;
          tu = _tu; tv = _tv;
        }
#endif
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DTLVERTEX, *LPD3DTLVERTEX;

/*
 * Untransformed/lit vertices
 */
typedef struct _D3DLVERTEX {
    union {
    D3DVALUE     x;             /* Homogeneous coordinates */
    D3DVALUE     dvX;
    };
    union {
    D3DVALUE     y;
    D3DVALUE     dvY;
    };
    union {
    D3DVALUE     z;
    D3DVALUE     dvZ;
    };
    DWORD            dwReserved;
    union {
    D3DCOLOR     color;         /* Vertex color */
    D3DCOLOR     dcColor;
    };
    union {
    D3DCOLOR     specular;      /* Specular component of vertex */
    D3DCOLOR     dcSpecular;
    };
    union {
    D3DVALUE     tu;            /* Texture coordinates */
    D3DVALUE     dvTU;
    };
    union {
    D3DVALUE     tv;
    D3DVALUE     dvTV;
    };
#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)
    _D3DLVERTEX() { }
    _D3DLVERTEX(const D3DVECTOR& v,
                D3DCOLOR _color, D3DCOLOR _specular,
                float _tu, float _tv)
        { x = v.x; y = v.y; z = v.z; dwReserved = 0;
          color = _color; specular = _specular;
          tu = _tu; tv = _tv;
        }
#endif
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DLVERTEX, *LPD3DLVERTEX;

/*
 * Untransformed/unlit vertices
 */

typedef struct _D3DVERTEX {
    union {
    D3DVALUE     x;             /* Homogeneous coordinates */
    D3DVALUE     dvX;
    };
    union {
    D3DVALUE     y;
    D3DVALUE     dvY;
    };
    union {
    D3DVALUE     z;
    D3DVALUE     dvZ;
    };
    union {
    D3DVALUE     nx;            /* Normal */
    D3DVALUE     dvNX;
    };
    union {
    D3DVALUE     ny;
    D3DVALUE     dvNY;
    };
    union {
    D3DVALUE     nz;
    D3DVALUE     dvNZ;
    };
    union {
    D3DVALUE     tu;            /* Texture coordinates */
    D3DVALUE     dvTU;
    };
    union {
    D3DVALUE     tv;
    D3DVALUE     dvTV;
    };
#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)
    _D3DVERTEX() { }
    _D3DVERTEX(const D3DVECTOR& v, const D3DVECTOR& n, float _tu, float _tv)
        { x = v.x; y = v.y; z = v.z;
          nx = n.x; ny = n.y; nz = n.z;
          tu = _tu; tv = _tv;
        }
#endif
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DVERTEX, *LPD3DVERTEX;


/*
 * Matrix, viewport, and tranformation structures and definitions.
 */

#ifndef D3DMATRIX_DEFINED
typedef struct _D3DMATRIX {
#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)
    union {
        struct {
#endif

#endif /* DIRECT3D_VERSION >= 0x0500 */
            D3DVALUE        _11, _12, _13, _14;
            D3DVALUE        _21, _22, _23, _24;
            D3DVALUE        _31, _32, _33, _34;
            D3DVALUE        _41, _42, _43, _44;

#if(DIRECT3D_VERSION >= 0x0500)
#if (defined __cplusplus) && (defined D3D_OVERLOADS)
        };
        D3DVALUE m[4][4];
    };
    _D3DMATRIX() { }
    _D3DMATRIX( D3DVALUE _m00, D3DVALUE _m01, D3DVALUE _m02, D3DVALUE _m03,
                D3DVALUE _m10, D3DVALUE _m11, D3DVALUE _m12, D3DVALUE _m13,
                D3DVALUE _m20, D3DVALUE _m21, D3DVALUE _m22, D3DVALUE _m23,
                D3DVALUE _m30, D3DVALUE _m31, D3DVALUE _m32, D3DVALUE _m33
        )
        {
                m[0][0] = _m00; m[0][1] = _m01; m[0][2] = _m02; m[0][3] = _m03;
                m[1][0] = _m10; m[1][1] = _m11; m[1][2] = _m12; m[1][3] = _m13;
                m[2][0] = _m20; m[2][1] = _m21; m[2][2] = _m22; m[2][3] = _m23;
                m[3][0] = _m30; m[3][1] = _m31; m[3][2] = _m32; m[3][3] = _m33;
        }

    D3DVALUE& operator()(int iRow, int iColumn) { return m[iRow][iColumn]; }
    const D3DVALUE& operator()(int iRow, int iColumn) const { return m[iRow][iColumn]; }
#if(DIRECT3D_VERSION >= 0x0600)
    friend _D3DMATRIX operator* (const _D3DMATRIX&, const _D3DMATRIX&);
#endif /* DIRECT3D_VERSION >= 0x0600 */
#endif
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DMATRIX;
#define D3DMATRIX_DEFINED
#endif
typedef struct _D3DMATRIX *LPD3DMATRIX;

#if (defined __cplusplus) && (defined D3D_OVERLOADS)
#include "d3dvec.inl"
#endif

typedef struct _D3DVIEWPORT {
    DWORD       dwSize;
    DWORD       dwX;
    DWORD       dwY;        /* Top left */
    DWORD       dwWidth;
    DWORD       dwHeight;   /* Dimensions */
    D3DVALUE    dvScaleX;   /* Scale homogeneous to screen */
    D3DVALUE    dvScaleY;   /* Scale homogeneous to screen */
    D3DVALUE    dvMaxX;     /* Min/max homogeneous x coord */
    D3DVALUE    dvMaxY;     /* Min/max homogeneous y coord */
    D3DVALUE    dvMinZ;
    D3DVALUE    dvMaxZ;     /* Min/max homogeneous z coord */
} D3DVIEWPORT, *LPD3DVIEWPORT;

#if(DIRECT3D_VERSION >= 0x0500)
typedef struct _D3DVIEWPORT2 {
    DWORD       dwSize;
    DWORD       dwX;
    DWORD       dwY;        /* Viewport Top left */
    DWORD       dwWidth;
    DWORD       dwHeight;   /* Viewport Dimensions */
    D3DVALUE    dvClipX;        /* Top left of clip volume */
    D3DVALUE    dvClipY;
    D3DVALUE    dvClipWidth;    /* Clip Volume Dimensions */
    D3DVALUE    dvClipHeight;
    D3DVALUE    dvMinZ;         /* Min/max of clip Volume */
    D3DVALUE    dvMaxZ;
} D3DVIEWPORT2, *LPD3DVIEWPORT2;
#endif /* DIRECT3D_VERSION >= 0x0500 */

#if(DIRECT3D_VERSION >= 0x0700)
typedef struct _D3DVIEWPORT7 {
    DWORD       dwX;
    DWORD       dwY;            /* Viewport Top left */
    DWORD       dwWidth;
    DWORD       dwHeight;       /* Viewport Dimensions */
    D3DVALUE    dvMinZ;         /* Min/max of clip Volume */
    D3DVALUE    dvMaxZ;
} D3DVIEWPORT7, *LPD3DVIEWPORT7;
#endif /* DIRECT3D_VERSION >= 0x0700 */

/*
 * Values for clip fields.
 */

#if(DIRECT3D_VERSION >= 0x0700)

// Max number of user clipping planes, supported in D3D.
#define D3DMAXUSERCLIPPLANES 32

// These bits could be ORed together to use with D3DRENDERSTATE_CLIPPLANEENABLE
//
#define D3DCLIPPLANE0 (1 << 0)
#define D3DCLIPPLANE1 (1 << 1)
#define D3DCLIPPLANE2 (1 << 2)
#define D3DCLIPPLANE3 (1 << 3)
#define D3DCLIPPLANE4 (1 << 4)
#define D3DCLIPPLANE5 (1 << 5)

#endif /* DIRECT3D_VERSION >= 0x0700 */

#define D3DCLIP_LEFT                0x00000001L
#define D3DCLIP_RIGHT               0x00000002L
#define D3DCLIP_TOP             0x00000004L
#define D3DCLIP_BOTTOM              0x00000008L
#define D3DCLIP_FRONT               0x00000010L
#define D3DCLIP_BACK                0x00000020L
#define D3DCLIP_GEN0                0x00000040L
#define D3DCLIP_GEN1                0x00000080L
#define D3DCLIP_GEN2                0x00000100L
#define D3DCLIP_GEN3                0x00000200L
#define D3DCLIP_GEN4                0x00000400L
#define D3DCLIP_GEN5                0x00000800L

/*
 * Values for d3d status.
 */
#define D3DSTATUS_CLIPUNIONLEFT         D3DCLIP_LEFT
#define D3DSTATUS_CLIPUNIONRIGHT        D3DCLIP_RIGHT
#define D3DSTATUS_CLIPUNIONTOP          D3DCLIP_TOP
#define D3DSTATUS_CLIPUNIONBOTTOM       D3DCLIP_BOTTOM
#define D3DSTATUS_CLIPUNIONFRONT        D3DCLIP_FRONT
#define D3DSTATUS_CLIPUNIONBACK         D3DCLIP_BACK
#define D3DSTATUS_CLIPUNIONGEN0         D3DCLIP_GEN0
#define D3DSTATUS_CLIPUNIONGEN1         D3DCLIP_GEN1
#define D3DSTATUS_CLIPUNIONGEN2         D3DCLIP_GEN2
#define D3DSTATUS_CLIPUNIONGEN3         D3DCLIP_GEN3
#define D3DSTATUS_CLIPUNIONGEN4         D3DCLIP_GEN4
#define D3DSTATUS_CLIPUNIONGEN5         D3DCLIP_GEN5

#define D3DSTATUS_CLIPINTERSECTIONLEFT      0x00001000L
#define D3DSTATUS_CLIPINTERSECTIONRIGHT     0x00002000L
#define D3DSTATUS_CLIPINTERSECTIONTOP       0x00004000L
#define D3DSTATUS_CLIPINTERSECTIONBOTTOM    0x00008000L
#define D3DSTATUS_CLIPINTERSECTIONFRONT     0x00010000L
#define D3DSTATUS_CLIPINTERSECTIONBACK      0x00020000L
#define D3DSTATUS_CLIPINTERSECTIONGEN0      0x00040000L
#define D3DSTATUS_CLIPINTERSECTIONGEN1      0x00080000L
#define D3DSTATUS_CLIPINTERSECTIONGEN2      0x00100000L
#define D3DSTATUS_CLIPINTERSECTIONGEN3      0x00200000L
#define D3DSTATUS_CLIPINTERSECTIONGEN4      0x00400000L
#define D3DSTATUS_CLIPINTERSECTIONGEN5      0x00800000L
#define D3DSTATUS_ZNOTVISIBLE               0x01000000L
/* Do not use 0x80000000 for any status flags in future as it is reserved */

#define D3DSTATUS_CLIPUNIONALL  (       \
        D3DSTATUS_CLIPUNIONLEFT |   \
        D3DSTATUS_CLIPUNIONRIGHT    |   \
        D3DSTATUS_CLIPUNIONTOP  |   \
        D3DSTATUS_CLIPUNIONBOTTOM   |   \
        D3DSTATUS_CLIPUNIONFRONT    |   \
        D3DSTATUS_CLIPUNIONBACK |   \
        D3DSTATUS_CLIPUNIONGEN0 |   \
        D3DSTATUS_CLIPUNIONGEN1 |   \
        D3DSTATUS_CLIPUNIONGEN2 |   \
        D3DSTATUS_CLIPUNIONGEN3 |   \
        D3DSTATUS_CLIPUNIONGEN4 |   \
        D3DSTATUS_CLIPUNIONGEN5     \
        )

#define D3DSTATUS_CLIPINTERSECTIONALL   (       \
        D3DSTATUS_CLIPINTERSECTIONLEFT  |   \
        D3DSTATUS_CLIPINTERSECTIONRIGHT |   \
        D3DSTATUS_CLIPINTERSECTIONTOP   |   \
        D3DSTATUS_CLIPINTERSECTIONBOTTOM    |   \
        D3DSTATUS_CLIPINTERSECTIONFRONT |   \
        D3DSTATUS_CLIPINTERSECTIONBACK  |   \
        D3DSTATUS_CLIPINTERSECTIONGEN0  |   \
        D3DSTATUS_CLIPINTERSECTIONGEN1  |   \
        D3DSTATUS_CLIPINTERSECTIONGEN2  |   \
        D3DSTATUS_CLIPINTERSECTIONGEN3  |   \
        D3DSTATUS_CLIPINTERSECTIONGEN4  |   \
        D3DSTATUS_CLIPINTERSECTIONGEN5      \
        )

#define D3DSTATUS_DEFAULT   (           \
        D3DSTATUS_CLIPINTERSECTIONALL   |   \
        D3DSTATUS_ZNOTVISIBLE)


/*
 * Options for direct transform calls
 */
#define D3DTRANSFORM_CLIPPED       0x00000001l
#define D3DTRANSFORM_UNCLIPPED     0x00000002l

typedef struct _D3DTRANSFORMDATA {
    DWORD           dwSize;
    LPVOID      lpIn;           /* Input vertices */
    DWORD           dwInSize;       /* Stride of input vertices */
    LPVOID      lpOut;          /* Output vertices */
    DWORD           dwOutSize;      /* Stride of output vertices */
    LPD3DHVERTEX    lpHOut;         /* Output homogeneous vertices */
    DWORD           dwClip;         /* Clipping hint */
    DWORD           dwClipIntersection;
    DWORD           dwClipUnion;    /* Union of all clip flags */
    D3DRECT         drExtent;       /* Extent of transformed vertices */
} D3DTRANSFORMDATA, *LPD3DTRANSFORMDATA;

/*
 * Structure defining position and direction properties for lighting.
 */
typedef struct _D3DLIGHTINGELEMENT {
    D3DVECTOR dvPosition;           /* Lightable point in model space */
    D3DVECTOR dvNormal;             /* Normalised unit vector */
} D3DLIGHTINGELEMENT, *LPD3DLIGHTINGELEMENT;

/*
 * Structure defining material properties for lighting.
 */
typedef struct _D3DMATERIAL {
    DWORD           dwSize;
    union {
    D3DCOLORVALUE   diffuse;        /* Diffuse color RGBA */
    D3DCOLORVALUE   dcvDiffuse;
    };
    union {
    D3DCOLORVALUE   ambient;        /* Ambient color RGB */
    D3DCOLORVALUE   dcvAmbient;
    };
    union {
    D3DCOLORVALUE   specular;       /* Specular 'shininess' */
    D3DCOLORVALUE   dcvSpecular;
    };
    union {
    D3DCOLORVALUE   emissive;       /* Emissive color RGB */
    D3DCOLORVALUE   dcvEmissive;
    };
    union {
    D3DVALUE        power;          /* Sharpness if specular highlight */
    D3DVALUE        dvPower;
    };
    D3DTEXTUREHANDLE    hTexture;       /* Handle to texture map */
    DWORD           dwRampSize;
} D3DMATERIAL, *LPD3DMATERIAL;

#if(DIRECT3D_VERSION >= 0x0700)

typedef struct _D3DMATERIAL7 {
    union {
    D3DCOLORVALUE   diffuse;        /* Diffuse color RGBA */
    D3DCOLORVALUE   dcvDiffuse;
    };
    union {
    D3DCOLORVALUE   ambient;        /* Ambient color RGB */
    D3DCOLORVALUE   dcvAmbient;
    };
    union {
    D3DCOLORVALUE   specular;       /* Specular 'shininess' */
    D3DCOLORVALUE   dcvSpecular;
    };
    union {
    D3DCOLORVALUE   emissive;       /* Emissive color RGB */
    D3DCOLORVALUE   dcvEmissive;
    };
    union {
    D3DVALUE        power;          /* Sharpness if specular highlight */
    D3DVALUE        dvPower;
    };
} D3DMATERIAL7, *LPD3DMATERIAL7;

#endif /* DIRECT3D_VERSION >= 0x0700 */

#if(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DLIGHTTYPE {
    D3DLIGHT_POINT          = 1,
    D3DLIGHT_SPOT           = 2,
    D3DLIGHT_DIRECTIONAL    = 3,
// Note: The following light type (D3DLIGHT_PARALLELPOINT)
// is no longer supported from D3D for DX7 onwards.
    D3DLIGHT_PARALLELPOINT  = 4,
#if(DIRECT3D_VERSION < 0x0500) // For backward compatible headers
    D3DLIGHT_GLSPOT         = 5,
#endif
    D3DLIGHT_FORCE_DWORD    = 0x7fffffff, /* force 32-bit size enum */
} D3DLIGHTTYPE;

#else
typedef enum _D3DLIGHTTYPE D3DLIGHTTYPE;
#define D3DLIGHT_PARALLELPOINT  (D3DLIGHTTYPE)4
#define D3DLIGHT_GLSPOT         (D3DLIGHTTYPE)5

#endif //(DIRECT3D_VERSION < 0x0800)

/*
 * Structure defining a light source and its properties.
 */
typedef struct _D3DLIGHT {
    DWORD           dwSize;
    D3DLIGHTTYPE    dltType;            /* Type of light source */
    D3DCOLORVALUE   dcvColor;           /* Color of light */
    D3DVECTOR       dvPosition;         /* Position in world space */
    D3DVECTOR       dvDirection;        /* Direction in world space */
    D3DVALUE        dvRange;            /* Cutoff range */
    D3DVALUE        dvFalloff;          /* Falloff */
    D3DVALUE        dvAttenuation0;     /* Constant attenuation */
    D3DVALUE        dvAttenuation1;     /* Linear attenuation */
    D3DVALUE        dvAttenuation2;     /* Quadratic attenuation */
    D3DVALUE        dvTheta;            /* Inner angle of spotlight cone */
    D3DVALUE        dvPhi;              /* Outer angle of spotlight cone */
} D3DLIGHT, *LPD3DLIGHT;

#if(DIRECT3D_VERSION >= 0x0700)

typedef struct _D3DLIGHT7 {
    D3DLIGHTTYPE    dltType;            /* Type of light source */
    D3DCOLORVALUE   dcvDiffuse;         /* Diffuse color of light */
    D3DCOLORVALUE   dcvSpecular;        /* Specular color of light */
    D3DCOLORVALUE   dcvAmbient;         /* Ambient color of light */
    D3DVECTOR       dvPosition;         /* Position in world space */
    D3DVECTOR       dvDirection;        /* Direction in world space */
    D3DVALUE        dvRange;            /* Cutoff range */
    D3DVALUE        dvFalloff;          /* Falloff */
    D3DVALUE        dvAttenuation0;     /* Constant attenuation */
    D3DVALUE        dvAttenuation1;     /* Linear attenuation */
    D3DVALUE        dvAttenuation2;     /* Quadratic attenuation */
    D3DVALUE        dvTheta;            /* Inner angle of spotlight cone */
    D3DVALUE        dvPhi;              /* Outer angle of spotlight cone */
} D3DLIGHT7, *LPD3DLIGHT7;

#endif /* DIRECT3D_VERSION >= 0x0700 */

#if(DIRECT3D_VERSION >= 0x0500)
/*
 * Structure defining a light source and its properties.
 */

/* flags bits */
#define D3DLIGHT_ACTIVE         0x00000001
#define D3DLIGHT_NO_SPECULAR    0x00000002
#define D3DLIGHT_ALL (D3DLIGHT_ACTIVE | D3DLIGHT_NO_SPECULAR)

/* maximum valid light range */
#define D3DLIGHT_RANGE_MAX      ((float)sqrt(FLT_MAX))

typedef struct _D3DLIGHT2 {
    DWORD           dwSize;
    D3DLIGHTTYPE    dltType;        /* Type of light source */
    D3DCOLORVALUE   dcvColor;       /* Color of light */
    D3DVECTOR       dvPosition;     /* Position in world space */
    D3DVECTOR       dvDirection;    /* Direction in world space */
    D3DVALUE        dvRange;        /* Cutoff range */
    D3DVALUE        dvFalloff;      /* Falloff */
    D3DVALUE        dvAttenuation0; /* Constant attenuation */
    D3DVALUE        dvAttenuation1; /* Linear attenuation */
    D3DVALUE        dvAttenuation2; /* Quadratic attenuation */
    D3DVALUE        dvTheta;        /* Inner angle of spotlight cone */
    D3DVALUE        dvPhi;          /* Outer angle of spotlight cone */
    DWORD           dwFlags;
} D3DLIGHT2, *LPD3DLIGHT2;

#endif /* DIRECT3D_VERSION >= 0x0500 */
typedef struct _D3DLIGHTDATA {
    DWORD                dwSize;
    LPD3DLIGHTINGELEMENT lpIn;      /* Input positions and normals */
    DWORD                dwInSize;  /* Stride of input elements */
    LPD3DTLVERTEX        lpOut;     /* Output colors */
    DWORD                dwOutSize; /* Stride of output colors */
} D3DLIGHTDATA, *LPD3DLIGHTDATA;

#if(DIRECT3D_VERSION >= 0x0500)
/*
 * Before DX5, these values were in an enum called
 * D3DCOLORMODEL. This was not correct, since they are
 * bit flags. A driver can surface either or both flags
 * in the dcmColorModel member of D3DDEVICEDESC.
 */
#define D3DCOLOR_MONO   1
#define D3DCOLOR_RGB    2

typedef DWORD D3DCOLORMODEL;
#endif /* DIRECT3D_VERSION >= 0x0500 */

/*
 * Options for clearing
 */
#define D3DCLEAR_TARGET            0x00000001l  /* Clear target surface */
#define D3DCLEAR_ZBUFFER           0x00000002l  /* Clear target z buffer */
#if(DIRECT3D_VERSION >= 0x0600)
#define D3DCLEAR_STENCIL           0x00000004l  /* Clear stencil planes */
#endif /* DIRECT3D_VERSION >= 0x0600 */

/*
 * Execute buffers are allocated via Direct3D.  These buffers may then
 * be filled by the application with instructions to execute along with
 * vertex data.
 */

/*
 * Supported op codes for execute instructions.
 */
typedef enum _D3DOPCODE {
    D3DOP_POINT                 = 1,
    D3DOP_LINE                  = 2,
    D3DOP_TRIANGLE      = 3,
    D3DOP_MATRIXLOAD        = 4,
    D3DOP_MATRIXMULTIPLY    = 5,
    D3DOP_STATETRANSFORM        = 6,
    D3DOP_STATELIGHT        = 7,
    D3DOP_STATERENDER       = 8,
    D3DOP_PROCESSVERTICES       = 9,
    D3DOP_TEXTURELOAD       = 10,
    D3DOP_EXIT                  = 11,
    D3DOP_BRANCHFORWARD     = 12,
    D3DOP_SPAN          = 13,
    D3DOP_SETSTATUS     = 14,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DOP_FORCE_DWORD           = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DOPCODE;

typedef struct _D3DINSTRUCTION {
    BYTE bOpcode;   /* Instruction opcode */
    BYTE bSize;     /* Size of each instruction data unit */
    WORD wCount;    /* Count of instruction data units to follow */
} D3DINSTRUCTION, *LPD3DINSTRUCTION;

/*
 * Structure for texture loads
 */
typedef struct _D3DTEXTURELOAD {
    D3DTEXTUREHANDLE hDestTexture;
    D3DTEXTUREHANDLE hSrcTexture;
} D3DTEXTURELOAD, *LPD3DTEXTURELOAD;

/*
 * Structure for picking
 */
typedef struct _D3DPICKRECORD {
    BYTE     bOpcode;
    BYTE     bPad;
    DWORD    dwOffset;
    D3DVALUE dvZ;
} D3DPICKRECORD, *LPD3DPICKRECORD;

/*
 * The following defines the rendering states which can be set in the
 * execute buffer.
 */

#if(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DSHADEMODE {
    D3DSHADE_FLAT              = 1,
    D3DSHADE_GOURAUD           = 2,
    D3DSHADE_PHONG             = 3,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DSHADE_FORCE_DWORD       = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DSHADEMODE;

typedef enum _D3DFILLMODE {
    D3DFILL_POINT          = 1,
    D3DFILL_WIREFRAME          = 2,
    D3DFILL_SOLID          = 3,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DFILL_FORCE_DWORD        = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DFILLMODE;

typedef struct _D3DLINEPATTERN {
    WORD    wRepeatFactor;
    WORD    wLinePattern;
} D3DLINEPATTERN;

#endif //(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DTEXTUREFILTER {
    D3DFILTER_NEAREST          = 1,
    D3DFILTER_LINEAR           = 2,
    D3DFILTER_MIPNEAREST       = 3,
    D3DFILTER_MIPLINEAR        = 4,
    D3DFILTER_LINEARMIPNEAREST = 5,
    D3DFILTER_LINEARMIPLINEAR  = 6,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DFILTER_FORCE_DWORD      = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DTEXTUREFILTER;

#if(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DBLEND {
    D3DBLEND_ZERO              = 1,
    D3DBLEND_ONE               = 2,
    D3DBLEND_SRCCOLOR          = 3,
    D3DBLEND_INVSRCCOLOR       = 4,
    D3DBLEND_SRCALPHA          = 5,
    D3DBLEND_INVSRCALPHA       = 6,
    D3DBLEND_DESTALPHA         = 7,
    D3DBLEND_INVDESTALPHA      = 8,
    D3DBLEND_DESTCOLOR         = 9,
    D3DBLEND_INVDESTCOLOR      = 10,
    D3DBLEND_SRCALPHASAT       = 11,
    D3DBLEND_BOTHSRCALPHA      = 12,
    D3DBLEND_BOTHINVSRCALPHA   = 13,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DBLEND_FORCE_DWORD       = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DBLEND;

#endif //(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DTEXTUREBLEND {
    D3DTBLEND_DECAL            = 1,
    D3DTBLEND_MODULATE         = 2,
    D3DTBLEND_DECALALPHA       = 3,
    D3DTBLEND_MODULATEALPHA    = 4,
    D3DTBLEND_DECALMASK        = 5,
    D3DTBLEND_MODULATEMASK     = 6,
    D3DTBLEND_COPY             = 7,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DTBLEND_ADD              = 8,
    D3DTBLEND_FORCE_DWORD      = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DTEXTUREBLEND;

#if(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DTEXTUREADDRESS {
    D3DTADDRESS_WRAP           = 1,
    D3DTADDRESS_MIRROR         = 2,
    D3DTADDRESS_CLAMP          = 3,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DTADDRESS_BORDER         = 4,
    D3DTADDRESS_FORCE_DWORD    = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DTEXTUREADDRESS;

typedef enum _D3DCULL {
    D3DCULL_NONE               = 1,
    D3DCULL_CW                 = 2,
    D3DCULL_CCW                = 3,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DCULL_FORCE_DWORD        = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DCULL;

typedef enum _D3DCMPFUNC {
    D3DCMP_NEVER               = 1,
    D3DCMP_LESS                = 2,
    D3DCMP_EQUAL               = 3,
    D3DCMP_LESSEQUAL           = 4,
    D3DCMP_GREATER             = 5,
    D3DCMP_NOTEQUAL            = 6,
    D3DCMP_GREATEREQUAL        = 7,
    D3DCMP_ALWAYS              = 8,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DCMP_FORCE_DWORD         = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DCMPFUNC;

#if(DIRECT3D_VERSION >= 0x0600)
typedef enum _D3DSTENCILOP {
    D3DSTENCILOP_KEEP           = 1,
    D3DSTENCILOP_ZERO           = 2,
    D3DSTENCILOP_REPLACE        = 3,
    D3DSTENCILOP_INCRSAT        = 4,
    D3DSTENCILOP_DECRSAT        = 5,
    D3DSTENCILOP_INVERT         = 6,
    D3DSTENCILOP_INCR           = 7,
    D3DSTENCILOP_DECR           = 8,
    D3DSTENCILOP_FORCE_DWORD    = 0x7fffffff, /* force 32-bit size enum */
} D3DSTENCILOP;
#endif /* DIRECT3D_VERSION >= 0x0600 */

typedef enum _D3DFOGMODE {
    D3DFOG_NONE                = 0,
    D3DFOG_EXP                 = 1,
    D3DFOG_EXP2                = 2,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DFOG_LINEAR              = 3,
    D3DFOG_FORCE_DWORD         = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DFOGMODE;

#if(DIRECT3D_VERSION >= 0x0600)
typedef enum _D3DZBUFFERTYPE {
    D3DZB_FALSE                 = 0,
    D3DZB_TRUE                  = 1, // Z buffering
    D3DZB_USEW                  = 2, // W buffering
    D3DZB_FORCE_DWORD           = 0x7fffffff, /* force 32-bit size enum */
} D3DZBUFFERTYPE;
#endif /* DIRECT3D_VERSION >= 0x0600 */

#endif //(DIRECT3D_VERSION < 0x0800)

#if(DIRECT3D_VERSION >= 0x0500)
typedef enum _D3DANTIALIASMODE {
    D3DANTIALIAS_NONE          = 0,
    D3DANTIALIAS_SORTDEPENDENT = 1,
    D3DANTIALIAS_SORTINDEPENDENT = 2,
    D3DANTIALIAS_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DANTIALIASMODE;

// Vertex types supported by Direct3D
typedef enum _D3DVERTEXTYPE {
    D3DVT_VERTEX        = 1,
    D3DVT_LVERTEX       = 2,
    D3DVT_TLVERTEX      = 3,
    D3DVT_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DVERTEXTYPE;

#if(DIRECT3D_VERSION < 0x0800)

// Primitives supported by draw-primitive API
typedef enum _D3DPRIMITIVETYPE {
    D3DPT_POINTLIST     = 1,
    D3DPT_LINELIST      = 2,
    D3DPT_LINESTRIP     = 3,
    D3DPT_TRIANGLELIST  = 4,
    D3DPT_TRIANGLESTRIP = 5,
    D3DPT_TRIANGLEFAN   = 6,
    D3DPT_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DPRIMITIVETYPE;

#endif //(DIRECT3D_VERSION < 0x0800)

#endif /* DIRECT3D_VERSION >= 0x0500 */
/*
 * Amount to add to a state to generate the override for that state.
 */
#define D3DSTATE_OVERRIDE_BIAS      256

/*
 * A state which sets the override flag for the specified state type.
 */
#define D3DSTATE_OVERRIDE(type) (D3DRENDERSTATETYPE)(((DWORD) (type) + D3DSTATE_OVERRIDE_BIAS))

#if(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DTRANSFORMSTATETYPE {
    D3DTRANSFORMSTATE_WORLD         = 1,
    D3DTRANSFORMSTATE_VIEW          = 2,
    D3DTRANSFORMSTATE_PROJECTION    = 3,
#if(DIRECT3D_VERSION >= 0x0700)
    D3DTRANSFORMSTATE_WORLD1        = 4,  // 2nd matrix to blend
    D3DTRANSFORMSTATE_WORLD2        = 5,  // 3rd matrix to blend
    D3DTRANSFORMSTATE_WORLD3        = 6,  // 4th matrix to blend
    D3DTRANSFORMSTATE_TEXTURE0      = 16,
    D3DTRANSFORMSTATE_TEXTURE1      = 17,
    D3DTRANSFORMSTATE_TEXTURE2      = 18,
    D3DTRANSFORMSTATE_TEXTURE3      = 19,
    D3DTRANSFORMSTATE_TEXTURE4      = 20,
    D3DTRANSFORMSTATE_TEXTURE5      = 21,
    D3DTRANSFORMSTATE_TEXTURE6      = 22,
    D3DTRANSFORMSTATE_TEXTURE7      = 23,
#endif /* DIRECT3D_VERSION >= 0x0700 */
#if(DIRECT3D_VERSION >= 0x0500)
    D3DTRANSFORMSTATE_FORCE_DWORD     = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DTRANSFORMSTATETYPE;

#else

//
// legacy transform state names
//
typedef enum _D3DTRANSFORMSTATETYPE D3DTRANSFORMSTATETYPE;
#define D3DTRANSFORMSTATE_WORLD         (D3DTRANSFORMSTATETYPE)1
#define D3DTRANSFORMSTATE_VIEW          (D3DTRANSFORMSTATETYPE)2
#define D3DTRANSFORMSTATE_PROJECTION    (D3DTRANSFORMSTATETYPE)3
#define D3DTRANSFORMSTATE_WORLD1        (D3DTRANSFORMSTATETYPE)4
#define D3DTRANSFORMSTATE_WORLD2        (D3DTRANSFORMSTATETYPE)5
#define D3DTRANSFORMSTATE_WORLD3        (D3DTRANSFORMSTATETYPE)6
#define D3DTRANSFORMSTATE_TEXTURE0      (D3DTRANSFORMSTATETYPE)16
#define D3DTRANSFORMSTATE_TEXTURE1      (D3DTRANSFORMSTATETYPE)17
#define D3DTRANSFORMSTATE_TEXTURE2      (D3DTRANSFORMSTATETYPE)18
#define D3DTRANSFORMSTATE_TEXTURE3      (D3DTRANSFORMSTATETYPE)19
#define D3DTRANSFORMSTATE_TEXTURE4      (D3DTRANSFORMSTATETYPE)20
#define D3DTRANSFORMSTATE_TEXTURE5      (D3DTRANSFORMSTATETYPE)21
#define D3DTRANSFORMSTATE_TEXTURE6      (D3DTRANSFORMSTATETYPE)22
#define D3DTRANSFORMSTATE_TEXTURE7      (D3DTRANSFORMSTATETYPE)23

#endif //(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DLIGHTSTATETYPE {
    D3DLIGHTSTATE_MATERIAL          = 1,
    D3DLIGHTSTATE_AMBIENT           = 2,
    D3DLIGHTSTATE_COLORMODEL        = 3,
    D3DLIGHTSTATE_FOGMODE           = 4,
    D3DLIGHTSTATE_FOGSTART          = 5,
    D3DLIGHTSTATE_FOGEND            = 6,
    D3DLIGHTSTATE_FOGDENSITY        = 7,
#if(DIRECT3D_VERSION >= 0x0600)
    D3DLIGHTSTATE_COLORVERTEX       = 8,
#endif /* DIRECT3D_VERSION >= 0x0600 */
#if(DIRECT3D_VERSION >= 0x0500)
    D3DLIGHTSTATE_FORCE_DWORD         = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DLIGHTSTATETYPE;

#if(DIRECT3D_VERSION < 0x0800)

typedef enum _D3DRENDERSTATETYPE {
    D3DRENDERSTATE_ANTIALIAS          = 2,    /* D3DANTIALIASMODE */
    D3DRENDERSTATE_TEXTUREPERSPECTIVE = 4,    /* TRUE for perspective correction */
    D3DRENDERSTATE_ZENABLE            = 7,    /* D3DZBUFFERTYPE (or TRUE/FALSE for legacy) */
    D3DRENDERSTATE_FILLMODE           = 8,    /* D3DFILL_MODE        */
    D3DRENDERSTATE_SHADEMODE          = 9,    /* D3DSHADEMODE */
    D3DRENDERSTATE_LINEPATTERN        = 10,   /* D3DLINEPATTERN */
    D3DRENDERSTATE_ZWRITEENABLE       = 14,   /* TRUE to enable z writes */
    D3DRENDERSTATE_ALPHATESTENABLE    = 15,   /* TRUE to enable alpha tests */
    D3DRENDERSTATE_LASTPIXEL          = 16,   /* TRUE for last-pixel on lines */
    D3DRENDERSTATE_SRCBLEND           = 19,   /* D3DBLEND */
    D3DRENDERSTATE_DESTBLEND          = 20,   /* D3DBLEND */
    D3DRENDERSTATE_CULLMODE           = 22,   /* D3DCULL */
    D3DRENDERSTATE_ZFUNC              = 23,   /* D3DCMPFUNC */
    D3DRENDERSTATE_ALPHAREF           = 24,   /* D3DFIXED */
    D3DRENDERSTATE_ALPHAFUNC          = 25,   /* D3DCMPFUNC */
    D3DRENDERSTATE_DITHERENABLE       = 26,   /* TRUE to enable dithering */
#if(DIRECT3D_VERSION >= 0x0500)
    D3DRENDERSTATE_ALPHABLENDENABLE   = 27,   /* TRUE to enable alpha blending */
#endif /* DIRECT3D_VERSION >= 0x0500 */
    D3DRENDERSTATE_FOGENABLE          = 28,   /* TRUE to enable fog blending */
    D3DRENDERSTATE_SPECULARENABLE     = 29,   /* TRUE to enable specular */
    D3DRENDERSTATE_ZVISIBLE           = 30,   /* TRUE to enable z checking */
    D3DRENDERSTATE_STIPPLEDALPHA      = 33,   /* TRUE to enable stippled alpha (RGB device only) */
    D3DRENDERSTATE_FOGCOLOR           = 34,   /* D3DCOLOR */
    D3DRENDERSTATE_FOGTABLEMODE       = 35,   /* D3DFOGMODE */
#if(DIRECT3D_VERSION >= 0x0700)
    D3DRENDERSTATE_FOGSTART           = 36,   /* Fog start (for both vertex and pixel fog) */
    D3DRENDERSTATE_FOGEND             = 37,   /* Fog end      */
    D3DRENDERSTATE_FOGDENSITY         = 38,   /* Fog density  */
#endif /* DIRECT3D_VERSION >= 0x0700 */
#if(DIRECT3D_VERSION >= 0x0500)
    D3DRENDERSTATE_EDGEANTIALIAS      = 40,   /* TRUE to enable edge antialiasing */
    D3DRENDERSTATE_COLORKEYENABLE     = 41,   /* TRUE to enable source colorkeyed textures */
    D3DRENDERSTATE_ZBIAS              = 47,   /* LONG Z bias */
    D3DRENDERSTATE_RANGEFOGENABLE     = 48,   /* Enables range-based fog */
#endif /* DIRECT3D_VERSION >= 0x0500 */

#if(DIRECT3D_VERSION >= 0x0600)
    D3DRENDERSTATE_STENCILENABLE      = 52,   /* BOOL enable/disable stenciling */
    D3DRENDERSTATE_STENCILFAIL        = 53,   /* D3DSTENCILOP to do if stencil test fails */
    D3DRENDERSTATE_STENCILZFAIL       = 54,   /* D3DSTENCILOP to do if stencil test passes and Z test fails */
    D3DRENDERSTATE_STENCILPASS        = 55,   /* D3DSTENCILOP to do if both stencil and Z tests pass */
    D3DRENDERSTATE_STENCILFUNC        = 56,   /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
    D3DRENDERSTATE_STENCILREF         = 57,   /* Reference value used in stencil test */
    D3DRENDERSTATE_STENCILMASK        = 58,   /* Mask value used in stencil test */
    D3DRENDERSTATE_STENCILWRITEMASK   = 59,   /* Write mask applied to values written to stencil buffer */
    D3DRENDERSTATE_TEXTUREFACTOR      = 60,   /* D3DCOLOR used for multi-texture blend */
#endif /* DIRECT3D_VERSION >= 0x0600 */

#if(DIRECT3D_VERSION >= 0x0600)

    /*
     * 128 values [128, 255] are reserved for texture coordinate wrap flags.
     * These are constructed with the D3DWRAP_U and D3DWRAP_V macros. Using
     * a flags word preserves forward compatibility with texture coordinates
     * that are >2D.
     */
    D3DRENDERSTATE_WRAP0              = 128,  /* wrap for 1st texture coord. set */
    D3DRENDERSTATE_WRAP1              = 129,  /* wrap for 2nd texture coord. set */
    D3DRENDERSTATE_WRAP2              = 130,  /* wrap for 3rd texture coord. set */
    D3DRENDERSTATE_WRAP3              = 131,  /* wrap for 4th texture coord. set */
    D3DRENDERSTATE_WRAP4              = 132,  /* wrap for 5th texture coord. set */
    D3DRENDERSTATE_WRAP5              = 133,  /* wrap for 6th texture coord. set */
    D3DRENDERSTATE_WRAP6              = 134,  /* wrap for 7th texture coord. set */
    D3DRENDERSTATE_WRAP7              = 135,  /* wrap for 8th texture coord. set */
#endif /* DIRECT3D_VERSION >= 0x0600 */
#if(DIRECT3D_VERSION >= 0x0700)
    D3DRENDERSTATE_CLIPPING            = 136,
    D3DRENDERSTATE_LIGHTING            = 137,
    D3DRENDERSTATE_EXTENTS             = 138,
    D3DRENDERSTATE_AMBIENT             = 139,
    D3DRENDERSTATE_FOGVERTEXMODE       = 140,
    D3DRENDERSTATE_COLORVERTEX         = 141,
    D3DRENDERSTATE_LOCALVIEWER         = 142,
    D3DRENDERSTATE_NORMALIZENORMALS    = 143,
    D3DRENDERSTATE_COLORKEYBLENDENABLE = 144,
    D3DRENDERSTATE_DIFFUSEMATERIALSOURCE    = 145,
    D3DRENDERSTATE_SPECULARMATERIALSOURCE   = 146,
    D3DRENDERSTATE_AMBIENTMATERIALSOURCE    = 147,
    D3DRENDERSTATE_EMISSIVEMATERIALSOURCE   = 148,
    D3DRENDERSTATE_VERTEXBLEND              = 151,
    D3DRENDERSTATE_CLIPPLANEENABLE          = 152,

#endif /* DIRECT3D_VERSION >= 0x0700 */

//
// retired renderstates - not supported for DX7 interfaces
//
    D3DRENDERSTATE_TEXTUREHANDLE      = 1,    /* Texture handle for legacy interfaces (Texture,Texture2) */
    D3DRENDERSTATE_TEXTUREADDRESS     = 3,    /* D3DTEXTUREADDRESS  */
    D3DRENDERSTATE_WRAPU              = 5,    /* TRUE for wrapping in u */
    D3DRENDERSTATE_WRAPV              = 6,    /* TRUE for wrapping in v */
    D3DRENDERSTATE_MONOENABLE         = 11,   /* TRUE to enable mono rasterization */
    D3DRENDERSTATE_ROP2               = 12,   /* ROP2 */
    D3DRENDERSTATE_PLANEMASK          = 13,   /* DWORD physical plane mask */
    D3DRENDERSTATE_TEXTUREMAG         = 17,   /* D3DTEXTUREFILTER */
    D3DRENDERSTATE_TEXTUREMIN         = 18,   /* D3DTEXTUREFILTER */
    D3DRENDERSTATE_TEXTUREMAPBLEND    = 21,   /* D3DTEXTUREBLEND */
    D3DRENDERSTATE_SUBPIXEL           = 31,   /* TRUE to enable subpixel correction */
    D3DRENDERSTATE_SUBPIXELX          = 32,   /* TRUE to enable correction in X only */
    D3DRENDERSTATE_STIPPLEENABLE      = 39,   /* TRUE to enable stippling */
#if(DIRECT3D_VERSION >= 0x0500)
    D3DRENDERSTATE_BORDERCOLOR        = 43,   /* Border color for texturing w/border */
    D3DRENDERSTATE_TEXTUREADDRESSU    = 44,   /* Texture addressing mode for U coordinate */
    D3DRENDERSTATE_TEXTUREADDRESSV    = 45,   /* Texture addressing mode for V coordinate */
    D3DRENDERSTATE_MIPMAPLODBIAS      = 46,   /* D3DVALUE Mipmap LOD bias */
    D3DRENDERSTATE_ANISOTROPY         = 49,   /* Max. anisotropy. 1 = no anisotropy */
#endif /* DIRECT3D_VERSION >= 0x0500 */
    D3DRENDERSTATE_FLUSHBATCH         = 50,   /* Explicit flush for DP batching (DX5 Only) */
#if(DIRECT3D_VERSION >= 0x0600)
    D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT=51, /* BOOL enable sort-independent transparency */
#endif /* DIRECT3D_VERSION >= 0x0600 */
    D3DRENDERSTATE_STIPPLEPATTERN00   = 64,   /* Stipple pattern 01...  */
    D3DRENDERSTATE_STIPPLEPATTERN01   = 65,
    D3DRENDERSTATE_STIPPLEPATTERN02   = 66,
    D3DRENDERSTATE_STIPPLEPATTERN03   = 67,
    D3DRENDERSTATE_STIPPLEPATTERN04   = 68,
    D3DRENDERSTATE_STIPPLEPATTERN05   = 69,
    D3DRENDERSTATE_STIPPLEPATTERN06   = 70,
    D3DRENDERSTATE_STIPPLEPATTERN07   = 71,
    D3DRENDERSTATE_STIPPLEPATTERN08   = 72,
    D3DRENDERSTATE_STIPPLEPATTERN09   = 73,
    D3DRENDERSTATE_STIPPLEPATTERN10   = 74,
    D3DRENDERSTATE_STIPPLEPATTERN11   = 75,
    D3DRENDERSTATE_STIPPLEPATTERN12   = 76,
    D3DRENDERSTATE_STIPPLEPATTERN13   = 77,
    D3DRENDERSTATE_STIPPLEPATTERN14   = 78,
    D3DRENDERSTATE_STIPPLEPATTERN15   = 79,
    D3DRENDERSTATE_STIPPLEPATTERN16   = 80,
    D3DRENDERSTATE_STIPPLEPATTERN17   = 81,
    D3DRENDERSTATE_STIPPLEPATTERN18   = 82,
    D3DRENDERSTATE_STIPPLEPATTERN19   = 83,
    D3DRENDERSTATE_STIPPLEPATTERN20   = 84,
    D3DRENDERSTATE_STIPPLEPATTERN21   = 85,
    D3DRENDERSTATE_STIPPLEPATTERN22   = 86,
    D3DRENDERSTATE_STIPPLEPATTERN23   = 87,
    D3DRENDERSTATE_STIPPLEPATTERN24   = 88,
    D3DRENDERSTATE_STIPPLEPATTERN25   = 89,
    D3DRENDERSTATE_STIPPLEPATTERN26   = 90,
    D3DRENDERSTATE_STIPPLEPATTERN27   = 91,
    D3DRENDERSTATE_STIPPLEPATTERN28   = 92,
    D3DRENDERSTATE_STIPPLEPATTERN29   = 93,
    D3DRENDERSTATE_STIPPLEPATTERN30   = 94,
    D3DRENDERSTATE_STIPPLEPATTERN31   = 95,

//
// retired renderstate names - the values are still used under new naming conventions
//
    D3DRENDERSTATE_FOGTABLESTART      = 36,   /* Fog table start    */
    D3DRENDERSTATE_FOGTABLEEND        = 37,   /* Fog table end      */
    D3DRENDERSTATE_FOGTABLEDENSITY    = 38,   /* Fog table density  */

#if(DIRECT3D_VERSION >= 0x0500)
    D3DRENDERSTATE_FORCE_DWORD        = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DRENDERSTATETYPE;

#else

typedef enum _D3DRENDERSTATETYPE D3DRENDERSTATETYPE;

//
// legacy renderstate names
//
#define D3DRENDERSTATE_TEXTUREPERSPECTIVE       (D3DRENDERSTATETYPE)4
#define D3DRENDERSTATE_ZENABLE                  (D3DRENDERSTATETYPE)7
#define D3DRENDERSTATE_FILLMODE                 (D3DRENDERSTATETYPE)8
#define D3DRENDERSTATE_SHADEMODE                (D3DRENDERSTATETYPE)9
#define D3DRENDERSTATE_LINEPATTERN              (D3DRENDERSTATETYPE)10
#define D3DRENDERSTATE_ZWRITEENABLE             (D3DRENDERSTATETYPE)14
#define D3DRENDERSTATE_ALPHATESTENABLE          (D3DRENDERSTATETYPE)15
#define D3DRENDERSTATE_LASTPIXEL                (D3DRENDERSTATETYPE)16
#define D3DRENDERSTATE_SRCBLEND                 (D3DRENDERSTATETYPE)19
#define D3DRENDERSTATE_DESTBLEND                (D3DRENDERSTATETYPE)20
#define D3DRENDERSTATE_CULLMODE                 (D3DRENDERSTATETYPE)22
#define D3DRENDERSTATE_ZFUNC                    (D3DRENDERSTATETYPE)23
#define D3DRENDERSTATE_ALPHAREF                 (D3DRENDERSTATETYPE)24
#define D3DRENDERSTATE_ALPHAFUNC                (D3DRENDERSTATETYPE)25
#define D3DRENDERSTATE_DITHERENABLE             (D3DRENDERSTATETYPE)26
#define D3DRENDERSTATE_ALPHABLENDENABLE         (D3DRENDERSTATETYPE)27
#define D3DRENDERSTATE_FOGENABLE                (D3DRENDERSTATETYPE)28
#define D3DRENDERSTATE_SPECULARENABLE           (D3DRENDERSTATETYPE)29
#define D3DRENDERSTATE_ZVISIBLE                 (D3DRENDERSTATETYPE)30
#define D3DRENDERSTATE_STIPPLEDALPHA            (D3DRENDERSTATETYPE)33
#define D3DRENDERSTATE_FOGCOLOR                 (D3DRENDERSTATETYPE)34
#define D3DRENDERSTATE_FOGTABLEMODE             (D3DRENDERSTATETYPE)35
#define D3DRENDERSTATE_FOGSTART                 (D3DRENDERSTATETYPE)36
#define D3DRENDERSTATE_FOGEND                   (D3DRENDERSTATETYPE)37
#define D3DRENDERSTATE_FOGDENSITY               (D3DRENDERSTATETYPE)38
#define D3DRENDERSTATE_EDGEANTIALIAS            (D3DRENDERSTATETYPE)40
#define D3DRENDERSTATE_ZBIAS                    (D3DRENDERSTATETYPE)47
#define D3DRENDERSTATE_RANGEFOGENABLE           (D3DRENDERSTATETYPE)48
#define D3DRENDERSTATE_STENCILENABLE            (D3DRENDERSTATETYPE)52
#define D3DRENDERSTATE_STENCILFAIL              (D3DRENDERSTATETYPE)53
#define D3DRENDERSTATE_STENCILZFAIL             (D3DRENDERSTATETYPE)54
#define D3DRENDERSTATE_STENCILPASS              (D3DRENDERSTATETYPE)55
#define D3DRENDERSTATE_STENCILFUNC              (D3DRENDERSTATETYPE)56
#define D3DRENDERSTATE_STENCILREF               (D3DRENDERSTATETYPE)57
#define D3DRENDERSTATE_STENCILMASK              (D3DRENDERSTATETYPE)58
#define D3DRENDERSTATE_STENCILWRITEMASK         (D3DRENDERSTATETYPE)59
#define D3DRENDERSTATE_TEXTUREFACTOR            (D3DRENDERSTATETYPE)60
#define D3DRENDERSTATE_WRAP0                    (D3DRENDERSTATETYPE)128
#define D3DRENDERSTATE_WRAP1                    (D3DRENDERSTATETYPE)129
#define D3DRENDERSTATE_WRAP2                    (D3DRENDERSTATETYPE)130
#define D3DRENDERSTATE_WRAP3                    (D3DRENDERSTATETYPE)131
#define D3DRENDERSTATE_WRAP4                    (D3DRENDERSTATETYPE)132
#define D3DRENDERSTATE_WRAP5                    (D3DRENDERSTATETYPE)133
#define D3DRENDERSTATE_WRAP6                    (D3DRENDERSTATETYPE)134
#define D3DRENDERSTATE_WRAP7                    (D3DRENDERSTATETYPE)135

#define D3DRENDERSTATE_CLIPPING                 (D3DRENDERSTATETYPE)136
#define D3DRENDERSTATE_LIGHTING                 (D3DRENDERSTATETYPE)137
#define D3DRENDERSTATE_EXTENTS                  (D3DRENDERSTATETYPE)138
#define D3DRENDERSTATE_AMBIENT                  (D3DRENDERSTATETYPE)139
#define D3DRENDERSTATE_FOGVERTEXMODE            (D3DRENDERSTATETYPE)140
#define D3DRENDERSTATE_COLORVERTEX              (D3DRENDERSTATETYPE)141
#define D3DRENDERSTATE_LOCALVIEWER              (D3DRENDERSTATETYPE)142
#define D3DRENDERSTATE_NORMALIZENORMALS         (D3DRENDERSTATETYPE)143
#define D3DRENDERSTATE_COLORKEYBLENDENABLE      (D3DRENDERSTATETYPE)144
#define D3DRENDERSTATE_DIFFUSEMATERIALSOURCE    (D3DRENDERSTATETYPE)145
#define D3DRENDERSTATE_SPECULARMATERIALSOURCE   (D3DRENDERSTATETYPE)146
#define D3DRENDERSTATE_AMBIENTMATERIALSOURCE    (D3DRENDERSTATETYPE)147
#define D3DRENDERSTATE_EMISSIVEMATERIALSOURCE   (D3DRENDERSTATETYPE)148
#define D3DRENDERSTATE_VERTEXBLEND              (D3DRENDERSTATETYPE)151
#define D3DRENDERSTATE_CLIPPLANEENABLE          (D3DRENDERSTATETYPE)152

//
// retired renderstates - not supported for DX7 interfaces
//
#define D3DRENDERSTATE_TEXTUREHANDLE     (D3DRENDERSTATETYPE)1
#define D3DRENDERSTATE_ANTIALIAS         (D3DRENDERSTATETYPE)2
#define D3DRENDERSTATE_TEXTUREADDRESS    (D3DRENDERSTATETYPE)3
#define D3DRENDERSTATE_WRAPU             (D3DRENDERSTATETYPE)5
#define D3DRENDERSTATE_WRAPV             (D3DRENDERSTATETYPE)6
#define D3DRENDERSTATE_MONOENABLE        (D3DRENDERSTATETYPE)11
#define D3DRENDERSTATE_ROP2              (D3DRENDERSTATETYPE)12
#define D3DRENDERSTATE_PLANEMASK         (D3DRENDERSTATETYPE)13
#define D3DRENDERSTATE_TEXTUREMAG        (D3DRENDERSTATETYPE)17
#define D3DRENDERSTATE_TEXTUREMIN        (D3DRENDERSTATETYPE)18
#define D3DRENDERSTATE_TEXTUREMAPBLEND   (D3DRENDERSTATETYPE)21
#define D3DRENDERSTATE_SUBPIXEL          (D3DRENDERSTATETYPE)31
#define D3DRENDERSTATE_SUBPIXELX         (D3DRENDERSTATETYPE)32
#define D3DRENDERSTATE_STIPPLEENABLE     (D3DRENDERSTATETYPE)39
#define D3DRENDERSTATE_OLDALPHABLENDENABLE  (D3DRENDERSTATETYPE)42
#define D3DRENDERSTATE_BORDERCOLOR       (D3DRENDERSTATETYPE)43
#define D3DRENDERSTATE_TEXTUREADDRESSU   (D3DRENDERSTATETYPE)44
#define D3DRENDERSTATE_TEXTUREADDRESSV   (D3DRENDERSTATETYPE)45
#define D3DRENDERSTATE_MIPMAPLODBIAS     (D3DRENDERSTATETYPE)46
#define D3DRENDERSTATE_ANISOTROPY        (D3DRENDERSTATETYPE)49
#define D3DRENDERSTATE_FLUSHBATCH        (D3DRENDERSTATETYPE)50
#define D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT (D3DRENDERSTATETYPE)51
#define D3DRENDERSTATE_STIPPLEPATTERN00  (D3DRENDERSTATETYPE)64
#define D3DRENDERSTATE_STIPPLEPATTERN01  (D3DRENDERSTATETYPE)65
#define D3DRENDERSTATE_STIPPLEPATTERN02  (D3DRENDERSTATETYPE)66
#define D3DRENDERSTATE_STIPPLEPATTERN03  (D3DRENDERSTATETYPE)67
#define D3DRENDERSTATE_STIPPLEPATTERN04  (D3DRENDERSTATETYPE)68
#define D3DRENDERSTATE_STIPPLEPATTERN05  (D3DRENDERSTATETYPE)69
#define D3DRENDERSTATE_STIPPLEPATTERN06  (D3DRENDERSTATETYPE)70
#define D3DRENDERSTATE_STIPPLEPATTERN07  (D3DRENDERSTATETYPE)71
#define D3DRENDERSTATE_STIPPLEPATTERN08  (D3DRENDERSTATETYPE)72
#define D3DRENDERSTATE_STIPPLEPATTERN09  (D3DRENDERSTATETYPE)73
#define D3DRENDERSTATE_STIPPLEPATTERN10  (D3DRENDERSTATETYPE)74
#define D3DRENDERSTATE_STIPPLEPATTERN11  (D3DRENDERSTATETYPE)75
#define D3DRENDERSTATE_STIPPLEPATTERN12  (D3DRENDERSTATETYPE)76
#define D3DRENDERSTATE_STIPPLEPATTERN13  (D3DRENDERSTATETYPE)77
#define D3DRENDERSTATE_STIPPLEPATTERN14  (D3DRENDERSTATETYPE)78
#define D3DRENDERSTATE_STIPPLEPATTERN15  (D3DRENDERSTATETYPE)79
#define D3DRENDERSTATE_STIPPLEPATTERN16  (D3DRENDERSTATETYPE)80
#define D3DRENDERSTATE_STIPPLEPATTERN17  (D3DRENDERSTATETYPE)81
#define D3DRENDERSTATE_STIPPLEPATTERN18  (D3DRENDERSTATETYPE)82
#define D3DRENDERSTATE_STIPPLEPATTERN19  (D3DRENDERSTATETYPE)83
#define D3DRENDERSTATE_STIPPLEPATTERN20  (D3DRENDERSTATETYPE)84
#define D3DRENDERSTATE_STIPPLEPATTERN21  (D3DRENDERSTATETYPE)85
#define D3DRENDERSTATE_STIPPLEPATTERN22  (D3DRENDERSTATETYPE)86
#define D3DRENDERSTATE_STIPPLEPATTERN23  (D3DRENDERSTATETYPE)87
#define D3DRENDERSTATE_STIPPLEPATTERN24  (D3DRENDERSTATETYPE)88
#define D3DRENDERSTATE_STIPPLEPATTERN25  (D3DRENDERSTATETYPE)89
#define D3DRENDERSTATE_STIPPLEPATTERN26  (D3DRENDERSTATETYPE)90
#define D3DRENDERSTATE_STIPPLEPATTERN27  (D3DRENDERSTATETYPE)91
#define D3DRENDERSTATE_STIPPLEPATTERN28  (D3DRENDERSTATETYPE)92
#define D3DRENDERSTATE_STIPPLEPATTERN29  (D3DRENDERSTATETYPE)93
#define D3DRENDERSTATE_STIPPLEPATTERN30  (D3DRENDERSTATETYPE)94
#define D3DRENDERSTATE_STIPPLEPATTERN31  (D3DRENDERSTATETYPE)95

//
// retired renderstates - not supported for DX8 interfaces
//
#define D3DRENDERSTATE_COLORKEYENABLE        (D3DRENDERSTATETYPE)41
#define D3DRENDERSTATE_COLORKEYBLENDENABLE   (D3DRENDERSTATETYPE)144

//
// retired renderstate names - the values are still used under new naming conventions
//
#define D3DRENDERSTATE_BLENDENABLE       (D3DRENDERSTATETYPE)27
#define D3DRENDERSTATE_FOGTABLESTART     (D3DRENDERSTATETYPE)36
#define D3DRENDERSTATE_FOGTABLEEND       (D3DRENDERSTATETYPE)37
#define D3DRENDERSTATE_FOGTABLEDENSITY   (D3DRENDERSTATETYPE)38

#endif //(DIRECT3D_VERSION < 0x0800)


#if(DIRECT3D_VERSION < 0x0800)

// Values for material source
typedef enum _D3DMATERIALCOLORSOURCE
{
    D3DMCS_MATERIAL = 0,                // Color from material is used
    D3DMCS_COLOR1   = 1,                // Diffuse vertex color is used
    D3DMCS_COLOR2   = 2,                // Specular vertex color is used
    D3DMCS_FORCE_DWORD = 0x7fffffff,    // force 32-bit size enum
} D3DMATERIALCOLORSOURCE;


#if(DIRECT3D_VERSION >= 0x0500)
// For back-compatibility with legacy compilations
#define D3DRENDERSTATE_BLENDENABLE      D3DRENDERSTATE_ALPHABLENDENABLE
#endif /* DIRECT3D_VERSION >= 0x0500 */

#if(DIRECT3D_VERSION >= 0x0600)

// Bias to apply to the texture coordinate set to apply a wrap to.
#define D3DRENDERSTATE_WRAPBIAS                 128UL

/* Flags to construct the WRAP render states */
#define D3DWRAP_U   0x00000001L
#define D3DWRAP_V   0x00000002L

#endif /* DIRECT3D_VERSION >= 0x0600 */

#if(DIRECT3D_VERSION >= 0x0700)

/* Flags to construct the WRAP render states for 1D thru 4D texture coordinates */
#define D3DWRAPCOORD_0   0x00000001L    // same as D3DWRAP_U
#define D3DWRAPCOORD_1   0x00000002L    // same as D3DWRAP_V
#define D3DWRAPCOORD_2   0x00000004L
#define D3DWRAPCOORD_3   0x00000008L

#endif /* DIRECT3D_VERSION >= 0x0700 */

#endif //(DIRECT3D_VERSION < 0x0800)

#define D3DRENDERSTATE_STIPPLEPATTERN(y) (D3DRENDERSTATE_STIPPLEPATTERN00 + (y))

typedef struct _D3DSTATE {
    union {
#if(DIRECT3D_VERSION < 0x0800)
    D3DTRANSFORMSTATETYPE   dtstTransformStateType;
#endif //(DIRECT3D_VERSION < 0x0800)
    D3DLIGHTSTATETYPE   dlstLightStateType;
    D3DRENDERSTATETYPE  drstRenderStateType;
    };
    union {
    DWORD           dwArg[1];
    D3DVALUE        dvArg[1];
    };
} D3DSTATE, *LPD3DSTATE;


/*
 * Operation used to load matrices
 * hDstMat = hSrcMat
 */
typedef struct _D3DMATRIXLOAD {
    D3DMATRIXHANDLE hDestMatrix;   /* Destination matrix */
    D3DMATRIXHANDLE hSrcMatrix;   /* Source matrix */
} D3DMATRIXLOAD, *LPD3DMATRIXLOAD;

/*
 * Operation used to multiply matrices
 * hDstMat = hSrcMat1 * hSrcMat2
 */
typedef struct _D3DMATRIXMULTIPLY {
    D3DMATRIXHANDLE hDestMatrix;   /* Destination matrix */
    D3DMATRIXHANDLE hSrcMatrix1;  /* First source matrix */
    D3DMATRIXHANDLE hSrcMatrix2;  /* Second source matrix */
} D3DMATRIXMULTIPLY, *LPD3DMATRIXMULTIPLY;

/*
 * Operation used to transform and light vertices.
 */
typedef struct _D3DPROCESSVERTICES {
    DWORD        dwFlags;    /* Do we transform or light or just copy? */
    WORD         wStart;     /* Index to first vertex in source */
    WORD         wDest;      /* Index to first vertex in local buffer */
    DWORD        dwCount;    /* Number of vertices to be processed */
    DWORD    dwReserved; /* Must be zero */
} D3DPROCESSVERTICES, *LPD3DPROCESSVERTICES;

#define D3DPROCESSVERTICES_TRANSFORMLIGHT   0x00000000L
#define D3DPROCESSVERTICES_TRANSFORM        0x00000001L
#define D3DPROCESSVERTICES_COPY         0x00000002L
#define D3DPROCESSVERTICES_OPMASK       0x00000007L

#define D3DPROCESSVERTICES_UPDATEEXTENTS    0x00000008L
#define D3DPROCESSVERTICES_NOCOLOR      0x00000010L


#if(DIRECT3D_VERSION >= 0x0600)


#if(DIRECT3D_VERSION < 0x0800)

/*
 * State enumerants for per-stage texture processing.
 */
typedef enum _D3DTEXTURESTAGESTATETYPE
{
    D3DTSS_COLOROP        =  1, /* D3DTEXTUREOP - per-stage blending controls for color channels */
    D3DTSS_COLORARG1      =  2, /* D3DTA_* (texture arg) */
    D3DTSS_COLORARG2      =  3, /* D3DTA_* (texture arg) */
    D3DTSS_ALPHAOP        =  4, /* D3DTEXTUREOP - per-stage blending controls for alpha channel */
    D3DTSS_ALPHAARG1      =  5, /* D3DTA_* (texture arg) */
    D3DTSS_ALPHAARG2      =  6, /* D3DTA_* (texture arg) */
    D3DTSS_BUMPENVMAT00   =  7, /* D3DVALUE (bump mapping matrix) */
    D3DTSS_BUMPENVMAT01   =  8, /* D3DVALUE (bump mapping matrix) */
    D3DTSS_BUMPENVMAT10   =  9, /* D3DVALUE (bump mapping matrix) */
    D3DTSS_BUMPENVMAT11   = 10, /* D3DVALUE (bump mapping matrix) */
    D3DTSS_TEXCOORDINDEX  = 11, /* identifies which set of texture coordinates index this texture */
    D3DTSS_ADDRESS        = 12, /* D3DTEXTUREADDRESS for both coordinates */
    D3DTSS_ADDRESSU       = 13, /* D3DTEXTUREADDRESS for U coordinate */
    D3DTSS_ADDRESSV       = 14, /* D3DTEXTUREADDRESS for V coordinate */
    D3DTSS_BORDERCOLOR    = 15, /* D3DCOLOR */
    D3DTSS_MAGFILTER      = 16, /* D3DTEXTUREMAGFILTER filter to use for magnification */
    D3DTSS_MINFILTER      = 17, /* D3DTEXTUREMINFILTER filter to use for minification */
    D3DTSS_MIPFILTER      = 18, /* D3DTEXTUREMIPFILTER filter to use between mipmaps during minification */
    D3DTSS_MIPMAPLODBIAS  = 19, /* D3DVALUE Mipmap LOD bias */
    D3DTSS_MAXMIPLEVEL    = 20, /* DWORD 0..(n-1) LOD index of largest map to use (0 == largest) */
    D3DTSS_MAXANISOTROPY  = 21, /* DWORD maximum anisotropy */
    D3DTSS_BUMPENVLSCALE  = 22, /* D3DVALUE scale for bump map luminance */
    D3DTSS_BUMPENVLOFFSET = 23, /* D3DVALUE offset for bump map luminance */
#if(DIRECT3D_VERSION >= 0x0700)
    D3DTSS_TEXTURETRANSFORMFLAGS = 24, /* D3DTEXTURETRANSFORMFLAGS controls texture transform */
#endif /* DIRECT3D_VERSION >= 0x0700 */
    D3DTSS_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DTEXTURESTAGESTATETYPE;

#if(DIRECT3D_VERSION >= 0x0700)
// Values, used with D3DTSS_TEXCOORDINDEX, to specify that the vertex data(position
// and normal in the camera space) should be taken as texture coordinates
// Low 16 bits are used to specify texture coordinate index, to take the WRAP mode from
//
#define D3DTSS_TCI_PASSTHRU                             0x00000000
#define D3DTSS_TCI_CAMERASPACENORMAL                    0x00010000
#define D3DTSS_TCI_CAMERASPACEPOSITION                  0x00020000
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR          0x00030000
#endif /* DIRECT3D_VERSION >= 0x0700 */

/*
 * Enumerations for COLOROP and ALPHAOP texture blending operations set in
 * texture processing stage controls in D3DRENDERSTATE.
 */
typedef enum _D3DTEXTUREOP
{
// Control
    D3DTOP_DISABLE    = 1,      // disables stage
    D3DTOP_SELECTARG1 = 2,      // the default
    D3DTOP_SELECTARG2 = 3,

// Modulate
    D3DTOP_MODULATE   = 4,      // multiply args together
    D3DTOP_MODULATE2X = 5,      // multiply and  1 bit
    D3DTOP_MODULATE4X = 6,      // multiply and  2 bits

// Add
    D3DTOP_ADD          =  7,   // add arguments together
    D3DTOP_ADDSIGNED    =  8,   // add with -0.5 bias
    D3DTOP_ADDSIGNED2X  =  9,   // as above but left  1 bit
    D3DTOP_SUBTRACT     = 10,   // Arg1 - Arg2, with no saturation
    D3DTOP_ADDSMOOTH    = 11,   // add 2 args, subtract product
                                // Arg1 + Arg2 - Arg1*Arg2
                                // = Arg1 + (1-Arg1)*Arg2

// Linear alpha blend: Arg1*(Alpha) + Arg2*(1-Alpha)
    D3DTOP_BLENDDIFFUSEALPHA    = 12, // iterated alpha
    D3DTOP_BLENDTEXTUREALPHA    = 13, // texture alpha
    D3DTOP_BLENDFACTORALPHA     = 14, // alpha from D3DRENDERSTATE_TEXTUREFACTOR
    // Linear alpha blend with pre-multiplied arg1 input: Arg1 + Arg2*(1-Alpha)
    D3DTOP_BLENDTEXTUREALPHAPM  = 15, // texture alpha
    D3DTOP_BLENDCURRENTALPHA    = 16, // by alpha of current color

// Specular mapping
    D3DTOP_PREMODULATE            = 17,     // modulate with next texture before use
    D3DTOP_MODULATEALPHA_ADDCOLOR = 18,     // Arg1.RGB + Arg1.A*Arg2.RGB
                                            // COLOROP only
    D3DTOP_MODULATECOLOR_ADDALPHA = 19,     // Arg1.RGB*Arg2.RGB + Arg1.A
                                            // COLOROP only
    D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,  // (1-Arg1.A)*Arg2.RGB + Arg1.RGB
                                            // COLOROP only
    D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,  // (1-Arg1.RGB)*Arg2.RGB + Arg1.A
                                            // COLOROP only

// Bump mapping
    D3DTOP_BUMPENVMAP           = 22, // per pixel env map perturbation
    D3DTOP_BUMPENVMAPLUMINANCE  = 23, // with luminance channel
    // This can do either diffuse or specular bump mapping with correct input.
    // Performs the function (Arg1.R*Arg2.R + Arg1.G*Arg2.G + Arg1.B*Arg2.B)
    // where each component has been scaled and offset to make it signed.
    // The result is replicated into all four (including alpha) channels.
    // This is a valid COLOROP only.
    D3DTOP_DOTPRODUCT3          = 24,

    D3DTOP_FORCE_DWORD = 0x7fffffff,
} D3DTEXTUREOP;

/*
 * Values for COLORARG1,2 and ALPHAARG1,2 texture blending operations
 * set in texture processing stage controls in D3DRENDERSTATE.
 */
#define D3DTA_SELECTMASK        0x0000000f  // mask for arg selector
#define D3DTA_DIFFUSE           0x00000000  // select diffuse color
#define D3DTA_CURRENT           0x00000001  // select result of previous stage
#define D3DTA_TEXTURE           0x00000002  // select texture color
#define D3DTA_TFACTOR           0x00000003  // select RENDERSTATE_TEXTUREFACTOR
#if(DIRECT3D_VERSION >= 0x0700)
#define D3DTA_SPECULAR          0x00000004  // select specular color
#endif /* DIRECT3D_VERSION >= 0x0700 */
#define D3DTA_COMPLEMENT        0x00000010  // take 1.0 - x
#define D3DTA_ALPHAREPLICATE    0x00000020  // replicate alpha to color components

#endif //(DIRECT3D_VERSION < 0x0800)

/*
 *  IDirect3DTexture2 State Filter Types
 */
typedef enum _D3DTEXTUREMAGFILTER
{
    D3DTFG_POINT        = 1,    // nearest
    D3DTFG_LINEAR       = 2,    // linear interpolation
    D3DTFG_FLATCUBIC    = 3,    // cubic
    D3DTFG_GAUSSIANCUBIC = 4,   // different cubic kernel
    D3DTFG_ANISOTROPIC  = 5,    //
#if(DIRECT3D_VERSION >= 0x0700)
#endif /* DIRECT3D_VERSION >= 0x0700 */
    D3DTFG_FORCE_DWORD  = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMAGFILTER;

typedef enum _D3DTEXTUREMINFILTER
{
    D3DTFN_POINT        = 1,    // nearest
    D3DTFN_LINEAR       = 2,    // linear interpolation
    D3DTFN_ANISOTROPIC  = 3,    //
    D3DTFN_FORCE_DWORD  = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMINFILTER;

typedef enum _D3DTEXTUREMIPFILTER
{
    D3DTFP_NONE         = 1,    // mipmapping disabled (use MAG filter)
    D3DTFP_POINT        = 2,    // nearest
    D3DTFP_LINEAR       = 3,    // linear interpolation
    D3DTFP_FORCE_DWORD  = 0x7fffffff,   // force 32-bit size enum
} D3DTEXTUREMIPFILTER;

#endif /* DIRECT3D_VERSION >= 0x0600 */

/*
 * Triangle flags
 */

/*
 * Tri strip and fan flags.
 * START loads all three vertices
 * EVEN and ODD load just v3 with even or odd culling
 * START_FLAT contains a count from 0 to 29 that allows the
 * whole strip or fan to be culled in one hit.
 * e.g. for a quad len = 1
 */
#define D3DTRIFLAG_START            0x00000000L
#define D3DTRIFLAG_STARTFLAT(len) (len)     /* 0 < len < 30 */
#define D3DTRIFLAG_ODD              0x0000001eL
#define D3DTRIFLAG_EVEN             0x0000001fL

/*
 * Triangle edge flags
 * enable edges for wireframe or antialiasing
 */
#define D3DTRIFLAG_EDGEENABLE1          0x00000100L /* v0-v1 edge */
#define D3DTRIFLAG_EDGEENABLE2          0x00000200L /* v1-v2 edge */
#define D3DTRIFLAG_EDGEENABLE3          0x00000400L /* v2-v0 edge */
#define D3DTRIFLAG_EDGEENABLETRIANGLE \
        (D3DTRIFLAG_EDGEENABLE1 | D3DTRIFLAG_EDGEENABLE2 | D3DTRIFLAG_EDGEENABLE3)

/*
 * Primitive structures and related defines.  Vertex offsets are to types
 * D3DVERTEX, D3DLVERTEX, or D3DTLVERTEX.
 */

/*
 * Triangle list primitive structure
 */
typedef struct _D3DTRIANGLE {
    union {
    WORD    v1;            /* Vertex indices */
    WORD    wV1;
    };
    union {
    WORD    v2;
    WORD    wV2;
    };
    union {
    WORD    v3;
    WORD    wV3;
    };
    WORD        wFlags;       /* Edge (and other) flags */
} D3DTRIANGLE, *LPD3DTRIANGLE;

/*
 * Line list structure.
 * The instruction count defines the number of line segments.
 */
typedef struct _D3DLINE {
    union {
    WORD    v1;            /* Vertex indices */
    WORD    wV1;
    };
    union {
    WORD    v2;
    WORD    wV2;
    };
} D3DLINE, *LPD3DLINE;

/*
 * Span structure
 * Spans join a list of points with the same y value.
 * If the y value changes, a new span is started.
 */
typedef struct _D3DSPAN {
    WORD    wCount; /* Number of spans */
    WORD    wFirst; /* Index to first vertex */
} D3DSPAN, *LPD3DSPAN;

/*
 * Point structure
 */
typedef struct _D3DPOINT {
    WORD    wCount;     /* number of points     */
    WORD    wFirst;     /* index to first vertex    */
} D3DPOINT, *LPD3DPOINT;


/*
 * Forward branch structure.
 * Mask is logically anded with the driver status mask
 * if the result equals 'value', the branch is taken.
 */
typedef struct _D3DBRANCH {
    DWORD   dwMask;     /* Bitmask against D3D status */
    DWORD   dwValue;
    BOOL    bNegate;        /* TRUE to negate comparison */
    DWORD   dwOffset;   /* How far to branch forward (0 for exit)*/
} D3DBRANCH, *LPD3DBRANCH;

/*
 * Status used for set status instruction.
 * The D3D status is initialised on device creation
 * and is modified by all execute calls.
 */
typedef struct _D3DSTATUS {
    DWORD       dwFlags;    /* Do we set extents or status */
    DWORD   dwStatus;   /* D3D status */
    D3DRECT drExtent;
} D3DSTATUS, *LPD3DSTATUS;

#define D3DSETSTATUS_STATUS     0x00000001L
#define D3DSETSTATUS_EXTENTS        0x00000002L
#define D3DSETSTATUS_ALL    (D3DSETSTATUS_STATUS | D3DSETSTATUS_EXTENTS)

#if(DIRECT3D_VERSION >= 0x0500)
typedef struct _D3DCLIPSTATUS {
    DWORD dwFlags; /* Do we set 2d extents, 3D extents or status */
    DWORD dwStatus; /* Clip status */
    float minx, maxx; /* X extents */
    float miny, maxy; /* Y extents */
    float minz, maxz; /* Z extents */
} D3DCLIPSTATUS, *LPD3DCLIPSTATUS;

#define D3DCLIPSTATUS_STATUS        0x00000001L
#define D3DCLIPSTATUS_EXTENTS2      0x00000002L
#define D3DCLIPSTATUS_EXTENTS3      0x00000004L

#endif /* DIRECT3D_VERSION >= 0x0500 */
/*
 * Statistics structure
 */
typedef struct _D3DSTATS {
    DWORD        dwSize;
    DWORD        dwTrianglesDrawn;
    DWORD        dwLinesDrawn;
    DWORD        dwPointsDrawn;
    DWORD        dwSpansDrawn;
    DWORD        dwVerticesProcessed;
} D3DSTATS, *LPD3DSTATS;

/*
 * Execute options.
 * When calling using D3DEXECUTE_UNCLIPPED all the primitives
 * inside the buffer must be contained within the viewport.
 */
#define D3DEXECUTE_CLIPPED       0x00000001l
#define D3DEXECUTE_UNCLIPPED     0x00000002l

typedef struct _D3DEXECUTEDATA {
    DWORD       dwSize;
    DWORD       dwVertexOffset;
    DWORD       dwVertexCount;
    DWORD       dwInstructionOffset;
    DWORD       dwInstructionLength;
    DWORD       dwHVertexOffset;
    D3DSTATUS   dsStatus;   /* Status after execute */
} D3DEXECUTEDATA, *LPD3DEXECUTEDATA;

/*
 * Palette flags.
 * This are or'ed with the peFlags in the PALETTEENTRYs passed to DirectDraw.
 */
#define D3DPAL_FREE 0x00    /* Renderer may use this entry freely */
#define D3DPAL_READONLY 0x40    /* Renderer may not set this entry */
#define D3DPAL_RESERVED 0x80    /* Renderer may not use this entry */


#if(DIRECT3D_VERSION >= 0x0600)

typedef struct _D3DVERTEXBUFFERDESC {
    DWORD dwSize;
    DWORD dwCaps;
    DWORD dwFVF;
    DWORD dwNumVertices;
} D3DVERTEXBUFFERDESC, *LPD3DVERTEXBUFFERDESC;

#define D3DVBCAPS_SYSTEMMEMORY      0x00000800l
#define D3DVBCAPS_WRITEONLY         0x00010000l
#define D3DVBCAPS_OPTIMIZED         0x80000000l
#define D3DVBCAPS_DONOTCLIP         0x00000001l

/* Vertex Operations for ProcessVertices */
#define D3DVOP_LIGHT       (1 << 10)
#define D3DVOP_TRANSFORM   (1 << 0)
#define D3DVOP_CLIP        (1 << 2)
#define D3DVOP_EXTENTS     (1 << 3)


#if(DIRECT3D_VERSION < 0x0800)

/* The maximum number of vertices user can pass to any d3d
   drawing function or to create vertex buffer with
*/
#define D3DMAXNUMVERTICES    ((1<<16) - 1)
/* The maximum number of primitives user can pass to any d3d
   drawing function.
*/
#define D3DMAXNUMPRIMITIVES  ((1<<16) - 1)

#if(DIRECT3D_VERSION >= 0x0700)

/* Bits for dwFlags in ProcessVertices call */
#define D3DPV_DONOTCOPYDATA (1 << 0)

#endif /* DIRECT3D_VERSION >= 0x0700 */

#endif //(DIRECT3D_VERSION < 0x0800)

//-------------------------------------------------------------------

#if(DIRECT3D_VERSION < 0x0800)

// Flexible vertex format bits
//
#define D3DFVF_RESERVED0        0x001
#define D3DFVF_POSITION_MASK    0x00E
#define D3DFVF_XYZ              0x002
#define D3DFVF_XYZRHW           0x004
#if(DIRECT3D_VERSION >= 0x0700)
#define D3DFVF_XYZB1            0x006
#define D3DFVF_XYZB2            0x008
#define D3DFVF_XYZB3            0x00a
#define D3DFVF_XYZB4            0x00c
#define D3DFVF_XYZB5            0x00e

#endif /* DIRECT3D_VERSION >= 0x0700 */
#define D3DFVF_NORMAL           0x010
#define D3DFVF_RESERVED1        0x020
#define D3DFVF_DIFFUSE          0x040
#define D3DFVF_SPECULAR         0x080

#define D3DFVF_TEXCOUNT_MASK    0xf00
#define D3DFVF_TEXCOUNT_SHIFT   8
#define D3DFVF_TEX0             0x000
#define D3DFVF_TEX1             0x100
#define D3DFVF_TEX2             0x200
#define D3DFVF_TEX3             0x300
#define D3DFVF_TEX4             0x400
#define D3DFVF_TEX5             0x500
#define D3DFVF_TEX6             0x600
#define D3DFVF_TEX7             0x700
#define D3DFVF_TEX8             0x800

#define D3DFVF_RESERVED2        0xf000  // 4 reserved bits

#else
#define D3DFVF_RESERVED1        0x020
#endif //(DIRECT3D_VERSION < 0x0800)

#define D3DFVF_VERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 )
#define D3DFVF_LVERTEX ( D3DFVF_XYZ | D3DFVF_RESERVED1 | D3DFVF_DIFFUSE | \
                         D3DFVF_SPECULAR | D3DFVF_TEX1 )
#define D3DFVF_TLVERTEX ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | \
                          D3DFVF_TEX1 )


typedef struct _D3DDP_PTRSTRIDE
{
    LPVOID lpvData;
    DWORD  dwStride;
} D3DDP_PTRSTRIDE;

#define D3DDP_MAXTEXCOORD 8

typedef struct _D3DDRAWPRIMITIVESTRIDEDDATA
{
    D3DDP_PTRSTRIDE position;
    D3DDP_PTRSTRIDE normal;
    D3DDP_PTRSTRIDE diffuse;
    D3DDP_PTRSTRIDE specular;
    D3DDP_PTRSTRIDE textureCoords[D3DDP_MAXTEXCOORD];
} D3DDRAWPRIMITIVESTRIDEDDATA, *LPD3DDRAWPRIMITIVESTRIDEDDATA;
//---------------------------------------------------------------------
// ComputeSphereVisibility return values
//
#define D3DVIS_INSIDE_FRUSTUM       0
#define D3DVIS_INTERSECT_FRUSTUM    1
#define D3DVIS_OUTSIDE_FRUSTUM      2
#define D3DVIS_INSIDE_LEFT          0
#define D3DVIS_INTERSECT_LEFT       (1 << 2)
#define D3DVIS_OUTSIDE_LEFT         (2 << 2)
#define D3DVIS_INSIDE_RIGHT         0
#define D3DVIS_INTERSECT_RIGHT      (1 << 4)
#define D3DVIS_OUTSIDE_RIGHT        (2 << 4)
#define D3DVIS_INSIDE_TOP           0
#define D3DVIS_INTERSECT_TOP        (1 << 6)
#define D3DVIS_OUTSIDE_TOP          (2 << 6)
#define D3DVIS_INSIDE_BOTTOM        0
#define D3DVIS_INTERSECT_BOTTOM     (1 << 8)
#define D3DVIS_OUTSIDE_BOTTOM       (2 << 8)
#define D3DVIS_INSIDE_NEAR          0
#define D3DVIS_INTERSECT_NEAR       (1 << 10)
#define D3DVIS_OUTSIDE_NEAR         (2 << 10)
#define D3DVIS_INSIDE_FAR           0
#define D3DVIS_INTERSECT_FAR        (1 << 12)
#define D3DVIS_OUTSIDE_FAR          (2 << 12)

#define D3DVIS_MASK_FRUSTUM         (3 << 0)
#define D3DVIS_MASK_LEFT            (3 << 2)
#define D3DVIS_MASK_RIGHT           (3 << 4)
#define D3DVIS_MASK_TOP             (3 << 6)
#define D3DVIS_MASK_BOTTOM          (3 << 8)
#define D3DVIS_MASK_NEAR            (3 << 10)
#define D3DVIS_MASK_FAR             (3 << 12)

#endif /* DIRECT3D_VERSION >= 0x0600 */

#if(DIRECT3D_VERSION < 0x0800)

#if(DIRECT3D_VERSION >= 0x0700)

// To be used with GetInfo()
#define D3DDEVINFOID_TEXTUREMANAGER    1
#define D3DDEVINFOID_D3DTEXTUREMANAGER 2
#define D3DDEVINFOID_TEXTURING         3

typedef enum _D3DSTATEBLOCKTYPE
{
    D3DSBT_ALL           = 1, // capture all state
    D3DSBT_PIXELSTATE    = 2, // capture pixel state
    D3DSBT_VERTEXSTATE   = 3, // capture vertex state
    D3DSBT_FORCE_DWORD   = 0xffffffff
} D3DSTATEBLOCKTYPE;

// The D3DVERTEXBLENDFLAGS type is used with D3DRENDERSTATE_VERTEXBLEND state.
//
typedef enum _D3DVERTEXBLENDFLAGS
{
    D3DVBLEND_DISABLE  = 0, // Disable vertex blending
    D3DVBLEND_1WEIGHT  = 1, // blend between 2 matrices
    D3DVBLEND_2WEIGHTS = 2, // blend between 3 matrices
    D3DVBLEND_3WEIGHTS = 3, // blend between 4 matrices
} D3DVERTEXBLENDFLAGS;

typedef enum _D3DTEXTURETRANSFORMFLAGS {
    D3DTTFF_DISABLE         = 0,    // texture coordinates are passed directly
    D3DTTFF_COUNT1          = 1,    // rasterizer should expect 1-D texture coords
    D3DTTFF_COUNT2          = 2,    // rasterizer should expect 2-D texture coords
    D3DTTFF_COUNT3          = 3,    // rasterizer should expect 3-D texture coords
    D3DTTFF_COUNT4          = 4,    // rasterizer should expect 4-D texture coords
    D3DTTFF_PROJECTED       = 256,  // texcoords to be divided by COUNTth element
    D3DTTFF_FORCE_DWORD     = 0x7fffffff,
} D3DTEXTURETRANSFORMFLAGS;

// Macros to set texture coordinate format bits in the FVF id

#define D3DFVF_TEXTUREFORMAT2 0         // Two floating point values
#define D3DFVF_TEXTUREFORMAT1 3         // One floating point value
#define D3DFVF_TEXTUREFORMAT3 1         // Three floating point values
#define D3DFVF_TEXTUREFORMAT4 2         // Four floating point values

#define D3DFVF_TEXCOORDSIZE3(CoordIndex) (D3DFVF_TEXTUREFORMAT3 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE2(CoordIndex) (D3DFVF_TEXTUREFORMAT2)
#define D3DFVF_TEXCOORDSIZE4(CoordIndex) (D3DFVF_TEXTUREFORMAT4 << (CoordIndex*2 + 16))
#define D3DFVF_TEXCOORDSIZE1(CoordIndex) (D3DFVF_TEXTUREFORMAT1 << (CoordIndex*2 + 16))


#endif /* DIRECT3D_VERSION >= 0x0700 */

#else
//
// legacy vertex blend names
//
typedef enum _D3DVERTEXBLENDFLAGS D3DVERTEXBLENDFLAGS;
#define D3DVBLEND_DISABLE  (D3DVERTEXBLENDFLAGS)0
#define D3DVBLEND_1WEIGHT  (D3DVERTEXBLENDFLAGS)1
#define D3DVBLEND_2WEIGHTS (D3DVERTEXBLENDFLAGS)2
#define D3DVBLEND_3WEIGHTS (D3DVERTEXBLENDFLAGS)3

#endif //(DIRECT3D_VERSION < 0x0800)

#pragma pack()
#pragma warning(default:4201)

#endif /* _D3DTYPES_H_ */

