require 'spec_helper'
require 'zlib'

RSpec.describe IMF::Image, '.detect_format' do
  describe 'Abnormal conditions' do
    context 'Given a non-existing filename' do
      it 'raises Errno::ENOENT', :with_tmpdir do
        expect {
          IMF::Image.detect_format(File.join(tmpdir, 'non_existing.jpg'))
        }.to raise_error(Errno::ENOENT)
      end
    end

    context 'Given a directory name' do
      it 'raises Errno::EISDIR', :with_tmpdir do
        expect {
          IMF::Image.detect_format(tmpdir)
        }.to raise_error(Errno::EISDIR)
      end
    end

    context 'Given a non-image file' do
      it 'returns nil', :run_in_tmpdir do
        IO.write('not_image.txt', 'not image file', mode: 'wb')
        expect(IMF::Image.detect_format('not_image.txt')).to eq(nil)
      end
    end

    context 'Given an object not a string nor an IO' do
      it 'raises ArgumentError' do
        expect {
          IMF::Image.detect_format(42)
        }.to raise_error(IMF::ImageSource::InvalidSourceError)
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

  shared_examples 'Normal conditions' do |image_format|
    context 'Given a filename of the image' do
      it "returns #{image_format.inspect}" do
        expect(IMF::Image.detect_format(image_filename)).to be_a(image_format)
      end
    end

    context 'Given a IO object of the image' do
      context 'When the IO object is readable' do
        it "returns #{image_format.inspect}, and does not close and rewind the passed IO object" do
          open(image_filename, "rb") do |io|
            expect(IMF::Image.detect_format(io)).to be_a(image_format)
            expect(io).not_to be_closed
            expect(io.tell).not_to eq(0)
          end
        end
      end
    end

    context 'Given a StringIO object of the image' do
      it "returns #{image_format}, and does not rewind the passed StringIO object" do
        strio = StringIO.new(IO.read(image_filename, mode: "rb"))
        expect(IMF::Image.detect_format(strio)).to be_a(image_format)
        expect(strio.tell).not_to eq(0)
      end
    end

    context 'Given a Zlib::GzipReader object of the gzipped image', :with_tmpdir do
      let(:gzipped_image_filename) do
        File.join(tmpdir, File.basename(image_filename) + '.gz')
      end

      before do
        Zlib::GzipWriter.open(gzipped_image_filename) do |gzio|
          gzio.write(IO.read(image_filename, mode: 'rb'))
        end
      end

      it "returns #{image_format}, and does not close and rewind the passed IO object" do
        Zlib::GzipReader.open(gzipped_image_filename) do |gzio|
          expect(IMF::Image.detect_format(gzio)).to be_a(image_format)
          expect(gzio).not_to be_closed
          expect(gzio.tell).not_to eq(0)
        end
      end
    end
  end

  context 'Given a GIF image' do
    let(:image_filename) do
      fixture_file("vimlogo-141x141.gif")
    end

    include_examples 'Normal conditions', IMF::FileFormat::GIF
  end

  context 'Given a JPEG image' do
    let(:image_filename) do
      fixture_file("momosan.jpg")
    end

    include_examples 'Normal conditions', IMF::FileFormat::JPEG
  end

  context 'Given a PNG image' do
    let(:image_filename) do
      fixture_file("vimlogo-141x141.png")
    end

    include_examples 'Normal conditions', IMF::FileFormat::PNG
  end

  context 'Given a WEBP image' do
    let(:image_filename) do
      fixture_file("momosan.webp")
    end

    include_examples 'Normal conditions', IMF::FileFormat::WEBP
  end
end
