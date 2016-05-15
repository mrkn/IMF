#include "IMF.h"
#include "internal.h"

VALUE imf_cIMF_FileFormat_Base;
static VALUE imf_cIMF_FileFormatRegistry;

static ID id_extnames;
static ID id_file_format_registry;

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
  imf_file_format_interface_t *iface = imf_file_format_interface(fmt_obj);
  imf_file_format_t *fmt = imf_get_file_format_data(fmt_obj);

  return iface->detect(fmt, imgsrc_obj) ? Qtrue : Qfalse;
}

VALUE
imf_file_format_load(VALUE fmt_obj, VALUE image_obj, VALUE imgsrc_obj)
{
  imf_file_format_interface_t *iface = imf_file_format_interface(fmt_obj);
  imf_file_format_t *fmt = imf_get_file_format_data(fmt_obj);
  imf_image_t *img = imf_get_image_data(image_obj);

  iface->load(fmt, img, imgsrc_obj);
  return image_obj;
}

void
Init_imf_file_format(void)
{
  VALUE mFileFormat = rb_path2class("IMF::FileFormat");
  imf_cIMF_FileFormat_Base = rb_define_class_under(mFileFormat, "Base", rb_cObject);
  rb_define_alloc_func(imf_cIMF_FileFormat_Base, imf_file_format_alloc);
  rb_define_singleton_method(imf_cIMF_FileFormat_Base, "extnames", imf_file_format_s_get_extnames, 0);
  rb_define_singleton_method(imf_cIMF_FileFormat_Base, "extnames=", imf_file_format_s_set_extnames, 1);
  rb_define_method(imf_cIMF_FileFormat_Base, "detect", imf_file_format_detect, 1);
  rb_define_method(imf_cIMF_FileFormat_Base, "load", imf_file_format_load, 2);
}
