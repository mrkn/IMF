require 'mkmf'

dir_config('libjpeg')
unless have_header('jpeglib.h') && have_library('jpeg')
  $stderr.puts "Unable to find libjpeg"
  abort
end

dir_config('libpng')
unless have_header('png.h') && have_library('png')
  $stderr.puts "Unable to find libpng"
  abort
end

create_makefile('libimf')
