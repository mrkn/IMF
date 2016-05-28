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

#include <stdio.h>

#include "internal.h"

#include "num_buffer.h"

#include <png.h>
#include <zlib.h>

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
  xfree(ptr);
}

static inline size_t
imf_image_data_size(imf_image_t const *const img)
{
  size_t const data_size = img->row_stride * img->height;
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

  img->row_stride = imf_calculate_row_stride(img->width, img->component_size, img->pixel_channels, 16);
  img->data = ALLOC_N(uint8_t, imf_image_data_size(img));
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

  path_value = rb_funcall(imgsrc_obj, id_path, 0);
  if (!NIL_P(path_value)) {
    FilePathStringValue(path_value);
    fmt_obj = imf_find_file_format_by_filename(path_value);
    if (NIL_P(fmt_obj))
      goto guess_format;
    if (imf_is_file_format(fmt_obj)) {
      if (imf_file_format_detect(fmt_obj, imgsrc_obj)) {
      detected_file_format:
        imf_file_format_load(fmt_obj, image_obj, imgsrc_obj);
        return image_obj;
      }
    }
  }

guess_format:
  fmt_obj = imf_detect_file_format(imgsrc_obj);
  if (imf_is_file_format(fmt_obj))
    goto detected_file_format;

unknown:
  rb_raise(rb_eRuntimeError, "Unknown image format");
}

static VALUE
imf_image_inspect(VALUE obj)
{
  char buf[64];
  imf_image_t *img = imf_get_image_data(obj);

  VALUE str;
  VALUE cname = rb_class_name(CLASS_OF(obj));
  str = rb_sprintf(
    "#<%"PRIsVALUE":%p size=%"PRIuSIZE"x%"PRIuSIZE" ch=%u bits=%d row_stride=%"PRIuSIZE">",
    cname, (void*)obj,
    img->width, img->height,
    img->pixel_channels,
    img->component_size * 8,
    img->row_stride
  );
  rb_obj_infect(str, obj);

  return str;
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

static VALUE
imf_image_get_pixel(VALUE obj, VALUE row_index_v, VALUE col_index_v)
{
  imf_image_t *img = imf_get_image_data(obj);
  ssize_t i, j, row_index, col_index;
  VALUE pixel;

  row_index = NUM2SSIZET(row_index_v);
  col_index = NUM2SSIZET(col_index_v);

  if (row_index < 0) {
    row_index += img->height;
    if (row_index < 0)
      return Qnil;
  }
  else if (row_index >= img->height)
    return Qnil;

  if (col_index < 0) {
    col_index += img->width;
    if (col_index < 0)
      return Qnil;
  }
  else if (col_index >= img->width)
    return Qnil;

  pixel = rb_ary_new_capa(img->pixel_channels);

  size_t const pixel_size = img->pixel_channels * img->component_size;
  uint8_t const *const row_base_ptr = img->data + row_index * img->row_stride;
  uint8_t const *pixel_ptr = row_base_ptr + col_index * pixel_size;

  switch (img->component_size) {
    case 1:
      for (i = 0; i < img->pixel_channels; ++i, ++pixel_ptr) {
        rb_ary_push(pixel, UINT2NUM(*pixel_ptr));
      }
      break;

    case 2:
      for (i = 0; i < img->pixel_channels; ++i, pixel_ptr += 2) {
        rb_ary_push(pixel, ULONG2NUM(*(uint16_t *)pixel_ptr));
      }
      break;
  }

  return pixel;
}

VALUE
imf_image_to_num_buffer(VALUE image_obj)
{
  imf_image_t *img = imf_get_image_data(image_obj);

  size_t shapes[3];
  size_t strides[2];

  shapes[0] = img->height;
  shapes[1] = img->width;
  shapes[2] = img->pixel_channels;

  strides[0] = img->row_stride / img->pixel_channels;
  strides[1] = img->pixel_channels;

  size_t data_size = img->height * img->row_stride;
  VALUE obj = nb_num_buffer_new_with_data(
      image_obj,
      NB_VALUE_UINT8, /* FIXME: should look nb->component_size */
      3,
      shapes,
      strides,
      img->data,
      data_size
  );

  return obj;
}

static VALUE
imf_image_s_from_num_buffer(VALUE klass, VALUE obj)
{
  VALUE image_obj;
  imf_image_t *img;
  nb_num_buffer_t *nb;

  nb = nb_get_num_buffer(obj);

  image_obj = imf_image_alloc(klass);
  img = imf_get_image_data(image_obj);

  nb->origin = image_obj;

  img->color_space = IMF_COLOR_SPACE_RGB;
  img->component_size = 1;
  img->height = nb->shape[0];
  img->width = nb->shape[1];
  img->pixel_channels = nb->shape[2];
  img->row_stride = nb->strides[0] * nb->strides[1];
  img->data = nb->data.as.u8_ptr;

  return image_obj;
}

static void
imf_png_error(png_structp png_ptr, png_const_charp msg)
{
  VALUE *error_message = (VALUE *) png_get_error_ptr(png_ptr);
  *error_message = rb_sprintf("PNG ERROR: %s", msg);
  png_default_error(png_ptr, msg);
}

static void
imf_png_warning(png_structp png_ptr, png_const_charp msg)
{
  rb_warn("PNG WARNING: %s", msg);
}

static VALUE
imf_image_save(VALUE image_obj, VALUE fname)
{
  VALUE error_message = Qnil;
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  int code = 0;
  FILE *fp;

  imf_image_t *img;
  img = imf_get_image_data(image_obj);

  FilePathStringValue(fname);
  fp = fopen(StringValueCStr(fname), "wb");
  if (fp == NULL) {
    rb_raise(rb_eRuntimeError, "unable to open file");
  }

  /* Initialize write structure */
  png_ptr = png_create_write_struct(
    PNG_LIBPNG_VER_STRING, &error_message, imf_png_error, imf_png_warning);
  if (png_ptr == NULL) {
    fclose(fp);
    rb_raise(rb_eRuntimeError, "unable to allocate PNG write struct");
  }

  /* Initialize info structure */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fclose(fp);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    rb_raise(rb_eRuntimeError, "unable to allocate PNG info struct");
  }

  /* Setup Exception handling */
  if (setjmp(png_jmpbuf(png_ptr))) {
    fclose(fp);
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    if (error_message != Qnil)
      rb_raise(rb_eRuntimeError, "%s", RSTRING_PTR(error_message));
    else
      rb_raise(rb_eRuntimeError, "Error during PNG creation");
  }

  png_init_io(png_ptr, fp);

  /* Write header (8 bit colour depth) */
  png_set_IHDR(
    png_ptr, info_ptr, img->width, img->height,
    8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE
  );
  png_write_info(png_ptr, info_ptr);

  /* Allocate memory for one row (3 bytes per pixel - RGB) */
  size_t const rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  VALUE buffer = rb_str_tmp_new(rowbytes);
  png_bytep row = (png_bytep) RSTRING_PTR(buffer);

  /* Write image data */
  size_t x, y;
  for (y = 0; y < img->height; ++y) {
    memcpy(row, img->data + y*img->row_stride, rowbytes);
    png_write_row(png_ptr, row);
  }

  /* End write */
  png_write_end(png_ptr, NULL);

  fclose(fp);
  png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  return image_obj;
}

void
Init_imf_image(void)
{
  imf_cIMF_Image = rb_define_class_under(imf_mIMF, "Image", rb_cObject);
  rb_define_alloc_func(imf_cIMF_Image, imf_image_alloc);

  rb_define_singleton_method(imf_cIMF_Image, "load_image", imf_image_s_load_image, -1);
  rb_define_method(imf_cIMF_Image, "inspect", imf_image_inspect, 0);
  rb_define_method(imf_cIMF_Image, "color_space", imf_image_get_color_space, 0);
  rb_define_method(imf_cIMF_Image, "has_alpha?", imf_image_has_alpha, 0);
  rb_define_method(imf_cIMF_Image, "component_size", imf_image_get_component_size, 0);
  rb_define_method(imf_cIMF_Image, "pixel_channels", imf_image_get_pixel_channels, 0);
  rb_define_method(imf_cIMF_Image, "width", imf_image_get_width, 0);
  rb_define_method(imf_cIMF_Image, "height", imf_image_get_height, 0);
  rb_define_method(imf_cIMF_Image, "row_stride", imf_image_get_row_stride, 0);
  rb_define_method(imf_cIMF_Image, "[]", imf_image_get_pixel, 2);

  rb_define_method(imf_cIMF_Image, "to_num_buffer", imf_image_to_num_buffer, 0);
  rb_define_singleton_method(imf_cIMF_Image, "from_num_buffer", imf_image_s_from_num_buffer, 1);
  rb_define_method(imf_cIMF_Image, "save", imf_image_save, 1);
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
