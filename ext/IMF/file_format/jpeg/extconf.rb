require 'mkmf'

$CFLAGS += " -I#{File.expand_path('../../../include', __FILE__)}"

dir_config('jpeg')

unless have_header('jpeglib.h') && have_library('jpeg')
  $stderr.puts 'jpeg library is required'
  abort
end

create_makefile('IMF/file_format/jpeg')
