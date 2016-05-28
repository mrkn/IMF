require "IMF/version"

module IMF
  class Error < StandardError
  end

  module FileFormat
  end
end

$LOAD_PATH.unshift File.expand_path("~/work/num_buffer/lib")
require "num_buffer"

require "IMF/native"
require "IMF/image"
require "IMF/file_format_registry"
require "IMF/file_format/jpeg"
require "IMF/file_format/png"
require "IMF/file_format/webp"
require "IMF/file_format/gif"
