/* Single-precision vector (AdvSIMD) exp10 function.

   Copyright (C) 2023 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include "v_math.h"
#include "poly_advsimd_f32.h"

#define ScaleBound 192.0f

static const struct data
{
  float32x4_t poly[5];
  float32x4_t shift, log10_2, log2_10_hi, log2_10_lo;
#if !WANT_SIMD_EXCEPT
  float32x4_t scale_thresh;
#endif
} data = {
  /* Coefficients generated using Remez algorithm with minimisation of relative
     error.
     rel error: 0x1.89dafa3p-24
     abs error: 0x1.167d55p-23 in [-log10(2)/2, log10(2)/2]
     maxerr: 1.85943 +0.5 ulp.  */
  .poly = { V4 (0x1.26bb16p+1f), V4 (0x1.5350d2p+1f), V4 (0x1.04744ap+1f),
	    V4 (0x1.2d8176p+0f), V4 (0x1.12b41ap-1f) },
  .shift = V4 (0x1.8p23f),
  .log10_2 = V4 (0x1.a934fp+1),
  .log2_10_hi = V4 (0x1.344136p-2),
  .log2_10_lo = V4 (-0x1.ec10cp-27),
#if !WANT_SIMD_EXCEPT
  .scale_thresh = V4 (ScaleBound)
#endif
};

#define ExponentBias v_u32 (0x3f800000)

#if WANT_SIMD_EXCEPT

# define SpecialBound 38.0f	       /* rint(log10(2^127)).  */
# define TinyBound v_u32 (0x20000000) /* asuint (0x1p-63).  */
# define BigBound v_u32 (0x42180000)  /* asuint (SpecialBound).  */
# define Thres v_u32 (0x22180000)     /* BigBound - TinyBound.  */

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t x, float32x4_t y, uint32x4_t cmp)
{
  /* If fenv exceptions are to be triggered correctly, fall back to the scalar
     routine to special lanes.  */
  return v_call_f32 (exp10f, x, y, cmp);
}

#else

# define SpecialBound 126.0f /* rint (log2 (2^127 / (1 + sqrt (2)))).  */
# define SpecialOffset v_u32 (0x82000000)
# define SpecialBias v_u32 (0x7f000000)

static float32x4_t VPCS_ATTR NOINLINE
special_case (float32x4_t poly, float32x4_t n, uint32x4_t e, uint32x4_t cmp1,
	      float32x4_t scale, const struct data *d)
{
  /* 2^n may overflow, break it up into s1*s2.  */
  uint32x4_t b = vandq_u32 (vclezq_f32 (n), SpecialOffset);
  float32x4_t s1 = vreinterpretq_f32_u32 (vaddq_u32 (b, SpecialBias));
  float32x4_t s2 = vreinterpretq_f32_u32 (vsubq_u32 (e, b));
  uint32x4_t cmp2 = vcagtq_f32 (n, d->scale_thresh);
  float32x4_t r2 = vmulq_f32 (s1, s1);
  float32x4_t r1 = vmulq_f32 (vfmaq_f32 (s2, poly, s2), s1);
  /* Similar to r1 but avoids double rounding in the subnormal range.  */
  float32x4_t r0 = vfmaq_f32 (scale, poly, scale);
  float32x4_t r = vbslq_f32 (cmp1, r1, r0);
  return vbslq_f32 (cmp2, r2, r);
}

#endif

/* Fast vector implementation of single-precision exp10.
   Algorithm is accurate to 2.36 ULP.
   _ZGVnN4v_exp10f(0x1.be2b36p+1) got 0x1.7e79c4p+11
				 want 0x1.7e79cp+11.  */
float32x4_t VPCS_ATTR V_NAME_F1 (exp10) (float32x4_t x)
{
  const struct data *d = ptr_barrier (&data);
#if WANT_SIMD_EXCEPT
  /* asuint(x) - TinyBound >= BigBound - TinyBound.  */
  uint32x4_t cmp = vcgeq_u32 (
      vsubq_u32 (vandq_u32 (vreinterpretq_u32_f32 (x), v_u32 (0x7fffffff)),
		 TinyBound),
      Thres);
  float32x4_t xm = x;
  /* If any lanes are special, mask them with 1 and retain a copy of x to allow
     special case handler to fix special lanes later. This is only necessary if
     fenv exceptions are to be triggered correctly.  */
  if (__glibc_unlikely (v_any_u32 (cmp)))
    x = vbslq_f32 (cmp, v_f32 (1), x);
#endif

  /* exp10(x) = 2^n * 10^r = 2^n * (1 + poly (r)),
     with poly(r) in [1/sqrt(2), sqrt(2)] and
     x = r + n * log10 (2), with r in [-log10(2)/2, log10(2)/2].  */
  float32x4_t z = vfmaq_f32 (d->shift, x, d->log10_2);
  float32x4_t n = vsubq_f32 (z, d->shift);
  float32x4_t r = vfmsq_f32 (x, n, d->log2_10_hi);
  r = vfmsq_f32 (r, n, d->log2_10_lo);
  uint32x4_t e = vshlq_n_u32 (vreinterpretq_u32_f32 (z), 23);

  float32x4_t scale = vreinterpretq_f32_u32 (vaddq_u32 (e, ExponentBias));

#if !WANT_SIMD_EXCEPT
  uint32x4_t cmp = vcagtq_f32 (n, v_f32 (SpecialBound));
#endif

  float32x4_t r2 = vmulq_f32 (r, r);
  float32x4_t poly
      = vfmaq_f32 (vmulq_f32 (r, d->poly[0]),
		   v_pairwise_poly_3_f32 (r, r2, d->poly + 1), r2);

  if (__glibc_unlikely (v_any_u32 (cmp)))
#if WANT_SIMD_EXCEPT
    return special_case (xm, vfmaq_f32 (scale, poly, scale), cmp);
#else
    return special_case (poly, n, e, cmp, scale, d);
#endif

  return vfmaq_f32 (scale, poly, scale);
}
