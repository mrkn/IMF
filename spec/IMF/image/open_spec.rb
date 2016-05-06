require 'spec_helper'

RSpec.describe IMF::Image, '.open' do
  subject(:image) do
    IMF::Image.open(image_filename)
  end

  describe 'Abnormal conditions' do
    context 'Given a non-existing pathname' do
      it 'raise Errno::ENOENT', :with_tmpdir do
        expect {
          IMF::Image.open(Pathname(tmpdir).join('non_existing.jpg'))
        }.to raise_error(Errno::ENOENT)
      end
    end

    context 'Given a directory pathname' do
      it 'raise Errno::EISDIR', :with_tmpdir do
        expect {
          IMF::Image.open(Pathname(tmpdir))
        }.to raise_error(Errno::EISDIR)
      end
    end

    context 'Given a non-existing filename string' do
      it 'raise Errno::ENOENT', :with_tmpdir do
        expect {
          IMF::Image.open(File.join(tmpdir, 'non_existing.jpg'))
        }.to raise_error(Errno::ENOENT)
      end
    end

    context 'Given a directory name string' do
      it 'raise Errno::EISDIR', :with_tmpdir do
        expect {
          IMF::Image.open(tmpdir)
        }.to raise_error(Errno::EISDIR)
      end
    end

    context 'Given a closed IO' do
      it 'raises ArgumentError' do
        expect {
          closed_io = $stdin.dup
          closed_io.close
          IMF::Image.open(closed_io)
        }.to raise_error(IOError)
      end
    end

    context 'Given an object not a string nor a pathname' do
      it 'raises ArgumentError' do
        expect {
          IMF::Image.open(42)
        }.to raise_error(IMF::ImageSource::InvalidSourceError)
      end
    end
  end

  context 'Given a JPEG image' do
    let(:image_filename) do
      fixture_file("momosan.jpg")
    end

    it 'returns an image object' do
      is_expected.to be_a(IMF::Image)
      expect(subject.color_space).to eq(:RGB)
      expect(subject.component_size).to eq(1)
      expect(subject.pixel_channels).to eq(3)
      expect(subject.width).to eq(809)
      expect(subject.height).to eq(961)
      expect(subject.row_stride).to eq(816)
    end

    context 'the given filename ends with ".png"', :run_in_tmpdir do
      let(:original_image_filename) do
        fixture_file("momosan.jpg")
      end

      let(:image_filename) do
        File.join(tmpdir, File.basename(original_image_filename, '.jpg') + '.png')
      end

      before do
        IO.write(image_filename, IO.read(original_image_filename, mode: 'rb'), mode: 'wb')
      end

      it 'returns an image object' do
        is_expected.to be_a(IMF::Image)
        expect(subject.color_space).to eq(:RGB)
        expect(subject.component_size).to eq(1)
        expect(subject.pixel_channels).to eq(3)
        expect(subject.width).to eq(809)
        expect(subject.height).to eq(961)
        expect(subject.row_stride).to eq(816)
      end
    end
  end

  xcontext 'Given a PNG image' do
    let(:image_filename) do
      fixture_file("momosan.png")
    end

    context 'the given filename ends with ".jpg"', :run_in_tmpdir do
      let(:original_image_filename) do
        fixture_file("momosan.png")
      end

      let(:image_filename) do
        File.join(tmpdir, File.basename(original_image_filename, '.png') + '.jpg')
      end

      before do
        IO.write(image_filename, IO.read(original_image_filename, mode: 'rb'), mode: 'wb')
      end

      it 'returns an image object' do
        is_expected.to be_a(IMF::Image)
      end
    end
  end

  xcontext 'Given a WEBP image' do
    let(:image_filename) do
      fixture_file("momosan.webp")
    end

    context 'the given filename ends with ".jpg"', :run_in_tmpdir do
      let(:original_image_filename) do
        fixture_file("momosan.webp")
      end

      let(:image_filename) do
        File.join(tmpdir, File.basename(original_image_filename, '.webp') + '.jpg')
      end

      before do
        IO.write(image_filename, IO.read(original_image_filename, mode: 'rb'), mode: 'wb')
      end

      it 'returns a correct image object' do
        is_expected.to be_a(IMF::Image)
      end
    end
  end
end
