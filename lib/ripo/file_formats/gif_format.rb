module Ripo
  module FileFormats
    class GifFormat
      FORMAT = :gif

      EXTENSIONS = %w[
        .gif
      ].map(&:freeze).freeze

      MIME = 'image/gif'.freeze

      MAGIC = "GIF8"

      def self.detect(io)
        magic = io.read(MAGIC.size)
        io.rewind
        MAGIC === magic
      end
    end

    require 'ripo/image'
    Ripo::Image.register_format(GifFormat)
  end
end
