require 'mkmf'

imf_include_dir = File.expand_path('../../include', __FILE__)
$CFLAGS += " -I#{imf_include_dir}"

num_buffer_dir = File.expand_path('~/work/num_buffer')
$CFLAGS += " -I#{num_buffer_dir}/include"

have_func('rb_ary_new_capa')

dir_config('jpeg')

unless have_header('jpeglib.h') && have_library('jpeg')
  $stderr.puts 'jpeg library is required'
  abort
end

dir_config('png')
dir_config('zlib')

unless have_header('png.h') && have_library('png') && have_header('zlib.h') && have_library('z')
  $stderr.puts 'libpng and zlib are required'
  abort
end

have_type('png_alloc_size_t')

create_makefile('IMF/native')
