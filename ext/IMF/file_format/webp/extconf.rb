require 'mkmf'

$CFLAGS += " -I#{File.expand_path('../../../include', __FILE__)}"

create_makefile('IMF/fileformat/webp')
