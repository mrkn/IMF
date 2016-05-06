#include "IMF.h"

#include "internal.h"

#include <jerror.h>

VALUE imf_cIMF_JpegSourceManager;

static ID id_read;

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
      imf_cIMF_JpegSourceManager,
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

VALUE
imf_jpeg_init_image_source(j_decompress_ptr cinfo, VALUE image_source)
{
  VALUE srcmgr = imf_jpeg_src_mgr_new(image_source);
  cinfo->src = (struct jpeg_source_mgr *) get_imf_jpeg_src_mgr(srcmgr);
  return srcmgr;
}

void
Init_imf_jpeg_src_mgr(void)
{
  imf_cIMF_JpegSourceManager = rb_define_class_under(imf_mIMF, "JpegSourceManager", rb_cData);

  id_read = rb_intern("read");
}
