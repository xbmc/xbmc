#ifndef fft_hh
#define fft_hh

static __inline long double sqr( long double arg )
{
  return arg * arg;
}


static __inline void swap( float &a, float &b )
{
  float t = a;  a = b;  b = t;
}

// (complex) fast fourier transformation
// Based on four1() in Numerical Recipes in C, Page 507-508.

// The input in data[1..2*nn] is replaced by its fft or inverse fft, depending
// only on isign (+1 for fft, -1 for inverse fft). The number of complex numbers
// n must be a power of 2 (which is not checked).

void fft( float data[], int nn, int isign );

void twochannelrfft(float data[], int n);
void twochanwithwindow(float data[], int n);	// test


#endif
