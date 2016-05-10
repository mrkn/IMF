require 'mkmf'

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

create_makefile('IMF/native')
