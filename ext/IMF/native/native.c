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
static ID id_read;
static ID id_rewind;

static ID id_ivar_path;

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

static bool
imf_jpeg_guess(VALUE image_source)
{
  VALUE jpeg_format = rb_path2class("IMF::FileFormat::JpegFormat");
  VALUE res = rb_funcall(jpeg_format, id_detect, 1, image_source);
  rb_funcall(image_source, id_rewind, 0);
  return RTEST(res);
}

static bool
imf_png_guess(VALUE image_source)
{
  VALUE png_format = rb_path2class("IMF::FileFormat::PngFormat");
  VALUE res = rb_funcall(png_format, id_detect, 1, image_source);
  rb_funcall(image_source, id_rewind, 0);
  return RTEST(res);
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
  img->color_space = IMF_COLOR_SPACE_RGB;
  IMF_IMAGE_UNSET_ALPHA(img);
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

static void
imf_png_error(png_structp png_ptr, png_const_charp msg)
{
  rb_raise(rb_eRuntimeError, "PNG ERROR: %s", msg);
}

static void
imf_png_warning(png_structp png_ptr, png_const_charp msg)
{
  rb_warn("PNG WARNING: %s", msg);
}

static png_voidp
imf_png_malloc(png_structp RB_UNUSED_VAR(png_ptr), png_alloc_size_t size)
{
  return xmalloc(size);
}

static void
imf_png_free(png_structp RB_UNUSED_VAR(png_ptr), png_voidp ptr)
{
  xfree(ptr);
}

struct imf_png_read_context {
  VALUE image_obj;
  VALUE image_source;
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_ptr;
};

#ifdef PNG_USER_MEM_SUPPORTED
# define IMF_PNG_TRY_WITH_GC(alloc_expr) alloc_expr
#else
# define IMF_PNG_TRY_WITH_GC(alloc_expr) { \
    int retried = 0; \
    do { \
      if (!(alloc_expr)) { \
        if (retried > 0) rb_memerror(); \
        rb_gc(); \
        retried++; \
        continue; \
      } \
    } while (0) \
  }
#endif

static inline png_structp
imf_png_create_read_struct(VALUE image_obj)
{
  png_structp png_ptr;

#ifdef PNG_USER_MEM_SUPPORTED
  png_ptr = png_create_read_struct_2(
    PNG_LIBPNG_VER_STRING,
    (png_voidp) image_obj, imf_png_error, imf_png_warning,
    NULL, imf_png_malloc, imf_png_free
  );
#else
  IMF_PNG_TRY_WITH_GC(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, image_obj, imf_png_error, imf_png_warning));
#endif

  return png_ptr;
}

static inline png_infop
imf_png_create_info_struct(png_structp png_ptr)
{
  png_infop info_ptr;

  IMF_PNG_TRY_WITH_GC(info_ptr = png_create_info_struct(png_ptr));

  return info_ptr;
}

static void
imf_png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
  png_voidp read_io_ptr = png_get_io_ptr(png_ptr);
  struct imf_png_read_context *ctx = (struct imf_png_read_context *) read_io_ptr;

  VALUE read_data = rb_funcall(ctx->image_source, id_read, 1, SIZET2NUM(length));
  memcpy(data, RSTRING_PTR(read_data), length);
}

static VALUE
imf_load_png_body(VALUE arg)
{
  struct imf_png_read_context *ctx = (struct imf_png_read_context *)arg;
  imf_image_t *img;
  volatile VALUE buffer;
  png_bytep buffer_ptr;

  assert(ctx != NULL);
  assert(is_imf_image(ctx->image_obj));
  assert(!NIL_P(ctx->image_source));
  assert(rb_obj_is_kind_of(ctx->image_source, imf_cIMF_ImageSource));

  img = get_imf_image(ctx->image_obj);

  ctx->png_ptr = imf_png_create_read_struct(ctx->image_obj);
  IMF_PNG_TRY_WITH_GC(ctx->info_ptr = png_create_info_struct(ctx->png_ptr));
  IMF_PNG_TRY_WITH_GC(ctx->end_ptr = png_create_info_struct(ctx->png_ptr));

  png_set_read_fn(ctx->png_ptr, (png_voidp) ctx, imf_png_read_data);

  png_read_info(ctx->png_ptr, ctx->info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type;
  int RB_UNUSED_VAR(interlace_method);
  int RB_UNUSED_VAR(compression_method);
  int RB_UNUSED_VAR(filter_method);

  png_get_IHDR(ctx->png_ptr, ctx->info_ptr, &width, &height,
      &bit_depth, &color_type, &interlace_method, &compression_method, &filter_method);

  img->width = width;
  img->height = height;

  if (bit_depth <= 8)
    img->component_size = 1;
  else {
    /* assume bit_depth == 16 here */
#if PNG_LIBPNG_VER >= 10504
    png_set_scale_16(ctx->png_ptr);
#else
    png_set_strip_16(ctx->png_ptr);
#endif
    /* TODO: Support bit_depth == 16 */
    /* img->component_size = 2; */
  }

  int png_channels;
  switch (color_type) {
    case PNG_COLOR_TYPE_GRAY:
      img->color_space = IMF_COLOR_SPACE_GRAY;
      IMF_IMAGE_UNSET_ALPHA(img);
      img->pixel_channels = 1;
      if (bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(ctx->png_ptr);
      png_channels = 1;
      break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
      img->color_space = IMF_COLOR_SPACE_GRAY;
      IMF_IMAGE_SET_ALPHA(img);
      img->pixel_channels = 2;
      png_channels = 2;
      break;
    case PNG_COLOR_TYPE_PALETTE:
      /* TODO: Support palette directly. */
      png_set_palette_to_rgb(ctx->png_ptr);
      /* pass through */
    case PNG_COLOR_TYPE_RGB:
      img->color_space = IMF_COLOR_SPACE_GRAY;
      IMF_IMAGE_UNSET_ALPHA(img);
      img->pixel_channels = 3;
#ifdef PNG_READ_FILLER_SUPPORTED
      png_set_filler(ctx->png_ptr, 0, PNG_FILLER_BEFORE);
      png_channels = 4;
#else
      png_channels = 3;
#endif
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
      img->color_space = IMF_COLOR_SPACE_RGB;
      IMF_IMAGE_SET_ALPHA(img);
      img->pixel_channels = 4;
      png_channels = 4;
      break;
    default:
      rb_raise(rb_eRuntimeError, "libpng: unknown color_type is given (%d)", color_type);
  }

  png_read_update_info(ctx->png_ptr, ctx->info_ptr);

  imf_image_allocate_image_buffer(img);

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
  /* allocate temporary scanline buffer */
  size_t const rowbytes = png_get_rowbytes(ctx->png_ptr, ctx->info_ptr);
  buffer = rb_str_tmp_new(rowbytes);
  buffer_ptr = (png_bytep) RSTRING_PTR(buffer);

  size_t y = 0;
  while (y < height) {
    size_t x, ch;
    png_read_row(ctx->png_ptr, buffer_ptr, NULL);

    size_t i = y * img->row_stride;
    for (x = 0; x < img->width; ++x) {
      size_t j = x * png_channels;
      for (ch = 0; ch < img->pixel_channels; ++ch) {
        img->channels[ch][i + x] = buffer_ptr[j + ch];
      }
    }

    ++y;
  }
#else
# error === PNG_SEQUENTIAL_READ_SUPPORTED is undefined ===
#endif

  /* release temporary buffer memory */
  buffer_ptr = NULL;
  rb_str_resize(buffer, 0L);

  png_read_end(ctx->png_ptr, ctx->end_ptr);

  return Qnil;
}

static VALUE
imf_load_png_ensure(VALUE arg)
{
  struct imf_png_read_context *ctx = (struct imf_png_read_context *)arg;

  assert(ctx != NULL);
  assert(ctx->png_ptr != NULL);
  assert(ctx->info_ptr != NULL);

  png_destroy_read_struct(&ctx->png_ptr, &ctx->info_ptr, &ctx->end_ptr);

  return Qnil;
}

static void
imf_load_png(VALUE image_obj, VALUE image_source)
{
  struct imf_png_read_context ctx;

  assert(is_imf_image(image_obj));
  assert(!NIL_P(image_source));
  assert(rb_obj_is_kind_of(image_source, imf_cIMF_ImageSource));

  ctx.image_obj = image_obj;
  ctx.image_source = image_source;
  ctx.png_ptr = NULL;
  ctx.info_ptr = NULL;
  ctx.end_ptr = NULL;

  rb_ensure(imf_load_png_body, (VALUE)&ctx, imf_load_png_ensure, (VALUE)&ctx);
}

static VALUE
imf_image_s_load_image(int argc, VALUE *argv, VALUE klass)
{
  VALUE obj, image_source, path_value;
  imf_image_t *img;
  char const *path, *e;
  long path_len;

  rb_scan_args(argc, argv, "10", &image_source);

  obj = imf_image_alloc(klass);
  img = get_imf_image(obj);

  /* TODO: Need to generalize */
  path_value = rb_ivar_get(image_source, id_ivar_path);
  if (!NIL_P(path_value)) {
    FilePathStringValue(path_value);
    path = StringValueCStr(path_value);
    path_len = RSTRING_LEN(path_value);
    e = ruby_enc_find_extname(path, &path_len, rb_enc_get(path_value));
    if (path_len <= 1)
      goto guess_format;
    if (memcmp(e, ".jpg", path_len) == 0 && imf_jpeg_guess(image_source)) {
    jpeg_format:
      imf_load_jpeg(obj, image_source);
      return obj;
    }
    if (memcmp(e, ".png", path_len) == 0 && imf_png_guess(image_source)) {
    png_format:
      imf_load_png(obj, image_source);
      return obj;
    }
  }

guess_format:
  if (imf_jpeg_guess(image_source))
    goto jpeg_format;
  if (imf_png_guess(image_source))
    goto png_format;

unknown:
  rb_raise(rb_eRuntimeError, "Unknown image format");
}

static VALUE
imf_image_get_color_space(VALUE obj)
{
  imf_image_t *img = get_imf_image(obj);

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
  imf_image_t *img = get_imf_image(obj);
  return IMF_IMAGE_HAS_ALPHA(img) ? Qtrue : Qfalse;
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
  rb_define_method(imf_cIMF_Image, "has_alpha?", imf_image_has_alpha, 0);
  rb_define_method(imf_cIMF_Image, "component_size", imf_image_get_component_size, 0);
  rb_define_method(imf_cIMF_Image, "pixel_channels", imf_image_get_pixel_channels, 0);
  rb_define_method(imf_cIMF_Image, "width", imf_image_get_width, 0);
  rb_define_method(imf_cIMF_Image, "height", imf_image_get_height, 0);
  rb_define_method(imf_cIMF_Image, "row_stride", imf_image_get_row_stride, 0);

  rb_require("IMF/file_format");
}

void Init_imf_jpeg_src_mgr(void);

void
Init_native(void)
{
  imf_mIMF = rb_define_module("IMF");

  Init_imf_image();
  Init_imf_jpeg_src_mgr();

  imf_cIMF_ImageSource = rb_define_class_under(imf_mIMF, "ImageSource", rb_cObject);

  id_detect = rb_intern("detect");
  id_read = rb_intern("read");
  id_rewind = rb_intern("rewind");

  id_ivar_path = rb_intern("@path");
}
