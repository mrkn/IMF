#include "IMF.h"
#include "internal.h"

VALUE imf_cIMF_FileFormat_Base;
static VALUE imf_cIMF_FileFormatRegistry;

static ID id_extnames;
static ID id_file_format_registry;

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

void
Init_imf_file_format(void)
{
  VALUE mFileFormat = rb_path2class("IMF::FileFormat");
  imf_cIMF_FileFormat_Base = rb_define_class_under(mFileFormat, "Base", rb_cObject);
  rb_define_singleton_method(imf_cIMF_FileFormat_Base, "extnames", imf_file_format_s_get_extnames, 0);
  rb_define_singleton_method(imf_cIMF_FileFormat_Base, "extnames=", imf_file_format_s_set_extnames, 1);
}
