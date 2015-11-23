require 'spec_helper'
require 'zlib'

RSpec.describe IMF::Image, '.detect_format' do
  describe 'Abnormal conditions' do
    context 'Given a non-existing filename' do
      it 'raises IOError', :with_tmpdir do
        expect {
          IMF::Image.detect_format(File.join(tmpdir, 'non_existing.jpg'))
        }.to raise_error(Errno::ENOENT)
      end
    end

    context 'Given a directory name' do
      it 'raises IOError', :with_tmpdir do
        expect {
          IMF::Image.detect_format(tmpdir)
        }.to raise_error(Errno::EISDIR)
      end
    end

    context 'Given an object not a string nor an IO' do
      it 'raises ArgumentError' do
        expect {
          IMF::Image.detect_format(42)
        }.to raise_error(ArgumentError)
      end
    end

    context 'When the IO object is not readable' do
      it 'raises IOError and does not close the passed IO object', :run_in_tmpdir do
        FileUtils.cp(fixture_file("vimlogo-141x141.gif"), "tmp.gif")
        open("tmp.gif", "wb") do |io|
          expect {
            IMF::Image.detect_format(io)
          }.to raise_error(IOError)
          expect(io).not_to be_closed
        end
      end
    end
  end

  context 'Given a GIF image' do
    let(:image_filename) do
      fixture_file("vimlogo-141x141.gif")
    end

    context 'Given a filename of the image' do
      it 'returns :gif' do
        expect(IMF::Image.detect_format(image_filename)).to eq(:gif)
      end
    end

    context 'Given a IO object of the image' do
      context 'When the IO object is readable' do
        it 'returns :gif and does not close the passed IO object' do
          open(image_filename, "rb") do |io|
            expect(IMF::Image.detect_format(io)).to eq(:gif)
            expect(io).not_to be_closed
          end
        end
      end
    end

    context 'Given a StringIO object of the image' do
      it 'returns :gif' do
        strio = StringIO.new(IO.read(image_filename, mode: "rb"))
        expect(IMF::Image.detect_format(strio)).to eq(:gif)
      end
    end

    context 'Given a Zlib::GzipReader object of the gzipped image' do
      let(:gzipped_image_filename) do
        image_filename + '.gz'
      end

      it 'returns :gif and does not close the passed IO object' do
        Zlib::GzipReader.open(gzipped_image_filename) do |gzio|
          expect(IMF::Image.detect_format(gzio)).to eq(:gif)
          expect(gzio).not_to be_closed
        end
      end
    end
  end

  context 'Given "fixtures/momosan.jpg"' do
    it 'returns :jpeg' do
      expect(IMF::Image.detect_format(fixture_file("momosan.jpg"))).to eq(:jpeg)
    end
  end

  context 'Given "fixtures/vimlogo-141x141.png"' do
    it 'returns :png' do
      expect(IMF::Image.detect_format(fixture_file("vimlogo-141x141.png"))).to eq(:png)
    end
  end

  context 'Given "fixtures/momosan.webp"' do
    it 'returns :webp' do
      expect(IMF::Image.detect_format(fixture_file("momosan.webp"))).to eq(:webp)
    end
  end

  context 'Given "fixtures/not_image.txt"' do
    it 'returns nil' do
      expect(IMF::Image.detect_format(fixture_file("not_image.txt"))).to eq(nil)
    end
  end
end
