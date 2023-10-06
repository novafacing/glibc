/* Helper functions to handle tunable strings.
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

#ifndef _DL_TUNABLES_PARSE_H
#define _DL_TUNABLES_PARSE_H 1

#ifndef DEFAULT_MEMCMP
# define DEFAULT_MEMCMP memcmp
#endif

/* Compare the contents of STRVAL with STR of size LEN.  The STR might not
   be null-terminated.   */
static inline bool
tunable_strcmp (const struct tunable_str_t *strval, const char *str,
		size_t len)
{
  return strval->len == len && DEFAULT_MEMCMP (strval->str, str, len) == 0;
}
#define tunable_strcmp_cte(__tunable, __str)				\
 ({									\
    __builtin_constant_p (__str)					\
    ? tunable_strcmp (&__tunable->strval, __str, sizeof (__str) - 1)	\
    : tunable_strcmp (&__tunable->strval, __str, strlen (__str));	\
 })

/* Helper function to iterate over string tunable composed by multiple
   suboptions separated by comma.  The tunable is represented as span (address
   and size) from GLIBC_TUNABLES, so it might not be null terminated.

   For instance, to print all the items:

     struct tunable_str_comma_t st;
     tunable_str_comma_init (&st, valp);

     struct tunable_str_t tstr;
     while (tunable_str_comma_next (&st, &tstr))
      {
        if (tstr.len == 0)
	  continue;

	_dl_printf ("[%s] %.*s (%d)\n", __func__,
		    (int) tstr.len,
		    tstr.str,
		    (int) tstr.len);
      }  */

struct tunable_str_comma_t
{
  const char *p;
  size_t plen;
  size_t maxplen;
};

static inline void
tunable_str_comma_init (struct tunable_str_comma_t *state, tunable_val_t *valp)
{
  state->p = valp->strval.str;
  state->plen = 0;
  state->maxplen = valp->strval.len;
}

static inline bool
tunable_str_comma_next (struct tunable_str_comma_t *state,
			struct tunable_str_t *str)
{
  if (*state->p == '\0' || state->plen >= state->maxplen)
    return false;

  const char *c;
  for (c = state->p; *c != ','; c++, state->plen++)
    if (*c == '\0' || state->plen == state->maxplen)
      break;

  str->str = state->p;
  str->len = c - state->p;

  state->p = c + 1;
  state->plen++;

  return true;
}

#endif
