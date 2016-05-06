#ifndef IMF_H
#define IMF_H 1

#include <ruby.h>

#ifndef SafeStringValueCStr
# define SafeStringValueCStr(v) (rb_check_safe_obj(rb_string_value(&v)), StringValueCStr(v))
#endif

#include <assert.h>

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# ifndef HAVE_TYPE__BOOL
typedef int _Bool;
# endif
# ifndef __cplusplus
typedef _Bool bool;
# endif
# define true 1
# define false 0
#endif

RUBY_EXTERN VALUE imf_mIMF;
RUBY_EXTERN VALUE imf_cIMF_Image;
RUBY_EXTERN VALUE imf_cIMF_ImageSource;

#endif /* IMF_H */
