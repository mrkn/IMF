module IMF
  class ImageSource
    INITIAL_BUFFER_SIZE = 8192

    class InvalidSourceError < IMF::Error
    end

    def initialize(source)
      @buffer = '\0' * INITIAL_BUFFER_SIZE
      @read_size = 0
      @pos = 0

      case source
      when String
        init_with_path(source)
      when IO, File
        init_with_io(source)
      else
        init_with_readable(source)
      end
    end

    attr_reader :path

    def read(length = nil, outbuf = '')
      init_with_io(File.open(@path, 'rb')) unless @source

      if length.nil?
        data = @source.read
        @buffer[@read_size, data.length] = data
        @read_size += data.length
        beg, @pos = @pos, @read_size
        outbuf.replace @buffer[beg..-1]
      elsif length > 0
        tail_length = @read_size - @pos
        shortage_length = length - tail_length
        if shortage_length <= 0
          outbuf.replace @buffer[@pos, length]
          @pos += length
        else
          data = @source.read(shortage_length)
          @buffer[@read_size, data.length] = data
          @read_size += data.length
          beg, @pos = @pos, @read_size
          outbuf.replace @buffer[beg, length]
        end
      end

      outbuf
    end

    def rewind
      @pos = 0
    end

    private

    def init_with_path(path)
      unless File.exist? path
        raise Errno::ENOENT
      end

      if File.directory? path
        raise Errno::EISDIR
      end

      unless File.readable? path
        raise Errno::EPERM
      end

      @path = path
      @source = nil
    end

    def init_with_io(io)
      # check readability
      begin
        io.read_nonblock(0)
      rescue SystemCallError
        # ignored
      end

      @source = io
    end

    def init_with_readable(obj)
      if obj.respond_to?(:read)
        @source = obj
      else
        raise InvalidSourceError
      end
    end
  end
end
