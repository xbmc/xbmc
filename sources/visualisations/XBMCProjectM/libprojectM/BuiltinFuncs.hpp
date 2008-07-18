//
// C++ Interface: BuiltinFuncs
//
// Description: 
//
//
// Author: Carmelo Piccione <carmelo.piccione@gmail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef _BUILTIN_FUNCS_HPP
#define _BUILTIN_FUNCS_HPP

#include "Common.hpp"
#include "Func.hpp"
#include <cmath>
#include <cstdlib>
#include <cassert>

#include "RandomNumberGenerators.hpp"

/* Wrappers for all the builtin functions
   The arg_list pointer is a list of floats. Its
   size is equal to the number of arguments the parameter
   takes */
class FuncWrappers {

/* Values to optimize the sigmoid function */
static const int R =  32767;
static const int RR = 65534;

public:

static inline float int_wrapper(float * arg_list) {

return floor(arg_list[0]);

}


static inline float sqr_wrapper(float * arg_list) {

return pow(2, (int)arg_list[0]);
}


static inline float sign_wrapper(float * arg_list) {

return -arg_list[0];
}

static inline float min_wrapper(float * arg_list) {

if (arg_list[0] > arg_list[1])
return arg_list[1];

return arg_list[0];
}

static inline float max_wrapper(float * arg_list) {

if (arg_list[0] > arg_list[1])
return arg_list[0];

return arg_list[1];
}

/* consult your AI book */
static inline float sigmoid_wrapper(float * arg_list) {
return (RR / (1 + exp( -(((float)(arg_list[0])) * arg_list[1]) / R) - R));
}


static inline float bor_wrapper(float * arg_list) {

return (float)((int)arg_list[0] || (int)arg_list[1]);
}

static inline float band_wrapper(float * arg_list) {
return (float)((int)arg_list[0] && (int)arg_list[1]);
}

static inline float bnot_wrapper(float * arg_list) {
return (float)(!(int)arg_list[0]);
}

static inline float if_wrapper(float * arg_list) {

if ((int)arg_list[0] == 0)
return arg_list[2];
return arg_list[1];
}


static inline float rand_wrapper(float * arg_list) {
float l=1;

//  printf("RAND ARG:(%d)\n", (int)arg_list[0]);
if ((int)arg_list[0] > 0)
	l  = (float) RandomNumberGenerators::uniformInteger((int)arg_list[0]);

return l;
}

static inline float equal_wrapper(float * arg_list) {
	return (arg_list[0] == arg_list[1]);
}


static inline float above_wrapper(float * arg_list) {

return (arg_list[0] > arg_list[1]);
}


static inline float below_wrapper(float * arg_list) {

return (arg_list[0] < arg_list[1]);
}

static float sin_wrapper(float * arg_list) {

  assert(arg_list);
//return .5;
float d = sinf(*arg_list);
return d;
//return (sin (arg_list[0]));
}


static inline float cos_wrapper(float * arg_list) {
return (cos (arg_list[0]));
}

static inline float tan_wrapper(float * arg_list) {
return (tan(arg_list[0]));
}

static inline float asin_wrapper(float * arg_list) {
return (asin (arg_list[0]));
}

static inline float acos_wrapper(float * arg_list) {
return (acos (arg_list[0]));
}

static inline float atan_wrapper(float * arg_list) {
return (atan (arg_list[0]));
}

static inline float atan2_wrapper(float * arg_list) {
return (atan2 (arg_list[0], arg_list[1]));
}

static inline float pow_wrapper(float * arg_list) {
return (pow (arg_list[0], arg_list[1]));
}

static inline float exp_wrapper(float * arg_list) {
return (exp(arg_list[0]));
}

static inline float abs_wrapper(float * arg_list) {
return (fabs(arg_list[0]));
}

static inline float log_wrapper(float* arg_list) {
return (log (arg_list[0]));
}

static inline float log10_wrapper(float * arg_list) {
return (log10 (arg_list[0]));
}

static inline float sqrt_wrapper(float * arg_list) {
return (sqrt (arg_list[0]));
}


static inline float nchoosek_wrapper(float * arg_list) {
unsigned long cnm = 1UL;
int i, f;
int n, m;

n = (int)arg_list[0];
m = (int)arg_list[1];

if (m*2 >n) m = n-m;
for (i=1 ; i <= m; n--, i++)
{
if ((f=n) % i == 0)
f   /= i;
else  cnm /= i;
cnm *= f;
}
return (float)cnm;
}


static inline float fact_wrapper(float * arg_list) {


int result = 1;

int n = (int)arg_list[0];

while (n > 1) {
result = result * n;
n--;
}
return (float)result;
}
};

#include <map>
class BuiltinFuncs {

public:
    
    static int init_builtin_func_db();
    static int destroy_builtin_func_db();
    static int load_all_builtin_func();
    static int load_builtin_func( const std::string & name, float (*func_ptr)(float*), int num_args );

    static int insert_func( Func *func );
    static int remove_func( Func *func );
    static Func *find_func( const std::string & name );
private:
     static std::map<std::string, Func*> builtin_func_tree;
};

#endif
