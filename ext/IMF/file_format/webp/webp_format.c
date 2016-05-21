#include "IMF.h"

static size_t const WEBP_PREFIX_LENGTH = 16;

static char const WEBP_MAGIC1_BYTES[] = "RIFF";
static size_t const WEBP_MAGIC1_START = 0;
static size_t const WEBP_MAGIC1_LENGTH = sizeof(WEBP_MAGIC1_BYTES) - 1;

static char const WEBP_MAGIC2_BYTES[] = "WEBP";
static size_t const WEBP_MAGIC2_START = 8;
static size_t const WEBP_MAGIC2_LENGTH = sizeof(WEBP_MAGIC2_BYTES) - 1;

static char const WEBP_MAGIC3_BYTES[] = "VP8";
static size_t const WEBP_MAGIC3_START = 12;
static size_t const WEBP_MAGIC3_LENGTH = sizeof(WEBP_MAGIC3_BYTES) - 1;

static char const WEBP_MAGIC3_LAST_BYTES[] = " XL";

static ID id_detect;
static ID id_read;
static ID id_rewind;

typedef struct imf_webp_format imf_webp_format_t;
struct imf_webp_format {
  imf_file_format_t base;
  imf_image_t *img;
  VALUE image_source;
  VALUE buffer;
};

static char const *const webp_format_extnames[] = {
  ".webp", NULL
};

static int detect_webp(imf_file_format_t *fmt, VALUE image_source);

static imf_file_format_interface_t const webp_format_interface = {
  detect_webp,
  NULL
};

static void
webp_format_mark(void *ptr)
{
  imf_webp_format_t *fmt = (imf_webp_format_t *) ptr;
  rb_gc_mark(fmt->image_source);
  rb_gc_mark(fmt->buffer);
  imf_file_format_mark(ptr);
}

static void
webp_format_free(void *ptr)
{
  imf_webp_format_t *fmt = (imf_webp_format_t *) ptr;
  imf_file_format_free(ptr);
}

static size_t
webp_format_memsize(void const *ptr)
{
  return imf_file_format_memsize(ptr);
}

static rb_data_type_t const webp_format_data_type = {
  "imf/file_format/webp",
  {
    webp_format_mark,
    webp_format_free,
    webp_format_memsize,
  },
  &imf_file_format_data_type,
  (void *)&webp_format_interface,
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

static VALUE
webp_format_alloc(VALUE klass)
{
  imf_webp_format_t *fmt;
  VALUE obj = TypedData_Make_Struct(
      klass,
      imf_webp_format_t,
      &webp_format_data_type,
      fmt);
  return obj;
}

static int
detect_webp(imf_file_format_t *fmt, VALUE image_source)
{
  VALUE magic_value;
  char const *magic;

  magic_value = rb_funcall(image_source, id_read, 1, INT2FIX(WEBP_PREFIX_LENGTH));

  magic = StringValuePtr(magic_value);
  if (RSTRING_LEN(magic_value) < WEBP_PREFIX_LENGTH)
    return 0;

  if (memcmp(WEBP_MAGIC1_BYTES, magic + WEBP_MAGIC1_START, WEBP_MAGIC1_LENGTH) != 0)
    return 0;

  if (memcmp(WEBP_MAGIC2_BYTES, magic + WEBP_MAGIC2_START, WEBP_MAGIC2_LENGTH) != 0)
    return 0;

  if (memcmp(WEBP_MAGIC3_BYTES, magic + WEBP_MAGIC3_START, WEBP_MAGIC3_LENGTH) != 0)
    return 0;

  if (strchr(WEBP_MAGIC3_LAST_BYTES, magic[WEBP_PREFIX_LENGTH-1]) == NULL)
    return 0;

  return 1;
}

void
Init_webp(void)
{
  VALUE m, base, c;

  m = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
  base = rb_const_get(m, rb_intern_const("Base"));
  c = rb_define_class_under(m, "WEBP", base);

  rb_define_alloc_func(c, webp_format_alloc);

  id_detect = rb_intern("detect");
  id_read = rb_intern("read");
  id_rewind = rb_intern("rewind");

  imf_register_file_format(c, webp_format_extnames);
}
