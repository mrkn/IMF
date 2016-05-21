require "IMF/version"

module IMF
  class Error < StandardError
  end

  module FileFormat
  end
end

require "IMF/native"
require "IMF/image"
require "IMF/file_format_registry"
require "IMF/file_format/jpeg"
require "IMF/file_format/png"
require "IMF/file_format/webp"
require "IMF/file_format/gif"
