module IMF
  class FileFormatRegistry
    def initialize
      @file_format_table = nil
      @extname_table = nil
    end

    def file_formats
      return [] unless @file_format_table
      @file_format_table.keys
    end

    def file_formats_for_extname(extname)
      return [] unless @file_format_table
      @extname_table[extname].keys
    end

    def extnames
      return [] unless @extname_table
      @extname_table.keys
    end

    def register(file_format)
      check_file_format(file_format)
      file_format_table[file_format] = true
      register_for_extnames(file_format)
    end

    def unregister(file_format)
      check_file_format(file_format)
      file_format_table.delete(file_format)
      unregister_for_extnames(file_format)
    end

    def each(extname: nil, &block)
      if extname
        extname_table[extname].each_key(&block)
      else
        file_format_table.each_key(&block)
      end
    end

    private

    def check_file_format(file_format)
      unless file_format.is_a?(Class)
        raise TypeError, "file_format must be a subclass of IMF::FileFormat::Base"
      end
      unless file_format < IMF::FileFormat::Base
        raise ArgumentError, "file_format must be a subclass of IMF::FileFormat::Base"
      end
    end

    def register_for_extnames(file_format)
      file_format.extnames.each do |extname|
        register_extname(extname, file_format)
      end
    end

    def register_extname(extname, file_format)
      extname_table[extname][file_format] = true
    end

    def unregister_for_extnames(file_format)
      file_format.extnames.each do |extname|
        unregister_extname(extname, file_format)
      end
    end

    def unregister_extname(extname, file_format)
      extname_table[extname].delete(file_format)
      if extname_table[extname].empty?
        extname_table.delete(extname)
      end
    end

    def file_format_table
      @file_format_table ||= Hash.new(false)
    end

    def extname_table
      @extname_table ||= Hash.new {|h, k| h[k] = Hash.new(false) }
    end
  end

  module GlobalFileFormatRegistry
    def file_formats
      global_file_format_registry.file_formats
    end

    def register_file_format(file_format)
      global_file_format_registry.register(file_format)
    end

    def unregister_file_format(file_format)
      global_file_format_registry.unregister(file_format)
    end

    def each_file_format(extname: nil, &block)
      global_file_format_registry.each(extname: extname, &block)
    end

    def file_formats_for_filename(filename)
      extname = File.extname(filename)
      global_file_format_registry.file_formats_for_extname(extname)
    end

    private

    def global_file_format_registry
      @global_file_format_registry ||= FileFormatRegistry.new
    end
  end

  extend GlobalFileFormatRegistry
end
