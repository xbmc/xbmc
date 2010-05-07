#include <stdio.h>
#include <math.h>
#include <asm/msr.h>
#include "resample.h"

#define CLK    300.e6
#define LOOPS  20000


typedef double ( *ddf ) ( double );


float a1 [256];
float a2 [256];

void init ( void )
{
    int  i;
    
    for ( i = 0; i < sizeof(a1)/sizeof(*a1); i++ ) {
        a1 [i] = sin(i)+0.2*sin(1.8*i)+log(2+i);
	a2 [i] = cos(0.1*i);
    }
}

void test ( int no, scalar_t f )
{
    unsigned long long  t1;
    unsigned long long  t2;
    unsigned long long  t3;
    unsigned long long  t4;
    int                 l;
    double              last = 0;
    double              curr = 0;
    
    printf ( "[%3u] %22.14f\t\t", no, (double)f (a1,a2) );
    fflush ( stdout );

    do {
    rdtscll (t1);
    l = LOOPS;
    do
        ;
    while (--l);
    rdtscll (t2);
    rdtscll (t3);
    l = LOOPS;
    do
        f(a1,a2), f(a1,a2), f(a1,a2), f(a1,a2);
    while (--l);
    rdtscll (t4);
    last = curr;
    curr = (t4-t3-t2+t1) / CLK / LOOPS / 4 * 1.e9;
    } while ( fabs(curr-last) > 1.e-4 * (curr+last) );
    printf ("%8.2f ns\n", (curr+last) / 2 );
}

void testn ( scalarn_t f )
{
    unsigned long long  t1;
    unsigned long long  t2;
    unsigned long long  t3;
    unsigned long long  t4;
    int                 l;
    int                 i;
    double              last = 0;
    double              curr = 0;
    
    for ( i = 1; i <= 64; i += i<6 ? 1 : i<8 ? 2 : i ) {
        printf ( "[%3u] %22.14f\t\t", 4*i, (double)f (a1,a2,i) );
        fflush ( stdout );

    do {
        rdtscll (t1);
        l = LOOPS;
        do
            ;
        while (--l);
        rdtscll (t2);
        rdtscll (t3);
        l = LOOPS;
        do
            f(a1,a2,i), f(a1,a2,i), f(a1,a2,i), f(a1,a2,i);
        while (--l);
        rdtscll (t4);
    last = curr;
    curr = (t4-t3-t2+t1) / CLK / LOOPS / 4 * 1.e9;
    } while ( fabs(curr-last) > 1.e-4 * (curr+last) );
    printf ("%8.2f ns\n", (curr+last) / 2 );
    }
}

void test2 ( const char* name, ddf f )
{
    int     i;
    double  x;
    
    printf ( "\n%%%% %s\n\n", name );
    
    for ( i = -1000; i <= 1000; i++ ) {
        x = 1.e-3 * i;
	printf ( "%5d\t%12.8f\t%12.8f\t%12.8f\n", i, f(x), (f(x+5.e-5) - f(x-5.e-5))*1.e+4, (f(x+1.e-4) + f(x-1.e-4) - 2*f(x))*5.e+7 );
    }
    printf ( "%%%%\n" );
    fflush ( stdout );
}


int main ( int argc, char** argv )
{

#if 0

    test2 ( "Hann", hanning   );
    test2 ( "Hamm", hamming   );
    test2 ( "BM", blackman  );
    test2 ( "BM1",blackman1 );
    test2 ( "BM2",blackman2 );
    test2 ( "BMH N",blackmanharris_nuttall );
    test2 ( "MNH Min",blackmanharris_min4    );

#else

    init ();

    test ( 4, scalar04_float32       );
    test ( 4, scalar04_float32_i387  );
    test ( 4, scalar04_float32_3DNow );
    test ( 4, scalar04_float32_SIMD  );

    test ( 8, scalar08_float32       );
    test ( 8, scalar08_float32_i387  );
    test ( 8, scalar08_float32_3DNow );
    test ( 8, scalar08_float32_SIMD  );

    test ( 12, scalar12_float32       );
    test ( 12, scalar12_float32_i387  );
    test ( 12, scalar12_float32_3DNow );
    test ( 12, scalar12_float32_SIMD  );

    test ( 16, scalar16_float32       );
    test ( 16, scalar16_float32_i387  );
    test ( 16, scalar16_float32_3DNow );
    test ( 16, scalar16_float32_SIMD  );

    test ( 20, scalar20_float32       );
    test ( 20, scalar20_float32_i387  );
    test ( 20, scalar20_float32_3DNow );
    test ( 20, scalar20_float32_SIMD  );

    test ( 24, scalar24_float32       );
    test ( 24, scalar24_float32_i387  );
    test ( 24, scalar24_float32_3DNow );
    test ( 24, scalar24_float32_SIMD  );

    testn( scalar4n_float32       );
    testn( scalar4n_float32_i387  );
    testn( scalar4n_float32_3DNow );
    testn( scalar4n_float32_SIMD  );

#endif    
    
    return 0;
}

/* end of scalartest.c */
