#ifndef IMF_INTERNAL_H
#define IMF_INTERNAL_H 1

#include "IMF.h"

#ifndef HAVE_RB_ARY_NEW_CAPA
# define rb_ary_new_capa rb_ary_new2
#endif

/* Image */

imf_image_t *imf_get_image_data(VALUE obj);

/* FileFormat */

VALUE imf_file_format_detect(VALUE fmt_obj, VALUE imgsrc_obj);
VALUE imf_file_format_load(VALUE fmt_obj, VALUE image_obj, VALUE imgsrc_obj);

RUBY_EXTERN VALUE imf_cIMF_FileFormat_Base;

/* internal utilities */

static inline long
gcd_long(long x, long y)
{
  if (x < 0) x = -x;
  if (y < 0) y = -y;

  if (x == 0)
    return y;
  if (y == 0)
    return x;

  while (x > 0) {
    long t = x;
    x = y % x;
    y = t;
  }

  return y;
}

static inline long
lcm_long(long x, long y)
{
  if (x == 0 || y == 0)
    return 0;

  long z = gcd_long(x, y);
  z = x / z;
  z *= y;

  return (z < 0) ? -z : z;
}

static inline size_t
imf_calculate_row_stride(size_t const width, size_t const component_size, uint8_t const pixel_channels, uint8_t const alignment_size)
{
  size_t const pixel_size = pixel_channels * component_size;
  size_t const unit_size = lcm_long(pixel_size, alignment_size);
  size_t const row_size = pixel_size * width;
  size_t const row_stride = unit_size * ((row_size + unit_size - 1) / unit_size);
  return row_stride;
}

#endif /* IMF_INTERNAL_H */
