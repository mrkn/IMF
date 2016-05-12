#ifndef IMF_INTERNAL_H
#define IMF_INTERNAL_H 1

#include "IMF.h"

#undef EXTERN
#include <jpeglib.h>

#include <png.h>

#ifndef HAVE_TYPE_PNG_ALLOC_SIZE_T
typedef png_size_t png_alloc_size_t;
#endif

enum imf_color_space {
  IMF_COLOR_SPACE_GRAY = 0,
  IMF_COLOR_SPACE_RGB  = 1,
};

enum imf_image_flags {
  IMF_IMAGE_FLAG_HAS_ALPHA = (1<<0),
};

typedef struct imf_image imf_image_t;
struct imf_image {
  uint8_t flags;
  enum imf_color_space color_space;
  uint8_t component_size;
  uint8_t pixel_channels;
  size_t width;
  size_t row_stride;
  size_t height;
  uint8_t *data;
  uint8_t **channels;
};

#define IMF_IMAGE(ptr) ((imf_image_t *)(ptr))

#define IMF_IMAGE_FLAG_TEST(img, f) (IMF_IMAGE(img)->flags & (f))
#define IMF_IMAGE_FLAG_ANY(img, f) IMF_IMAGE_FLAG_TEST(img, f)
#define IMF_IMAGE_FLAG_ALL(img, f) (IMF_IMAGE_FLAG_TEST(img, f) == (f))
#define IMF_IMAGE_FLAG_SET(img, f) (void)(IMF_IMAGE(img)->flags |= (f))
#define IMF_IMAGE_FLAG_UNSET(img, f) (void)(IMF_IMAGE(img)->flags &= ~(f))
#define IMF_IMAGE_FLAG_TOGGLE(img, f) (void)(IMF_IMAGE(img)->flags ^= (f))

#define IMF_IMAGE_HAS_ALPHA(img) IMF_IMAGE_FLAG_TEST(img, IMF_IMAGE_FLAG_HAS_ALPHA)
#define IMF_IMAGE_SET_ALPHA(img) IMF_IMAGE_FLAG_SET(img, IMF_IMAGE_FLAG_HAS_ALPHA)
#define IMF_IMAGE_UNSET_ALPHA(img) IMF_IMAGE_FLAG_UNSET(img, IMF_IMAGE_FLAG_HAS_ALPHA)

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
