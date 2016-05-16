#include "IMF.h"

#include <png.h>

#ifndef HAVE_TYPE_PNG_ALLOC_SIZE_T
typedef png_size_t png_alloc_size_t;
#endif

static ID id_read;

typedef struct imf_png_format imf_png_format_t;
struct imf_png_format {
  imf_file_format_t base;
  imf_image_t *img;
  VALUE image_source;
  VALUE buffer;
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_ptr;
};

static char const *const extnames[] = {
  ".png", NULL
};

static int detect_png(imf_file_format_t *fmt, VALUE image_source);
static void load_png(imf_file_format_t *fmt, imf_image_t *img, VALUE image_source);

static imf_file_format_interface_t const png_format_interface = {
  extnames,
  detect_png,
  load_png
};

static void
png_format_mark(void *ptr)
{
  imf_png_format_t *fmt = (imf_png_format_t *) ptr;
  rb_gc_mark(fmt->image_source);
  rb_gc_mark(fmt->buffer);
  imf_file_format_mark(ptr);
}

static void
png_format_free(void *ptr)
{
  imf_png_format_t *fmt = (imf_png_format_t *) ptr;
  imf_file_format_free(ptr);
}

static size_t
png_format_memsize(void const *ptr)
{
  return imf_file_format_memsize(ptr);
}

static rb_data_type_t const png_format_data_type = {
  "imf/file_format/png",
  {
    png_format_mark,
    png_format_free,
    png_format_memsize,
  },
  &imf_file_format_data_type,
  (void *)&png_format_interface,
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

static VALUE
png_format_alloc(VALUE klass)
{
  imf_png_format_t *fmt;
  VALUE obj = TypedData_Make_Struct(
      klass,
      imf_png_format_t,
      &png_format_data_type,
      fmt);
  return obj;
}

static int
detect_png(imf_file_format_t *fmt, VALUE image_source)
{
  return 0;
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
imf_png_create_read_struct(imf_png_format_t *fmt)
{
  png_structp png_ptr;

#ifdef PNG_USER_MEM_SUPPORTED
  png_ptr = png_create_read_struct_2(
    PNG_LIBPNG_VER_STRING,
    (png_voidp) fmt, imf_png_error, imf_png_warning,
    NULL, imf_png_malloc, imf_png_free
  );
#else
  IMF_PNG_TRY_WITH_GC(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, fmt, imf_png_error, imf_png_warning));
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
  imf_png_format_t *fmt = (imf_png_format_t *) read_io_ptr;

  VALUE read_data = rb_funcall(fmt->image_source, id_read, 1, SIZET2NUM(length));
  memcpy(data, RSTRING_PTR(read_data), length);
}


static VALUE
load_png_body(VALUE arg)
{
  imf_png_format_t *fmt = (imf_png_format_t *) arg;
  imf_image_t *img;
  volatile VALUE buffer;
  png_bytep buffer_ptr;

  assert(fmt != NULL);
  assert(fmt->img != NULL);
  assert(!NIL_P(fmt->image_source));
  assert(rb_obj_is_kind_of(fmt->image_source, imf_cIMF_ImageSource));

  img = fmt->img;

  fmt->png_ptr = imf_png_create_read_struct(fmt);
  IMF_PNG_TRY_WITH_GC(fmt->info_ptr = png_create_info_struct(fmt->png_ptr));
  IMF_PNG_TRY_WITH_GC(fmt->end_ptr = png_create_info_struct(fmt->png_ptr));

  png_set_read_fn(fmt->png_ptr, (png_voidp) fmt, imf_png_read_data);

  png_read_info(fmt->png_ptr, fmt->info_ptr);

  png_uint_32 width, height;
  int bit_depth, color_type;
  int RB_UNUSED_VAR(interlace_method);
  int RB_UNUSED_VAR(compression_method);
  int RB_UNUSED_VAR(filter_method);

  png_get_IHDR(
    fmt->png_ptr, fmt->info_ptr,
    &width, &height,
    &bit_depth, &color_type,
    &interlace_method,
    &compression_method,
    &filter_method);

  img->width = width;
  img->height = height;

  if (bit_depth <= 8)
    img->component_size = 1;
  else {
    /* assume bit_depth == 16 here */
#if PNG_LIBPNG_VER >= 10504
    png_set_scale_16(fmt->png_ptr);
#else
    png_set_strip_16(fmt->png_ptr);
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
        png_set_expand_gray_1_2_4_to_8(fmt->png_ptr);
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
      png_set_palette_to_rgb(fmt->png_ptr);
      /* pass through */
    case PNG_COLOR_TYPE_RGB:
      img->color_space = IMF_COLOR_SPACE_GRAY;
      IMF_IMAGE_UNSET_ALPHA(img);
      img->pixel_channels = 3;
#ifdef PNG_READ_FILLER_SUPPORTED
      png_set_filler(fmt->png_ptr, 0, PNG_FILLER_BEFORE);
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

  png_read_update_info(fmt->png_ptr, fmt->info_ptr);

  imf_image_allocate_image_buffer(img);

#ifdef PNG_SEQUENTIAL_READ_SUPPORTED
  /* allocate temporary scanline buffer */
  size_t const rowbytes = png_get_rowbytes(fmt->png_ptr, fmt->info_ptr);
  fmt->buffer = rb_str_tmp_new(rowbytes);
  buffer_ptr = (png_bytep) RSTRING_PTR(fmt->buffer);

  size_t y = 0;
  while (y < height) {
    size_t x, ch;
    png_read_row(fmt->png_ptr, buffer_ptr, NULL);

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
  rb_str_resize(fmt->buffer, 0L);

  png_read_end(fmt->png_ptr, fmt->end_ptr);

  return Qnil;
}

static VALUE
load_png_ensure(VALUE arg)
{
  imf_png_format_t *fmt = (imf_png_format_t *) arg;

  assert(fmt != NULL);
  assert(fmt->png_ptr != NULL);
  assert(fmt->info_ptr != NULL);

  png_destroy_read_struct(&fmt->png_ptr, &fmt->info_ptr, &fmt->end_ptr);

  return Qnil;
}

static void
load_png(imf_file_format_t *base_fmt, imf_image_t *img, VALUE image_source)
{
  imf_png_format_t *fmt = (imf_png_format_t *) base_fmt;

  assert(fmt != NULL);
  assert(img != NULL);

  if (!rb_obj_is_kind_of(image_source, imf_cIMF_ImageSource)) {
    rb_raise(rb_eTypeError, "image_source must be an IMF::ImageSource object");
  }

  fmt->img = img;
  fmt->image_source = image_source;
  fmt->png_ptr = NULL;
  fmt->info_ptr = NULL;
  fmt->end_ptr = NULL;

  rb_ensure(load_png_body, (VALUE)fmt, load_png_ensure, (VALUE)fmt);
}

void
Init_png(void)
{
  VALUE mFileFormat, cBase, cPNG;

  mFileFormat = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
  cBase = rb_const_get(mFileFormat, rb_intern_const("Base"));
  cPNG = rb_define_class_under(mFileFormat, "PNG", cBase);

  rb_define_alloc_func(cPNG, png_format_alloc);

  id_read = rb_intern("read");
}
