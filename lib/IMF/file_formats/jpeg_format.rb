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
          mode, size = read_header()
          super(mode, size, fill_color: nil)
        end

        def load
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
