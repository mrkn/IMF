require 'libimf'

module IMF
  module FileFormats
    class JpegFormat
      FORMAT = :jpeg

      EXTENSIONS = %w[
        .jfif
        .jpe
        .jpg
        .jpeg
      ].map(&:freeze).freeze

      MIME = 'image/jpeg'.freeze

      MAGIC = "\xff\xd8".b.freeze

      def self.detect(io)
        magic = io.read(MAGIC.size)
        MAGIC === magic
      end

      def self.open(io, filename)
        JpegImageFile.new(io, filename)
        decoder = JpegDecoder.new(io)
        size = decoder.image_size
        mode = decoder.colorspace
      end

      class JpegImageFile < IMF::Image
        def initialize(io, filename)
          @io = io
          @filename = filename
          @decoder = IMF::JpegDecoder.new(@io)
          header = @decoder.read_header()
          if header.no_image
            raise "no image"  # TODO: specify exception class and appropriate message
          end
          size = [header.image_width, header.image_height]
          super(header.color_space, size, fill_color: nil)
        end

        def load
          @decoder.decode_image(self)
        end

        private

        def read_header()
        end
      end
    end

    require 'IMF/image'
    IMF::Image.register_format(JpegFormat)
  end
end
