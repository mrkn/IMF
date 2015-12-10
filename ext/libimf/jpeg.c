#include <ruby.h>
#include <jpeglib.h>

#define DEFAULT_BUFFER_SIZE 65536  /* == 64KB */

static VALUE mIMF;
static VALUE cJpegDecoder;
static VALUE sJpegHeader;

static ID id_read;
static ID id_GRAY;
static ID id_RGB;
static ID id_YCbCr;
static ID id_CMYK;
static ID id_YCCK;
static ID id_UNKNOWN;

struct imf_jpeg_error_mgr {
  jpeg_error_mgr base;
  jmp_buf error_jmp_buf;
};

struct imf_jpeg_decoder_struct {
  VALUE src_io;
  VALUE buffer;
  unsigned interrupt;

  imf_jpeg_error_mgr error_mgr;
};
typedef struct imf_jpeg_decoder_struct *imf_jpeg_decoder_ptr;

#define IMF_JPEG_DECODER(cinfo) ((imf_jpeg_decoder_ptr)(cinfo)->client_data)

#define IMF_JPEG_ALLOC_SMALL(cinfo, pool, type) \
  (type *)(* (cinfo)->mem->alloc_small)((j_common_ptr)(cinfo), (pool), sizeof(type))

static void
imf_jpeg_source_mgr_init_source(j_decompress_ptr cinfo)
{
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);
  struct jpeg_source_mgr *const src = cinfo->src;

  if (decoder->buffer == Qnil) {
    decoder->buffer = rb_str_tmp_new(DEFAULT_BUFFER_SIZE);
  }

  src->bytes_in_buffer = 0;
}

static boolean
imf_jpeg_source_mgr_fill_input_buffer_main(j_decompress_ptr cinfo)
{
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);
  struct jpeg_source_mgr *const src = cinfo->src;

  VALUE len = SIZET2NUM(rb_str_capacity(decoder->buffer));

  rb_funcall(decoder->src_io, id_read, len, decoder->buffer);
  /* TODO: treat exception occurred here */

  return TRUE;
}

static boolean
imf_jpeg_source_mgr_fill_input_buffer(j_decompress_ptr cinfo)
{
  return (boolean) rb_thread_call_with_gvl(imf_jpeg_fill_input_buffer_main, cinfo);
}

static void
imf_jpeg_source_mgr_skip_input_data_main(j_decompress_ptr cinfo, long num_bytes)
{
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);
  struct jpeg_source_mgr *const src = cinfo->src;

  while (num_bytes > (long) src->bytes_in_buffer) {
    num_bytes -= (long) src->bytes_in_buffer;
    imf_jpeg_fill_input_buffer_main(cinfo);
    /* NOTE: assume suspension will never be occurred */
    /* TODO: support suspension */
  }
}

/* NOTE: Use libjpeg's default resync_to_restart routine */
#define imf_jpeg_source_mgr_resync_to_restart jpeg_resync_to_restart

static void
imf_jpeg_source_mgr_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);
  struct jpeg_source_mgr *const src = cinfo->src;

  if (num_bytes > 0) {
    if (num_bytes > (long) src->bytes_in_buffer) {
      rb_thread_call_with_gvl(imf_jpeg_skip_input_data_main, cinfo);
    }
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void
imf_jpeg_source_mgr_term_source(j_decompress_ptr cinfo)
{
  /* do nothing */
}

static struct jpeg_source_mgr *
imf_jpeg_source_mgr_create(j_decompress_ptr cinfo)
{
  struct jpeg_source_mgr *src = IMF_JPEG_ALLOC_SMALL(cinfo, JPOOL_IMAGE, struct jpeg_source_mgr);
  src->init_source = imf_jpeg_source_mgr_init_source;
  src->fill_input_buffer = imf_jpeg_source_mgr_fill_input_buffer;
  src->skip_input_data = imf_jpeg_source_mgr_skip_input_data;
  src->resync_to_restart = imf_jpeg_source_mgr_jpeg_resync_to_restart;
  src->term_source = imf_jpeg_source_mgr_term_source;
  return src;
}

static j_decompress_ptr
imf_jpeg_decoder_create(void)
{
  imf_jpeg_decoder_ptr decoder = ALLOC(struct imf_jpeg_decoder_struct);
  decoder->src_io = Qnil;
  decoder->buffer = Qnil;
  decoder->interrupt = 0;

  j_decompress_ptr cinfo = ALLOC(struct jpeg_decompress_struct);
  cinfo->client_data = decoder;
  cinfo->err = jpeg_std_error((struct jpeg_error_mgr *)&decoder->error_mgr);
  jpeg_create_decompress(cinfo);

  cinfo->src = imf_jpeg_source_mgr_create()

  return cinfo;
}

static void
imf_jpeg_decoder_mark(j_decompress_ptr cinfo)
{
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);
  rb_gc_mark(decoder->src_io);
  rb_gc_mark(decoder->buffer);
}

static void
imf_jpeg_decoder_destroy(j_decompress_ptr cinfo)
{
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);
  jpeg_destroy_decompress(&decoder->cinfo);
}

static const rb_data_type_t imf_jpeg_decoder_type = {
  "imf_jpeg_decoder",
  {
    imf_jpeg_decoder_mark,
    imf_jpeg_decoder_destroy,
    imf_jpeg_decoder_memsize,
  },
  0, 0, RUBY_TYPED_FREE_IMMEDIATELY
};

static j_decompress_ptr
get_j_decompress_ptr(VALUE obj)
{
  j_decompress_ptr cinfo;
  TypedData_Get_Struct(obj, struct jpeg_decompress_struct, &imf_jpeg_decoder_type, cinfo);
  return cinfo;
}

static void
imf_jpeg_decoder_memsize(j_decompress_ptr cinfo)
{
  size_t size = sizeof(struct jpeg_decompress_struct);
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);
  if (decoder != NULL) {
    size += sizeof(struct imf_jpeg_decoder_struct);
    size += rb_str_capacity(decoder->buffer);
  }
  return size;
}

static VALUE
imf_jpeg_decoder_s_allocate(VALUE klass)
{
  j_decompress_ptr cinfo = imf_jpeg_decoder_create();
  VALUE obj = TypedData_Wrap_Struct(cJpegDecoder, &imf_jpeg_decoder_type, cinfo);
  return obj;
}

static void
check_respond_to_read(VALUE io)
{
  if (!rb_respond_to(io, id_read)) {
    rb_raise(rb_eArgError, "argument must respond to `read`");
  }
}

static void
check_image(VALUE obj)
{
  VALUE cImage = rb_path2class("IMF::Image");
  if (!RTEST(rb_obj_is_kind_of(obj, cImage))) {
    rb_raise(rb_eArgError, "argument must be a IMF::Image object");
  }
}

static VALUE
imf_jpeg_decoder_initialize(VALUE self, VALUE src_io)
{
  j_decompress_ptr cinfo = get_j_decompress_ptr(self);
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);

  check_respond_to_read(src_io);
  decoder->src_io = src_io;

  return self;
}

static VALUE
imf_jpeg_decoder_read_header(VALUE self)
{
  j_decompress_ptr cinfo = get_j_decompress_ptr(self);

  int retcode = jpeg_read_header(cinfo, FALSE);
  if (retcode == JPEG_SUSPENSION) {
    /* TODO: Support source suspension */
    return Qnil;
  }

  VALUE image_width    = UINT2NUM(cinfo->image_width);
  VALUE image_height   = UINT2NUM(cinfo->image_height);
  VALUE num_components = INT2NUM(cinfo->num_components);

  VALUE color_space;
  switch (cinfo->jpeg_color_space) {
    case JCS_GRAYSCALE:
      color_space = id_GRAY;
      break;
    case JCS_RGB:
      color_space = id_RGB;
      break;
    case JCS_YCbCr:
      color_space = id_YCbCr;
      break;
    case JCS_CMYK:
      color_space = id_CMYK;
      break;
    case JCS_YCCK:
      color_space = id_YCCK;
      break;
    default:
      color_space = id_UNKNOWN;
      break;
  }

  VALUE no_image = RTEST(retcode == JPEG_HEADER_TABLES_ONLY);

  return rb_struct_new(sJpegHeader, image_width, image_height, color_space, no_image);
}

static void
imf_jpeg_decoder_decode_image_main(j_decompress_ptr cinfo)
{
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);

  jpeg_start_decompress(cinfo);

  while (cinfo->output_scanline < cinfo->output_height) {
    if (decoder->interrupt) {
      jpeg_abort(cinfo);
      return;
    }

    jpeg_read_scanlines(cinfo, output_buffer, 1);
  }

  jpeg_finish_decompress(cinfo);
}

static void
imf_jpeg_decoder_ubf(j_decompress_ptr cinfo)
{
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);
  decoder->interrupt = 1;
}

static VALUE
imf_jpeg_decoder_decode_image(VALUE self, VALUE im)
{
  j_decompress_ptr cinfo = get_j_decompress_ptr(self);
  imf_jpeg_decoder_ptr decoder = IMF_JPEG_DECODER(cinfo);

  check_image(im);

  rb_thread_call_without_gvl(
    imf_jpeg_decoder_decode_image_main, cinfo,
    imf_jpeg_decoder_ubf, cinfo);

  return decoder->interrupt ? Qfalse : Qtrue;
}

static VALUE
imf_jpeg_decoder_

void
Init_libimf(void)
{
  mIMF = rb_define_module("IMF");

  id_read = rb_intern("read");
  id_GRAY = rb_intern("GRAY");
  id_RGB = rb_intern("RGB");
  id_CMYK = rb_intern("CMYK");
  id_YCbCr = rb_intern("YCbCr");
  id_YCCK = rb_intern("YCCK");

  cJpegDecoder = rb_define_class_under(mIMF, "JpegDecoder", rb_cData);
  rb_define_allocator(cJpegDecoder, imf_jpeg_decoder_s_allocate);
  rb_define_method(cJpegDecoder, "initialize", imf_jpeg_decoder_initialize, 2);
  rb_define_method(cJpegDecoder, "read_header", imf_jpeg_decoder_read_header, 0);
  rb_define_method(cJpegDecoder, "decode_image", imf_jpeg_decoder_decode_image, 1);

  sJpegHeader = rb_struct_define_under(mIMF, "JpegHeader",
                                       "image_width",
                                       "image_height",
                                       "num_components",
                                       "color_space",
                                       "no_image",
                                       NULL);
}
