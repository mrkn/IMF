require 'spec_helper'

module IMF
  module RSpec
    class TestFileFormat < IMF::FileFormat::Base
      self.extnames = %w[.test .tst]
    end

    class AnotherTestFileFormat < IMF::FileFormat::Base
      self.extnames = %w[.tst .atst]
    end
  end
end

RSpec.describe IMF::FileFormatRegistry do
  subject(:file_format_registry) do
    IMF::FileFormatRegistry.new
  end

  describe '#register' do
    context 'Given non-subclass of IMF::FileFormat::Base' do
      specify do
        expect {
          subject.register(42)
        }.to raise_error(TypeError)
      end

      specify do
        expect {
          subject.register(Object)
        }.to raise_error(ArgumentError)
      end

      specify do
        expect {
          subject.register(IMF::FileFormat::Base)
        }.to raise_error(ArgumentError)
      end
    end

    context 'Given a file format class' do
      it 'registers the file format' do
        subject.register(IMF::RSpec::TestFileFormat)
        expect(subject.file_formats).to eq([IMF::RSpec::TestFileFormat])
      end

      it 'also registers extnames associated to the file format' do
        subject.register(IMF::RSpec::TestFileFormat)
        subject.register(IMF::RSpec::AnotherTestFileFormat)
        expect(subject.extnames).to eq(%w[.test .tst .atst])
        expect(subject.file_formats_for_extname('.test')).to eq([IMF::RSpec::TestFileFormat])
        expect(subject.file_formats_for_extname('.tst')).to eq([IMF::RSpec::TestFileFormat, IMF::RSpec::AnotherTestFileFormat])
        expect(subject.file_formats_for_extname('.atst')).to eq([IMF::RSpec::AnotherTestFileFormat])
      end
    end

    context 'Given a registered file format class' do
      before do
        subject.register(IMF::RSpec::TestFileFormat)
      end

      specify do
        expect {
          subject.register(IMF::RSpec::TestFileFormat)
        }.not_to raise_error
      end

      it 'does not the duplicated file format' do
        subject.register(IMF::RSpec::TestFileFormat)
        expect(subject.extnames).to eq(%w[.test .tst])
        expect(subject.file_formats_for_extname('.test')).to eq([IMF::RSpec::TestFileFormat])
        expect(subject.file_formats_for_extname('.tst')).to eq([IMF::RSpec::TestFileFormat])
      end
    end
  end

  describe '#unregister' do
    context 'Given non-subclass of IMF::FileFormat::Base' do
      specify do
        expect {
          subject.unregister(42)
        }.to raise_error(TypeError)
      end

      specify do
        expect {
          subject.unregister(Object)
        }.to raise_error(ArgumentError)
      end

      specify do
        expect {
          subject.unregister(IMF::FileFormat::Base)
        }.to raise_error(ArgumentError)
      end
    end

    context 'Given a registered file format class' do
      before do
        subject.register(IMF::RSpec::TestFileFormat)
        subject.register(IMF::RSpec::AnotherTestFileFormat)
      end

      it 'unregisters only the given file format' do
        subject.unregister(IMF::RSpec::TestFileFormat)
        expect(subject.file_formats).to eq([IMF::RSpec::AnotherTestFileFormat])
        expect(subject.extnames).to eq(%w[.tst .atst])
        expect(subject.file_formats_for_extname('.tst')).to eq([IMF::RSpec::AnotherTestFileFormat])
        expect(subject.file_formats_for_extname('.atst')).to eq([IMF::RSpec::AnotherTestFileFormat])
      end
    end

    context 'Given unknown image format' do
      specify do
        expect {
          subject.unregister(IMF::RSpec::TestFileFormat)
        }.not_to raise_error
      end
    end
  end
end

RSpec.describe IMF, '.file_formats' do
  subject do
    IMF.file_formats
  end

  it { is_expected.to be_an(Array) }
end

RSpec.describe IMF, '.register_file_format' do
  it 'registers a file format' do
    IMF.register_file_format(IMF::RSpec::TestFileFormat)
    expect(IMF.file_formats).to include(IMF::RSpec::TestFileFormat)
    IMF.unregister_file_format(IMF::RSpec::TestFileFormat)
  end
end

RSpec.describe IMF, '.unregister_file_format' do
  pending
end

RSpec.describe IMF, '.each_file_format' do
  before do
    IMF.register_file_format(IMF::RSpec::TestFileFormat)
  end

  let(:registered_file_formats) do
    IMF.file_formats
  end

  specify do
    expect { |probe|
      IMF.each_file_format(&probe)
    }.to yield_successive_args(*registered_file_formats)
  end

  context 'Given an extname parameter' do
    before do
      IMF.register_file_format(IMF::RSpec::AnotherTestFileFormat)
    end

    specify do
      expect { |probe|
        IMF.each_file_format(extname: '.tst', &probe)
      }.to yield_successive_args(IMF::RSpec::TestFileFormat, IMF::RSpec::AnotherTestFileFormat)
    end
  end
end
