#include "IMF.h"
#include "internal.h"

VALUE imf_cIMF_FileFormat_Base;
static VALUE imf_cIMF_FileFormatRegistry;

static ID id_detect;
static ID id_extnames;
static ID id_file_formats_for_filename;
static ID id_register_file_format;
static ID id_rewind;

void
imf_file_format_mark(void *ptr)
{
}

void
imf_file_format_free(void *ptr)
{
}

size_t
imf_file_format_memsize(void const *ptr)
{
  return 0;
}

rb_data_type_t const imf_file_format_data_type = {
  "imf/file_format/base",
  {
    imf_file_format_mark,
    imf_file_format_free,
    imf_file_format_memsize,
  },
#ifdef RUBY_TYPED_FREE_IMMEDIATELY
  0, 0,
  RUBY_TYPED_FREE_IMMEDIATELY
#endif
};

int
imf_is_file_format_class(VALUE klass)
{
  VALUE m, base;
  if (!RB_TYPE_P(klass, T_CLASS))
    return 0;
  m = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
  base = rb_const_get(m, rb_intern_const("Base"));
  return rb_class_inherited_p(klass, base);
}

static VALUE
imf_file_format_alloc(VALUE klass)
{
  VALUE obj = TypedData_Wrap_Struct(
      klass,
      &imf_file_format_data_type,
      NULL
    );
  return obj;
}

static VALUE
imf_file_format_s_get_format_name(VALUE klass)
{
  char const CLASSPATH_PREFIX[] = "IMF::FileFormat::";
  char const *p, *name;

  name = rb_class2name(klass);
  if (strncmp(CLASSPATH_PREFIX, name, sizeof(CLASSPATH_PREFIX) - 1) == 0)
    return rb_str_new_cstr(name + sizeof(CLASSPATH_PREFIX) - 1);
  return rb_str_new_cstr(name);
}

static VALUE
imf_file_format_s_get_extnames(VALUE klass)
{
  if (!rb_ivar_defined(klass, id_extnames))
    return Qnil;
  return rb_ivar_get(klass, id_extnames);
}

static VALUE
imf_file_format_s_set_extnames(VALUE klass, VALUE extnames)
{
  Check_Type(extnames, T_ARRAY);
  rb_ivar_set(klass, id_extnames, extnames);
  return extnames;
}

static inline imf_file_format_t *
imf_get_file_format_data(VALUE obj)
{
   imf_file_format_t *ptr;
   TypedData_Get_Struct(obj, imf_file_format_t, &imf_file_format_data_type, ptr);
   return ptr;
}

VALUE
imf_file_format_detect(VALUE fmt_obj, VALUE imgsrc_obj)
{
  int res = 0;

  imf_file_format_interface_t *iface = imf_file_format_interface(fmt_obj);
  imf_file_format_t *fmt = imf_get_file_format_data(fmt_obj);

  if (iface != NULL && iface->detect != NULL) {
    res = iface->detect(fmt, imgsrc_obj);
    rb_funcall(imgsrc_obj, id_rewind, 0);
  }

  return res ? Qtrue : Qfalse;
}

VALUE
imf_file_format_load(VALUE fmt_obj, VALUE image_obj, VALUE imgsrc_obj)
{
  imf_file_format_interface_t *iface = imf_file_format_interface(fmt_obj);
  imf_file_format_t *fmt = imf_get_file_format_data(fmt_obj);
  imf_image_t *img = imf_get_image_data(image_obj);

  if (iface != NULL && iface->load != NULL)
    iface->load(fmt, img, imgsrc_obj);

  return image_obj;
}

void
imf_register_file_format(VALUE file_format, char const *const *extnames)
{
  VALUE m, base;
  m = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));
  base = rb_const_get(m, rb_intern_const("Base"));
  if (rb_class_inherited_p(file_format, base)) {
    char const *const *p;
    VALUE ary;
    ary = rb_ary_new();
    for (p = extnames; *p != NULL; ++p) {
      rb_ary_push(ary, rb_str_new_cstr(*p));
    }
    imf_file_format_s_set_extnames(file_format, ary);
    rb_funcall(imf_mIMF, id_register_file_format, 1, file_format);
  }
}

VALUE
imf_find_file_format_by_filename(VALUE path_value)
{
  VALUE ary, fmt_klass;

  FilePathStringValue(path_value);
  ary = rb_funcall(imf_mIMF, id_file_formats_for_filename, 1, path_value);
  if (RARRAY_LEN(ary) == 0)
    return Qnil;

  fmt_klass = RARRAY_AREF(ary, 0);
  if (!imf_is_file_format_class(fmt_klass))
    return Qnil;

  return rb_class_new_instance(0, NULL, fmt_klass);
}

VALUE
imf_detect_file_format(VALUE imgsrc_obj)
{
  return rb_funcall(imf_cIMF_Image, rb_intern_const("detect_format"), 1, imgsrc_obj);
}

void
Init_imf_file_format(void)
{
  VALUE mFileFormat = rb_const_get(imf_mIMF, rb_intern_const("FileFormat"));

  imf_cIMF_FileFormat_Base = rb_define_class_under(mFileFormat, "Base", rb_cObject);
  rb_define_alloc_func(imf_cIMF_FileFormat_Base, imf_file_format_alloc);
  rb_define_singleton_method(imf_cIMF_FileFormat_Base, "format_name", imf_file_format_s_get_format_name, 0);
  rb_define_singleton_method(imf_cIMF_FileFormat_Base, "extnames", imf_file_format_s_get_extnames, 0);
  rb_define_singleton_method(imf_cIMF_FileFormat_Base, "extnames=", imf_file_format_s_set_extnames, 1);
  rb_define_method(imf_cIMF_FileFormat_Base, "detect", imf_file_format_detect, 1);
  rb_define_method(imf_cIMF_FileFormat_Base, "load", imf_file_format_load, 2);

  id_detect = rb_intern("detect");
  id_extnames = rb_intern("extnames");
  id_file_formats_for_filename = rb_intern("file_formats_for_filename");
  id_register_file_format = rb_intern("register_file_format");
  id_rewind = rb_intern("rewind");
}
