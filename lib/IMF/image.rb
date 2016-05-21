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
      image_source = ImageSource.new(file)
      IMF.each_file_format do |fmt_class|
        fmt = fmt_class.new
        return fmt if fmt.detect(image_source)
      end
      nil
    end

    def self.open(source)
      image_source = ImageSource.new(source)
      load_image(image_source)
    end
  end
end
