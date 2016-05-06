#ifndef IMF_INTERNAL_H
#define IMF_INTERNAL_H 1

#include "IMF.h"

#undef EXTERN
#include <jpeglib.h>

typedef struct imf_image imf_image_t;
struct imf_image {
  uint8_t component_size;
  uint8_t pixel_channels;
  size_t width;
  size_t row_stride;
  size_t height;
  uint8_t *data;
  uint8_t **channels;
};

#define IMF_IMAGE(ptr) ((imf_image_t *)(ptr))

bool is_imf_image(VALUE obj);
imf_image_t *get_imf_image(VALUE obj);

/* JpegSourceManager */

enum imf_jpeg_src_mgr_constants {
  IMF_JPEG_BUFFER_SIZE = 8192,
};

typedef struct imf_jpeg_src_mgr imf_jpeg_src_mgr_t;
struct imf_jpeg_src_mgr {
  struct jpeg_source_mgr pub;

  VALUE image_source;
  VALUE buffer;
  bool start_of_source;
};

#define IMF_JPEG_SRC_MGR(ptr) ((imf_jpeg_src_mgr_t *)(ptr))

imf_jpeg_src_mgr_t *get_imf_jpeg_src_mgr(VALUE obj);

VALUE imf_jpeg_init_image_source(j_decompress_ptr cinfo, VALUE image_source);

/* internal utilities */
static inline size_t
imf_calculate_row_stride(size_t const row_bytes, size_t const alignment_size)
{
  size_t const row_stride = (row_bytes + alignment_size - 1) & ~(alignment_size - 1);
  return row_stride;
}

#endif /* IMF_INTERNAL_H */
