module IMF
  module FileFormats
    class PngFormat
      FORMAT = :png

      EXTENSIONS = %w[
        .png
      ].map(&:freeze).freeze

      MIME = 'image/png'.freeze

      MAGIC = "\x89PNG\x0d\x0a\x1a\x0a".b.freeze

      def self.detect(io)
        magic = io.read(MAGIC.size)
        io.rewind
        MAGIC === magic
      end
    end

    require 'IMF/image'
    IMF::Image.register_format(PngFormat)
  end
end
