#   ifdef __ICL /* only Intel C compiler has fmath ??? */

    #include <mathf.h>

/* Nearest integer, absolute value, etc. */

    #define ceil ceilf
    #define fabs fabsf
    #define floor floorf
    #define fmod fmodf
    #define rint rintf
    #define hypot hypotf

/* Power functions */

    #define pow powf
    #define sqrt sqrtf

/* Exponential and logarithmic functions */

    #define exp expf
    #define log logf
    #define log10 log10f

/* Trigonometric functions */

    #define acos acosf
    #define asin asinf
    #define atan atanf
    #define cos cosf
    #define sin sinf
    #define tan tanf

/* Hyperbolic functions */
    #define cosh coshf
    #define sinh sinhf
    #define tanh tanhf

#   endif
