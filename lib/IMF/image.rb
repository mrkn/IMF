module IMF
  class Image
    require 'IMF/image/format_management'
    extend FormatManagement

    def self.detect_format(filename)
      open(filename, 'rb') do |io|
        each_registered_format do |format_name, format_plugin|
          if format_plugin.detect(io)
            return format_name
          end
        end
      end
      nil
    end
  end
end
