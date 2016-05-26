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
static inline size_t
imf_calculate_row_stride(size_t const width, size_t const component_size, size_t const pixel_channels, size_t const alignment_size)
{
  size_t const pixel_size = pixel_channels * component_size;
  size_t const row_size = pixel_size * width;
  size_t const row_stride = (row_size + alignment_size - 1) & ~(alignment_size - 1);
  return row_stride;
}

#endif /* IMF_INTERNAL_H */
