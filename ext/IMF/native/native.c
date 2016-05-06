#include "IMF.h"

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

static ID id_read;

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
is_imf_image(VALUE obj)
{
  return rb_typeddata_is_kind_of(obj, &imf_image_data_type);
}

imf_image_t *
get_imf_image(VALUE obj)
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

static void
imf_jpeg_error_exit(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX] = { '\0', };

  (*cinfo->err->format_message)(cinfo, buffer);

  rb_raise(rb_eRuntimeError, "JPEG ERROR: %s", buffer);
}

static void
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

static void
imf_load_jpeg(VALUE obj, VALUE image_source)
{
  imf_image_t *img;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  volatile VALUE srcmgr;
  volatile VALUE buffer;
  JOCTET *buffer_ptr;

  assert(is_imf_image(obj));
  assert(!NIL_P(image_source));
  assert(rb_obj_is_kind_of(image_source, imf_cIMF_ImageSource));

  img = get_imf_image(obj);

  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = imf_jpeg_error_exit;

  jpeg_create_decompress(&cinfo);

  /* setup source manager */
  srcmgr = imf_jpeg_init_image_source(&cinfo, image_source);

  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  /* allocate image buffer */
  img->component_size = sizeof(JSAMPLE);
  img->pixel_channels = cinfo.output_components;
  img->width = cinfo.output_width;
  img->height = cinfo.output_height;

  imf_image_allocate_image_buffer(img);

  /* allocate temporary scanline buffer */
  buffer = rb_str_tmp_new(sizeof(JSAMPLE) * cinfo.output_width * cinfo.output_components);
  buffer_ptr = (JOCTET *) RSTRING_PTR(buffer);

  size_t y = 0;
  while (cinfo.output_scanline < cinfo.output_height) {
    size_t x, ch;
    jpeg_read_scanlines(&cinfo, &buffer_ptr, 1);

    size_t i = y * img->row_stride;
    for (x = 0; x < img->width; ++x) {
      size_t j = x * img->pixel_channels;
      for (ch = 0; ch < img->pixel_channels; ++ch) {
        ((JSAMPLE *)img->channels[ch])[i + x] = buffer_ptr[j + ch];
      }
    }

    ++y;
  }

  /* release temporary buffer memory */
  buffer_ptr = NULL;
  rb_str_resize(buffer, 0L);

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
}

static VALUE
imf_image_s_load_image(int argc, VALUE *argv, VALUE klass)
{
  VALUE obj, image_source;
  imf_image_t *img;

  rb_scan_args(argc, argv, "10", &image_source);

  obj = imf_image_alloc(klass);
  img = get_imf_image(obj);

  imf_load_jpeg(obj, image_source);

  return obj;
}

static VALUE
imf_image_get_color_space(VALUE obj)
{
  return ID2SYM(rb_intern("RGB"));
}

static VALUE
imf_image_get_component_size(VALUE obj)
{
  imf_image_t *img = get_imf_image(obj);
  return INT2FIX(img->component_size);
}

static VALUE
imf_image_get_pixel_channels(VALUE obj)
{
  imf_image_t *img = get_imf_image(obj);
  return INT2FIX(img->pixel_channels);
}

static VALUE
imf_image_get_width(VALUE obj)
{
  imf_image_t *img = get_imf_image(obj);
  return UINT2NUM(img->width);
}

static VALUE
imf_image_get_height(VALUE obj)
{
  imf_image_t *img = get_imf_image(obj);
  return UINT2NUM(img->height);
}

static VALUE
imf_image_get_row_stride(VALUE obj)
{
  imf_image_t *img = get_imf_image(obj);
  return UINT2NUM(img->row_stride);
}

void
Init_imf_image(void)
{
  imf_cIMF_Image = rb_define_class_under(imf_mIMF, "Image", rb_cObject);
  rb_define_alloc_func(imf_cIMF_Image, imf_image_alloc);

  rb_define_singleton_method(imf_cIMF_Image, "load_image", imf_image_s_load_image, -1);
  rb_define_method(imf_cIMF_Image, "color_space", imf_image_get_color_space, 0);
  rb_define_method(imf_cIMF_Image, "component_size", imf_image_get_component_size, 0);
  rb_define_method(imf_cIMF_Image, "pixel_channels", imf_image_get_pixel_channels, 0);
  rb_define_method(imf_cIMF_Image, "width", imf_image_get_width, 0);
  rb_define_method(imf_cIMF_Image, "height", imf_image_get_height, 0);
  rb_define_method(imf_cIMF_Image, "row_stride", imf_image_get_row_stride, 0);

  id_read = rb_intern("read");
}

void Init_imf_jpeg_src_mgr(void);

void
Init_native(void)
{
  imf_mIMF = rb_define_module("IMF");

  Init_imf_image();
  Init_imf_jpeg_src_mgr();

  imf_cIMF_ImageSource = rb_define_class_under(imf_mIMF, "ImageSource", rb_cObject);
}
