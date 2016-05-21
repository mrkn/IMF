#include "IMF.h"

static char const GIF_MAGIC_BYTES[] = "GIF8";
static size_t const GIF_MAGIC_LENGTH = sizeof(GIF_MAGIC_BYTES) - 1;

static ID id_detect;
static ID id_read;
static ID id_rewind;

typedef struct imf_gif_format imf_gif_format_t;
struct imf_gif_format {
  imf_file_format_t base;
  imf_image_t *img;
  VALUE image_source;
  VALUE buffer;
};

static char const *const gif_format_extnames[] = {
  ".gif", NULL
};

static int detect_gif(imf_file_format_t *fmt, VALUE image_source);

static imf_file_format_interface_t const gif_format_interface = {
  detect_gif,
  NULL
};

static void
gif_format_mark(void *ptr)
{
  imf_gif_format_t *fmt = (imf_gif_format_t *) ptr;
  rb_gc_mark(fmt->image_source);
  rb_gc_mark(fmt->buffer);
  imf_file_format_mark(ptr);
}

static void
gif_format_free(void *ptr)
{
  imf_gif_format_t *fmt = (imf_gif_format_t *) ptr;
  imf_file_format_free(ptr);
}

static size_t
gif_format_memsize(void const *ptr)
{
  return imf_file_format_memsize(ptr);
}

static rb_data_type_t const gif_format_data_type = {
  "imf/file_format/gif",
  {
    gif_format_mark,
    gif_format_free,
    gif_format_memsize,
  },
  &imf_file_format_data_type,
  (void *)&gif_format_interface,
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

static VALUE
gif_format_alloc(VALUE klass)
{
  imf_gif_format_t *fmt;
  VALUE obj = TypedData_Make_Struct(
      klass,
      imf_gif_format_t,
      &gif_format_data_type,
      fmt);
  return obj;
}

static int
detect_gif(imf_file_format_t *fmt, VALUE image_source)
{
  VALUE magic_value;
  char const *magic;

  magic_value = rb_funcall(image_source, id_read, 1, INT2FIX(GIF_MAGIC_LENGTH));

  magic = StringValuePtr(magic_value);
  if (RSTRING_LEN(magic_value) == GIF_MAGIC_LENGTH &&
      memcmp(GIF_MAGIC_BYTES, magic, GIF_MAGIC_LENGTH) == 0)
    return 1;
  return 0;
}

void
Init_gif(void)
{
  VALUE m, base, c;

  m = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
  base = rb_const_get(m, rb_intern_const("Base"));
  c = rb_define_class_under(m, "GIF", base);

  rb_define_alloc_func(c, gif_format_alloc);

  id_detect = rb_intern("detect");
  id_read = rb_intern("read");
  id_rewind = rb_intern("rewind");

  imf_register_file_format(c, gif_format_extnames);
}
