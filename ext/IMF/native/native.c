#include "IMF.h"

#include <ruby/encoding.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef S_ISREG
# define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

#include "internal.h"

static inline int
imf_stat(char const *path, struct stat *st)
{
#ifdef _WIN32
  return rb_w32_ustati64(path, st);
#else
  return stat(path, st);
#endif
}

static inline bool
imf_file_p(char const *path)
{
  struct stat st;
  if (imf_stat(path, &st) < 0)
    return false;
  if (S_ISREG(st.st_mode))
    return true;
  return false;
}

VALUE imf_mIMF;
VALUE imf_cIMF_Image;
VALUE imf_cIMF_ImageSource;

static ID id_detect;
static ID id_path;
static ID id_read;
static ID id_rewind;

static void
imf_image_mark(void *ptr)
{
}

static void
imf_image_free(void *ptr)
{
  xfree(IMF_IMAGE(ptr)->data);
  xfree(IMF_IMAGE(ptr)->channels);
  xfree(ptr);
}

static inline size_t
imf_image_channel_size(imf_image_t const *const img)
{
  return img->row_stride * img->height;
}

static inline size_t
imf_image_data_size(imf_image_t const *const img)
{
  size_t const channel_size = imf_image_channel_size(img);
  size_t const data_size = img->pixel_channels * channel_size;
  return data_size;
}

static size_t
imf_image_memsize(void const *ptr)
{
  size_t const data_size = imf_image_data_size(IMF_IMAGE(ptr));
  return data_size + sizeof(imf_image_t);
}

static rb_data_type_t imf_image_data_type = {
  "imf_image",
  {
    imf_image_mark,
    imf_image_free,
    imf_image_memsize,
  },
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  0, 0,
  RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

bool
imf_is_image(VALUE obj)
{
  return rb_typeddata_is_kind_of(obj, &imf_image_data_type);
}

imf_image_t *
imf_get_image_data(VALUE obj)
{
  imf_image_t *ptr;
  TypedData_Get_Struct(obj, imf_image_t, &imf_image_data_type, ptr);
  return ptr;
}

static VALUE
imf_image_alloc(VALUE klass)
{
  imf_image_t *img;
  VALUE obj = TypedData_Make_Struct(klass, imf_image_t, &imf_image_data_type, img);
  return obj;
}

void
imf_image_allocate_image_buffer(imf_image_t *img)
{
  assert(img->width > 0);
  assert(img->height > 0);
  assert(img->pixel_channels > 0);
  assert(img->component_size > 0);

  img->row_stride = imf_calculate_row_stride(img->width * img->component_size, 16);
  img->data = ALLOC_N(uint8_t, imf_image_data_size(img));
  img->channels = ALLOC_N(uint8_t *, img->pixel_channels);

  size_t const channel_size = imf_image_channel_size(img);
  size_t const channel_size_x2 = 2 * channel_size;
  size_t const channel_size_x3 = 3 * channel_size;

  uint8_t *data = img->data;
  uint8_t **channels = img->channels;

  size_t i;
  for (i = 0; i + 2 < img->pixel_channels; i += 3) {
    channels[0] = data;
    channels[1] = data + channel_size;
    channels[2] = data + channel_size_x2;
    data += channel_size_x3;
    channels += 3;
  }
  for (; i < img->pixel_channels; ++i) {
    channels[0] = data;
    data += channel_size;
    ++channels;
  }
}

static VALUE
imf_jpeg_guess(VALUE image_source)
{
  VALUE c, fmt_obj;
  rb_require("IMF/file_format/jpeg");
  c = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
  c = rb_const_get(c, rb_intern_const("JPEG"));
  fmt_obj = rb_class_new_instance(0, NULL, c);
  if (RTEST(imf_file_format_detect(fmt_obj, image_source)))
    return fmt_obj;
  return Qnil;
}

static VALUE
imf_png_guess(VALUE image_source)
{
  VALUE c, fmt_obj;
  rb_require("IMF/file_format/png");
  c = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
  c = rb_const_get(c, rb_intern_const("PNG"));
  fmt_obj = rb_class_new_instance(0, NULL, c);
  if (RTEST(imf_file_format_detect(fmt_obj, image_source)))
    return fmt_obj;
  return Qnil;
}

VALUE
imf_find_file_format_by_filename(VALUE path_value)
{
  char const *path, *e;
  long path_len;

  Check_Type(path_value, T_STRING);

  path = StringValueCStr(path_value);
  path_len = RSTRING_LEN(path_value);
  e = ruby_enc_find_extname(path, &path_len, rb_enc_get(path_value));
  if (path_len <= 1)
    return Qnil;
  if (memcmp(e, ".jpg", path_len) == 0) {
    VALUE c, fmt_obj;
    rb_require("IMF/file_format/jpeg");
    c = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
    c = rb_const_get(c, rb_intern_const("JPEG"));
    fmt_obj = rb_class_new_instance(0, NULL, c);
    return fmt_obj;
  }

  if (memcmp(e, ".png", path_len) == 0) {
    VALUE c, fmt_obj;
    rb_require("IMF/file_format/png");
    c = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
    c = rb_const_get(c, rb_intern_const("PNG"));
    fmt_obj = rb_class_new_instance(0, NULL, c);
    return fmt_obj;
  }

  return Qnil;
}

VALUE
imf_detect_file_format(VALUE imgsrc_obj)
{
  VALUE fmt_obj;

  fmt_obj = imf_jpeg_guess(imgsrc_obj);
  if (!NIL_P(fmt_obj))
    return fmt_obj;

  fmt_obj = imf_png_guess(imgsrc_obj);
  if (!NIL_P(fmt_obj))
    return fmt_obj;

  return Qnil;
}

static VALUE
imf_image_s_load_image(int argc, VALUE *argv, VALUE klass)
{
  VALUE image_obj, imgsrc_obj, path_value, fmt_obj;
  imf_image_t *img;

  /* TODO: support optional loading parameters */
  rb_scan_args(argc, argv, "10", &imgsrc_obj);

  image_obj = imf_image_alloc(klass);
  img = imf_get_image_data(image_obj);

  /* TODO: Need to generalize */
  path_value = rb_funcall(imgsrc_obj, id_path, 0);
  if (!NIL_P(path_value)) {
    FilePathStringValue(path_value);
    fmt_obj = imf_find_file_format_by_filename(path_value);
    if (NIL_P(fmt_obj))
      goto guess_format;
    if (rb_typeddata_is_kind_of(fmt_obj, &imf_file_format_data_type)) {
      if (imf_file_format_detect(fmt_obj, imgsrc_obj)) {
      detected_file_format:
        imf_file_format_load(fmt_obj, image_obj, imgsrc_obj);
        return image_obj;
      }
    }
  }

guess_format:
  fmt_obj = imf_detect_file_format(imgsrc_obj);
  if (rb_typeddata_is_kind_of(fmt_obj, &imf_file_format_data_type))
    goto detected_file_format;

unknown:
  rb_raise(rb_eRuntimeError, "Unknown image format");
}

static VALUE
imf_image_get_color_space(VALUE obj)
{
  imf_image_t *img = imf_get_image_data(obj);

  switch (img->color_space) {
    case IMF_COLOR_SPACE_GRAY:
      return ID2SYM(rb_intern("GRAY"));
    case IMF_COLOR_SPACE_RGB:
      return ID2SYM(rb_intern("RGB"));
    default:
      return Qnil;
  }
}

static VALUE
imf_image_has_alpha(VALUE obj)
{
  imf_image_t *img = imf_get_image_data(obj);
  return IMF_IMAGE_HAS_ALPHA(img) ? Qtrue : Qfalse;
}

static VALUE
imf_image_get_component_size(VALUE obj)
{
  imf_image_t *img = imf_get_image_data(obj);
  return INT2FIX(img->component_size);
}

static VALUE
imf_image_get_pixel_channels(VALUE obj)
{
  imf_image_t *img = imf_get_image_data(obj);
  return INT2FIX(img->pixel_channels);
}

static VALUE
imf_image_get_width(VALUE obj)
{
  imf_image_t *img = imf_get_image_data(obj);
  return UINT2NUM(img->width);
}

static VALUE
imf_image_get_height(VALUE obj)
{
  imf_image_t *img = imf_get_image_data(obj);
  return UINT2NUM(img->height);
}

static VALUE
imf_image_get_row_stride(VALUE obj)
{
  imf_image_t *img = imf_get_image_data(obj);
  return UINT2NUM(img->row_stride);
}

void
Init_imf_image(void)
{
  imf_cIMF_Image = rb_define_class_under(imf_mIMF, "Image", rb_cObject);
  rb_define_alloc_func(imf_cIMF_Image, imf_image_alloc);

  rb_define_singleton_method(imf_cIMF_Image, "load_image", imf_image_s_load_image, -1);
  rb_define_method(imf_cIMF_Image, "color_space", imf_image_get_color_space, 0);
  rb_define_method(imf_cIMF_Image, "has_alpha?", imf_image_has_alpha, 0);
  rb_define_method(imf_cIMF_Image, "component_size", imf_image_get_component_size, 0);
  rb_define_method(imf_cIMF_Image, "pixel_channels", imf_image_get_pixel_channels, 0);
  rb_define_method(imf_cIMF_Image, "width", imf_image_get_width, 0);
  rb_define_method(imf_cIMF_Image, "height", imf_image_get_height, 0);
  rb_define_method(imf_cIMF_Image, "row_stride", imf_image_get_row_stride, 0);

  rb_require("IMF/file_format");
}

void Init_imf_file_format(void);

void
Init_native(void)
{
  imf_mIMF = rb_define_module("IMF");

  Init_imf_image();
  Init_imf_file_format();

  imf_cIMF_ImageSource = rb_define_class_under(imf_mIMF, "ImageSource", rb_cObject);

  id_detect = rb_intern("detect");
  id_path = rb_intern("path");
  id_read = rb_intern("read");
  id_rewind = rb_intern("rewind");
}
