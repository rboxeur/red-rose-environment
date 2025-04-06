#ifndef _COMPLEX_IMPL_H
#define _COMPLEX_IMPL_H

#include <complex.h>
#include "libm.h"

#undef __CMPLX
#undef CMPLX
#undef CMPLXF
#undef CMPLXL

#define __CMPLX(x, y, t) \
	((union { t __z; t __xy[2]; }){.__xy = {(x),(y)}}.__z)

#define CMPLX(x, y) __CMPLX(x, y, _Dcomplex)
#define CMPLXF(x, y) __CMPLX(x, y, _Fcomplex)
#define CMPLXL(x, y) __CMPLX(x, y, long double)

hidden _Dcomplex __ldexp_cexp(_Dcomplex,int);
hidden _Fcomplex __ldexp_cexpf(_Fcomplex,int);

#endif
