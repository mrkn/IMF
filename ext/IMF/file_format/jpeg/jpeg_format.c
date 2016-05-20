#include "IMF.h"

#undef EXTERN
#include <jpeglib.h>
#include <jerror.h>

static char const JPEG_MAGIC_BYTES[] = "\xff\xd8";
static size_t const JPEG_MAGIC_LENGTH = sizeof(JPEG_MAGIC_BYTES) - 1;

static VALUE cSourceManager;
static ID id_detect;
static ID id_read;
static ID id_rewind;

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

static void
imf_jpeg_src_mgr_mark(void *ptr)
{
  rb_gc_mark(IMF_JPEG_SRC_MGR(ptr)->image_source);
}

static void
imf_jpeg_src_mgr_free(void *ptr)
{
  xfree(ptr);
}

static size_t
imf_jpeg_src_mgr_memsize(void const *ptr)
{
  return sizeof(imf_jpeg_src_mgr_t);
}

static rb_data_type_t imf_jpeg_src_mgr_data_type = {
  "imf_jpeg_src_mgr",
  {
    imf_jpeg_src_mgr_mark,
    imf_jpeg_src_mgr_free,
    imf_jpeg_src_mgr_memsize,
  },
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  0, 0,
  RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

imf_jpeg_src_mgr_t *
get_imf_jpeg_src_mgr(VALUE obj)
{
  imf_jpeg_src_mgr_t *srcmgr;
  TypedData_Get_Struct(obj, imf_jpeg_src_mgr_t, &imf_jpeg_src_mgr_data_type, srcmgr);
  return srcmgr;
}

static void
imf_jpeg_src_mgr_init_source(j_decompress_ptr cinfo)
{
  imf_jpeg_src_mgr_t *srcmgr = IMF_JPEG_SRC_MGR(cinfo->src);
  srcmgr->start_of_source = true;
}

static boolean
imf_jpeg_src_mgr_fill_input_buffer(j_decompress_ptr cinfo)
{
  imf_jpeg_src_mgr_t *srcmgr = IMF_JPEG_SRC_MGR(cinfo->src);
  srcmgr->buffer = rb_funcall(srcmgr->image_source, id_read, 1, INT2FIX(IMF_JPEG_BUFFER_SIZE));
  if (NIL_P(srcmgr->buffer))
    srcmgr->buffer = rb_str_tmp_new(2);
  if (NIL_P(srcmgr->buffer) || RSTRING_LEN(srcmgr->buffer) == 0) {
    if (srcmgr->start_of_source)
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a face EOI marker */
    rb_str_resize(srcmgr->buffer, 2);
    RSTRING_PTR(srcmgr)[0] = '\xFF';
    RSTRING_PTR(srcmgr)[1] = JPEG_EOI;
  }

  srcmgr->pub.next_input_byte = (JOCTET const *) RSTRING_PTR(srcmgr->buffer);
  srcmgr->pub.bytes_in_buffer = RSTRING_LEN(srcmgr->buffer);
  srcmgr->start_of_source = false;

  return TRUE;
}

static void
imf_jpeg_src_mgr_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr *src = cinfo->src;

  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.  */
  if (num_bytes > 0) {
    while (num_bytes > (long) src->bytes_in_buffer) {
      num_bytes -= (long) src->bytes_in_buffer;

      /* note we assume that fill_input_buffer will never return FALSE,
       * so suspension need not be handled.  */
      (void) (*src->fill_input_buffer) (cinfo);
    }
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void
imf_jpeg_src_mgr_term_source(j_decompress_ptr cinfo)
{
  /* nothing to do */
}

static VALUE
imf_jpeg_src_mgr_new(VALUE image_source)
{
  imf_jpeg_src_mgr_t *srcmgr;
  VALUE obj = TypedData_Make_Struct(
      cSourceManager,
      imf_jpeg_src_mgr_t,
      &imf_jpeg_src_mgr_data_type,
      srcmgr);

  srcmgr->image_source = image_source;
  srcmgr->buffer = rb_str_tmp_new(IMF_JPEG_BUFFER_SIZE);
  srcmgr->pub.init_source = imf_jpeg_src_mgr_init_source;
  srcmgr->pub.fill_input_buffer = imf_jpeg_src_mgr_fill_input_buffer;
  srcmgr->pub.skip_input_data = imf_jpeg_src_mgr_skip_input_data;
  srcmgr->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  srcmgr->pub.term_source = imf_jpeg_src_mgr_term_source;
  srcmgr->pub.bytes_in_buffer = 0;
  srcmgr->pub.next_input_byte = NULL;

  return obj;
}

static VALUE
init_source_manager(j_decompress_ptr cinfo, VALUE image_source)
{
  VALUE srcmgr = imf_jpeg_src_mgr_new(image_source);
  cinfo->src = (struct jpeg_source_mgr *) get_imf_jpeg_src_mgr(srcmgr);
  return srcmgr;
}

typedef struct imf_jpeg_format imf_jpeg_format_t;
struct imf_jpeg_format {
  imf_file_format_t base;
  struct jpeg_decompress_struct cinfo;
  VALUE srcmgr;
  VALUE buffer;
  int running;
};

static char const *const extnames[] = {
  ".jpg", ".jpeg", ".jpe", ".jfif", NULL
};

static int detect_jpeg(imf_file_format_t *fmt, VALUE image_source);
static void load_jpeg(imf_file_format_t *fmt, imf_image_t *img, VALUE image_source);

static imf_file_format_interface_t const jpeg_format_interface = {
  extnames,
  detect_jpeg,
  load_jpeg
};

static void
jpeg_format_mark(void *ptr)
{
  imf_jpeg_format_t *fmt = (imf_jpeg_format_t *) ptr;
  rb_gc_mark(fmt->srcmgr);
  rb_gc_mark(fmt->buffer);
  imf_file_format_mark(ptr);
}

static void
jpeg_format_free(void *ptr)
{
  imf_jpeg_format_t *fmt = (imf_jpeg_format_t *) ptr;
  imf_file_format_free(ptr);
}

static size_t
jpeg_format_memsize(void const *ptr)
{
  return imf_file_format_memsize(ptr);
}

static rb_data_type_t const jpeg_format_data_type = {
  "imf/file_format/jpeg",
  {
    jpeg_format_mark,
    jpeg_format_free,
    jpeg_format_memsize,
  },
  &imf_file_format_data_type,
  (void *)&jpeg_format_interface,
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

static VALUE
jpeg_format_alloc(VALUE klass)
{
  imf_jpeg_format_t *fmt;
  VALUE obj = TypedData_Make_Struct(
      klass,
      imf_jpeg_format_t,
      &jpeg_format_data_type,
      fmt);
  return obj;
}

static int
detect_jpeg(imf_file_format_t *fmt, VALUE image_source)
{
  VALUE magic_value;
  char const *magic;

  magic_value = rb_funcall(image_source, id_read, 1, INT2FIX(JPEG_MAGIC_LENGTH));
  rb_funcall(image_source, id_rewind, 0);

  magic = StringValuePtr(magic_value);
  if (RSTRING_LEN(magic_value) == JPEG_MAGIC_LENGTH &&
      memcmp(JPEG_MAGIC_BYTES, magic, JPEG_MAGIC_LENGTH) == 0)
    return 1;
  return 0;
}

static void
jpeg_error_exit(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX] = { '\0', };

  (*cinfo->err->format_message)(cinfo, buffer);

  rb_raise(rb_eRuntimeError, "JPEG ERROR: %s", buffer);
}

static void
load_jpeg(imf_file_format_t *base_fmt, imf_image_t *img, VALUE image_source)
{
  imf_jpeg_format_t *fmt = (imf_jpeg_format_t *) base_fmt;
  struct jpeg_decompress_struct *cinfo;
  struct jpeg_error_mgr jerr;
  JOCTET *buffer_ptr;

  assert(img != NULL);

  if (!rb_obj_is_kind_of(image_source, imf_cIMF_ImageSource)) {
    rb_raise(rb_eTypeError, "image_source must be an IMF::ImageSource object");
  }

  cinfo = &fmt->cinfo;
  cinfo->err = jpeg_std_error(&jerr);
  cinfo->err->error_exit = jpeg_error_exit;

  jpeg_create_decompress(cinfo);
  fmt->running = 1;

  /* setup source manager */
  fmt->srcmgr = init_source_manager(cinfo, image_source);

  jpeg_read_header(cinfo, TRUE);
  jpeg_start_decompress(cinfo);

  /* allocate image buffer */
  img->color_space = IMF_COLOR_SPACE_RGB;
  IMF_IMAGE_UNSET_ALPHA(img);
  img->component_size = sizeof(JSAMPLE);
  img->pixel_channels = cinfo->output_components;
  img->width = cinfo->output_width;
  img->height = cinfo->output_height;

  imf_image_allocate_image_buffer(img);

  /* allocate temporary scanline buffer */
  fmt->buffer = rb_str_tmp_new(sizeof(JSAMPLE) * cinfo->output_width * cinfo->output_components);
  buffer_ptr = (JOCTET *) RSTRING_PTR(fmt->buffer);

  size_t y = 0;
  while (cinfo->output_scanline < cinfo->output_height) {
    size_t x, ch;
    jpeg_read_scanlines(cinfo, &buffer_ptr, 1);

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
  rb_str_resize(fmt->buffer, 0L);

  jpeg_finish_decompress(cinfo);
  jpeg_destroy_decompress(cinfo);
  fmt->running = 0;
}

void
Init_jpeg(void)
{
  VALUE mFileFormat, cBase, cJPEG;

  mFileFormat = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
  cBase = rb_const_get(mFileFormat, rb_intern_const("Base"));
  cJPEG = rb_define_class_under(mFileFormat, "JPEG", cBase);
  cSourceManager = rb_define_class_under(cJPEG, "SourceManager", rb_cData);

  rb_define_alloc_func(cJPEG, jpeg_format_alloc);

  id_detect = rb_intern("detect");
  id_read = rb_intern("read");
  id_rewind = rb_intern("rewind");
}
