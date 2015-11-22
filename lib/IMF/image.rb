require 'stringio'
require 'pathname'
require 'uri'

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
        if format_plugin.detect(io)
          return format_name
        end
      end
      nil
    ensure
      io.close if io && io != file
    end
  end
end
