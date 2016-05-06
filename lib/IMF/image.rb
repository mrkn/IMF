require 'stringio'
require 'pathname'
require 'uri'

require 'IMF/native'
require 'IMF/image_source'

module IMF
  class Image
    require 'IMF/image/format_management'
    extend FormatManagement

    def self.detect_format(file)
      case file
      when String, Pathname, URI
        io = Kernel.open(file, 'rb')
      when -> (obj) { obj.respond_to?(:read) }
        io = file
      else
        raise ArgumentError, "The argument must be a filename or a reeadable IO"
      end
      each_registered_format do |format_name, format_plugin|
        io.rewind
        if format_plugin.detect(io)
          return format_name
        end
      end
      nil
    ensure
      io.close if io && io != file
    end

    def self.open(source)
      image_source = ImageSource.new(source)
      load_image(image_source)
    end
  end
end
