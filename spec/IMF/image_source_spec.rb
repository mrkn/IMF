require 'spec_helper'

require 'stringio'
require 'zlib'

RSpec.describe IMF::ImageSource do
  let(:image_path) do
    fixture_file('momosan.jpg')
  end

  subject(:image_source) do
    IMF::ImageSource.new(source)
  end

  shared_context 'image file source' do
    let(:source) do
      image_path
    end
  end

  shared_context 'gzipped io source' do
    let(:gzipped_source) do
      StringIO.new.tap { |strio|
        Zlib::GzipWriter.wrap(strio) do |gzio|
          gzio.write(IO.read(image_path, mode: 'rb'))
        end
      }.string
    end

    let(:source) do
      Zlib::GzipReader.wrap(StringIO.new(gzipped_source))
    end
  end

  describe '#read' do
    shared_examples 'Reading bytes' do
      context 'When reading bytes separatedly' do
        let(:read_lengths) do
          [2, 3, 5, 7, 9, 11, 13]
        end

        let(:total_length) do
          read_lengths.inject(:+)
        end

        let(:read_bytes) do
          read_lengths.map { |length|
            image_source.read(length)
          }.join('')
        end

        let(:expected_bytes) do
          IO.read(image_path, total_length, mode: 'rb')
        end

        it 'reads bytes correctly' do
          expect(read_bytes).to eq(expected_bytes)
        end
      end
    end

    context 'Given image_source is initialized with a image file path' do
      include_context 'image file source'

      include_examples 'Reading bytes'
    end

    context 'Given image_source is initialized with an IO object' do
      context 'When the IO object is readable' do
        let(:source) do
          open(image_path, 'rb')
        end

        include_examples 'Reading bytes'
      end

      context 'When the IO object is not readable', :run_in_tmpdir do
        let(:tmpfile_path) do
          File.expand_path(File.basename(image_path), tmpdir)
        end

        before do
          IO.write(tmpfile_path, IO.read(image_path, mode: 'rb'), mode: 'wb')
        end

        let(:source) do
          open(tmpfile_path, 'wb')
        end

        it do
          expect { image_source.read(1) }.to raise_error(IOError)
        end
      end
    end

    context 'Given image_source is initialized with a readable' do
      context 'When the readable is a StringIO' do
        let(:source) do
          StringIO.new(IO.read(image_path, mode: 'rb'))
        end

        include_examples 'Reading bytes'
      end

      context 'the readable is a Zlib::GzipReader' do
        include_context 'gzipped io source'

        include_examples 'Reading bytes'
      end
    end
  end

  describe '#rewind' do
    shared_examples 'rewind' do
      it 'rewind the reading cursor to the beginning of file' do
        lead_1024_bytes = IO.read(image_path, 1024, mode: 'rb')
        image_source.read(128)
        image_source.rewind
        expect(image_source.read(1024)).to eq(lead_1024_bytes)
      end
    end

    context 'Given image_source is initialized with a image file path' do
      include_context 'image file source'

      include_examples 'rewind'
    end

    context 'Given image_source is initialized with a Zlib::GzipReader object' do
      include_context 'gzipped io source'

      include_examples 'rewind'
    end
  end

  describe '#path' do
    subject do
      image_source.path
    end

    context 'Given image_source is initialized with a path' do
      include_context 'image file source'

      it { is_expected.to eq(image_path) }
    end

    context 'Given image_source is initialized with a Zlib::GzipReader object' do
      include_context 'gzipped io source'

      it { is_expected.to eq(nil) }
    end
  end
end
