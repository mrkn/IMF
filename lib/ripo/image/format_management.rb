module Ripo
  class Image
    module FormatManagement
      def register_format(format_plugin)
        check_requirements(format_plugin)
        add_format(format_plugin)
      end

      def each_registered_format
        registered_formats.each(&proc)
      end

      private

      def check_requirements(format_plugin)
        format_plugin::FORMAT
      end

      def registered_formats
        @registered_formats ||= {}
      end

      def add_format(format_plugin)
        registered_formats[format_plugin::FORMAT] = format_plugin
      end
    end
  end
end
