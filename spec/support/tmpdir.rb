require 'tmpdir'

module IMF
  module RSpec
    module TmpdirHelper
      def mktmpdir(suffix=nil, tmpdir=nil, &block)
        prefix_suffix = 'IMF'
        prefix_suffix = [prefix_suffix, String(suffix)] if suffix
        Dir.mktmpdir(prefix_suffix, tmpdir, &block)
      end
    end
  end
end

RSpec.configure do |config|
  config.include IMF::RSpec::TmpdirHelper
end

RSpec.shared_context 'With tmpdir', :with_tmpdir do
  let!(:tmpdir) do
    mktmpdir
  end

  after :each do
    FileUtils.remove_entry_secure tmpdir
  end
end

RSpec.shared_context 'Run examples in tmpdir', :run_in_tmpdir do
  include_context 'With tmpdir'

  let!(:last_pwd) do
    Dir.pwd
  end

  before :each do
    Dir.chdir(tmpdir)
  end

  after :each do
    Dir.chdir(last_pwd)
  end
end
