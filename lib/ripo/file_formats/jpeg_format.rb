module Ripo
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
        io.rewind
        MAGIC === magic
      end
    end

    require 'ripo/image'
    Ripo::Image.register_format(JpegFormat)
  end
end
