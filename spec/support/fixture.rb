module IMF
  module RSpec
    module FixtureHelper
      def fixtures_dir
        File.expand_path('../../fixtures', __FILE__)
      end

      def fixture_file(filename)
        File.join(fixtures_dir, filename)
      end
    end
  end
end

RSpec.configuration.include IMF::RSpec::FixtureHelper
