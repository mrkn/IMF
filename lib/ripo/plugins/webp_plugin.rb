module Ripo
  module Plugins
    class WebpPlugin
      FORMAT = :webp

      EXTENSIONS = %w[
        .webp
      ].map(&:freeze).freeze

      MIME = 'image/png'.freeze

      PREFIX_SIZE = 16
      MAGICS = {
        0...4  => "RIFF".b.freeze,
        8...12 => "WEBP".b.freeze,
        12...16 => /\AVP8[ XL]\z/,
      }.freeze

      def self.detect(io)
        prefix = io.read(PREFIX_SIZE)
        io.rewind
        MAGICS.all? do |range, magic|
          magic === prefix[range]
        end
      end
    end

    require 'ripo/image'
    Ripo::Image.register_format(WebpPlugin)
  end
end
