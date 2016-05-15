require "bundler/gem_tasks"
require "rake/extensiontask"
require "rspec/core/rake_task"

Rake::ExtensionTask.new('IMF/native')
Rake::ExtensionTask.new('IMF/file_format/jpeg')
RSpec::Core::RakeTask.new(:spec)
