module IMF
  module FileFormat
    class WebpFormat
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
        MAGICS.all? do |range, magic|
          magic === prefix[range]
        end
      end
    end

    require 'IMF/image'
    IMF::Image.register_format(WebpFormat)
  end
end
