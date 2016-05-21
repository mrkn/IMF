#ifndef IMF_H
#define IMF_H 1

#include <ruby.h>

#ifndef SafeStringValueCStr
# define SafeStringValueCStr(v) (rb_check_safe_obj(rb_string_value(&v)), StringValueCStr(v))
#endif

#ifndef RARRAY_AREF
# define RARRAY_AREF(ary, n) (RARRAY_PTR(ary)[n])
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

/* IMF */

VALUE imf_find_file_format_by_filename(VALUE path_value);
VALUE imf_detect_file_format(VALUE imgsrc_obj);

/* Image */

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

bool imf_is_image(VALUE obj);
void imf_image_allocate_image_buffer(imf_image_t *img);

/* FileFormat */

typedef struct imf_file_format imf_file_format_t;
struct imf_file_format {
};

typedef int imf_file_format_detect_func(imf_file_format_t *fmt, VALUE detect);
typedef void imf_file_format_load_func(imf_file_format_t *fmt, imf_image_t *img, VALUE src);

typedef struct imf_file_format_interface imf_file_format_interface_t;
struct imf_file_format_interface {
  imf_file_format_detect_func *detect;
  imf_file_format_load_func *load;
};

#define imf_file_format_interface(obj) ( \
  (imf_file_format_interface_t *) RTYPEDDATA_TYPE(obj)->data )

void imf_file_format_mark(void *ptr);
void imf_file_format_free(void *ptr);
size_t imf_file_format_memsize(void const *ptr);
RUBY_EXTERN rb_data_type_t const imf_file_format_data_type;

int imf_is_file_format_class(VALUE klass);

static inline int
imf_is_file_format(VALUE obj)
{
  return rb_typeddata_is_kind_of(obj, &imf_file_format_data_type);
}

void imf_register_file_format(VALUE file_format, char const *const *extnames);

/* Classes and Modules */

RUBY_EXTERN VALUE imf_mIMF;
RUBY_EXTERN VALUE imf_cIMF_Image;
RUBY_EXTERN VALUE imf_cIMF_ImageSource;

#endif /* IMF_H */
