/* Multiple versions of memcpy. AARCH64 version.
   Copyright (C) 2017-2023 Free Software Foundation, Inc.
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

/* Define multiple versions only for the definition in libc.  */

#if IS_IN (libc)
/* Redefine memcpy so that the compiler won't complain about the type
   mismatch with the IFUNC selector in strong_alias, below.  */
# undef memcpy
# define memcpy __redirect_memcpy
# include <string.h>
# include <init-arch.h>

extern __typeof (__redirect_memcpy) __libc_memcpy;

extern __typeof (__redirect_memcpy) __memcpy_generic attribute_hidden;
extern __typeof (__redirect_memcpy) __memcpy_thunderx attribute_hidden;
extern __typeof (__redirect_memcpy) __memcpy_thunderx2 attribute_hidden;
extern __typeof (__redirect_memcpy) __memcpy_a64fx attribute_hidden;
extern __typeof (__redirect_memcpy) __memcpy_sve attribute_hidden;
extern __typeof (__redirect_memcpy) __memcpy_mops attribute_hidden;

static inline __typeof (__redirect_memcpy) *
select_memcpy_ifunc (void)
{
  INIT_ARCH ();

  if (mops)
    return __memcpy_mops;

  if (sve && HAVE_AARCH64_SVE_ASM)
    {
      if (IS_A64FX (midr))
	return __memcpy_a64fx;
      return __memcpy_sve;
    }

  if (IS_THUNDERX (midr))
    return __memcpy_thunderx;

  if (IS_THUNDERX2 (midr) || IS_THUNDERX2PA (midr))
    return __memcpy_thunderx2;

  return __memcpy_generic;
}

libc_ifunc (__libc_memcpy, select_memcpy_ifunc ());

# undef memcpy
strong_alias (__libc_memcpy, memcpy);
#endif
