#ifndef IMF_INTERNAL_H
#define IMF_INTERNAL_H 1

#include "IMF.h"

/* Image */

imf_image_t *imf_get_image_data(VALUE obj);

/* FileFormat */

VALUE imf_file_format_detect(VALUE fmt_obj, VALUE imgsrc_obj);
VALUE imf_file_format_load(VALUE fmt_obj, VALUE image_obj, VALUE imgsrc_obj);

RUBY_EXTERN VALUE imf_cIMF_FileFormat_Base;

/* internal utilities */
static inline size_t
imf_calculate_row_stride(size_t const row_bytes, size_t const alignment_size)
{
  size_t const row_stride = (row_bytes + alignment_size - 1) & ~(alignment_size - 1);
  return row_stride;
}

#endif /* IMF_INTERNAL_H */
