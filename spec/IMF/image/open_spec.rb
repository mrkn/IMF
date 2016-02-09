require 'spec_helper'

RSpec.xdescribe IMF::Image, '.open' do
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

    context 'Given an object not a string nor a pathname' do
      it 'raises ArgumentError' do
        expect {
          IMF::Image.open(42)
        }.to raise_error(ArgumentError)
      end
    end
  end

  context 'Given a GIF image' do
    let(:image_filename) do
      fixture_file("vimlogo-141x141.gif")
    end

  end

  context 'Given a JPEG image' do
    let(:image_filename) do
      fixture_file("momosan.jpg")
    end

  end
end
